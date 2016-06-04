
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


} hash_t;

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

  lz17_hash_t* hash = init_hash(); 

  // byte-index
  int bindex;
  for (bindex = 0; bindex < avail_in - hash_in_size; ++bindex) {
    int bhash = lz17_hash(in + bindex, 4);
    // if the hash table contains a match, then check and extend it
    if (hash_contains(hash, bhash)) {

    }
    // update 
    hash_update(hash, bhash, in + bindex, in);
  };

}
