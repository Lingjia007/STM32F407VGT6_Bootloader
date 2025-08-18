#ifndef __UART4_H__
#define __UART4_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void UART4_Circle_Init(void);

unsigned int  UART4_Send(unsigned char *data, unsigned short len);
unsigned int  UART4_Recv(unsigned char *data, unsigned short len);
unsigned char UART4_At( unsigned short offset);
unsigned char UART4_At2( unsigned short offset);
void UART4_Drop( unsigned short LenToDrop);
unsigned int UART4_GetDataCount( void );

void UART4_Free(void);
void UART4_Send_(unsigned char *data, unsigned short len);
void UART4_IdleCallback(void);
void UART4_TxCpltCallback(void);

#ifdef __cplusplus
}
#endif

#endif
