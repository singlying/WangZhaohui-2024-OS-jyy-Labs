# M1:打印进程树(pstree)

> 实现Linux下的 `pstree`命令

## 实验目的

- 了解Linux进程管理和进程树结构

- 使用C语言读取Linux系统进程信息

- 实现一个模拟`pstree`命令的程序

  ​

## 实验环境

- 操作系统Linux ：使用Windows `wsl` (Windows Subsystem for Linux)运行的`Ubuntu`系统
- 编译器GCC  调试器GDB
- 构建和编译程序：`Makefile`



## 实验原理

### 进程树pstree

​	进程通过派生产生新的进程，进程之间也存在着父子关系，所有的进程组合在一起构成了进程树的概念。

Linux系统中`pstree`命令可以打印当前进程树。

> pstree[OPTION]

- `-p` 或 `--show-pids`: 打印每个进程的进程号。
- `-n` 或 `--numeric-sort`: 按照pid的数值从小到大顺序输出一个进程的直接孩子。
- `-V` 或 `--version`: 打印版本信息。

### Linux    /proc

​	`procfs`进程文件系统提供了Linux系统中的进程信息。对于某个进程号[PID]的进程，打开/proc/[PID]目录，该目录下保存了与该进程相关的信息。

​	实验中需要获取每个进程的名称，进程号，父进程等信息，构建进程树并打印。



## 实验步骤

### 代码实现

- 读取`/proc`目录中的进程信息并进行存储
- 根据进程的父子关系构建进程树
- 打印进程树

### Makefile编写

- 编写`Makefiel`来实现自动化的程序编译和执行，以及后续的调试运行



## 具体实现细节

### 获取命令行参数

​	main函数的参数是进程"初始状态的一部分"，由操作系统负责将其存储在内存中。在C语言中，直接访问main函数的参数即可实现对指令附带参数的访问，下面是一个实例：

~~~c
#include <stdio.h>
#include <assert.h>
#include <getopt>  //处理参数需要调用的库

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]); // C 标准保证
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]); // C 标准保证
  return 0;
}
~~~

### 获取proc信息

​          使用C库函数`opendir`打开 `/proc` 目录，读取 `/proc` 目录下的每个子目录

~~~c
while ((dent = readdir(srcdir)) != NULL)
{
    if (dent->d_name[0] < '0' || dent->d_name[0] > '9')
        continue;
    // Save the pids.
    p->pid = atoi(dent->d_name);

~~~

​        程序通过检查目录名是否以数字为开头，以此为标准判断是否为进程目录。

~~~c
while ((fscanf(f, "%s", buf) != EOF))
{
    if (strcmp(buf, "Name:") == 0)
        fscanf(f, "%s", p->name);   //进程名称
    if (strcmp(buf, "PPid:") == 0)
        fscanf(f, "%d", &ppids[p - procs]);  //父进程信息
}
~~~

​        打开 `/proc/[PID]/status` 文件的路径，从中读取进程的详细信息。

### 构建进程树并打印

​        通过维护两个数组`procs`和`ppids`来保存对应的父子进程信息。

~~~c
for (int i = 0; i < proc_count; ++i)
    for (int j = 0; j < proc_count; ++j)
        if (ppids[j] == procs[i].pid)
            procs[i].children[procs[i].child_num++] = procs[j].pid;
~~~

​	打印进程树使用递归实现，从根目录开始逐层打印。



## 可能存在的问题












