#include "circle_usart4.h"
#include <stdio.h>
#include <string.h>
#include "circular_buffer_v1.3.h"

#define UART4_BUF_SIZE         2048
#define MAX_RING_BUF_SIZE       3500
//#define DMA_DOUBLE_BUFFER_MODE  1

extern UART_HandleTypeDef huart4;
extern DMA_HandleTypeDef hdma_uart4_rx;
extern DMA_HandleTypeDef hdma_uart4_tx;

// buffer for DMA transport
unsigned char UART4_RxBuf0[ UART4_BUF_SIZE ] = {0};
unsigned char UART4_RxBuf1[ UART4_BUF_SIZE ] = {0};
unsigned char UART4_TxBuf[ UART4_BUF_SIZE ]  = {0};
//buffer for Circular buffer
#ifndef CIRC_BUFF_MALLOC
unsigned char UART4_TxBuff[ MAX_RING_BUF_SIZE ] = {0};
unsigned char UART4_RxBuff[ MAX_RING_BUF_SIZE ] = {0};
#endif


CircBuf_t UART4_RxCBuf, UART4_TxCBuf;

// 由HAL_UART_RxCpltCallback或IDLE中断调用，更新环形缓冲
void UART4_IdleCallback(void)
{
    // 停止DMA接收，计算接收到的数据长度
    HAL_DMA_Abort(&hdma_uart4_rx);

    uint32_t received_len = UART4_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart4_rx);

    CircBuf_Push(&UART4_RxCBuf, UART4_RxBuf0, received_len);
	
	// 处理数据示例：打印收到的数据长度
    //printf("Received %d bytes\n", received_len);

    // 重新启动DMA循环接收
    HAL_UART_Receive_DMA(&huart4, UART4_RxBuf0, UART4_BUF_SIZE);
}

// 发送完成回调，继续发送环形缓冲剩余数据
void UART4_TxCpltCallback(void)
{
    if (CircBuf_GetUsedSize(&UART4_TxCBuf) > 0)
    {
        uint32_t len = CircBuf_Pop(&UART4_TxCBuf, UART4_TxBuf, UART4_BUF_SIZE);
        HAL_UART_Transmit_DMA(&huart4, UART4_TxBuf, len);
    }
}

// 初始化环形缓冲和DMA接收
void UART4_Circle_Init(void)
{
#ifndef CIRC_BUFF_MALLOC
    CircBuf_Init(&UART4_RxCBuf, UART4_RxBuff, MAX_RING_BUF_SIZE);
    CircBuf_Init(&UART4_TxCBuf, UART4_TxBuff, MAX_RING_BUF_SIZE);
#else
    CircBuf_Alloc(&UART4_RxCBuf, MAX_RING_BUF_SIZE);
    CircBuf_Alloc(&UART4_TxCBuf, MAX_RING_BUF_SIZE);
#endif

    // 启动DMA循环接收
    HAL_UART_Receive_DMA(&huart4, UART4_RxBuf0, UART4_BUF_SIZE);
}

//void UART4_Send_(unsigned char *data, unsigned short len)
//{
//	memcpy(&UART4_TxBuf, data, len);
//	DMA_SetCurrDataCounter(DMA1_Stream4, len);
//	DMA_Cmd(DMA1_Stream4, ENABLE);
//}

unsigned int UART4_Send(unsigned char *data, unsigned short len)
{
    uint32_t was_empty = (CircBuf_GetUsedSize(&UART4_TxCBuf) == 0);

    uint32_t pushed = CircBuf_Push(&UART4_TxCBuf, data, len);

    if (was_empty && pushed > 0)
    {
        uint32_t send_len = CircBuf_Pop(&UART4_TxCBuf, UART4_TxBuf, UART4_BUF_SIZE);
        HAL_UART_Transmit_DMA(&huart4, UART4_TxBuf, send_len);
    }

    return pushed;
}

unsigned int UART4_Recv(unsigned char *data, unsigned short len)
{
	if (data == NULL) return 0;
		return CircBuf_Pop(&UART4_RxCBuf, data, len);
}

unsigned char UART4_At( unsigned short offset)
{
    return CircBuf_At(&UART4_RxCBuf, offset);
}

void UART4_Drop( unsigned short LenToDrop)
{
    CircBuf_Drop(&UART4_RxCBuf, LenToDrop);
}

unsigned int UART4_GetDataCount( void )
{
    return CircBuf_GetUsedSize(&UART4_RxCBuf);
}

void UART4_Free(void)
{
    CircBuf_Free(&UART4_RxCBuf);
}

unsigned char UART4_At2( unsigned short offset)
{
    return UART4_RxBuf0[offset];
}

int fputc(int ch, FILE *f)
{  
    while( (UART4->SR & 0x40) == 0 );
    
    UART4->DR = (uint8_t) ch;
    
    return ch;
}







