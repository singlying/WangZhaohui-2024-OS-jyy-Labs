#include <stdio.h>   
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <fcntl.h>    // 文件控制选项
#include <assert.h>  
#include <wait.h>     // 进程等待函数
#include <dlfcn.h>    // 动态链接库操作函数
#include <stdbool.h>

// 根据目标机器架构定义编译选项
#if defined(__i386__)
  #define TARGET "-m32"
#elif defined(__x86_64__)
  #define TARGET "-m64"
#endif

// 全局变量
static char line[4096];          // 用于存储用户输入的代码行
static char tmp[4];              // 用于临时存储用户输入的前几个字符
static char src_filename[32];    // 源代码临时文件名
static char dst_filename[32];    // 目标共享库文件名
static int (*f)();               // 函数指针，用于指向动态加载的函数
static int value;                // 存储计算结果

// 编译并执行用户输入的代码
void compile(bool func) {
  // 生成临时文件名
  sprintf(src_filename, "/tmp/func_c_XXXXXX");
  sprintf(dst_filename, "/tmp/func_so_XXXXXX");

  // 创建临时文件
  if (mkstemp(src_filename) == -1) {
    printf("\033[1;31m      Mkstemp Failed!\033[0m\n");
    return;
  }
  if (mkstemp(dst_filename) == -1) {
    printf("\033[1;31m      Mkstemp Failed!\033[0m\n");
    return;
  }

  // 将用户输入写入源代码临时文件
  FILE *fp = fopen(src_filename, "w");
  if (func) {
    fprintf(fp, "%s", line);  // 如果是函数定义，直接写入
  } else {
    fprintf(fp, "int wrap_func(){return (%s);}", line);  // 如果是表达式
  }
  fclose(fp);

  // 准备GCC编译命令
  char *exec_argv[] = {"gcc", TARGET, "-x", "c", "-fPIC", "-w", "-shared", "-o", dst_filename, src_filename, NULL};

  // 创建子进程进行编译
  int pid = fork();
  if (pid == 0) {
    // 重定向输出和错误流到/dev/null
    int fd = open("/dev/null", O_RDWR);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    execvp(exec_argv[0], exec_argv);
  } else {

    int status;
    wait(&status);

    // 检查编译是否成功
    if (status != 0) {
      printf("\033[1;31m      Compile Error!\033[0m\n");
    } else {
      // 动态加载编译后的共享库
      void *handle = dlopen(dst_filename, RTLD_NOW | RTLD_GLOBAL);
      if (!handle) {
        printf("\033[1;31m      Compile Error!\033[0m\n");
      } else {
        if (func) {
          printf("\033[1;32m      Added: \033[1;30m%s\033[0m", line);
        } else {
          f = dlsym(handle, "wrap_func");
          value = f();
          printf("\033[1;32m      Result: \033[1;30m%d\033[0m\n", value);
          dlclose(handle);
        }
      }
    }
  }

  // 删除临时文件
  unlink(src_filename);
  unlink(dst_filename);
}

int main(int argc, char *argv[]) {
  while (1) {
    printf("crepl> ");
    memset(line, '\0', sizeof(line));  // 清空输入缓冲区
    memset(tmp, '\0', sizeof(tmp));    // 清空临时缓冲区
    fflush(stdout);  // 刷新标准输出缓冲区

    if (!fgets(line, sizeof(line), stdin)) {
      break;  // 读取失败，退出循环
    }

    sscanf(line, "%3s", tmp);
    compile(strncmp(line, "int", 3) == 0);
  }

  return 0;
}