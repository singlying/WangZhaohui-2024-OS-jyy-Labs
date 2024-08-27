# L1:hello,bare-metal!

> 为计算机硬件编程

## 实验目的

- 在 AbstractMachine 上进行计算机硬件编程的过程和实践

- 无操作系统环境下直接与硬件交互的挑战与学习价值

- 了解图片文件结构相关知识

  ​

## AbstractMachine

> Bare-Metal : 在没有操作系统介入的情况下，直接与硬件进行交互的编程方式。

### 概念

`抽象计算机`是 是裸机上的 C 语言运行环境，提供 5 组 (15 个) 主要 API，可以实现各类系统软件 (如操作系统)：

### 基本组件

### 实现方式




## 开发环境

### 配置环境

## 实验步骤

 ### 图片加载到屏幕展示

  ### 优化内存管理



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












