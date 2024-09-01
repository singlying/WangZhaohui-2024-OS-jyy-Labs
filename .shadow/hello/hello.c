#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>
#include "image_data.h"

#define AM_KEY_ESC 1
#define WIDTH 200
#define LENTH 200

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

  // printf("Screen width: %d\n", w);
  // printf("Screen height: %d\n", h);
}

// 随机生成图片
uint32_t* load_image(const char *filename, int *width, int *height) {
  *width = WIDTH;
  *height = LENTH;

  // color array
  uint32_t colors[] = {
    0xff0000,  // 红色
    0x00ff00,  // 绿色
    0x0000ff,  // 蓝色
    0xffff00,  // 黄色
    0xff00ff,  // 洋红
    0x00ffff,  // 青色
    0x000000,  // 黑色
    0xffffff,  // 白色
    0x808080,  // 灰色
    0x800000,  // 栗色
    0x008000,  // 橄榄绿
    0x000080   // 海军蓝
  };
  int num_colors = sizeof(colors) / sizeof(colors[0]);

  srand(0x56ffac); // 自由定义的一个数字

  // 分配内存并生成一个简单的图像（随机颜色填充）
  uint32_t *image_data = (uint32_t *)malloc(*width * *height * sizeof(uint32_t));
  for (int i = 0; i < (*width) * (*height); i++) {
    int random_index = rand() % num_colors;
    image_data[i] = colors[random_index];
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

// 从图片加载
void draw_image_from_array(unsigned char *image_data, int img_width, int img_height) {
  int x_offset = (w - img_width) / 2;
  int y_offset = (h - img_height) / 2;
  int row_padded = (img_width * 3 + 3) & (~3); // BMP 行的字节数对齐到4的倍数

  for (int y = 0; y < img_height; y++) {
    // 计算当前行的起始索引，反向遍历行
    int row_index = (img_height - 1 - y) * row_padded;
    for (int x = 0; x < img_width; x++) {
      int pixel_index = row_index + x * 3;
      uint32_t color = (image_data[pixel_index + 2] << 16) |
                      (image_data[pixel_index + 1] << 8) |
                      (image_data[pixel_index]);

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

  // 提取图片的宽度和高度（这需要从 BMP 头部读取）
  int img_width = *(int*)&p1_bmp[18];
  int img_height = *(int*)&p1_bmp[22];
  // uint32_t *image_data = load_image("image.bmp", &img_width, &img_height);
  // draw_image(image_data, img_width, img_height);


  draw_image_from_array(p1_bmp + 54, img_width, img_height);  // 54 字节偏移到 BMP 数据部分

 // 等待用户按键退出
  puts("Press ESC to exit...\n");
  while (1) {
    AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
    ioe_read(AM_INPUT_KEYBRD, &event);
    if (event.keycode == AM_KEY_ESC && event.keydown) {
      // free(image_data);  // 释放图片内存
      halt(0);
    }
  }

  return 0;
}