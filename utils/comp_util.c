#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>


#include "arith_coding.h"
#include "compress.h"

#include "utils/lz17utils_opt.h"


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
  int errorflag = 0;
  int extract = 0;
  int compress = 0;

  struct gengetopt_args_info args_info;


  if (errorflag) {
    fprintf(stderr, "usage: . . . ");
    exit(2);
  }

  if (cmdline_parser (argc, argv, &args_info) != 0)
      exit(1) ;

  input_filename = args_info.input_arg;
  output_filename = args_info.output_arg;
  compress = args_info.compress_flag;

  extract = args_info.extract_flag;

  FILE* input_stream  = fopen(input_filename, "r");
  FILE* output_stream = fopen(output_filename, "w");

  size_t input_size   = fsize(input_filename);
  size_t read_buffer_size = input_size;

  char* read_buffer    = malloc(input_size * sizeof(char));

  size_t read_size = fread(read_buffer, sizeof(char), read_buffer_size, input_stream);

  lz17_entropy_mode_t entropy_mode = LZ17_NO_ENTROPY_CODING;
  if (args_info.arith_coding_given) entropy_mode = LZ17_AC_ENTROPY_CODING;

  if (compress) {
    size_t output_buffer_size = input_size * 2;
    char* output_buffer  = malloc(output_buffer_size * sizeof(char));

    lz17_state_t* zstate = malloc(sizeof(lz17_state_t));
    lz17_compressInit(zstate, entropy_mode);


    int compressed_size = lz17_compressBufferToBuffer(zstate, output_buffer, output_buffer_size, read_buffer, read_size);

    // copy compressed buffer to output file
    fwrite(output_buffer, compressed_size, sizeof(char), output_stream);


    free(read_buffer);
    free(output_buffer);
    free(zstate);

  } else if (extract) {
    size_t output_buffer_size = lz17_bufferExtractExpandedSize(read_buffer);
    char*  output_buffer      = malloc(output_buffer_size * sizeof(char));

    int decompressed_size = lz17_decompressBufferToBuffer(output_buffer, output_buffer_size, read_buffer, read_size); 
    fwrite(output_buffer, decompressed_size, sizeof(char), output_stream);

    free(read_buffer);
    free(output_buffer);

  } else assert(0 && "unsupported action");

  fclose(input_stream);
  fclose(output_stream);

  return 0;
}
