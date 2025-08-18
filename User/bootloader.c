#include "bootloader.h"
#include "circle_usart4.h"
#include <stdio.h>
#include <string.h>
#include "circular_buffer_v1.3.h"

extern CRC_HandleTypeDef hcrc;
extern CircBuf_t UART4_RxCBuf, UART4_TxCBuf;
static uint8_t cmd_buf[64];
static uint8_t data_buf[BLOCK_SIZE + 4];

static uint32_t total_len = 0;
static uint32_t expected_crc = 0;
static uint32_t received_len = 0;

static BootloaderState state = WAIT_HEAD;

typedef void (*pFunction)(void);

void Bootloader_UART_SendAck(const char *ack)
{
    UART4_Send((uint8_t *)ack, 4);
}

void JumpToApp(void)
{
    uint32_t jumpAddress;
    pFunction Jump_To_Application;

    if (((*(__IO uint32_t*)APP_ADDRESS) & 0x2FFE0000) == 0x20000000)
    {
        __disable_irq();

        SCB->VTOR = APP_ADDRESS;

        __set_MSP(*(__IO uint32_t*)APP_ADDRESS);

        jumpAddress = *(__IO uint32_t*)(APP_ADDRESS + 4);
        Jump_To_Application = (pFunction)jumpAddress;

        Jump_To_Application();
    }
}

void Flash_Write(uint32_t address, uint8_t *data, uint16_t length)
{
	HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < length; i += 4) // 按字写（32bit）
    {
        uint32_t word = data[i] | (data[i+1] << 8) | (data[i+2] << 16) | (data[i+3] << 24);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, word) != HAL_OK)
        {
            // 写失败处理
            while (1);
        }
    }

    HAL_FLASH_Lock();
}

void Flash_Erase_App(void)
{
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    uint32_t page_error;

	erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
	erase.Banks        = FLASH_BANK_1;
	erase.Sector       = FLASH_SECTOR_5; // APP 起始扇区
	erase.NbSectors    = FLASH_SECTOR_MAX - FLASH_SECTOR_5 + 1;
	erase.VoltageRange = FLASH_VOLTAGE_RANGE;

	if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
	{
		// 擦除失败处理
		while (1);
	}

    HAL_FLASH_Lock();
}

/* CRC计算 */
uint32_t calc_crc32_hw(uint8_t *data, uint32_t length)
{
    uint32_t crc;

    // HAL CRC 硬件要求32位对齐和长度是32位倍数，这里手动填充
    uint32_t words = length / 4;
    uint32_t rem = length % 4;

    crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)data, words);

    if (rem)
    {
        uint8_t tail[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        for (uint32_t i = 0; i < rem; i++)
            tail[i] = data[words * 4 + i];
        crc = HAL_CRC_Accumulate(&hcrc, (uint32_t *)tail, 1);
    }

    return crc;
}

void Bootloader_Task(void)
{
    uint8_t temp[4];
	
    while (CircBuf_GetUsedSize(&UART4_RxCBuf) >= 4)
    {
        switch(state)
        {
            case WAIT_HEAD:
			{
				if(CircBuf_GetUsedSize(&UART4_RxCBuf) < 4)
					return;

				CircBuf_Read(&UART4_RxCBuf, temp, 4);     // 读取4字节，但不丢弃
				if (memcmp(temp, "HEAD", 4) == 0)
				{
					if (CircBuf_GetUsedSize(&UART4_RxCBuf) < 12)
						return; // 4字节HEAD + 8字节长度和CRC

					CircBuf_Drop(&UART4_RxCBuf, 4);
					CircBuf_Pop(&UART4_RxCBuf, cmd_buf, 8);
					total_len = *(uint32_t*)&cmd_buf[0];
					expected_crc = *(uint32_t*)&cmd_buf[4];
					Bootloader_UART_SendAck("ACKH");
					Flash_Erase_App();
					received_len = 0;
					state = WAIT_DATA;
				}
				else
				{
					CircBuf_Drop(&UART4_RxCBuf, 1);       // 不匹配则丢弃1字节，尝试同步
				}
				break;
			}

            case WAIT_TOTAL_LEN_CRC:
                if(CircBuf_GetUsedSize(&UART4_RxCBuf) >= 8)
                {
                    CircBuf_Pop(&UART4_RxCBuf, cmd_buf, 8);
                    total_len = *(uint32_t*)&cmd_buf[0];
                    expected_crc = *(uint32_t*)&cmd_buf[4];
                    Bootloader_UART_SendAck("ACKH");
                    Flash_Erase_App();
                    received_len = 0;
                    state = WAIT_DATA;
                }
                else
                {
                    return; // 等待更多数据
                }
                break;

            case WAIT_DATA:
                if(CircBuf_GetUsedSize(&UART4_RxCBuf) >= 4)
                {
                    CircBuf_Read(&UART4_RxCBuf, temp, 4);
                    if(memcmp(temp, "DATA", 4) != 0)
                    {
                        Bootloader_UART_SendAck("NACK");
                        CircBuf_Drop(&UART4_RxCBuf, 1);
                        break;
                    }
                    if(CircBuf_GetUsedSize(&UART4_RxCBuf) < 10)
                        return; // 等待更多数据

                    CircBuf_Drop(&UART4_RxCBuf, 4);
                    CircBuf_Pop(&UART4_RxCBuf, cmd_buf, 6);
                    uint32_t offset = *(uint32_t*)&cmd_buf[0];
                    uint16_t len = *(uint16_t*)&cmd_buf[4];

                    if(len > BLOCK_SIZE)
                    {
                        Bootloader_UART_SendAck("NACK");
                        break;
                    }

					if(CircBuf_GetUsedSize(&UART4_RxCBuf) < len)
							return; // 数据不够，等下次
                    CircBuf_Pop(&UART4_RxCBuf, data_buf, len);

                    uint8_t pad = (4 - (len & 3)) & 3;
                    for(uint8_t i = 0; i < pad; i++)
                        data_buf[len + i] = 0xFF;

                    __disable_irq();
                    Flash_Write(APP_ADDRESS + offset, data_buf, len + pad);
                    __enable_irq();

                    received_len += len;

                    Bootloader_UART_SendAck("ACKD");

                    if(received_len >= total_len)
                    {
                        state = PROCESSING;
                    }
                }
                else
                {
                    return;
                }
                break;

            case PROCESSING:
            {
                uint32_t crc = calc_crc32_hw((uint8_t*)APP_ADDRESS, total_len);
                if(crc == expected_crc)
                {
                    Bootloader_UART_SendAck("OK__");
                    HAL_Delay(100);
                    JumpToApp();
                }
                else
                {
                    Bootloader_UART_SendAck("ERR_");
                    state = WAIT_HEAD;
                }
                break;
            }
        }
    }
}
