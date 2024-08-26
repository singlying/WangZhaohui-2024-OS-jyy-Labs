#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

#define AM_KEY_ESC 1

static int w, h; // Screen size

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,

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

// 模拟加载图片数据的函数
uint32_t* load_image(const char *filename, int *width, int *height) {
  // 假设图片大小为 100x100
  *width = 100;
  *height = 100;

  // 分配内存并生成一个简单的图像（随机颜色填充）
  uint32_t *image_data = (uint32_t *)malloc(*width * *height * sizeof(uint32_t));
  for (int i = 0; i < (*width) * (*height); i++) {
    image_data[i] = 0xff0000;  // 使用红色填充
  }
  return image_data;
}

void draw_image(uint32_t *image_data, int img_width, int img_height) {
  int x_offset = (w - img_width) / 2;
  int y_offset = (h - img_height) / 2;

  for (int y = 0; y < img_height; y++) {
    for (int x = 0; x < img_width; x++) {
      // 获取图像中的像素颜色
      uint32_t color = image_data[y * img_width + x];

      // 绘制像素到屏幕上
      AM_GPU_FBDRAW_T event = {
        .x = x_offset + x,
        .y = y_offset + y,
        .w = 1,
        .h = 1,
        .pixels = &color,
        .sync = 1,
      };
      ioe_write(AM_GPU_FBDRAW, &event);
    }
  }
}

int main(const char *args)
{
  // 初始化 I/O 环境
  ioe_init();

  // 获取并显示设备的显示信息
  get_display_info();

  // 加载图片
  int img_width, img_height;
  uint32_t *image_data = load_image("image.bmp", &img_width, &img_height);

 // 等待用户按键退出
  puts("Press ESC to exit...\n");
  while (1) {
    AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
    ioe_read(AM_INPUT_KEYBRD, &event);
    if (event.keycode == AM_KEY_ESC && event.keydown) {
      free(image_data);  // 释放图片内存
      halt(0);
    }
  }

  return 0;
}