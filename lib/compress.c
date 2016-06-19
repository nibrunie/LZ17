#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>


#include "lib/compress.h"



/** error code */
enum {
  /** no error */
  LZ17_OK = 0
};


typedef union {
  char c;
  unsigned char u;
} uc_t;

void lz17_compressInit(lz17_state_t* state, lz17_entropy_mode_t entropy_mode)
{
  state->entropy_mode = entropy_mode;
  state->ac_state = malloc(sizeof(ac_state_t));

  if (entropy_mode == LZ17_AC_ENTROPY_CODING) {
    init_state(state->ac_state, 16 /* precision */);
    reset_uniform_probability(state->ac_state);

    reset_prob_table(state->ac_state);
    state->update_count = 0;
    state->update_range = 1024;
    state->range_clear  = 1;
  }
}

/** hash structure to keep track of previous matches
 *  key is a hash of input byte
 *  value is the address of the input byte whose hash value is equal to key
 */
typedef struct {
  int key_size;
  int value_number;

  char** table;
} hash_t;

hash_t* hash_init(int key_size, int value_number)
{
  hash_t* hash = malloc(sizeof(hash_t));
  hash->table = calloc(value_number, sizeof(char*));

  hash->key_size = key_size;
  hash->value_number = value_number;

  return hash;
}


/** return the address store in the hash table at the entry
 *  with key equal to @p hash_value
 *  @param hash hash_table 
 *  @param hash_value value used as key to search the table
 *  @return NULL if no value is associated with the key, of the address of the previous
 *  byte string which matches the key value
 */
char* hash_contains(hash_t* hash, unsigned hash_value) 
{
  char* value_addr = hash->table[hash_value % hash->value_number];

  return value_addr;
}

int hash_update(hash_t* hash, unsigned hash_value, char* baddr, char* in) 
{
  hash->table[hash_value % hash->value_number] = baddr;

  return LZ17_OK;
}

/** test if two byte string macthes for at least @p match_size symbol
 *  @param addr0 first address to match
 *  @param addr1 second address to match
 *  @param match_size minimal matching length to consider a match valid
 *  @return 0 if no match, 1 otherwise
 */

int value_matches(char* addr0, char* addr1, int match_size)
{
  return !memcmp(addr0, addr1, match_size);
}

/** return the match length between two byte buffers
 *  @param addr0 first buffer to match
 *  @param addr1 second buffer to match
 *  @param match_bound upper bound for the match_length
 *  @return byte size of the match
 */
int get_match_length(char* addr0, char* addr1, int match_bound)
{
  int match_length = 0;
  for (match_length = 0; match_length <= match_bound; match_length++) {
    if (addr0[match_length] != addr1[match_length]) break;
  }
  return match_length;
}

enum {
  BACK_REF = 0x80,
  LITTERAL = 0x00,
  MAX_MATCH_SYMBOL = 0x7f
};

typedef enum {
  /** LZ17 compression */
  LZ17_HEADER = 0x11,
  /** LZ17 compression with arithmetic coding */
  LZ17_HEADER_AC = 0x12
} lz17_header_byte_t;

#define is_valid_lz17_header(byte) ((byte) == LZ17_HEADER || (byte) == LZ17_HEADER_AC)

// match length encoded by MAX_MATCH_SYMBOL
#define MAX_MATCH_SYMBOL_LENGTH 127
#define MAX_CPY_LENGTH          127
#define MAX_MATCH_OFFSET        ((1 << 16) - 1)

#define MAX_CPY_SYMBOL 0x7f


/** code copied from Yann's Collet / Cyan4973 while waiting to implement link with 
 *  xxhash or other hash function 
 */


static const uint32_t PRIME32_1 = 2654435761U;
static const uint32_t PRIME32_2 = 2246822519U;
static const uint32_t PRIME32_3 = 3266489917U;
static const uint32_t PRIME32_4 =  668265263U;
static const uint32_t PRIME32_5 =  374761393U;

#define XXH_rotl32(x, s) ((x << s) | (x >> (32 - s)))


static unsigned XXH32_round(unsigned seed, unsigned input)
{
    seed += input * PRIME32_2;
    seed  = XXH_rotl32(seed, 13);
    seed *= PRIME32_1;
    return seed;
}

#define LZ17_HASH_SEED (0x1337deadu)

#define UCP(x) ((unsigned char*)(x))
#define U(x) ((unsigned) (x))

/** read 4-byte from a char* buffer */
#define READU32(x) ((U(UCP(x)[0]) << 0) | (U(UCP(x)[1]) << 8) | (U(UCP(x)[2]) << 16) | (U(UCP(x)[3]) << 24))

#define READU24(x) ((U(UCP(x)[0]) << 0) | (U(UCP(x)[1]) << 8) | (U(UCP(x)[2]) << 16))

#define READU16(x) ((U(UCP(x)[0]) << 0) | (U(UCP(x)[1]) << 8))

/** Hash 4-Byte from @p in */
unsigned lz17_hash4(char* in)
{
  return XXH32_round(LZ17_HASH_SEED, READU32(in));
}

/** Hash 3-Byte from @p in */
unsigned lz17_hash3(char* in)
{
  return XXH32_round(LZ17_HASH_SEED, READU24(in));
}

static void __emit_header_byte(lz17_state_t* zstate, char byte) 
{
  *(zstate->next_out) = byte;
  zstate->next_out++;
}

static void __emit_byte(lz17_state_t* zstate, char byte) 
{
  if (zstate->entropy_mode == LZ17_NO_ENTROPY_CODING) {
    __emit_header_byte(zstate, byte);
  } else if (zstate->entropy_mode == LZ17_AC_ENTROPY_CODING) {
    encode_character(zstate->next_out, byte, (zstate->ac_state));
    // updating probability
    uc_t table_index;
    table_index.c = byte;
    zstate->ac_state->prob_table[table_index.u]++;
    zstate->update_count++;
    if (zstate->update_count >= zstate->update_range)
    {
      transform_count_to_cumul(zstate->ac_state, zstate->update_count);
      if (zstate->range_clear)
      {
        zstate->update_count = 0;
        int j;
        reset_prob_table(zstate->ac_state);
      }
    }
  } // FIXME, final else / assert
}

static void __emit_header_4byte_le(lz17_state_t* zstate, int offset) 
{
  __emit_header_byte(zstate, offset & 0xff);
  __emit_header_byte(zstate, (offset >> 8) & 0xff);
  __emit_header_byte(zstate, (offset >> 16) & 0xff);
  __emit_header_byte(zstate, (offset >> 24) & 0xff);
}

static  __emit_2byte_le(lz17_state_t* zstate, int offset) 
{
  __emit_byte(zstate, offset & 0xff);
  __emit_byte(zstate, (offset >> 8) & 0xff);
}

static void __emit_match_offset(lz17_state_t* zstate, int offset) 
{
  assert(sizeof(int) == 4);
  __emit_2byte_le(zstate, offset);
}


int lz17_compressBufferToBuffer(lz17_state_t* zstate, char* out, size_t avail_out, char* in, size_t avail_in)
{
  // number of byte used to compute the hash-value
  const int hash_in_size = 4;
  const int hash_size = 1 << 16;
  const int min_match_size = 3;

  const char* out_start = out;
  zstate->next_out  = out;
  zstate->avail_out = avail_out;

  hash_t* hash = hash_init(4, hash_size); 
  
  // put LZ17 specific header
  switch(zstate->entropy_mode)
  {
  case LZ17_NO_ENTROPY_CODING:
    __emit_header_byte(zstate, LZ17_HEADER);
    break;
  case LZ17_AC_ENTROPY_CODING:
    __emit_header_byte(zstate, LZ17_HEADER_AC);
    break;
  default:
    assert(0 && "unsupported entropy_mode");
  }

  // enqueud expected decompressed size
  __emit_header_4byte_le(zstate, avail_in);

  // byte-index
  int bindex = -1, litteral_length = 0;
  char* litteral_cpy_addr = NULL;
  for (bindex = 0; bindex < avail_in - hash_in_size;) {
    unsigned bhash = lz17_hash3(in + bindex);
    char* value_addr = NULL;
    char* search_addr = in + bindex;
    // if the hash table contains a match, then check and extend it
    if (value_addr = hash_contains(hash, bhash)) {
      int match_length = get_match_length(value_addr, in + bindex, avail_in - bindex);
      // initial match offset
      int match_offset = (in + bindex - value_addr);
      if (match_length > min_match_size && match_offset <= MAX_MATCH_OFFSET) {
        // if there is a pending litteral copy flush it
        if (litteral_length > 0) {
          while (litteral_length > MAX_CPY_LENGTH) {
            __emit_byte(zstate, LITTERAL | MAX_CPY_SYMBOL);
            int k;
            for (k = 0; k < MAX_CPY_LENGTH; ++k) __emit_byte(zstate, litteral_cpy_addr[k]);
            litteral_cpy_addr += MAX_CPY_LENGTH;
            litteral_length   -= MAX_CPY_LENGTH; 
          }
          if (litteral_length > 0) {
            __emit_byte(zstate, LITTERAL | litteral_length);
            int k;
            for (k = 0; k < litteral_length; ++k) __emit_byte(zstate, litteral_cpy_addr[k]);
          }
          litteral_cpy_addr = NULL;
          litteral_length   = 0;
        }

        // emit a back-reference
        size_t total_match_length = match_length;
        while (match_length > MAX_MATCH_SYMBOL_LENGTH) {
          __emit_byte(zstate, BACK_REF | MAX_MATCH_SYMBOL);
          __emit_match_offset(zstate, (in + bindex - value_addr));

          value_addr += MAX_MATCH_SYMBOL_LENGTH;
          match_length -= MAX_MATCH_SYMBOL_LENGTH;
          bindex += MAX_MATCH_SYMBOL_LENGTH;
        }
        if (match_length > 0) {
          __emit_byte(zstate, BACK_REF | match_length);
          __emit_match_offset(zstate, (in + bindex - value_addr));

          value_addr   += match_length;
          bindex += match_length;
          match_length -= match_length;
        }
        // bindex += total_match_length; 

      } else {
        // emit a litteral copy
        if (litteral_length == 0) litteral_cpy_addr = in + bindex;
        litteral_length++;
        bindex++;
      };
      
    } else {
      // emit a litteral copy
      if (litteral_length == 0) litteral_cpy_addr = in + bindex;
      litteral_length++;
      bindex++;
    }
    // update 
    hash_update(hash, bhash, search_addr, in);
  };

  // flushing between outside hash_in_size
  if (bindex < avail_in) {
    if (litteral_length == 0) litteral_cpy_addr = in + bindex;
      litteral_length += (avail_in - bindex);
  };

  // flush litteral copy if necessary
  if (litteral_length > 0) {
    while (litteral_length > MAX_CPY_LENGTH) {
      __emit_byte(zstate, LITTERAL | MAX_CPY_SYMBOL);
      int k;
      for (k = 0; k < MAX_CPY_LENGTH; ++k) __emit_byte(zstate, litteral_cpy_addr[k]);
      litteral_cpy_addr += MAX_CPY_LENGTH;
      litteral_length   -= MAX_CPY_LENGTH; 
    }
    if (litteral_length > 0) {
      __emit_byte(zstate, LITTERAL | litteral_length);
      int k;
      for (k = 0; k < litteral_length; ++k) __emit_byte(zstate, litteral_cpy_addr[k]);
    }
    litteral_cpy_addr = NULL;
    litteral_length   = 0;
  }

  if (zstate->entropy_mode == LZ17_AC_ENTROPY_CODING) {
    select_value(zstate->next_out, (zstate->ac_state));
    size_t encoded_size = (zstate->ac_state->out_index + 7) / 8;
    // next_out is not shifted by arithmetic coding
    // but was by header insertion, the following formula
    // allow us to add arithmetic coded size + header size
    return (zstate->next_out + encoded_size) - out_start;
  } else {
    return zstate->next_out - out_start;
  };
}

int lz17_bufferExtractExpandedSize(char* in)
{
  int bindex = 0;

  // read and check headers
  char buffer_header = in[bindex++];
  assert(is_valid_lz17_header(buffer_header));

  int expanded_size = READU32(in + bindex);
  bindex += 4;
  
  return expanded_size; 
};


char __lz17_decode_character(lz17_state_t* zstate) 
{
  char new_char = decode_character(zstate->next_in, zstate->ac_state); 
  // updating probability
  uc_t table_index;
  table_index.c = new_char;
  zstate->ac_state->prob_table[table_index.u]++;
  zstate->update_count++;
  if (zstate->update_count >= zstate->update_range)
  {
    transform_count_to_cumul(zstate->ac_state, zstate->update_count);
    if (zstate->range_clear)
    {
      zstate->update_count = 0;
      int j;
      reset_prob_table(zstate->ac_state);
    }
  }
  return new_char;
}

char __read_byte(lz17_state_t* zstate) {
  if (zstate->entropy_mode == LZ17_NO_ENTROPY_CODING) {
    zstate->avail_in--;
    return *(zstate->next_in++);
  } else if (zstate->entropy_mode == LZ17_AC_ENTROPY_CODING) {
    char new_char = __lz17_decode_character(zstate);
    zstate->avail_in--;
    return new_char;
  };
}


/** read a match offset from in + bindex
 *  and update bindex accordingly
 *  @param in input array
 *  @param pindex adresse of the current read index in @p in
 *  @return read match_offset (while updating *pindex value)
 */
static int __read_match_offset(lz17_state_t* zstate) 
{
  if (zstate->entropy_mode == LZ17_NO_ENTROPY_CODING) {
    int match_offset = READU16(zstate->next_in);
    zstate->next_in += 2;
    zstate->avail_in -= 2;
    return match_offset;
  } else if (zstate->entropy_mode == LZ17_AC_ENTROPY_CODING) {
    uc_t char_lo, char_hi;
    char_lo.c = __lz17_decode_character(zstate);
    char_hi.c = __lz17_decode_character(zstate);
    int match_offset = char_lo.u | ((int) char_hi.u << 8);
    return match_offset;
  }
}


int lz17_decompressBufferToBuffer(char* out, size_t avail_out, char* in, size_t avail_in)
{
  int bindex = 0;

  lz17_state_t* zstate = malloc(sizeof(lz17_state_t));

  // read and check headers
  char buffer_header = in[bindex++];
  assert(is_valid_lz17_header(buffer_header));

  switch(buffer_header) {
  case LZ17_HEADER:
    zstate->entropy_mode = LZ17_NO_ENTROPY_CODING;
    break;
  case LZ17_HEADER_AC:
    zstate->entropy_mode = LZ17_AC_ENTROPY_CODING;
    break;
  default:
    assert(0 && "unsupported LZ17 headers");
  }



  int decompressed_size = 0;

  int buffer_size = READU32(in + bindex);
  bindex += 4;
  assert(buffer_size <= avail_out);

  int header_size = bindex;
  zstate->next_in  = in + header_size;
  zstate->avail_in = avail_in - header_size; 
  zstate->ac_state = malloc(sizeof(ac_state_t));

  // initializing internal state
  if (zstate->entropy_mode == LZ17_AC_ENTROPY_CODING) {
    init_state(zstate->ac_state, 16 /* precision */);
    reset_uniform_probability(zstate->ac_state);
    init_decoding(zstate->next_in, zstate->ac_state);

    reset_prob_table(zstate->ac_state);
    zstate->update_count = 0;
    zstate->update_range = 1024;
    zstate->range_clear  = 1;
  }

  char* out_start = out;

  // read expected decompressed size
  while (decompressed_size < buffer_size && zstate->avail_in > 0) {
    char current_byte = __read_byte(zstate);
    if (current_byte & BACK_REF) {
      // back-reference
      int match_length = current_byte & MAX_MATCH_SYMBOL;
      int match_offset = __read_match_offset(zstate);
      int k;
      for (k = 0; k < match_length; ++k) out[k] = out[-match_offset + k];

      out += match_length;
      decompressed_size += match_length;
    } else if (!(current_byte & BACK_REF)) {
      int copy_length = current_byte & MAX_MATCH_SYMBOL;
      int k;
      for (k = 0; k < copy_length; ++k) out[k] = __read_byte(zstate);

       out += copy_length;
       decompressed_size += copy_length;

    } else {
      assert(0 && "unrecognized byte pattern");
    }

  }
  assert(decompressed_size == (out - out_start));

  return out - out_start;;

}

int lz17_displayCompressedStream(char* in, size_t avail_in)
{
  int bindex = 0;
  while (bindex < avail_in) {
    if (in[bindex] & BACK_REF) {
      // back-reference
      int match_length = in[bindex] & MAX_MATCH_SYMBOL;
      bindex++;
      int match_offset = READU32(in + bindex);
      bindex += 4;
      printf("BACK_REF(len=%d,offset=%d)\n", match_length, match_offset);
      int k;
    } else if (!(in[bindex] & BACK_REF)) {
      int copy_length = in[bindex] & MAX_MATCH_SYMBOL;
      bindex++;
      int k;
      printf("COPY(len=%d) [ ", copy_length);
      for (k = 0; k < copy_length; ++k) printf("%2x ", in[bindex + k]);
      printf(" ]\n");

       bindex += copy_length;

    } else {
      assert(0 && "unrecognized byte pattern");
    }

  }

  return LZ17_OK;

}

