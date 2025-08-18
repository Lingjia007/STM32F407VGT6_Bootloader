#include "dwt_delay.h"


void dwt_delay_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // ???CYCCNT
}

void delay_us(uint32_t nus)
{
    uint32_t ticks = nus * (SystemCoreClock / 1000000); 
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < ticks); 
}

void delay_ms(uint16_t nms)
{	 
	for(uint16_t i = 0; i < nms; i++)
		delay_us(1000);		    
} 
