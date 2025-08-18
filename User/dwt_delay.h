#ifndef _DWT_DELAY_
#define _DWT_DELAY_


#include "stm32f4xx_hal.h"

void dwt_delay_init(void);
void delay_us(uint32_t nus);
void delay_ms(uint16_t nms);

#endif
