# L2: 内核线程管理 (kmt)

> 完成多处理器操作系统内核中的内核线程 API

## 实验目的和要求

**目的**：

- 实验在pmm的基础上，增加中断和线程管理的功能
- 完成相关组件的结构体定义
- 实现kmt的全部API

**要求**：

- 实现中断处理和上下文切换

- Safety 和 Liveness：设计调度的策略在多个处理器中调度线程，使系统中能够被执行的线程尽可能不发生饥饿。

  Hard-test: 在线程数不过多的前提 (十几) 下，我们要求每个可运行的线程，给定足够长 (例如数秒) 的时间，能够被调度到每个处理器上执行.

- 本地通过给定的官方测试用例

-  `x86-qemu` 和 `x86_64-qemu`下工作

  ​


## 开发环境

- **AbstractMachine**

> 在实验报告Abstract Machine中，已经介绍了所用到的AbstractMachine的所需内容

本次实验在 pmm 的基础上，增加中断和线程管理的功能。所以程序保留了L1实验中的pmm实现，也进行了一些维护和优化，主要是增加安全性。




## 实验准备

和网课上演示的thread-os类似，本次需要实现操作系统接管中断/系统调用/异常时的回调函数，并且实现线程的生命周期函数以及同步机制，或者说本次实验是thread-os的扩展。

### 框架代码的变更

实验的框架代码在`main()`函数中增加了`ioe_init()`和`cte_init()`的调用，主要功能是在中断打开后处理系统调用和异常。这些函数的调用不会立即生效，因为此时中断仍然是关闭的。

~~~c
int main() {
    ioe_init();
    cte_init(os->trap);
    os->init();
    // 在这之前一直都只有一个处理器在执行，编号为 0 的CPU
    mpe_init(os->run);
    return 1;
}
~~~

### OS模块的初始化

对于初始化来讲，在实验中可以控制`os->init()` 的行为，执行模块的初始化，例如pmm和kmt。

在 `os->init()` 初始化完成后，`mpe_init` 会让每个处理器都运行相同的 `os->run()` 代码。此时，操作系统就真正化身成为了中断处理程序。

本次实验在 os 模块中新增了 `trap` 和 `on_irq` 两个函数，分别是系统中唯一中断/系统调用的入口和中断处理程序的回调注册。

为了增加代码的可维护性，防止在增加新的功能时都直接去修改 `os->trap` 的代码，框架代码中提供了中断处理API：

~~~c
os->on_irq(seq, event, handler)
~~~

调用 `os->on_irq` 向操作系统内核 “注册” 一个中断处理程序：在 `os->trap(ev, ctx)` 执行时，当 `ev.event` (事件编号) 和 `event` 匹配时，调用 `handler(event, ctx);`。

对于 `os->trap()` 的实现：

~~~c
static Context *os_trap(Event ev, Context *ctx) {
    Context *next = NULL;
    for (auto &h: handlers_sorted_by_seq) {
        if (h.event == EVENT_NULL || h.event == ev.event) {
            Context *r = h.handler(ev, ctx);
            panic_on(r && next, "return to multiple contexts");
            if (r) next = r;
        }
    }
    panic_on(!next, "return to NULL context");
    panic_on(sane_context(next), "return to invalid context");
    return next;
}
~~~

### PMM模块

pmm 与之前行为一致，但是需要增加线程安全的内存分配，允许线程调用`pmm->alloc()`和`pmm->free()`。此外，允许在中断处理程序中分配和回收内存。

但是为了简化实验，约定`pmm->alloc()`和`pmm->free()`被中断。



## 实验实现

~~~c
MODULE_DEF(kmt) = {.init = kmt_init,
                   .create = kmt_create,
                   .teardown = kmt_teardown,
                   .spin_init = spin_init,
                   .spin_lock = spin_lock,
                   .spin_unlock = spin_unlock,
                   .sem_init = sem_init,
                   .sem_wait = sem_wait,
                   .sem_signal = sem_signal};
~~~

通过定义`kmt`模块的接口，本实验主要实现kmt模块的以下部分：

### 任务管理

- **任务结构体 (task_t)**：表示一个任务（或线程），包括其名称、状态、上下文和栈空间。`currents`数组保存每个CPU当前正在执行的任务。
- **switch_boot_pcb()**：初始化`idle`任务（空闲任务）并将每个CPU的当前任务设置为`idle`。`idle`任务是在没有其他任务可运行时CPU执行的任务。
- **kmt_context_save()**：在中断发生时保存当前任务的上下文，并将任务状态设置为`RUNNABLE`，以便它可以在后续调度中被重新选择。
- **kmt_schedule()**：任务调度器，用于选择下一个要运行的任务。如果没有可运行的任务，它会选择`idle`任务。调度器会根据任务的ID循环选择下一个可运行任务，并切换到该任务的上下文。

### 自旋锁

- **spin_lock() 和 spin_unlock()**：用于保护关键区段，确保在多核环境下不会发生竞态条件。自旋锁使用原子操作实现，确保在一个时刻只有一个CPU可以进入被保护的临界区。
- **push_off() 和 pop_off()**：关闭和恢复中断，确保在关键区段内不会被中断打断，防止死锁。

### 信号量

- **sem_init()**：初始化信号量，设置初始值和名称，并初始化内部的自旋锁。
- **sem_wait()**：减少信号量的计数，如果计数小于零，当前任务会被阻塞并加入等待队列。任务的状态被设置为`BLOCK`，并让出CPU执行其他任务。
- **sem_signal()**：增加信号量的计数，如果计数小于等于零，从等待队列中取出一个任务并将其状态设置为`RUNNABLE`，使其可以被调度。

### 模块初始化

- **kmt_init()**：初始化任务管理模块，注册中断处理函数用于保存上下文和执行调度，并初始化自旋锁和`idle`任务。




## 具体实现细节

>任务管理

### 初始化`idle`任务

~~~c
static void switch_boot_pcb() {
    for (int i = 0; i < cpu_count(); i++) {
        ...// initial
    }
}
~~~

初始化`idle`任务，并将每个CPU的当前任务设置为`idle`。`idle`任务是当系统中没有其他可运行任务时，CPU所执行的任务。这个函数确保系统在启动时，每个CPU都有一个默认的任务。

### 上下文保存 (`kmt_context_save`)

~~~c
// Context保存在stack的最底部（kcontext已经做了初始化）
static Context *kmt_context_save(Event ev, Context *ctx) {
    panic_on(ienabled() != false, "应该关中断!");
    panic_on((current->fence1 != FENCE || current->fence2 != FENCE), "stack overflow!");
    if (current->status != BLOCK) current->status = RUNNABLE;
    current->context = ctx;
    return NULL;
}
~~~

`kmt_context_save()`函数在中断或异常发生时被调用，保存当前任务的上下文到任务的`context`字段中。此函数确保在任务被中断时，其执行状态可以被正确恢复。该函数还将任务的状态设置为`RUNNABLE`，以便在下一次调度中能够重新选择该任务执行。

### 任务调度 (`kmt_schedule`)

~~~c
static Context *kmt_schedule(Event ev, Context *ctx) {
    int index = current->id, i = 0;
    while (i < Total_Nr) {
        index = (index + 1) % Total_Nr;
        if (tasks[index]->status == RUNNABLE) break;
        i++;
    }
    if (i == Total_Nr) {  // 使用idle进程
        current = &idle[cpu_current()];
    } else {
        current = tasks[index];
    }
    current->status = RUNNING;
    panic_on((current->fence1 != FENCE || current->fence2 != FENCE), "stack overflow!");
    return current->context;
}
~~~

`kmt_schedule()`函数是任务调度器的核心，实现了轮询调度算法。它通过遍历任务列表，寻找下一个状态为`RUNNABLE`的任务。如果找不到可运行的任务，调度器将选择`idle`任务执行。调度器确保在多任务环境下，每个任务都能公平地获得CPU的执行时间。

> 同步机制

### 自旋锁 (`spinlock_t`)

spinlock参考了xv-6的实现，用于在多线程或多处理器环境中保护共享资源，防止竞态条件的发生。工作原理是通过不断地检查锁的状态，直到锁被释放，而不是将线程挂起，从而避免了线程切换的开销。

1. ~~~c
   struct spinlock {  // 结构体定义
       lock_t val; // 锁的状态
       const char *name; // 名称 用于调试和诊断
       cpu_t *cpu; // 指向当前的CPU 多处理器系统
   };
   ~~~

2. 获取锁 (`spin_lock`)

   ~~~c
   static void spin_lock(spinlock_t *lk) {
       push_off(lk);
       if (holding(lk)) lockPanic(lk, "acquire! lock's name is %s, cur-cpu = %d\n", lk->name, cpu_current());
       while (atomic_xchg(&lk->val, 1))
           ;
       __sync_synchronize();
       lk->cpu = &cpus[cpu_current()]; // 将当前CPU标记为持有锁的CPU。
   }
   ~~~

   - **push_off()**：在获取锁之前，首先调用`push_off()`关闭中断，以确保在关键区段内不会被中断打断，防止死锁。
   - **holding()**：检查当前CPU是否已经持有该锁。如果是，则触发`lockPanic()`函数，提示锁的获取存在问题（如重复加锁）。
   - **atomic_xchg()**：这是一个原子操作，用于尝试将锁的状态从未加锁（0）改为加锁（1）。如果锁已被其他CPU或线程持有（`lk->val`为1），这个操作会不断重复，直到锁被释放。
   - **__sync_synchronize()**：确保在锁定之前，所有的内存操作都已经完成。这是一个内存屏障，用于防止编译器和CPU重新排序内存操作。

3.  **释放锁 (`spin_unlock`)**

   ~~~c
   static void spin_unlock(spinlock_t *lk) {
       if (!holding(lk))
           lockPanic(lk, "Should acquire the lock! current CPU = #%d lk->cpu = %d\n", cpu_current(), lk->cpu);
       lk->cpu = NULL; // 清除当前持有锁的CPU标记。
       __sync_synchronize(); // 再次设置内存屏障，确保在释放锁之前的所有内存操作都已经完成。
       atomic_xchg(&lk->val, 0);
       pop_off(lk);
   }
   ~~~

   - **holding()**：检查当前CPU是否持有锁。如果没有，则触发`lockPanic()`函数，提示锁的释放存在问题（如释放未持有的锁）。
   - **atomic_xchg(&lk->val, 0)**：原子操作将锁的状态从加锁（1）改为未加锁（0），释放锁。
   - 恢复之前的中断状态，如果在获取锁时关闭了中断，那么在释放锁时根据需要重新开启中断。

4. 锁错误处理 (`lockPanic`)

   ~~~c
   void lockPanic(spinlock_t *lk, const char *fmt, ...) {
       char out[2048];
       va_list va;
       va_start(va, fmt);
       vsprintf(out, fmt, va);
       va_end(va);
       putstr(out);
       panic("lock 的错误：异常退出!");
   }
   ~~~

   - `lockPanic()`：用于在自旋锁操作中检测到错误时输出调试信息，并终止程序运行。它是一个调试工具，用于帮助开发者识别锁相关的错误。

### 信号量 (`sem_t`)

信号量（`Semaphore`）用于管理并发资源访问的同步机制。通过信号量，可以控制多个线程对共享资源的访问，防止竞态条件的发生。

当信号量的计数值大于零时，表示有可用资源，任务可以直接访问资源，信号量的值减少。

当信号量的计数值小于或等于零时，表示资源已被占用，任务需要等待。等待的任务会被阻塞，并加入信号量的等待队列中，直到资源可用。

当资源被释放时，通过`sem_signal()`将等待的任务从队列中取出，并将其状态设置为`RUNNABLE`，使其能够被调度器重新调度。

- 信号量的初始化 (`sem_init`)

  ~~~c
  static void sem_init(sem_t *sem, const char *name, int value) {
      sem->name = name;
      sem->count = value;  // init
      spin_init(&(sem->lock), NULL);  // 初始化与信号量关联的自旋锁
      assert(sem->wait_list == NULL); // 保存由于信号量不可用而被阻塞的任务
  }
  ~~~

- 等待操作 (`sem_wait`)

  ~~~c
  static void sem_wait(sem_t *sem) {
      int Flag = 0;
      spin_lock(&(sem->lock));
      sem->count--;
      if (sem->count < 0) {                   // 说明当前的线程不能够继续执行
          atomic_xchg(&(current->block), 1);  // 当前任务的状态修改为不可执行
          current->status = BLOCK;
          current->next = sem->wait_list;
          sem->wait_list = current; // 当前任务加入信号量的等待队列
          Flag = 1; // 表及需要进行context切换
      }
      spin_unlock(&(sem->lock));
      if (Flag) {
          panic_on(ienabled() == false, "不应该关中断!");
          yield();  // 切换到其他进程
      }
  }
  ~~~

  - **信号量递减 (sem->count--)**：每次调用`sem_wait()`时，信号量的计数值`sem->count`会减少1，表示请求占用一个资源。

  - **判断资源是否可用 (if (sem->count < 0))**：如果信号量的值小于零，意味着当前没有可用资源，当前任务需要等待。

  - **阻塞当前任务**：

    **atomic_xchg(&(current->block), 1)**：将当前任务的`block`状态设置为1，表示任务已被阻塞。

    **current->status = BLOCK**：将当前任务的状态设置为`BLOCK`，表示任务已被阻塞，不能继续执行。

    **current->next = sem->wait_list**：将当前任务加入信号量的等待队列`sem->wait_list`中。

    **Flag = 1**：标记需要进行上下文切换。

  - **释放锁 (spin_unlock(&(sem->lock)))**：释放信号量的自旋锁，使得其他任务可以访问信号量。

  - **上下文切换 (yield())**：如果当前任务被阻塞，调用`yield()`进行上下文切换，将CPU的控制权交给其他任务。

- 信号量释放 (`sem_signal`)

  ~~~c
  static void sem_signal(sem_t *sem) {
      spin_lock(&(sem->lock));
      sem->count++; // ++
      if (sem->count <= 0) {  // 说明原先的count严格小于 0
          assert(sem->wait_list); // 确保等待队列中有任务等待
          task_t *head = sem->wait_list;
          sem->wait_list = head->next; // 更新等待队列
          atomic_xchg(&(head->block), 0);  // 将当前的任务的状态修改为可执行
          head->status = RUNNABLE;
      }
      spin_unlock(&(sem->lock));
  }
  ~~~

  - **信号量递增 (sem->count++)**：每次调用`sem_signal()`时，信号量的计数值`sem->count`会增加1，表示释放一个资源。
  - **检查等待队列 (if (sem->count <= 0))**
  - **释放锁 (spin_unlock(&(sem->lock)))**



## 一些实验经历

最开始的思路是来自`thread-os`，从这个基础框架上尝试进行一些修改。同时，对于同步机制的实现，自旋锁和信号量，参考了xv-6的实现。

这可以说是遇到的最困难的一个实验了，综合了各种可能出BUG的项目，pmm，kmt，多处理器综合在一起，需要小心翼翼的实现每一步，每一处细节没有考虑好就会在程序运行到某个地方产生报错，不断的disgard changs回滚项目，各种莫名其妙的error。

感觉实验的主要难度来自于并发，多处理器编程导致一切共享的数据都可能被同时访问，这就导致各种非常令人疑惑的BUG。需要不断的调试，追踪才能知道究竟是哪个地方产生了冲突(甚至有可能直接将Linux直接error掉)。

除了不间断的BUG，由于缺少了online judge，除了在给出的测试样例之外，还需要手动增加测试用例，检验程序的正确性和性能。不过幸好老师已经给出了一些做测试的思路，沿着老师的思路，借助python也是顺利将test完成了。



















