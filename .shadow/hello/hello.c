#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

static int w, h; // Screen size

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = {AM_KEYS(KEYNAME)};

static inline void puts(const char *s)
{
  for (; *s; s++)
    putch(*s);
}

// 获取显示长宽信息
void get_display_info()
{
  // 创建一个 AM_GPU_CONFIG_T 结构体来存储 GPU 配置
  AM_GPU_CONFIG_T info = {0};

  // 使用 ioe_read 函数读取 GPU 的配置
  ioe_read(AM_GPU_CONFIG, &info);

  // 提取屏幕的宽度和高度
  w = info.width;
  h = info.height;

  // 打印获取到的屏幕宽度和高度信息
  printf("Screen width: %d\n", w);
  printf("Screen height: %d\n", h);
}

// 键盘事件处理
void print_key()
{
  AM_INPUT_KEYBRD_T event = {.keycode = AM_KEY_NONE};
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown)
  {
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
  }
}

int main(const char *args)
{
  // 初始化 I/O 环境
  ioe_init();

  // 获取并显示设备的显示信息
  get_display_info();

  // ... 其他代码 ...

  return 0;
}