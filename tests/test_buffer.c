#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "lib/compress.h"

int test(char* input, int input_size)
{
  // allocating buffers
  char* compressed   = malloc(sizeof(char) * input_size * 2);
  char* decompressed = malloc(sizeof(char) * input_size * 2);
  assert(compressed && decompressed && "compressed/decompressed buffer allocation failed");

  int compressed_size = lz17_compressBufferToBuffer(compressed, input_size * 2, input, input_size);

  printf("compressed size is %d B\n", compressed_size);
  // lz17_displayCompressedStream(compressed, compressed_size);

  int ret = lz17_decompressBufferToBuffer(decompressed, input_size, compressed, compressed_size);

  if (memcmp(decompressed, input, input_size)) {
    printf("ERROR: decompressed buffer does not match input\n");
    int i;
    for (i = 0; i < input_size && decompressed[i] == input[i]; ++i);
    printf("mismatch @index %d, decompressed[]=%x vs input[]=%x\n", i, decompressed[i], input[i]);
    return 1;
  }

  // free-ing buffer
  free(compressed);
  free(decompressed);

  return 0;
}


int main(void) {
  srand(0);
  size_t input_size = 512;
  char* input        = malloc(sizeof(char) * input_size);

  int i;
  for (i = 0; i < input_size; ++i) input[i] = 0x2a; // rand();
  printf("testing compression/decompression on 64B uniform buffer\n");
  if (!test(input, 64)) printf("success\n");
  else { printf("failure\n"); return 1;};

  printf("testing compression/decompression on 512B uniform buffer\n");
  if (!test(input, 512)) printf("success\n");
  else { printf("failure\n"); return 1;};

  for (i = 0; i < input_size; ++i) input[i] = 0x2a + (rand() % 4);
  printf("testing compression/decompression on an almost uniform 512B buffer\n");
  if (!test(input, 512)) printf("success\n");
  else { printf("failure\n"); return 1;};

  free(input);

  return 0;
}
