#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>


#include "compress.h"


extern char *optarg;
extern int optind, opterr, optopt;



off_t fsize(const char *filename) {
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
}

int main(int argc, char** argv) { 
  char* input_filename;
  char* output_filename;
  char copt;
  int errorflag = 0;
  int extract = 0;
  int compress = 0;
  char c = '?';

  while ((c = getopt(argc, argv, "xci:o:")) != -1)
  {
    switch(c) {
    case 'i':
      input_filename = optarg;
      break;
    case 'o':
      output_filename = optarg;
      break;
    case ':':       
      /* -f or -o without operand */
      fprintf(stderr, "Option -%c requires an operand\n", optopt);
      errorflag++;
      break;
    case 'x':
      extract = 1;
      if (compress) fprintf(stderr, "Option -x and -x and incompatible\n");
      break;
    case 'c':
      compress = 1;
      if (extract) fprintf(stderr, "Option -x and -x and incompatible\n");
      break;
    case '?':
    default: 
      fprintf(stderr, "Unrecognized option: -%c\n", optopt);
      errorflag++;
      break;
    };
  }

  if (errorflag) {
    fprintf(stderr, "usage: . . . ");
    exit(2);
  }

  FILE* input_stream  = fopen(input_filename, "r");
  FILE* output_stream = fopen(output_filename, "w");

  size_t input_size   = fsize(input_filename);
  size_t read_buffer_size = input_size;

  char* read_buffer    = malloc(input_size * sizeof(char));

  size_t read_size = fread(read_buffer, sizeof(char), read_buffer_size, input_stream);

  if (compress) {
    size_t output_buffer_size = input_size * 2;
    char* output_buffer  = malloc(output_buffer_size * sizeof(char));

    int compressed_size = lz17_compressBufferToBuffer(output_buffer, output_buffer_size, read_buffer, read_size);

    // copy compressed buffer to output file
    fwrite(output_buffer, compressed_size, sizeof(char), output_stream);

    free(read_buffer);
    free(output_buffer);

  } else if (extract) {
    size_t output_buffer_size = input_size * 2;
    char*  output_buffer  = malloc(output_buffer_size * sizeof(char));

    int decompressed_size = lz17_decompressBufferToBuffer(output_buffer, output_buffer_size, read_buffer, read_size); 
    fwrite(output_buffer, decompressed_size, sizeof(char), output_stream);

    free(read_buffer);
    free(output_buffer);

  } else assert(0 && "unsupported action");

  fclose(input_stream);
  fclose(output_stream);

  return 0;
}
