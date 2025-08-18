#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

#define BLOCK_SIZE         256       // 你实际最大数据块大小
#define APP_ADDRESS 0x08020000
#define FLASH_VOLTAGE_RANGE  FLASH_VOLTAGE_RANGE_3 // 2.7~3.6V
#define FLASH_SECTOR_MAX     11   // F405 有到 Sector 11
#define PAGE_SIZE   2048  // STM32F103C8 is 2KB/page
#define BLOCK_SIZE 256

typedef enum {
    WAIT_HEAD,
    WAIT_TOTAL_LEN_CRC,
    WAIT_DATA,
    PROCESSING,
} BootloaderState;

void Bootloader_Task(void);

#ifdef __cplusplus
}
#endif

#endif
