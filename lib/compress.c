
/** error code */
enum {
  /** no error */
  LZ17_OK = 0
};

/** hash structure to keep track of previous matches
 *  key is a hash of input byte
 *  value is the address of the input byte whose hash value is equal to key
 */
typedef struct {
  int key_size;
  int value_number;

  char* table;
} hash_t;

hash_t* hash_init(int key_size, int value_number)
{
  hash_t hash = malloc(sizeof(hash_t));
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
char* hash_contains(hash_t* hash, int hash_value) 
{
  assert(hash_value < hash->value_number && "hash index exceeds hash table size");
  char* value_addr = hash->table[hash_value];

  return value_addr;
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
int match_length(char* addr0, char* addr1, int match_bound)
{
  int match_length = 0
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

// match length encoded by MAX_MATCH_SYMBOL
#define MAX_MATCH_SYMBOL_LENGTH 127
#define MAX_CPY_LENGTH          127


/** code copied from Yann's Collet / Cyan4973 while waiting to implement link with 
 *  xxhash or other hash function 
 */


static const U32 PRIME32_1 = 2654435761U;
static const U32 PRIME32_2 = 2246822519U;
static const U32 PRIME32_3 = 3266489917U;
static const U32 PRIME32_4 =  668265263U;
static const U32 PRIME32_5 =  374761393U;

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
#define U(x) (unsigned (x))

/** read 4-byte from a char* buffer */
#define READU32(in) ((U(UCP(x)[0]) << 0) | (U(UCP(x)[1]) << 8) | (U(UCP(x)[2]) << 16) | (U(UCP(x)[3]) << 24))

/** Hash 4-Byte from @p in */
unsigned lz17_hash4(char* in)
{
  return XXH32_round(LZ17_HASH_SEED, READU32(in));
}

static char* emit_byte(char* out, char byte) 
{
  *out = byte;
  return ++out;
}

static char* emit_match_offset(char* out, int offset) 
{
  out = emit_byte(out, offset & 0xff);
  out = emit_byte(out, (offset >> 8) & 0xff);
  out = emit_byte(out, (offset >> 16) & 0xff);
  out = emit_byte(out, (offset >> 24) & 0xff);
  return out;
}


int lz17_compressBufferToBuffer(char* out, size_t avail_out, char* in, size_t avail_in)
{
  // number of byte used to compute the hash-value
  const int hash_in_size = 4;
  const int hash_size = 1 << 12;
  const int min_match_size = 4;

  lz17_hash_t* hash = hash_init(4, hash_size); 

  // byte-index
  int bindex = -1, litteral_length = 0;
  char* litteral_cpy_addr = NULL;
  for (bindex = 0; bindex < avail_in - hash_in_size; ++bindex) {
    int bhash = lz17_hash4(in + bindex);
    char* value_addr = NULL;
    // if the hash table contains a match, then check and extend it
    if (value_addr = hash_contains(hash, bhash)) {
      int match_length = match_length(value_addr, in + b_index, avail_in - b_index);
      if (match_length > min_match_size) {
        // if there is a pending litteral copy flush it
        if (litteral_length > 0) {
          while (litteral_length > MAX_CPY_LENGTH) {
            out = emit_byte(out, LITTERAL | MAX_CPY_SYMBOL);
            int k;
            for (k = 0; k < MAX_CPY_LENGTH; ++k) out = emit_byte(out, litteral_cpy_addr[k]);
            litteral_cpy_addr += MAX_CPY_LENGTH;
            litteral_length   -= MAX_CPY_LENGTH; 
          }
          if (litteral_length > 0) {
            out = emit_byte(out, LITTERAL | litteral_length);
            int k;
            for (k = 0; k < litteral_length; ++k) out = emit_byte(out, litteral_cpy_addr[k]);
          }
          litteral_cpy_addr = NULL;
          litteral_length   = 0;
        }

        // emit a back-reference
        size_t total_match_length = match_length;
        while (match_length > MAX_MATCH_SYMBOL_LENGTH) {
          out = emit_byte(out, BACK_REF | MAX_MATCH_SYMBOL);
          out = emit_match_offset(out, (in + b_index - value_addr));

          value_addr += MAX_MATCH_SYMBOL_LENGTH;
          match_length -= MAX_MATCH_SYMBOL_LENGTH;
        }
        if (match_length > 0) {
          out = emit_byte(out, BACK_REF | match_length);
          out = emit_match_offset(out, (in + b_index - value_addr));

          value_addr   += match_length;
          match_length -= match_length;
        }
        bindex += total_match_length; 

      } else {
        // emit a litteral copy
        if (litteral_length == 0) litteral_cpy_addr = in + bindex;
        litteral_length++;
      };
      
    } else {
      // emit a litteral copy
      if (litteral_length == 0) litteral_cpy_addr = in + bindex;
      litteral_length++;
    }
    // update 
    hash_update(hash, bhash, in + bindex, in);
  };

  // flush litteral copy if necessary
  if (litteral_length > 0) {
    while (litteral_length > MAX_CPY_LENGTH) {
      out = emit_byte(out, LITTERAL | MAX_CPY_SYMBOL);
      int k;
      for (k = 0; k < MAX_CPY_LENGTH; ++k) out = emit_byte(out, litteral_cpy_addr[k]);
      litteral_cpy_addr += MAX_CPY_LENGTH;
      litteral_length   -= MAX_CPY_LENGTH; 
    }
    if (litteral_length > 0) {
      out = emit_byte(out, LITTERAL | litteral_length);
      int k;
      for (k = 0; k < litteral_length; ++k) out = emit_byte(out, litteral_cpy_addr[k]);
    }
    litteral_cpy_addr = NULL;
    litteral_length   = 0;
  }

}

int lz17_decompressBufferToBuffer(char* out, size_t avail_out, char* in, size_t avail_in)
{
  int bindex = 0;
  while (bindex < avail_in) {
    if (in[bindex] & BACK_REF) {
      // back-reference
      int match_length = in[bindex] & MAX_MATCH_SYMBOL;
      int match_offset = READU32(in + bindex);
      int k;
      for (k = 0; k < match_length; ++k) out[k] = out[-match_offset + k];

      out += match_length;
      bindex += 5;
    } else if (in[index] & BACK_REF == 0) {
      int copy_length = in[bindex] & MAX_MATCH_SYMBOL;
      bindex++;
      int k;
      for (k = 0; k < match_length; ++k) out[k] = in[bindex + k];

       out += match_length;
       in += match_length;

    } else {
      assert(0 && "unrecognized byte pattern");
    }

  }

  return LZ17_OK;

}
