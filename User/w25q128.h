/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * Change Logs:
 * Date           Author       Notes
 * 2024-08-02     LCKFB-LP     first version
 * 2025-01-27     Lingma       移植到HAL库
 */
#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include "stm32f4xx_hal.h"
#include "spi.h"

// W25Q128指令表
#define W25X_WriteEnable 0x06
#define W25X_WriteDisable 0x04
#define W25X_ReadStatusReg 0x05
#define W25X_WriteStatusReg 0x01
#define W25X_ReadData 0x03
#define W25X_FastReadData 0x0B
#define W25X_FastReadDual 0x3B
#define W25X_PageProgram 0x02
#define W25X_BlockErase 0xD8
#define W25X_SectorErase 0x20
#define W25X_ChipErase 0xC7
#define W25X_PowerDown 0xB9
#define W25X_ReleasePowerDown 0xAB
#define W25X_DeviceID 0xAB
#define W25X_ManufactDeviceID 0x90
#define W25X_JedecDeviceID 0x9F

// 定义W25Q128的CS引脚宏
#define W25QXX_CS_GPIO_PORT GPIOA
#define W25QXX_CS_PIN GPIO_PIN_4
#define W25QXX_CS_ON(x) HAL_GPIO_WritePin(W25QXX_CS_GPIO_PORT, W25QXX_CS_PIN, (x) ? GPIO_PIN_RESET : GPIO_PIN_SET)

// W25Q128相关函数声明
void w25q128_init(void);
uint8_t spi_read_write_byte(uint8_t dat);
uint16_t W25Q128_readID(void);
void W25Q128_write_enable(void);
void W25Q128_wait_busy(void);
void W25Q128_erase_sector(uint32_t addr);
void W25Q128_write(uint8_t *buffer, uint32_t addr, uint16_t numbyte);
void W25Q128_read(uint8_t *buffer, uint32_t read_addr, uint16_t read_length);

#endif
