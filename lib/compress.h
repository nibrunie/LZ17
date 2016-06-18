/** \defgroup compress compress
 *  \brief compression functions using LZ17 algorithms and format
 *   @{
 */

#include "arith_coding.h" 

typedef enum {
  /** lz17 compression without arithmetic coding */
  LZ17_NO_ENTROPY_CODING = 0,
  /** lz17 compression with arithmetic coding */
  LZ17_AC_ENTROPY_CODING = 1
} lz17_entropy_mode_t;


/** compression internal state */
typedef struct {
  /** entropy mode selected */
  lz17_entropy_mode_t entropy_mode;
  /** If arithmetic coding is used, store internal coder/decoder state */
  ac_state_t ac_state;

  /** amount of currently available room in output buffer */
  int   avail_out;
  /** next byte to be written in output buffer */
  char* next_out;
} lz17_state_t;

/** Initialize compression state 
 *  @param state compression internal state
 *  @param entropy_mode mode for entropy coding
 */
void lz17_compressInit(lz17_state_t* state, lz17_entropy_mode_t entropy_mode);

/** Try to compress @p avail_in from @p in buffer into @p out without exceeding @p avail_out bytes
 *  @param zstate internal compression state
 *  @param out output buffer
 *  @param avail_out available size in @p out
 *  @param in input data
 *  @param avail_in size of input data
 *  @return positive compressed size on success, otherwise negative error code 
 */
int lz17_compressBufferToBuffer(lz17_state_t* zstate, char* out, size_t avail_out, char* in, size_t avail_in);

/** Decompress from a byte buffer into a byte buffer
 * @param out output buffer to receive decompress data
 * @param avail_out size of available space for decompression in @p out
 * @param int input buffer containing compress frame
 * @param avail_in size of input data
 * @return positive compressed size on success, otherwise negative error code 
 */
int lz17_decompressBufferToBuffer(char* out, size_t avail_out, char* in, size_t avail_in);


/** Extract the expanded size as stored in a lz17 buffer, assuming lz17 is a
 *  complete buffer with lz17 header
 *  @param in input buffer
 *  @return expanded size of decompressed @in
 */
int lz17_bufferExtractExpandedSize(char* in);

/** Display the content (lz17 commands) of a compressed stream
 *  @param in input stream (compressed)
 *  @param avail_in size of input stream
 *  @return error code on error, LZ17_OK on success
 */
int lz17_displayCompressedStream(char* in, size_t avail_in);

/** @} */
