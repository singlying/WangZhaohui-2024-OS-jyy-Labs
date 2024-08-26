#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

typedef struct BlockHeader {
  size_t size;          // Block size including header
  int is_free;          // Free flag: 1 if block is free, 0 otherwise
  struct BlockHeader *next; // Pointer to the next block in the heap
} BlockHeader;

static BlockHeader *free_list = NULL;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
// #if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
//   panic("Not implemented");
// #endif
//   return NULL;

  if (size == 0) return NULL;

  BlockHeader *current = free_list;
  BlockHeader *prev = NULL;

  // Align size to the nearest multiple of sizeof(size_t) to avoid fragmentation
  size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);

  while (current != NULL) {
    if (current->is_free && current->size >= size) {
      // If the current block is free and large enough, use it
      current->is_free = 0;
      return (void *)(current + 1); // Return pointer after the header
    }
    prev = current;
    current = current->next;
  }

  // If no suitable block found, allocate a new block
  // void *heap_end = heap.end;
  if ((char *)heap.start + sizeof(BlockHeader) + size > (char *)heap.end) {
    return NULL; // No more space left in the heap
  }

  current = (BlockHeader *)heap.start;
  current->size = sizeof(BlockHeader) + size;
  current->is_free = 0;
  current->next = NULL;

  heap.start = (char *)heap.start + current->size;

  if (prev != NULL) {
    prev->next = current;
  } else {
    free_list = current;
  }

  return (void *)(current + 1);
}

void free(void *ptr) {
  if (ptr == NULL) return;

  BlockHeader *block = (BlockHeader *)ptr - 1;
  block->is_free = 1;

  // Optional: Coalesce adjacent free blocks (not implemented here)
}

#endif
