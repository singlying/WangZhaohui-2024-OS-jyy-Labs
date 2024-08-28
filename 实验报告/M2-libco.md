# M2:协程库 (libco)

> 实现轻量级用户态`协程`(Coroutine)

## 实验目的和要求

**目的**：

- 多处理器编程：入门
- 使用一个操作系统线程实现主动切换的多个执行流
- 协程的概念和工作原理，与线程的区别
- 动手实现分时操作系统内核中具有奠基性意义的机制：上下文切换

**要求**：**实现协程切换函数**

- 实现协程库的基本功能：co_start(), co_wait(), co_yield()
- 能够通过给出的测试用例-生产者消费者模型
- 实现32-bit和64-bit版本





## 实验环境

- 操作系统Linux ：使用Windows `wsl` (Windows Subsystem for Linux)运行的`Ubuntu  22.04`系统
- 编译器GCC  调试器GDB
- 构建和编译程序：`Makefile`





## 编译和运行

### 动态链接库

框架代码的 `co.c` 中没有 `main` 函数——它并不会被编译成一个能直接在 Linux 上执行的二进制文件。编译脚本会生成共享库，共享库中不需要main函数。在makefile中写明了如何编译共享库：

~~~makefile
$(NAME)-64.so: $(DEPS) # 64bit shared library
	gcc -fPIC -shared -m64 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)
~~~

### 编译链接

在main()函数所在文件的makefile中，使用如下的编译命令编译32-bit和64-bit可执行文件。

~~~makefile
libco-test-64: main.c
	gcc -I.. -L.. -m64 main.c -o libco-test-64 -lco-64

libco-test-32: main.c
	gcc -I.. -L.. -m32 main.c -o libco-test-32 -lco-32
~~~

- `-I `代表 include path，使我们可以 `#include <co.h>`
- `-L` 选项代表增加 link search path
- `-l` 选项代表链接某个库，链接时会自动加上 `lib` 的前缀，即 `-lco-64` 会依次在库函数的搜索路径中查找 `libco-64.so` 和 `libco-64.a`，直到找到为止。

如果不设置 `LD_LIBRARY_PATH` 环境变量，且程序依赖的共享库（如 `libco-64.so`）不在默认的系统库路径中，系统将无法找到所需的共享库，产生error。



## 实现分析

本实验的主要工作是在协程数据结构的基础上完成三个协程调用函数的实现。

### 数据结构

协程的实现主要依赖于以下几个关键的数据结构和枚举类型：

#### `enum co_status`

定义了以下几种协程的状态：

- **CO_NEW (== 1)：** 新创建的协程，还未执行过。
- **CO_RUNNING：** 协程已经开始执行，但尚未完成。
- **CO_WAITING：** 协程处于等待状态，通常是在调用`co_wait()`后等待其他协程完成。
- **CO_DEAD：** 协程已经结束执行，但其资源尚未释放。

#### `struct co`

协程的主要数据结构，每个协程实例都对应一个`struct co`结构体：

~~~c
struct co {
  char name[30];
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]__attribute__((aligned(16))); // 协程的堆栈
};
~~~

#### 全局变量和指针

- `struct co *current`：指向当前运行的协程 (初始时，需要为 main 分配协程对象)
- `int co_num`：记录当前存在的协程数量。
- `struct co *co_list[CO_SIZE]`：协程数组，存储了程序中所有的协程。在程序中最多支持CO_SIZE个协程。



### co_start()

**功能**：创建一个新的协程并初始化其状态。为协程指定一个名称、入口函数和参数，并将协程添加到全局协程列表中，以便后续管理和调度。

**具体实现**：

~~~c
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co* res = (struct co*)malloc(sizeof(struct co));  // 为新协程分配内存
  ... // 初始化协程状态
  co_list[co_num++] = res;  // 将新创建的协程添加到协程列表中
  return res;  // 返回创建的协程
}

~~~



### co_wait()

**功能**：等待指定的协程执行完成并进行资源清理。

**具体实现**：

1. 设定waiter和wait，更改对应的协程状态

   ~~~c
     co->waiter = current;
     current->status = CO_WAITING;
   ~~~

2. 等待wait协程结束

   ~~~c
     while (co->status != CO_DEAD) {
       co_yield();
     }
   ~~~

3. 清理资源并调整全局表



### co_yield()

**功能**：负责在协程之间进行主动切换。

**具体实现**：

1. 保存当前协程的上下文

   - 函数开始时，调用 `setjmp(current->context)` 保存当前协程的寄存器上下文，返回 0 时表示保存成功并继续执行下一步操作；返回非零值时表示从 `longjmp` 返回。

2. 获取下一个要执行的协程

   调用 `get_next_co()` 随机选择下一个要执行的协程，检查下一个协程的状态

   - **CO_NEW**： 协程第一次执行。需要调整其状态的 `CO_RUNNING`，并通过汇编指令切换堆栈指针到新协程的堆栈顶部，然后调用协程的入口函数 (`func`) 执行协程代码。

     汇编指令用于调整栈指针并调用协程函数：

     - 对于 x86_64 架构，使用 `movq` 指令将寄存器值和堆栈指针调整到新协程的环境。
     - 对于 x86 架构，使用 `movl` 指令完成类似的操作。

     如果当前协程的 `waiter`（等待者）存在，则恢复等待协程的上下文，通过 `longjmp` 跳转到等待协程继续执行。

     ~~~c
         if (next->status == CO_NEW) {
           next->status = CO_RUNNING;
           asm volatile( // 汇编指令切换堆栈指针
           #if __x86_64__
     		...
           #else
     		...
           #endif
           );

           next->status = CO_DEAD;

           if (current->waiter) { // 如果waiter存在，回复等待协程的上下文
             current = current->waiter;
             longjmp(current->context, 1);
           }
           co_yield();
         }
     ~~~

     ​

   - **CO_RUNNING**：协程此前已经执行过，此时直接通过 `longjmp` 恢复上下文，继续执行。

3. **从 longjmp 返回**：如果 `setjmp` 返回非零值，说明协程切换回了当前协程，函数结束。



## 协程调度逻辑

协程调度的核心在于 `co_start`、`get_next_co` 和 `co_yield` 函数之间的配合。协程通过 `co_start` 注册到调度器中，`get_next_co` 随机选择下一个可执行的协程，而 `co_yield` 负责上下文切换和状态管理。通过这种调度机制，即使在单线程环境中，多个协程也能够交替执行，实现类似多线程的并发效果。

- 协程的创建： 初始化一个新的协程对象，并将其加入到协程列表中等待调度执行。
- 调度选择：`get_next_co` 负责在所有注册的协程中选择下一个要执行的协程。函数的设计是为了模拟一种简单的协程调度策略，即随机选择一个协程来执行，当然也可以使用更复杂的优先级或时间片调度策略。
- 上下文切换 ：保存当前协程的上下文，使用 `setjmp` 保存当前协程的寄存器状态和程序计数器。这样，协程可以在稍后恢复并从保存的位置继续执行。



## 可能存在的问题

### 调度方式

代码中的`get_next_co()`函数通过随机选择下一个协程进行调度，可以采用其他的调度算法，比如课上学过的：

- **轮转调度**：确保每个协程都能获得公平的执行机会。
- **优先级调度**：如果不同的协程具有不同的重要性，可以引入优先级调度机制，根据优先级选择下一个执行的协程。

### 堆栈分配

代码中每个协程的堆栈大小固定为`32KB`（`STACK_SIZE`），显然不是最佳选择。

如果采用动态分配的方法，可以避免不必要的浪费。但是实现难度较大，本实验中并没有实现。

### 资源管理和释放

协程库中的资源管理有些微妙，因为 `co_wait` 执行的时候，有两种不同的可能性：

1. 此时协程已经结束 (`func` 返回)，这是完全可能的。此时，`co_wait` 应该直接回收资源。
2. 此时协程尚未结束，因此 `co_wait` 不能继续执行，必须调用 `co_yield` 切换到其他协程执行，直到协程结束后唤醒。

需要考虑好每一种可能的情况，保证程序不会在任何一种情况下 crash 或造成资源泄漏。假设每个协程都会被 `co_wait` 一次，且在 `co_wait` 返回时释放内存是一个几乎不可避免的设计：如果允许在任意时刻、任意多次等待任意协程，那么协程创建时分配的资源就无法做到自动回收了——即便一个协程结束，我们也无法预知未来是否还会执行对它的 `co_wait`，而对已经回收的 (非法) 指针的 `co_wait` 将导致 undefined behavior。

除此之外，在实际的应用中，如果在协程执行过程中出现异常，也需要对应的异常处理中断机制。








