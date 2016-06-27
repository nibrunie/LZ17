#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "lib/compress.h"

int test_generic(char* input, int input_size, lz17_entropy_mode_t entropy_mode)
{
  // allocating buffers
  char* compressed   = malloc(sizeof(char) * input_size * 2);
  char* decompressed = malloc(sizeof(char) * input_size * 2);
  assert(compressed && decompressed && "compressed/decompressed buffer allocation failed");

  lz17_state_t* zstate = malloc(sizeof(lz17_state_t));
  lz17_compressInit(zstate, entropy_mode);

  int compressed_size = lz17_compressBufferToBuffer(zstate, compressed, input_size * 2, input, input_size);

  printf("compressed size is %d B\n", compressed_size);
  // lz17_displayCompressedStream(compressed, compressed_size);

  int ret = lz17_decompressBufferToBuffer(decompressed, input_size, compressed, compressed_size);

  if (memcmp(decompressed, input, input_size)) {
    printf("ERROR: decompressed buffer does not match input\n");
    int i, j;
    for (i = 0; i < input_size && decompressed[i] == input[i]; ++i);
    for (j = i -2; j < i+6; ++j) printf("mismatch @index %d, decompressed[]=%x vs input[]=%x\n", j, decompressed[j], input[j]);
    return 1;
  }

  // free-ing buffer
  free(zstate);
  free(compressed);
  free(decompressed);

  return 0;
}

int test(char* input, int input_size) {
  return test_generic(input, input_size, LZ17_NO_ENTROPY_CODING);
}

int test_ac(char* input, int input_size) {
  return test_generic(input, input_size, LZ17_AC_ENTROPY_CODING);
}

int main(void) {
  const lz17_entropy_mode_t lz17_entropy_modes[2] = {LZ17_NO_ENTROPY_CODING, LZ17_AC_ENTROPY_CODING};
  srand(0);
  int i;
  for (i = 0; i < 2; ++i)
  {
    lz17_entropy_mode_t entropy_mode = lz17_entropy_modes[i];

    switch (entropy_mode) {
    case LZ17_AC_ENTROPY_CODING:
      printf("testing LZ17 with arithmetic coding\n");
      break;
    case LZ17_NO_ENTROPY_CODING:
      printf("testing LZ17 without arithmetic coding\n");
      break;
    };
    {
      size_t input_size = 1 << 18;
      char* input        = malloc(sizeof(char) * input_size);

      int i;
      for (i = 0; i < input_size; ++i) input[i] = 0x2a; // rand();

      for (i = 0; i < input_size; ++i) input[i] = 0x2a + (rand() % 3);
      printf("testing compression/decompression on an almost uniform %dB buffer\n", input_size);
      if (!test_generic(input, input_size, entropy_mode)) printf("success\n");
      else { printf("failure\n"); return 1;};

      free(input);
    }
    {
      size_t input_size = 512;
      char* input        = malloc(sizeof(char) * input_size);

      int i;
      for (i = 0; i < input_size; ++i) input[i] = 0x2a; // rand();
      printf("testing compression/decompression on 64B uniform buffer\n");
      if (!test_generic(input, 64, entropy_mode)) printf("success\n");
      else { printf("failure\n"); return 1;};

      printf("testing compression/decompression on 512B uniform buffer\n");
      if (!test_generic(input, 512, entropy_mode)) printf("success\n");
      else { printf("failure\n"); return 1;};

      for (i = 0; i < input_size; ++i) input[i] = 0x2a + (rand() % 3);
      printf("testing compression/decompression on an almost uniform 512B buffer\n");
      if (!test_generic(input, 512, entropy_mode)) printf("success\n");
      else { printf("failure\n"); return 1;};

      free(input);
    }
  }

  return 0;
}
