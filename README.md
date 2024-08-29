# M5: File Recovery (frecov)

> 格式化磁盘的图片数据恢复

## 实验目的和要求

**目的**：

- 了解数据恢复相关知识
- 了解FAT文件系统
- 实现PBM数据恢复

**要求**：**恢复PMB文件**

- 在规定的时间内尽可能多的恢复更多的图片文件(需要Online Judge)
- 对于每个恢复出的文件，输出第一个字符串是该文件的 SHA1 fingerprint，接下来输出图片的文件名





## 实验环境

- 操作系统Linux ：使用Windows `wsl` (Windows Subsystem for Linux)运行的`Ubuntu  22.04`系统
- 编译器GCC  调试器GDB
- 构建和编译程序：`Makefile`





## 实验原理

### 数据恢复的可能性

对于不了解文件系统的人来说有一个经典的问题：为什么下载一个文件需要那么长时间，但是删除确非常快？这就涉及到了磁盘的格式化的原理。

把存储设备上的文件系统看成是一个数据结构 (例如，不妨假设成是我们熟悉的二叉树)，不难理解我们只要破坏数据结构的 “根节点”，也就是

~~~c
root->left = root->right = NULL;
~~~

数据结构的其他部分也就永久地丢失了——数据结构就完成了一次完美的 “内存泄漏”。

紧接着就带来了一个问题：快速格式化 (指针赋值) 也意味着我们能够通过遍历存储设备 (内存) 的方式把数据结构找回来：

~~~c
root->left = find_left();
root->right = find_left();
~~~

因此，我们可以通过 “扫描磁盘” 的方式一定程度地恢复出文件系统中已经被删除的文件。



### FAT32文件系统相关概述

本次实验采用的样例镜像为FAT32文件系统，需要了解一定的相关知识方便实现数据恢复。

FAT32文件系统由四个主要部分组成：

- **引导区（Boot Sector）**：包含了与文件系统有关的重要信息，如每个簇的大小、保留扇区的数量、FAT表的数量和大小等。
- **FAT表（File Allocation Table）**：记录了每个簇的使用情况和文件在磁盘上的分布情况。
- **根目录区（Root Directory）**：保存了文件和目录的相关信息，如文件名、文件大小、起始簇号、创建时间等。
- **数据区（Data Region）**：存储实际文件数据的区域。

当用户删除文件时，文件的实际数据并未从磁盘上清除。相反，系统只是在FAT表和目录项中将该文件的首字母标记为删除（通常将文件名的第一个字节改为0xE5），并将文件所在的簇标记为可用。这意味着，只要删除后的簇未被新的数据覆盖，文件仍然可以被恢复。

文件恢复的关键在于扫描FAT表和目录区，识别那些标记为删除但数据尚未被覆盖的文件，并重建它们的文件名和簇链。程序可以通过读取根目录区和FAT表，查找符合特定条件的目录项（如文件类型为BMP的文件），并重新将这些簇链接在一起，从而实现文件的恢复。

### BMP文件格式

BMP是一种不经过压缩的图像文件格式。在数据恢复的过程中，需要识别并解析一下这些头部信息

- BMP文件头（位于文件开始部分）包含了文件类型、文件大小等基本信息。
- BMP信息头包含了图像宽度、高度、色深、压缩方式等详细信息。

除此之外，格式化和删除操作可能导致文件数据碎片化，分散存储在磁盘的不同位置。恢复BMP文件时，必须正确处理碎片化情况，重建完整的文件数据。



## 实验步骤

### 格式化磁盘镜像

实验中使用~/M5-frecov.img作为磁盘镜像，挂载到 ~/mnt/后可以进行查看。

在非挂载的情况下，执行以下命令进行磁盘格式化：

~~~
sudo mkfs.fat -v -F 32 -S 512 -s 8
~~~

之后便可以尝试进行磁盘的数据恢复了。

### 初始化文件映射

- 首先，程序接收一个命令行参数，该参数是待恢复的磁盘映像文件名。程序通过`open()`函数打开该文件，并使用`mmap()`函数将整个磁盘映像文件映射到内存中。这使得程序能够直接访问磁盘映像中的数据。
- 程序接着通过`struct fat_header`结构体解析FAT文件系统的引导扇区，确保该映像文件符合FAT文件系统的格式（通过检查`Signature_word`字段是否为0xAA55）。

### 确定数据区的起始位置

- 根据FAT文件系统的引导扇区信息，计算出数据区的起始位置。数据区包含了实际的文件数据和根目录信息。起始位置由保留扇区数、FAT表扇区数和根目录起始簇号等信息计算得到。

### 遍历目录项

- 程序通过遍历数据区的目录项（`struct DIR`结构体）来寻找可能的文件。这些目录项包含了文件的基本信息，例如文件名、文件属性、起始簇号等。
- 程序首先检查目录项的有效性，通过跳过目录项数据为0x00或0xE5的项（这通常表示目录项无效或已删除），以及跳过长文件名的目录项（通过检查`DIR_Attr`字段是否为0x0F）。

### 识别并恢复BMP文件

- 如果检测到目录项中的文件扩展名字段为"BMP"，程序会进一步检查文件名，以确定是短文件名还是长文件名
  - 对于短文件名，程序直接从目录项中提取文件名并将其打印出来。
  - 对于长文件名，程序通过倒推前面的目录项来组装完整的文件名。长文件名由多个目录项组成，程序通过逐个检查这些目录项的`LDIR_Name1`、`LDIR_Name2`和`LDIR_Name3`字段，拼接出完整的文件名。
- 最终，程序将恢复的文件名打印出来，并输出与之对应的SHA值（这里的SHA值是程序中定义的一个常量，用于标识恢复的文件）。




## 具体实现细节

### 获得需要的磁盘镜像

- 创建文件系统镜像，`mkfs.fat`工具

  ```
  $ cat /dev/zero | head -c $(( 1024 * 1024 * 64 )) > fs.img
  ```

- 在空间文件中创建 FAT-32 文件系统

  ~~~
  $ mkfs.fat -v -F 32 -S 512 -s 8 fs.img
  mkfs.fat 4.1 (2017-01-24)
  WARNING: Not enough clusters for a 32 bit FAT!
  fs.img has 64 heads and 32 sectors per track,
  hidden sectors 0x0000;
  logical sector size is 512,
  using 0xf8 media descriptor, with 131072 sectors;
  drive number 0x80;
  filesystem has 2 32-bit FATs and 8 sectors per cluster.
  FAT size is 128 sectors, and provides 16348 clusters.
  There are 32 reserved sectors.
  Volume ID is 6f71a2db, no volume label.
  ~~~

  使用file命令可以看到镜像被正确格式化

  ~~~
  $ file fs.img
  fs.img: DOS/MBR boot sector, code offset 0x58+2, OEM-ID "mkfs.fat", sectors/cluster 8, Media descriptor 0xf8, sectors/track 32, heads 64, sectors 131072 (volumes > 32 MB), FAT (32 bit), sectors/FAT 128, serial number 0x166d2b7d, unlabeled
  ~~~

- 挂载文件系统

  ~~~
  mount fs.img /mnt
  ~~~

  在文件系统中多次重复增加pmb图片，删除bmp图片的操作，便可以获得一个存在一定碎片的文件系统。

- 执行卸载umount之后，进行快速格式化

  ~~~
  $ mkfs.fat -v -F 32 -S 512 -s 8 fs.img
  ~~~

### 文件映射与FAT文件系统的头部解析

~~~c
int fd = open(filename, O_RDONLY);
assert(fd != -1);
struct stat buf;
fstat(fd, &buf);
void *fat_fs = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
assert(fat_fs != MAP_FAILED);
struct fat_header *header = fat_fs;
assert(header->Signature_word == 0xaa55);
~~~

- 首先打开磁盘映像文件并检查是否成功打开（`assert(fd != -1)`）。随后，通过`fstat`函数获取文件的大小信息，并使用`mmap`函数将文件内容映射到内存中。
- `mmap`函数的使用允许程序通过指针直接访问磁盘映像中的数据，而不需要逐字节读取文件。
- `fat_fs`指向的是整个磁盘映像的起始位置，通过将其强制转换为`struct fat_header`结构体指针，可以访问并解析FAT文件系统的引导扇区信息。
- 通过检查`header->Signature_word == 0xaa55`，程序确保该磁盘映像确实是一个有效的FAT文件系统。

### 计算数据区的起始位置

~~~c
void *data_begin = (void *)(intptr_t)((header->BPB_RsvdSecCnt + header->BPB_NumFATs * header->BPB_FATSz32 + (header->BPB_RootClus - 2) * header->BPB_SecPerClus) * header->BPB_BytsPerSec);
~~~

- 这里的`data_begin`计算出了数据区的起始位置，这个位置标志着FAT文件系统中真正存储文件数据的区域。
- `header->BPB_RsvdSecCnt`：保留扇区数，标识了从文件系统起始到第一个FAT表之间的扇区数量。
- `header->BPB_NumFATs * header->BPB_FATSz32`：所有FAT表占用的总扇区数。
- `(header->BPB_RootClus - 2) * header->BPB_SecPerClus`：数据区起始位置的相对偏移，这里减去2是因为根目录起始簇号通常从2开始。
- 最后乘以`header->BPB_BytsPerSec`将结果转换为字节偏移量。

### 倒推恢复长文件名

~~~c
if (dir->data[6] == '~') {
    memset(long_name_buf, 0, sizeof(long_name_buf));
    long_name_lenth = 0;
    struct fat_long_dir *long_dir = (struct fat_long_dir *)(dir - 1);
    while (long_dir->LDIR_Attr == 0x0F) {
        // 拼接长文件名
        if (getbit(long_dir->LDIR_Ord, 6) == 1) {  // 检测长文件名结束
            printf("%s %s\n", sha, long_name_buf);
            break;
        }
        long_dir--;
    }
}
~~~

- 当检测到短文件名目录项中包含"~"符号时，程序认为这是一个长文件名的缩写，并通过倒推前面的目录项来恢复完整的长文件名。
- 通过检查`long_dir->LDIR_Attr`是否为`0x0F`来确定当前目录项是长文件名的一部分。程序将每个长文件名片段拼接到`long_name_buf`中，直到找到长文件名的结束标记（通过检查`LDIR_Ord`中的第六位）。最终恢复的长文件名与SHA值一起被打印输出。



## 可能存在的问题

### 恢复成功率的检测

长文件名的恢复逻辑依赖于多个目录项的顺序和完整性。如果文件系统中的这些目录项因格式化或其他原因受损，恢复过程可能失败。

但是关于恢复内容是否准确，我们目前并没有一个很好的检测方法，只能从原理上来说，从目录相中检测的文件名和相关信息是正确的。

对于进一步的优化，可能需要在恢复长文件名时，添加校验机制以确保目录项的完整性，并提供错误报告或日志功能，以便了解恢复失败的原因。

### 内存管理

在扫描文件系统时，当前代码逐一检查每个目录项，这在处理大容量磁盘时可能会导致性能瓶颈，这也是为什么Online Judge会要求运行时间。

同时，实验中对于图片大小进行了约定，也就是说，对于一些体量相差较大的文件恢复，实验中并没有给出相应的优化，可能导致在扫描时进行许多不必要的工作。



## 实验中遇到的一些问题和解决方法

### 挂载镜像文件

由于使用wsl中运行的Linux系统，导致原有的Windows系统中的磁盘全部被镜像到了/mnt下面，无法使用正常的手段对于磁盘文件进行挂载。

首先尝试在Windows下进行挂载，但是遇到了无法正常打开文件的问题(文件本身完好，至今还不知道原因)。在Linux下可以正常挂载，但使用sudo命令强制挂载会导致Windows的磁盘镜像无法连接。

最终解决方法是，尝试挂载到自定义文件夹，并手动对磁盘镜像文件分配loop设备

~~~
$ sudo losetup -f
$ sudo losetup /dev/loop0 ~/M5-frecov.img
$ sudo mount /dev/loop0 ~/mnt
$ sudo umount ~/mnt
$ sudo losetup -d /dev/loop0
~~~

这样便可以正常进行挂载，查看文件。

### 文件名乱码问题

由于不同操作系统和环境之间的编码差异，在扫描文件系统时，出现了文件名乱码的情况，尤其是对于一些中文编码的问题，使得问题较为复杂。

解决的方法，处于可实现的角度，文件系统的文件名明确为utf-8编码，但是中文名称有设计到了GBK编码，曾经尝试使用合适的字符集转库进行转换，但是经过尝试实现较为麻烦。最终选择统一为utf-8编码进行输出，当然这导致了一个不好的后果，最终的文件名可能不具有实际意义。


