#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>


#include "arith_coding.h" 
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
  int arith_coding = 0;
  char c = '?';

  while ((c = getopt(argc, argv, "xcai:o:")) != -1)
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
    case 'a':
      arith_coding = 1;
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

    if (arith_coding) {
      ac_state_t encoder_state;
      const int update_range = 64;

      char* encoded_buffer = malloc(compressed_size * sizeof(char));
      // push compressed size (to allow decoding)
      encoded_buffer[0] = compressed_size & 0xff;
      encoded_buffer += 4;
      // copy lz17 header and expanded size
      memcpy(encoded_buffer, output_buffer, 5);
      init_state(&encoder_state, 16);
      reset_uniform_probability(&encoder_state);
      encode_value_with_update(encoded_buffer + 5, output_buffer + 5, compressed_size - 5, &encoder_state, update_range, 0 /* range clear */); 

      size_t encoded_size = (encoder_state.out_index + 7) / 8;
      // copy compressed buffer to output file
      fwrite(encoded_buffer, encoded_size + 5, sizeof(char), output_stream);

      free(encoded_buffer);
    } else {
      // copy compressed buffer to output file
      fwrite(output_buffer, compressed_size, sizeof(char), output_stream);
    }


    free(read_buffer);
    free(output_buffer);

  } else if (extract) {
    size_t output_buffer_size = lz17_bufferExtractExpandedSize(read_buffer);
    char*  output_buffer  = malloc(output_buffer_size * sizeof(char));

    if (arith_coding) {
      char* decoded_buffer = malloc(output_buffer_size * sizeof(char));
      memcpy(decoded_buffer, read_buffer, 5);
      ac_state_t encoder_state;
      const int update_range = 64;
      reset_uniform_probability(&encoder_state);
      decode_value_with_update(decoded_buffer + 5, read_buffer + 5, &encoder_state, output_buffer_size, update_range, 0/* range clear */);
      
      int decompressed_size = lz17_decompressBufferToBuffer(output_buffer, output_buffer_size, decoded_buffer, output_buffer_size); 
      fwrite(output_buffer, decompressed_size, sizeof(char), output_stream);

    } else {
      int decompressed_size = lz17_decompressBufferToBuffer(output_buffer, output_buffer_size, read_buffer, read_size); 
      fwrite(output_buffer, decompressed_size, sizeof(char), output_stream);
    };

    free(read_buffer);
    free(output_buffer);

  } else assert(0 && "unsupported action");

  fclose(input_stream);
  fclose(output_stream);

  return 0;
}
