# M4: C Read-Eval-Print-Loop (crepl)

> 实现一个`C shell`

## 实验目的和要求

**目的**：

- 理解动态编译和动态加载
- 交互式的C shell
- 学会使用动态链接库

**要求**：**实现一个C shell**

- 实现一个简单的C语言Read-Eval-Print-Loop (REPL)系统
- `crepl` - 逐行从 stdin 中输入单行 C 语言代码，并根据输入内容分别处理：
  - 如果输入一个 C 函数的定义，则把函数编译并加载到进程的地址空间中；
  - 如果输入是一个 C 语言表达式，则把它的值输出。
- 约定实验只处理int类型数据和返回值和int类型的函数





## 实验环境

- 操作系统Linux ：使用Windows `wsl` (Windows Subsystem for Linux)运行的`Ubuntu  22.04`系统
- 编译器GCC  调试器GDB
- 构建和编译程序：`Makefile`





## 实验步骤和细节

### 实现REPL主循环

~~~c
int main(int argc, char *argv[]) {
  while (1) {
    printf("crepl> ");
    memset(line,'\0',sizeof(line));
    memset(tmp,'\0',sizeof(tmp));
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    sscanf(line,"%3s",tmp);
    compile(strncmp(line,"int",3)==0);
  }
}
~~~

在本实验中，`main()`函数实现了一个基本的REPL（Read-Eval-Print-Loop）主循环。REPL循环通过以下步骤实现：

1. **输入读取**：程序使用`fgets()`从标准输入读取用户输入，并将其存储在`line`字符数组中。`fgets()`函数能够处理用户输入的整行内容，包括表达式和函数定义。
2. **输入预处理**：使用`sscanf()`函数从输入中提取前三个字符，并存储在`tmp`数组中。这一步用于判断用户输入的是一个函数定义还是一个表达式计算。
3. **调用编译函数**：根据输入的前三个字符，判断是否为函数定义（即以`int`开头的输入）。如果是函数定义，传递`true`参数给`compile()`函数；否则，传递`false`。这一步决定了`compile()`函数的执行逻辑。
4. **循环控制**：在循环的每次迭代结束后，REPL会继续等待用户的下一次输入，直到用户终止输入（例如按下`Ctrl+D`）。

### 代码编译与动态加载

`compile()`函数是REPL的核心，它负责将用户输入的代码动态编译为共享库文件，并加载执行。其步骤如下：

1. **创建临时文件**：`mkstemp()`函数被用来创建两个临时文件，一个用于存储用户输入的C代码（`src_filename`），另一个用于存储编译后的共享库文件（`dst_filename`）。如果文件创建失败，程序会输出错误信息并中断操作。
2. **写入源代码**：根据输入类型（函数定义或表达式），将代码写入临时文件。对于表达式，代码会被包裹在一个返回整数的函数中（`wrap_func`），以便能够在后续步骤中调用。
3. **动态编译**：使用`fork()`创建一个子进程，子进程调用`execvp()`运行GCC命令，将用户输入的代码编译为共享库文件。为了避免不必要的输出，`dup2()`将子进程的标准输出和标准错误重定向到`/dev/null`。
4. **检查编译结果**：父进程等待子进程完成编译，并检查编译状态。如果编译失败，程序会输出相应的错误信息。
5. **动态加载与执行**：
   - 编译成功后，程序使用`dlopen()`动态加载编译好的共享库文件。
   - 如果用户输入的是表达式，使用`dlsym()`获取`wrap_func`函数的地址，并调用该函数执行表达式计算。计算结果通过`printf()`输出。
   - 如果用户输入的是函数定义，程序仅输出函数定义已被添加的信息。
6. **清理工作**：无论操作成功与否，临时文件（源代码文件和共享库文件）都会被删除以避免系统中的临时文件堆积。

### 动态链接与执行

在本实验中，程序使用了POSIX标准的动态链接库（`libdl`）来加载和执行编译后的C代码。以下是具体步骤：

1. **动态链接**：`dlopen()`函数被用来动态加载共享库文件。`RTLD_NOW`标志确保所有符号在加载时立即解析，而`RTLD_GLOBAL`标志允许其他共享库引用此共享库的符号。
2. **获取函数指针**：对于表达式，`dlsym()`函数被用来获取`wrap_func`函数的地址，并将其赋值给函数指针`f`。此时，`f`指向编译后的C函数。
3. **调用并执行**：通过调用函数指针`f()`，程序执行用户输入的表达式。结果通过`printf()`输出。对于函数定义，程序仅提示函数已被添加，不做进一步操作。
4. **错误处理**：如果在动态加载或执行过程中出现错误（例如未能成功加载共享库或未找到目标符号），程序会输出相应的错误信息并中止操作。



## 演示示例

- 计算表达式

~~~bash
crepl> 3 + 5
	Result: 8
~~~

- 函数定义与调用

~~~bash
crepl> int add(int a, int b) { return a + b; }
	Added: int add(int a, int b) { return a + b; }
crepl> add(10, 20)
	Result: 30
~~~

- 编译错误

~~~
crepl> int add(int a, int b) { return a + ; }
      Compile Error!
~~~

在以上的实验中，函数和表达式均可以调用之前定义过的函数，这对于交互式的 C Shell 来说是 “自然” 的需求。当然，为了简化处理，假设函数和表达式都不会访问全局的状态 (变量) 或调用任何库函数，重复定义重名函数并没有做出过多处理。



## 实验总结-心路历程

实验整体较为简单，最重要的一点是如何实现动态编译。

这看上去是一个非常高端的操作，但是因为有非常强大的工具GCC，其实真正实现起来并不高端。

通过将用户输入的代码写入临时文件并调用GCC来编译成共享库文件，可以实现动态编译的目标。其中，如何管理临时文件，我又去了解了Linux临时文件的相关机制，实现了用`mkstemp()`的管理。

之后需要生成共享库动态加载并执行，这依赖于`libdl`库，需要了解`dlopen()`、`dlsym()`以及`dlclose()`函数的使用。

总结，一个看似简单的项目其实用到了非常多没有接触到的知识，各种奇怪/神奇的库，各种Bug，各种新奇的实现一度让我怀疑这是不是C语言。但是Every step counts! 还是最终实现了目标。



## RTFM：参考的手册

![img](https://jyywiki.cn/pages/OS/img/rtfm.jpg)

**WTFM！READ THE FRIENDLY MANUAL!**  来自课程文档中的一张经典图片。

copilot时代，AI生成的代码会给人一种“我也行”的错觉。但是无论如何，在学习阶段，手册都是不可替代的：读一读 dlopen 相关库函数的手册、elf (5)，“遍历” 式的学习可以让学习者不仅了解 “这一个知识”，还可以发散地理解与它相关的概念体系。即便使用了 AI 生成的代码，好好研读和学习也是很有必要的。



1. **GCC手册**
   GNU Compiler Collection (GCC) 官方文档
   URL: [https://gcc.gnu.org/onlinedocs/]()
2. **C语言标准库参考手册**
   《The C Programming Language》 by Brian W. Kernighan and Dennis M. Ritchie
   ISBN: 978-0131103627
3. **POSIX标准文档**
   Portable Operating System Interface (POSIX) 标准
   URL: [https://pubs.opengroup.org/onlinepubs/9699919799/]()
4. **libdl 动态链接库参考**
   Dynamic Linking Library (`libdl`) 使用指南
   URL: [https://man7.org/linux/man-pages/man3/dlopen.3.html]()
5. **Linux System Programming, 2nd Edition** by Robert Love
   O'Reilly Media, 2013
   ISBN: 978-1449339531
6. **Advanced Linux Programming** by CodeSourcery LLC
   New Riders Publishing, 2001
   ISBN: 978-0735710436
7. **UNIX编程环境**
   《The UNIX Programming Environment》 by Brian W. Kernighan and Rob Pike
   ISBN: 978-0139376817