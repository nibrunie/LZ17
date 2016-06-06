#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "lib/compress.h"


int main(void) {
  srand(0);
  size_t input_size = 64;

  char* input        = malloc(sizeof(char) * input_size);
  char* compressed   = malloc(sizeof(char) * input_size * 2);
  char* decompressed = malloc(sizeof(char) * input_size * 2);

  int i;
  for (i = 0; i < input_size; ++i) input[i] = 0x2a; // rand();

  int compressed_size = lz17_compressBufferToBuffer(compressed, input_size * 2, input, input_size);

  printf("compressed size is %d\n", compressed_size);

  int ret = lz17_decompressBufferToBuffer(decompressed, input_size, compressed, compressed_size);

  if (memcmp(decompressed, input, input_size)) {
    printf("ERROR: decompressed buffer does not match input\n");
    for (i = 0; i < input_size && decompressed[i] == input[i]; ++i);
    printf("mismatch @index %d, decompressed[]=%x vs input[]=%x\n", i, decompressed[i], input[i]);
    return 1;
  }

  return 0;
}
