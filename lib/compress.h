/** \defgroup compress compress
 *  \brief compression functions using LZ17 algorithms and format
 *   @{
 */

/** Try to compress @p avail_in from @p in buffer into @p out without exceeding @p avail_out bytes
 *  @param out output buffer
 *  @param avail_out available size in @p out
 *  @param in input data
 *  @param avail_in size of input data
 *  @return positive compressed size on success, otherwise negative error code 
 */
int lz17_compressBufferToBuffer(char* out, size_t avail_out, char* in, size_t avail_in);

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
