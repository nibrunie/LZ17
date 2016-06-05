
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

/** Try to compress @p avail_in from @p in buffer into @p out without exceeding @p avail_out bytes
 *  @param out output buffer
 *  @param avail_out available size in @p out
 *  @param in input data
 *  @param avail_in size of input data
 *  @return error code on error or LZ17_OK on success
 */
int lz17_compressBufferToBuffer(char* out, size_t avail_out, char* in, size_t avail_in)
{
  // number of byte used to compute the hash-value
  const int hash_in_size = 4;
  const int hash_size = 1 << 12;
  const int min_match_size = 4;

  lz17_hash_t* hash = hash_init(4, hash_size); 

  // byte-index
  int bindex;
  for (bindex = 0; bindex < avail_in - hash_in_size; ++bindex) {
    int bhash = lz17_hash(in + bindex, 4);
    char* value_addr = NULL;
    // if the hash table contains a match, then check and extend it
    if (value_addr = hash_contains(hash, bhash)) {
      int match_length = match_length(value_addr, in + b_index, avail_in - b_index);
      if (match_length > min_match_size) {
        // emit a back-reference

      } else {
        // emit a litteral copy
      };
      
    }
    // update 
    hash_update(hash, bhash, in + bindex, in);
  };

}
