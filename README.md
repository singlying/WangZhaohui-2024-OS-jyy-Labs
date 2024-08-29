# M3: 系统调用 Profiler (sperf)

> 实现统计程序系统调用时间占比的工具

## 实验目的和要求

**目的**：

- 加深对分析状态机执行轨迹的工具的认识
- 深入了解系统调用

**要求**：**打印系统调用和时间占比**

- 实现命令 sperf COMMAND [ARG]

  运行COMMAND命令，并为COMMAND命令传入ARG参数，之后统计命令执行的系统调用所占的时间

- sperf 假设 `COMMAND` 是单进程、单线程的，无需处理多进程和多线程的情况

- 实现32-bit和64-bit版本

  ​



## 实验环境

- 操作系统Linux ：使用Windows `wsl` (Windows Subsystem for Linux)运行的`Ubuntu  22.04`系统
- 编译器GCC  调试器GDB
- 构建和编译程序：`Makefile`





## 实验原理

### 系统调用

系统调用是操作系统提供给用户程序与内核进行交互的一种机制。用户程序无法直接访问硬件资源，如文件、内存、网络接口等，它们必须通过系统调用请求操作系统内核提供的服务。系统调用通常包括文件操作、进程控制、内存管理、设备控制等。

本实验需要实现对于系统调用的跟踪和分析，需要通过一定的手段获取到系统调用的相关信息。

### `strace`工具

在Linux系统中，获取进程执行时产生的系统调用信息通常使用`strace`工具。`strace`能够跟踪并记录指定命令执行过程中涉及的所有系统调用及其参数、返回值和耗时等信息。

通过man指令，我们可以了解到strace的相关信息：

~~~
$ man strace
SYNOPSIS
       strace [-ACdffhikqqrtttTvVwxxyyzZ] [-I n] [-b execve] [-e expr]... [-O overhead] [-S sortby] [-U columns]
              [-a column] [-o file] [-s strsize] [-X format] [-P path]... [-p pid]... [--seccomp-bpf] { -p pid |
              [-DDD] [-E var[=val]]... [-u username] command [args] }

       strace -c [-dfwzZ] [-I n] [-b execve] [-e expr]... [-O overhead] [-S sortby] [-U columns] [-P path]...
              [-p pid]... [--seccomp-bpf] { -p pid | [-DDD] [-E var[=val]]... [-u username] command [args] }
~~~

### 正则表达式获取系统调用信息

从 `strace` 获取的输出包含了大量的信息，我们需要从中提取出**系统调用名称**和**执行时间**这两个关键数据。为此，我们使用了正则表达式来匹配和提取所需的信息。

~~~
#include <regex.h>
~~~

正则表达式有着很高的效率和灵活性，使程序能够高效地处理文本数据，提取出符合特定模式的信息。



## 实验步骤

在终端中运行生成的 `sperf` 可执行文件，并传入需要分析的命令。例如，分析 `ls` 命令：

~~~
./sperf ls
~~~

### 解析和提取系统调用文件

- 程序中首先使用正则表达式从 `strace` 的输出中提取系统调用的名称。提取的系统调用名称将被存储在一个动态分配的字符数组中。
- 接下来，通过另一个正则表达式提取每个系统调用的执行时间。执行时间以秒为单位，形如 `<0.001>`。
- 对于每一行 `strace` 输出，程序检查当前系统调用名称是否已经存在于结构体数组 `infos` 中。如果存在，则累加执行时间；如果不存在，则创建新的结构体元素，并记录系统调用的名称和执行时间。

### 结果处理和排序

- 程序将所有系统调用按照执行时间从大到小排序，并选择执行时间最长的前十个系统调用。
- 对这些系统调用，计算它们占总执行时间的百分比，并将结果输出到终端。

### 输出结果

**结果输出**：

- 程序在终端显示执行时间最长的前十个系统调用，输出格式为：

  ```scss
  syscall_name, time_taken, (percentage%)
  ```

- 每条记录显示系统调用的名称、累积执行时间以及占总执行时间的百分比。



## 具体实现细节

### 提取系统调用信息

- 提取系统调用名称

  ```regex
  [a-z0-9]+\(
  ```

  - `[a-z0-9]+`：匹配一个或多个由小写字母和数字组成的字符串，即系统调用的名称。
  - `\(`：匹配一个左括号，表示系统调用名称之后跟随的参数列表的开始。

  ```c
  char* get_syscall(const char* line) {
      regex_t re;
      regcomp(&re, "[a-z0-9]+\\(", REG_EXTENDED);
      regmatch_t match;
      char* buf = NULL;
      if (regexec(&re, line, 1, &match, 0) == 0) {
          buf = (char*)malloc((match.rm_eo) * sizeof(char));
          strncpy(buf, line, match.rm_eo - 1);
          buf[match.rm_eo - 1] = '\0';
      }
      regfree(&re);
      return buf;
  }
  ```

  1. **编译正则表达式**：使用 `regcomp` 函数将正则表达式模式编译为正则表达式对象 `re`。

  2. **执行匹配**：使用 `regexec` 函数在给定的字符串 `line` 中执行匹配操作。

  3. 提取结果

     - 如果匹配成功，`match` 结构体将包含匹配到的子串的起始和结束位置。
     - 使用 `strncpy` 将匹配到的系统调用名称复制到新分配的字符串 `buf` 中，并添加字符串结束符 `\0`。

  4. **释放资源**：使用 `regfree` 释放编译的正则表达式对象。

     ​

- 提取系统调用执行时间

- ```regex
  <0\.[0-9]+>
  ```

  - `<` 和 `>`：精确匹配尖括号，表示执行时间被包含在这两个符号之间。
  - `0\.[0-9]+`：匹配一个以 `0.` 开头，后跟一个或多个数字的字符串，即表示系统调用的执行时间，精确到微秒级别。

  ```c
  double get_time(const char* line) {
      regex_t re;
      regcomp(&re, "<0\\.[0-9]+>", REG_EXTENDED);
      regmatch_t match;
      char buf[16]; // 假设时间字符串长度不会超过15
      if (regexec(&re, line, 1, &match, 0) == 0) {
          strncpy(buf, line + match.rm_so + 1, match.rm_eo - match.rm_so - 2);
          buf[match.rm_eo - match.rm_so - 2] = '\0';
          regfree(&re);
          return atof(buf);
      }
      regfree(&re);
      return 0.0;
  }
  ```

  1. **编译正则表达式**：使用 `regcomp` 编译正则表达式模式。
  2. **执行匹配**：在输入字符串 `line` 中执行匹配操作。
  3. 提取结果：
     - 如果匹配成功，使用 `strncpy` 将匹配到的时间字符串（去掉尖括号）复制到缓冲区 `buf` 中。
     - 使用 `atof` 将字符串转换为 `double` 类型的浮点数，表示系统调用的执行时间。
  4. **释放资源**：使用 `regfree` 释放编译的正则表达式对象。

### 创建子进程

~~~c
int pid = fork();

if (pid == 0) {
  close(pipefd[0]);
  for (char* str = path; ; str = NULL) {
    char* token = strtok(str, ":");
    if (!token) {
      break;
    }
    // Try to execve with token + "/strace"
    char* filename = (char*)malloc(sizeof(char) * strlen(token) + strlen(strace_str) + 2);
    strcpy(filename, token);
    strcat(filename, "/");
    strcat(filename, strace_str);
    dup2(pipefd[1], fileno(stderr));
    execve(filename, exec_argv, envp);
    fflush(stderr);
    free(filename);
  }
} else {
  close(pipefd[1]);
  FILE* file = fdopen(pipefd[0], "r");
  if (file == NULL) {
    exit(EXIT_FAILURE);
  }
  // ... 读取并处理子进程输出
}
~~~

- **功能**：主函数使用 `fork` 创建一个子进程，在子进程中通过 `execve` 调用 `strace`，并将 `strace` 的输出通过管道传回父进程处理。
- 流程：
  1. **fork 创建子进程**：父进程继续执行读取操作，子进程通过 `execve` 执行 `strace`。
  2. **execve 调用 strace**：子进程通过 `strtok` 解析 `PATH` 环境变量，并逐一尝试执行 `strace`。
  3. **管道通信**：通过 `pipefd` 创建的管道，子进程的 `stderr` 被重定向到管道的写端，父进程从读端读取子进程的输出。
  4. **文件指针 fdopen**：父进程使用 `fdopen` 将管道的文件描述符转换为文件指针，以便更方便地读取 `strace` 的输出并进行处理。



## 可能存在的问题

### **strace 与被追踪程序的 stderr 冲突**：

在实验中，`strace` 默认会将追踪输出发送到标准错误（stderr）。如果被追踪的程序也向标准错误输出内容，可能会导致 `strace` 输出与程序输出混淆，从而破坏 `strace` 的追踪结果。这种冲突可能会影响到系统调用信息的准确解析。例如，如果被追踪的程序输出的内容与 `strace` 追踪的信息混在一起，可能导致在解析系统调用时得到错误的结果。

**解决方法**：在实验中已经通过将程序的标准错误重定向到 `/dev/null` 的方式，避免了被追踪程序的输出干扰 `strace` 的追踪结果。这样不会产生信息混淆，得到错误的系统调用。

### 解析 strace 输出中的字符串问题

当使用正则表达式解析 `strace` 输出中的系统调用和时间信息时，可能会遇到一些特殊情况。例如，某些输出可能会包含当前正在试图匹配的模式，例如 `", 1) = 100 <99999.9>`，这些内容可能会影响到解析的准确性，导致程序在提取系统调用名称和时间时出错。

目前的实验代码并未完全解决这一问题。如果被追踪程序输出的内容包含与 `strace` 输出格式相似的字符串（如 `", 1) = 100 <99999.9>`），可能会导致解析的错误。要解决这个问题，可能需要更复杂的正则表达式或额外的逻辑来区分和处理 `strace` 输出与被追踪程序的输出，或者采用一些别的实现方法

### 一些极端情况

实验中假设 `strace` 的输出格式是标准的，但在某些极端情况下，例如输出被刻意构造为类似 `strace` 的格式，可能会导致解析误差。这类极端情况虽然不常见，但仍可能会影响到实验结果的准确性。

比如如下的输出：

~~~
$ strace -T echo -n '", 1) = 100 <99999.9>' > /dev/null
ioctl(1, TCGETS, 0x7fff80723570)        = -1 ENOTTY <0.000100>
write(1, "\", 1) = 100 <99999.9>", 21)  = 21 <0.000149>
close(1)   
~~~

如果只试图匹配 `<>` 中的数字，就的确会在一些场景下产生错误。目前的实验代码未专门针对这些极端情况进行处理，因此在面对这些特殊情况时，可能仍会遇到解析错误的问题。






