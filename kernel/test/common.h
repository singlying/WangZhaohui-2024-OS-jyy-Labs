#include <kernel.h>
#include <assert.h>
#include <stdlib.h>

typedef struct {
  void *start, *end;
} Area;

extern Area heap;