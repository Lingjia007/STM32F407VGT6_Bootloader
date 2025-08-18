#ifndef __CIRC_BUF__
#define __CIRC_BUF__

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(a, b)       (((a) > (b)) ? (b) : (a))
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))

//循环缓冲区结构体
typedef struct CIRC_BUF
{
    unsigned int Size;
    unsigned int Header;
    unsigned int Tailer;
    unsigned char *Buffer;
} CircBuf_t;

/**
 * @brief     Check if Num is power of 2
 *
 * @param[in] Num   the number to check
 *
 * @return          1 if Num is power of 2
 */
unsigned long long IsPowerOf2 (unsigned long long Num);


/**
 * @brief     calculate the minimum number that round up to the next power of 2
 *
 * @param[in] Num   the number to calculate
 *
 * @return          the number that round up to the next power of 2 (0x100 if Num is 0xf0, 0x81, 0xa3 ... )
 */
unsigned long RoundUp_PowerOf2 (unsigned long Num);


/**
 * @brief     calculate the minimum number that round down to the next power of 2
 *
 * @param[] Num the number to check
 *
 * @return    the number that round up to the last power of 2 (4 if Num is 5,6,7, 8 if Num is 9,10,11 ... )
 */
unsigned long RoundDown_PowerOf2(unsigned long Num);


/**
 * @brief     Init the Circular buffer with a array
 *
 * @param[in] CBuf      The circular buffer to initial
 * @param[in] Buff      the buffer for circular buffer to store data
 * @param[in] Size      the size of buffer
 *
 * @return      the Round Down(Power Of 2) size that the circular  buffer to be used
 */
int          CircBuf_Init(CircBuf_t *CBuf, unsigned char *Buff, unsigned int Size);


/**
 * @brief     Circular Buffer initialization
 *
 * @param[in] CBuf      the circular buffer to initialization
 * @param[in] Size      size of the circular buffer
 *
 * @return    1 if memery allocation success
 *            0 if fail
 */
int          CircBuf_Alloc (CircBuf_t *CBuf, unsigned int Size);


/**
 * @brief     delete circular buffer and release the memery
 *
 * @param[in] CBuf  the circular buffer to delete
 */
void         CircBuf_Free (CircBuf_t *CBuf);


/**
 * @brief     put data into the circular buffer
 *
 * @param[in] CBuf      the circular buffer that will store the data
 * @param[in] data      the data to store into circular buffer
 * @param[in] LenToPush  the length of data to store into circular buffer
 *
 * @return      the actual size stored into circular buffer
 */
unsigned int CircBuf_Push (CircBuf_t *CBuf, unsigned char *data, unsigned int LenToPush);

/**
 * @brief     get data from circular buffer
 *
 * @param[in] CBuf      the circular buffer that stored data
 * @param[in] data      target buffer that will store the data that from circular buffer
 * @param[in] LenToPop  the length that wan't to get from circular buffer
 *
 * @return      actual length that get from circular buffer
 */
unsigned int CircBuf_Pop  (CircBuf_t *CBuf, unsigned char *data, unsigned int LenToPop);


/**
 * @brief     get one char from circular buffer
 *
 * @param[in] CBuf      the circular buffer that stored data
 * @param[n] data       target buffer that will store the data that from circular buffer
 *
 * @return              actual length that get from circular buffer
 */
unsigned int CircBuf_PopOneChar (CircBuf_t *CBuf, unsigned char *data);


/**
 * @brief     for access data at Tailer + offset
 *
 * @param[in] CBuf      the circular buffer that stored data
 * @param[in] offset    the offset of Tailer
 *
 * @return              the data at Buffer[Tailer + offset]
 */
unsigned char CircBuf_At(CircBuf_t *CBuf, unsigned int offset);


/**
 * @brief     get data from circular buffer
 *
 * @param[in] CBuf      the circular buffer that stored data
 * @param[in] data      target buffer that will store the data that from circular buffer
 * @param[in] LenToRead  the length that wan't to get from circular buffer
 *
 * @return      actual length that get from circular buffer
 */
unsigned int CircBuf_Read(CircBuf_t *CBuf, unsigned char *data, unsigned int LenToRead);

/**
 * @brief     drop the the size of data at tailer
 *
 * @param[in] CBuf          the circular buffer that stored data
 * @param[in] LenToDrop     the size of data at tailer of circular_buffer to drop
 */
void         CircBuf_Drop(CircBuf_t *CBuf, unsigned int LenToDrop);


/**
 * @brief     get the Available memery size of circular buffer
 *
 * @param[in] CBuf  the circular buffer to get size
 *
 * @return          Available size of the circular buffer
 */
unsigned int CircBuf_GetAvalaibleSize (CircBuf_t *CBuf);


/**
* @brief     get the used memery size of circular buffer
*
* @param[in] CBuf  the circular buffer to get size
*
* @return          used size of the circular buffer
*/
unsigned int CircBuf_GetUsedSize (CircBuf_t *CBuf);


/**
 * @brief     check if the circular buffer is empty
 *
 * @param[in] CBuf  the circular buffer to check
 *
 * @return          1   if no data stored in the circular buffer
 *                  0   if the size of circular buffer equals to 0
 *                      or some data stored in the circular buffer
 */
unsigned int CircBuf_IsEmpty (CircBuf_t *CBuf);


/**
 * @brief     check if the circular buffer is full
 *
 * @param[in] CBuf  the circular buffer to check
 *
 * @return      1 if the size of circular buffer equals to 0
 *                  or no Available space of circular buffer
 */
unsigned int CircBuf_IsFull (CircBuf_t *CBuf);

#ifdef __cplusplus
}
#endif

#endif

