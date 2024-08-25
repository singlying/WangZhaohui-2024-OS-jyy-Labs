#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>    //POSIX操作系统API（如 getopt）
#include <sys/stat.h>  //文件属性和类型的操作
#include <sys/types.h> //文件属性和类型的操作
#include <dirent.h>    //目录操作
#include <string.h>
#include <getopt.h> //解析命令行选项

typedef struct
{
  char name[1024];
  pid_t pid;
  pid_t *children;
  int child_num;
} proc; // 定义 进程

// 函数PrintTree用于打印进程树
// 参数p为当前进程，procs为进程数组，depth为当前深度，pflag为是否打印pid的标志
void PrintTree(proc *p, proc *procs, int depth, int pflag)
{
  // 如果当前深度大于0，则打印空格和竖线
  if (depth > 0)
  {
    printf("\n%*s |\n", (depth - 1) * 4, "");
    printf("%*s", (depth - 1) * 4, "");
    printf(" +--");
  }
  // 打印当前进程的名称
  printf("%s", p->name);
  // 如果pflag为真，则打印当前进程的pid
  if (pflag)
  {
    printf("(%d)", p->pid);
  }
  // 遍历当前进程的子进程
  for (int i = 0; i < p->child_num; ++i)
  {
    // 在进程数组中查找子进程
    proc *tmp = procs;
    while (tmp != NULL)
    {
      if (tmp->pid == p->children[i])
      {
        break;
      }
      ++tmp;
    }
    // 递归调用PrintTree函数，打印子进程
    PrintTree(tmp, procs, depth + 1, pflag);
  }
}

// 比较函数，用于根据pid对进程数组进行排序
int compare_procs(const void *a, const void *b)
{
  proc *proc_a = (proc *)a;
  proc *proc_b = (proc *)b;
  return proc_a->pid - proc_b->pid;
}

int main(int argc, char *argv[])
{
  // Get the arguments.
  char c;
  int nflag = 0, pflag = 0, vflag = 0;
  struct option long_options[] = {
      {"show-pids", no_argument, &pflag, 1},
      {"numeric-sort", no_argument, &nflag, 1},
      {"version", no_argument, &vflag, 1},
      {0, 0, 0, 0}};

  while ((c = getopt_long(argc, argv, "npV", long_options, NULL)) != -1)
  {
    switch (c)
    {
    case 'n':
      nflag = 1;
      break;
    case 'p':
      pflag = 1;
      break;
    case 'V':
      vflag = 1;
      break;
    case 0:
      break;
    case '?':
      printf("Unknown option: %s\n", argv[optind - 1]);
      return 1;
    default:
      printf("Unexpected option: %c\n", c);
      return 1;
    }
  }

  // 如果是-v 直接退出
  if (vflag)
  {
    printf("Linux pstree, version 1.0.0\n");
    return 0;
  }

  // Read max pid.
  FILE *pid_max_f = fopen("/proc/sys/kernel/pid_max", "r");
  assert(pid_max_f != NULL);
  if (pid_max_f == NULL)
  {
    perror("No pid_max or it can not be visitted");
    return -1;
  }
  int pid_max;
  fscanf(pid_max_f, "%d", &pid_max);
  fclose(pid_max_f);

  char *base_path = "/proc";
  struct dirent *dent;
  DIR *srcdir = opendir(base_path);
  if (srcdir == NULL)
  {
    perror("Open fail");
    return -1;
  }

  // Proc array.
  proc *procs;
  procs = malloc(sizeof(proc) * (pid_max + 1));
  assert(procs != NULL);
  pid_t *ppids;
  ppids = malloc(sizeof(pid_t) * pid_max); // 存储父进程pid
  assert(ppids != NULL);
  proc *p = procs;

  // Read proc directory.
  while ((dent = readdir(srcdir)) != NULL)
  {
    // 如果名称不是数字起始，说明不是proc文件
    if (dent->d_name[0] < '0' || dent->d_name[0] > '9')
      continue;

    // Save the pids.
    p->pid = atoi(dent->d_name);

    // Read the stat to get name and ppid.
    char path[13 + strlen(dent->d_name) + 1], buf[1024];
    memset(path, 0, sizeof(path));
    strcat(path, base_path);
    strcat(path, "/");
    strcat(path, dent->d_name);
    strcat(path, "/status");
    FILE *f = fopen(path, "r"); // proc/[PID]/status
    while ((fscanf(f, "%s", buf) != EOF))
    {
      if (strcmp(buf, "Name:") == 0)
      {
        fscanf(f, "%s", p->name);
      }
      if (strcmp(buf, "PPid:") == 0)
      {
        fscanf(f, "%d", &ppids[p - procs]);
      }
    }
    fclose(f);
    ++p;
  }
  int proc_count = p - procs;
  printf("Total proc: %d\n", proc_count);

  // 如果 -n 参数被设置，则按PID排序
  if (nflag)
  {
    qsort(procs, proc_count, sizeof(proc), compare_procs);
  }

  // Build the tree.
  for (int i = 0; i < proc_count; ++i)
  {
    procs[i].children = malloc(sizeof(pid_t) * proc_count);
    procs[i].child_num = 0;
  }

  for (int i = 0; i < proc_count; ++i)
  {
    for (int j = 0; j < proc_count; ++j)
    {
      if (ppids[j] == procs[i].pid)
      {
        procs[i].children[procs[i].child_num++] = procs[j].pid;
      }
    }
  }

  for (int i = 0; i < proc_count; ++i)
  {
    if (ppids[i] == 0)
    {
      PrintTree(&procs[i], procs, 0, pflag);
    }
  }
  printf("\n");

  closedir(srcdir);
  return 0;
}