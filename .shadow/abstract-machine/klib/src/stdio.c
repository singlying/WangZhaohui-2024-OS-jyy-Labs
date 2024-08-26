#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// output a single character
static void putc(char ch)
{
  putch(ch); // AM 提供的输出字符函数
}

int printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int count = 0;

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
        case 'c': {
          char ch = (char)va_arg(args, int);
          putc(ch);
          count++;
          break;
        }
        case 's': {
          char *str = va_arg(args, char*);
          while (*str) {
            putc(*str++);
            count++;
          }
          break;
        }
        case 'd': {
          int num = va_arg(args, int);
          if (num < 0) {
            putc('-');
            num = -num;
            count++;
          }
          char buffer[16];
          int i = 0;
          do {
            buffer[i++] = (num % 10) + '0';
            num /= 10;
          } while (num > 0);
          while (i--) {
            putc(buffer[i]);
            count++;
          }
          break;
        }
        case 'x': {
          unsigned int num = va_arg(args, unsigned int);
          char buffer[16];
          int i = 0;
          do {
            int digit = num % 16;
            buffer[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
            num /= 16;
          } while (num > 0);
          while (i--) {
            putc(buffer[i]);
            count++;
          }
          break;
        }
        case '%': {
          putc('%');
          count++;
          break;
        }
        default: {
          // If we encounter an unknown format specifier, print it as is
          putc('%');
          putc(*fmt);
          count += 2;
          break;
        }
      }
    } else {
      putc(*fmt);
      count++;
    }
    fmt++;
  }

  va_end(args);
  return count;
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...)
{
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

#endif
