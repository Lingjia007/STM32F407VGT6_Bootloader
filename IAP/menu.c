/**
  ******************************************************************************
  * @file    IAP/IAP_Main/Src/menu.c
  * @author  MCD Application Team

  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file,
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded
  *             - configuring the write protection of the Flash sectors where the
  *               user loads his binary file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/** @addtogroup STM32F4xx_IAP_Main
 * @{
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include "ymodem.h"
#include "ff.h"
#include "lfs_spi_flash_adapter.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
uint8_t aFileName[FILE_NAME_LENGTH];

/* External variables --------------------------------------------------------*/
extern FATFS SDFatFS; /* File system object for SD logical drive */

/* Private function prototypes -----------------------------------------------*/
void SerialDownload(void);
void SerialUpload(void);
void TFCard_Update(void);
void TFCardToLFS_Update(void);
void LFS_Update(void);

/* Private defines -----------------------------------------------------------*/
#define MAX_BIN_FILES 10          // 最大支持的bin文件数量
#define BIN_FILE_EXTENSION ".bin" // bin文件扩展名

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Download a file via serial port
 * @param  None
 * @retval None
 */
void SerialDownload(void)
{
  uint8_t number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  Serial_PutString((uint8_t *)"Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  result = Ymodem_Receive(&size);
  if (result == COM_OK)
  {
    HAL_Delay(100);
    Serial_PutString((uint8_t *)"\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
    Serial_PutString(aFileName);
    Int2Str(number, size);
    Serial_PutString((uint8_t *)"\n\r Size: ");
    Serial_PutString(number);
    Serial_PutString((uint8_t *)" Bytes\r\n");
    Serial_PutString((uint8_t *)"-------------------\n");
  }
  else if (result == COM_LIMIT)
  {
    Serial_PutString((uint8_t *)"\n\n\rThe image size is higher than the allowed space memory!\n\r");
  }
  else if (result == COM_DATA)
  {
    Serial_PutString((uint8_t *)"\n\n\rVerification failed!\n\r");
  }
  else if (result == COM_ABORT)
  {
    Serial_PutString((uint8_t *)"\r\n\nAborted by user.\n\r");
  }
  else
  {
    Serial_PutString((uint8_t *)"\n\rFailed to receive the file!\n\r");
  }
}

/**
 * @brief  Upload a file via serial port.
 * @param  None
 * @retval None
 */
void SerialUpload(void)
{
  uint8_t status = 0;

  Serial_PutString((uint8_t *)"\n\n\rSelect Receive File\n\r");

  HAL_UART_Receive(&UartHandle, &status, 1, RX_TIMEOUT);
  if (status == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit((uint8_t *)APPLICATION_ADDRESS, (const uint8_t *)"UploadedFlashImage.bin", USER_FLASH_SIZE);

    if (status != 0)
    {
      Serial_PutString((uint8_t *)"\n\rError Occurred while Transmitting File\n\r");
    }
    else
    {
      Serial_PutString((uint8_t *)"\n\rFile uploaded successfully \n\r");
    }
  }
}

/**
 * @brief  TF卡更新函数
 * @param  None
 * @retval None
 */
void TFCard_Update(void)
{
  FRESULT res;
  DIR dir;
  FILINFO fno;
  char bin_files[MAX_BIN_FILES][256]; // 存储bin文件名
  uint8_t bin_count = 0;
  uint8_t key = 0;
  uint32_t file_size = 0;
  uint8_t buffer[4096]; // 读取缓冲区
  UINT bytes_read;
  uint32_t flash_address = APPLICATION_ADDRESS;
  image_header_t *header = (image_header_t *)APPLICATION_ADDRESS;

  // 初始化SD卡和FATFS
  Serial_PutString((uint8_t *)"\r\nInitializing TF card...\r\n");
  res = f_mount(&SDFatFS, "0:", 1);
  if (res != FR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to mount TF card!\r\n");
    return;
  }

  // 扫描TF卡上的bin文件
  Serial_PutString((uint8_t *)"Scanning for bin files...\r\n");
  res = f_opendir(&dir, "0:/");
  if (res != FR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to open directory!\r\n");
    f_mount(NULL, "0:", 0);
    return;
  }

  // 列出所有bin文件
  Serial_PutString((uint8_t *)"\r\nFound bin files:\r\n");
  while (1)
  {
    res = f_readdir(&dir, &fno);
    if (res != FR_OK || fno.fname[0] == 0)
    {
      break;
    }

    // 检查是否是bin文件
    if (!(fno.fattrib & AM_DIR))
    {
      char *ext = strrchr(fno.fname, '.');
      if (ext != NULL && strcmp(ext, BIN_FILE_EXTENSION) == 0)
      {
        if (bin_count < MAX_BIN_FILES)
        {
          strcpy(bin_files[bin_count], fno.fname);
          Serial_PutString((uint8_t *)"  ");
          Serial_PutString((uint8_t *)"[");
          Int2Str((uint8_t *)buffer, bin_count + 1);
          Serial_PutString((uint8_t *)buffer);
          Serial_PutString((uint8_t *)"] ");
          Serial_PutString((uint8_t *)fno.fname);
          Serial_PutString((uint8_t *)"\r\n");
          bin_count++;
        }
      }
    }
  }
  f_closedir(&dir);

  if (bin_count == 0)
  {
    Serial_PutString((uint8_t *)"No bin files found!\r\n");
    f_mount(NULL, "0:", 0);
    return;
  }

  // 提示用户选择bin文件
  Serial_PutString((uint8_t *)"\r\nPlease select a bin file (1-");
  Int2Str((uint8_t *)buffer, bin_count);
  Serial_PutString((uint8_t *)buffer);
  Serial_PutString((uint8_t *)") or press 'a' to abort: ");

  // 等待用户输入
  while (1)
  {
    if (HAL_UART_Receive(&UartHandle, &key, 1, RX_TIMEOUT) == HAL_OK)
    {
      if (key == 'a' || key == 'A')
      {
        Serial_PutString((uint8_t *)"\r\nOperation aborted!\r\n");
        f_mount(NULL, "0:", 0);
        return;
      }
      else if (key >= '1' && key <= '0' + bin_count)
      {
        break;
      }
      else
      {
        Serial_PutString((uint8_t *)"Invalid input!\r\n");
      }
    }
  }

  // 打开选中的bin文件
  uint8_t file_index = key - '1';
  Serial_PutString((uint8_t *)"\r\nOpening file: ");
  Serial_PutString((uint8_t *)bin_files[file_index]);
  Serial_PutString((uint8_t *)"\r\n");

  // 添加驱动器号前缀
  char full_path[260];
  sprintf(full_path, "0:/%s", bin_files[file_index]);

  FIL file;
  res = f_open(&file, full_path, FA_READ);
  if (res != FR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to open file!\r\n");
    f_mount(NULL, "0:", 0);
    return;
  }

  // 获取文件大小
  file_size = f_size(&file);
  Serial_PutString((uint8_t *)"File size: ");
  Int2Str((uint8_t *)buffer, file_size);
  Serial_PutString((uint8_t *)buffer);
  Serial_PutString((uint8_t *)" bytes\r\n");

  // 检查Flash空间是否足够
  if (file_size > USER_FLASH_SIZE)
  {
    Serial_PutString((uint8_t *)"Error: File size exceeds Flash capacity!\r\n");
    f_close(&file);
    f_mount(NULL, "0:", 0);
    return;
  }

  // 擦除Flash
  Serial_PutString((uint8_t *)"Erasing Flash...\r\n");
  if (FLASH_If_Erase(APPLICATION_ADDRESS) != FLASHIF_OK)
  {
    Serial_PutString((uint8_t *)"Flash erase failed!\r\n");
    f_close(&file);
    f_mount(NULL, "0:", 0);
    return;
  }

  // 读取并写入bin文件到Flash
  Serial_PutString((uint8_t *)"Writing file to Flash...\r\n");
  uint32_t total_written = 0;
  while (total_written < file_size)
  {
    uint32_t bytes_to_read = (file_size - total_written) > sizeof(buffer) ? sizeof(buffer) : (file_size - total_written);
    res = f_read(&file, buffer, bytes_to_read, &bytes_read);
    if (res != FR_OK || bytes_read != bytes_to_read)
    {
      Serial_PutString((uint8_t *)"File read error!\r\n");
      f_close(&file);
      f_mount(NULL, "0:", 0);
      return;
    }

    // 写入Flash - 计算正确的32位字数，向上取整
    uint32_t word_count = (bytes_read + 3) / 4; // 确保非4字节倍数也能正确处理
    if (FLASH_If_Write(flash_address, (uint32_t *)buffer, word_count) != FLASHIF_OK)
    {
      Serial_PutString((uint8_t *)"Flash write failed!\r\n");
      f_close(&file);
      f_mount(NULL, "0:", 0);
      return;
    }

    flash_address += bytes_read;
    total_written += bytes_read;
  }

  Serial_PutString((uint8_t *)"\r\nFile written successfully!\r\n");

  // 检查应用程序是否有效
  Serial_PutString((uint8_t *)"Checking application validity...\r\n");
  if (header->ih_magic == UBOOT_MAGIC)
  {
    Serial_PutString((uint8_t *)"Valid U-Boot header found!\r\n");
    Serial_PutString((uint8_t *)"Application name: ");
    Serial_PutString((uint8_t *)header->ih_name);
    Serial_PutString((uint8_t *)"\r\n");
  }
  else
  {
    // 检查栈指针
    uint32_t sp = *(uint32_t *)APPLICATION_ADDRESS;
    if ((sp >= 0x20000000U) && (sp <= 0x2002FFFFU))
    {
      Serial_PutString((uint8_t *)"No U-Boot header, but valid stack pointer found!\r\n");
    }
    else
    {
      Serial_PutString((uint8_t *)"Warning: Application may not be valid!\r\n");
    }
  }

  // 关闭文件并卸载文件系统
  f_close(&file);
  f_mount(NULL, "0:", 0);
  Serial_PutString((uint8_t *)"TF card update completed!\r\n");
}

/**
 * @brief  Display the Main Menu on HyperTerminal
 * @param  None
 * @retval None
 */
void Main_Menu(void)
{
  uint8_t key = 0;

  Serial_PutString((uint8_t *)"\r\n======================================================================");
  Serial_PutString((uint8_t *)"\r\n=              (C) COPYRIGHT 2016 STMicroelectronics                 =");
  Serial_PutString((uint8_t *)"\r\n=                                                                    =");
  Serial_PutString((uint8_t *)"\r\n=          STM32F4xx In-Application Programming Application          =");
  Serial_PutString((uint8_t *)"\r\n=                                                                    =");
  Serial_PutString((uint8_t *)"\r\n=                       By MCD Application Team                      =");
  Serial_PutString((uint8_t *)"\r\n======================================================================");
  Serial_PutString((uint8_t *)"\r\n\r\n");

  while (1)
  {

    /* Test if any sector of Flash memory where user application will be loaded is write protected */
    FlashProtection = FLASH_If_GetWriteProtectionStatus();

    Serial_PutString((uint8_t *)"\r\n======================== Main Menu =========================\r\n\n");
    Serial_PutString((uint8_t *)"  Download image to the internal Flash ----------------- 1\r\n\n");
    Serial_PutString((uint8_t *)"  Download image from TF to the internal Flash --------- 2\r\n\n");
    Serial_PutString((uint8_t *)"  Download image from LFS to the internal Flash -------- 3\r\n\n");
    Serial_PutString((uint8_t *)"  Upload image from the internal Flash ----------------- 4\r\n\n");
    Serial_PutString((uint8_t *)"  Execute the loaded application ----------------------- 5\r\n\n");
    Serial_PutString((uint8_t *)"  Store image from TF to SPI-Flash LFS ----------------- 6\r\n\n");

    if (FlashProtection != FLASHIF_PROTECTION_NONE)
    {
      Serial_PutString((uint8_t *)"  Disable the write protection ------------------------- 6\r\n\n");
    }
    else
    {
      Serial_PutString((uint8_t *)"  Enable the write protection -------------------------- 7\r\n\n");
    }
    Serial_PutString((uint8_t *)"============================================================\r\n\n");

    /* Clean the input path */
    __HAL_UART_FLUSH_DRREGISTER(&UartHandle);

    /* Receive key */
    HAL_UART_Receive(&UartHandle, &key, 1, RX_TIMEOUT);

    switch (key)
    {
    case '1':
      /* Download user application in the Flash */
      SerialDownload();
      break;
    case '2':
      /* Update from TF card */
      TFCard_Update();
      break;
    case '3':
      /* Update from LFS */
      LFS_Update();
      break;
    case '4':
      /* Upload user application from the Flash */
      SerialUpload();
      break;
    case '5':
      Serial_PutString((uint8_t *)"Start program execution......\r\n\n");
      // /* execute the new program */
      // JumpAddress = *(__IO uint32_t *)(APPLICATION_ADDRESS + 4);
      // /* Jump to user application */
      // JumpToApplication = (pFunction)JumpAddress;
      // /* Deinitialize all used modules */
      // __disable_irq();
      // __HAL_RCC_PWR_CLK_DISABLE();
      // HAL_RCC_DeInit();
      // /* Initialize user application's Stack Pointer */
      // __set_MSP(*(__IO uint32_t *)APPLICATION_ADDRESS);
      // JumpToApplication();
      HAL_NVIC_SystemReset();
      break;
    case '6':
      /* Store image from TF to SPI-Flash LFS */
      TFCardToLFS_Update();
      break;
    case '7':
      if (FlashProtection != FLASHIF_PROTECTION_NONE)
      {
        /* Disable the write protection */
        if (FLASH_If_WriteProtectionConfig(OB_WRPSTATE_DISABLE) == HAL_OK)
        {
          Serial_PutString((uint8_t *)"Write Protection disabled...\r\n");
          Serial_PutString((uint8_t *)"System will now restart...\r\n");
          /* Launch the option byte loading */
          HAL_FLASH_OB_Launch();
          /* Ulock the flash */
          HAL_FLASH_Unlock();
        }
        else
        {
          Serial_PutString((uint8_t *)"Error: Flash write un-protection failed...\r\n");
        }
      }
      else
      {
        if (FLASH_If_WriteProtectionConfig(OB_WRPSTATE_ENABLE) == HAL_OK)
        {
          Serial_PutString((uint8_t *)"Write Protection enabled...\r\n");
          Serial_PutString((uint8_t *)"System will now restart...\r\n");
          /* Launch the option byte loading */
          HAL_FLASH_OB_Launch();
        }
        else
        {
          Serial_PutString((uint8_t *)"Error: Flash write protection failed...\r\n");
        }
      }
      break;
    default:
      Serial_PutString((uint8_t *)"Invalid Number ! ==> The number should be either 1, 2, 3, 4, 5, 6 or 7\r");
      break;
    }
  }
}

/**
 * @brief  从TF卡选择.bin文件存储到SPI-Flash的LFS文件系统
 * @param  None
 * @retval None
 */
void TFCardToLFS_Update(void)
{
  FRESULT res;
  DIR dir;
  FILINFO fno;
  char bin_files[MAX_BIN_FILES][256]; // 存储bin文件名
  uint8_t bin_count = 0;
  uint8_t key = 0;
  uint32_t file_size = 0;
  uint8_t buffer[4096]; // 读取缓冲区
  UINT bytes_read;
  uint32_t total_written = 0;
  int err;

  // 初始化SD卡和FATFS
  Serial_PutString((uint8_t *)"\r\nInitializing TF card...\r\n");
  res = f_mount(&SDFatFS, "0:", 1);
  if (res != FR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to mount TF card!\r\n");
    return;
  }

  // 初始化SPI Flash和LittleFS
  Serial_PutString((uint8_t *)"\r\nInitializing SPI Flash and LittleFS...\r\n");
  err = lfs_spi_flash_init();
  if (err != 0)
  {
    Serial_PutString((uint8_t *)"SPI Flash initialization failed!\r\n");
    f_mount(NULL, "0:", 0);
    return;
  }

  // 挂载LittleFS文件系统
  Serial_PutString((uint8_t *)"Mounting LittleFS...\r\n");
  err = lfs_spi_flash_mount(NULL); // 使用NULL让函数使用内部的全局实例
  if (err != LFS_ERR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to mount LittleFS!\r\n");
    f_mount(NULL, "0:", 0);
    return;
  }

  // 扫描TF卡上的bin文件
  Serial_PutString((uint8_t *)"Scanning TF card for bin files...\r\n");
  res = f_opendir(&dir, "0:/");
  if (res != FR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to open TF card directory!\r\n");
    lfs_spi_flash_unmount(NULL);
    f_mount(NULL, "0:", 0);
    return;
  }

  // 列出所有bin文件
  Serial_PutString((uint8_t *)"\r\nFound bin files on TF card:\r\n");
  while (1)
  {
    res = f_readdir(&dir, &fno);
    if (res != FR_OK || fno.fname[0] == 0)
    {
      break;
    }

    // 检查是否是bin文件
    if (!(fno.fattrib & AM_DIR))
    {
      char *ext = strrchr(fno.fname, '.');
      if (ext != NULL && strcmp(ext, BIN_FILE_EXTENSION) == 0)
      {
        if (bin_count < MAX_BIN_FILES)
        {
          strcpy(bin_files[bin_count], fno.fname);
          Serial_PutString((uint8_t *)"  ");
          Serial_PutString((uint8_t *)"[");
          Int2Str((uint8_t *)buffer, bin_count + 1);
          Serial_PutString((uint8_t *)buffer);
          Serial_PutString((uint8_t *)"] ");
          Serial_PutString((uint8_t *)fno.fname);
          Serial_PutString((uint8_t *)"\r\n");
          bin_count++;
        }
      }
    }
  }
  f_closedir(&dir);

  if (bin_count == 0)
  {
    Serial_PutString((uint8_t *)"No bin files found on TF card!\r\n");
    lfs_spi_flash_unmount(NULL);
    f_mount(NULL, "0:", 0);
    return;
  }

  // 提示用户选择bin文件
  Serial_PutString((uint8_t *)"\r\nPlease select a bin file (1-");
  Int2Str((uint8_t *)buffer, bin_count);
  Serial_PutString((uint8_t *)buffer);
  Serial_PutString((uint8_t *)") or press 'a' to abort: ");

  // 等待用户输入
  while (1)
  {
    if (HAL_UART_Receive(&UartHandle, &key, 1, RX_TIMEOUT) == HAL_OK)
    {
      if (key == 'a' || key == 'A')
      {
        Serial_PutString((uint8_t *)"\r\nOperation aborted!\r\n");
        lfs_spi_flash_unmount(NULL);
        f_mount(NULL, "0:", 0);
        return;
      }
      else if (key >= '1' && key <= '0' + bin_count)
      {
        break;
      }
      else
      {
        Serial_PutString((uint8_t *)"Invalid input!\r\n");
      }
    }
  }

  // 打开选中的bin文件
  uint8_t file_index = key - '1';
  Serial_PutString((uint8_t *)"\r\nOpening file: ");
  Serial_PutString((uint8_t *)bin_files[file_index]);
  Serial_PutString((uint8_t *)"\r\n");

  // 添加驱动器号前缀
  char full_path[260];
  sprintf(full_path, "0:/%s", bin_files[file_index]);

  FIL file;
  res = f_open(&file, full_path, FA_READ);
  if (res != FR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to open file!\r\n");
    lfs_spi_flash_unmount(NULL);
    f_mount(NULL, "0:", 0);
    return;
  }

  // 获取文件大小
  file_size = f_size(&file);
  Serial_PutString((uint8_t *)"File size: ");
  Int2Str((uint8_t *)buffer, file_size);
  Serial_PutString((uint8_t *)buffer);
  Serial_PutString((uint8_t *)" bytes\r\n");

  // 在LFS中创建或打开文件
  Serial_PutString((uint8_t *)"Creating file in LittleFS...\r\n");
  struct lfs_file lfs_file_obj;
  err = lfs_file_open(&lfs_instance, &lfs_file_obj, bin_files[file_index], LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
  if (err != LFS_ERR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to create file in LittleFS!\r\n");
    f_close(&file);
    lfs_spi_flash_unmount(NULL);
    f_mount(NULL, "0:", 0);
    return;
  }

  // 读取并写入bin文件到LFS
  Serial_PutString((uint8_t *)"Writing file to LittleFS...\r\n");
  total_written = 0;
  while (total_written < file_size)
  {
    uint32_t bytes_to_read = (file_size - total_written) > sizeof(buffer) ? sizeof(buffer) : (file_size - total_written);
    res = f_read(&file, buffer, bytes_to_read, &bytes_read);
    if (res != FR_OK || bytes_read != bytes_to_read)
    {
      Serial_PutString((uint8_t *)"File read error!\r\n");
      lfs_file_close(&lfs_instance, &lfs_file_obj);
      f_close(&file);
      lfs_spi_flash_unmount(NULL);
      f_mount(NULL, "0:", 0);
      return;
    }

    // 写入LFS文件系统
    int written = lfs_file_write(&lfs_instance, &lfs_file_obj, buffer, bytes_read);
    if (written < 0)
    {
      Serial_PutString((uint8_t *)"Failed to write to LittleFS file!\r\n");
      lfs_file_close(&lfs_instance, &lfs_file_obj);
      f_close(&file);
      lfs_spi_flash_unmount(NULL);
      f_mount(NULL, "0:", 0);
      return;
    }

    total_written += written;

    // 显示进度
    Serial_PutString((uint8_t *)"Progress: ");
    Int2Str((uint8_t *)buffer, (total_written * 100) / file_size);
    Serial_PutString((uint8_t *)buffer);
    Serial_PutString((uint8_t *)"%\r");
  }

  Serial_PutString((uint8_t *)"\r\nFile written successfully to LittleFS!\r\n");

  // 确保数据被刷新到存储
  err = lfs_file_sync(&lfs_instance, &lfs_file_obj);
  if (err != LFS_ERR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to sync file!\r\n");
  }

  // 关闭文件
  err = lfs_file_close(&lfs_instance, &lfs_file_obj);
  if (err != LFS_ERR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to close LittleFS file!\r\n");
  }

  f_close(&file);
  lfs_spi_flash_unmount(NULL);
  f_mount(NULL, "0:", 0);

  Serial_PutString((uint8_t *)"\r\nTF card to LittleFS update completed!\r\n");
}

/**
 * @brief  从LFS系统选择.bin文件并更新镜像到Flash
 * @param  None
 * @retval None
 */
void LFS_Update(void)
{
  int err;
  struct lfs_file file;
  char bin_files[MAX_BIN_FILES][256]; // 存储bin文件名
  uint8_t bin_count = 0;
  uint8_t key = 0;
  uint32_t file_size = 0;
  uint8_t buffer[4096]; // 读取缓冲区
  uint32_t flash_address = APPLICATION_ADDRESS;
  uint32_t total_read = 0;
  image_header_t *header = (image_header_t *)APPLICATION_ADDRESS;

  // 初始化SPI Flash和LittleFS
  Serial_PutString((uint8_t *)"\r\nInitializing SPI Flash and LittleFS...\r\n");
  err = lfs_spi_flash_init();
  if (err != 0)
  {
    Serial_PutString((uint8_t *)"SPI Flash initialization failed!\r\n");
    return;
  }

  // 挂载LittleFS文件系统
  Serial_PutString((uint8_t *)"Mounting LittleFS...\r\n");
  err = lfs_spi_flash_mount(NULL); // 使用NULL让函数使用内部的全局实例
  if (err != LFS_ERR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to mount LittleFS!\r\n");
    return;
  }

  // 扫描LFS上的bin文件
  Serial_PutString((uint8_t *)"Scanning LittleFS for bin files...\r\n");
  struct lfs_dir dir;
  err = lfs_dir_open(&lfs_instance, &dir, "/");
  if (err != LFS_ERR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to open LittleFS directory!\r\n");
    lfs_spi_flash_unmount(NULL);
    return;
  }

  // 列出所有bin文件
  Serial_PutString((uint8_t *)"\r\nFound bin files in LittleFS:\r\n");
  struct lfs_info info;
  while (lfs_dir_read(&lfs_instance, &dir, &info) > 0)
  {
    // 检查是否是bin文件
    if (info.type == LFS_TYPE_REG)
    {
      char *ext = strrchr(info.name, '.');
      if (ext != NULL && strcmp(ext, BIN_FILE_EXTENSION) == 0)
      {
        if (bin_count < MAX_BIN_FILES)
        {
          strcpy(bin_files[bin_count], info.name);
          Serial_PutString((uint8_t *)"  ");
          Serial_PutString((uint8_t *)"[");
          Int2Str((uint8_t *)buffer, bin_count + 1);
          Serial_PutString((uint8_t *)buffer);
          Serial_PutString((uint8_t *)"] ");
          Serial_PutString((uint8_t *)info.name);
          Serial_PutString((uint8_t *)" (size: ");
          Int2Str((uint8_t *)buffer, info.size);
          Serial_PutString((uint8_t *)buffer);
          Serial_PutString((uint8_t *)" bytes)\r\n");
          bin_count++;
        }
      }
    }
  }
  lfs_dir_close(&lfs_instance, &dir);

  if (bin_count == 0)
  {
    Serial_PutString((uint8_t *)"No bin files found in LittleFS!\r\n");
    lfs_spi_flash_unmount(NULL);
    return;
  }

  // 提示用户选择bin文件
  Serial_PutString((uint8_t *)"\r\nPlease select a bin file (1-");
  Int2Str((uint8_t *)buffer, bin_count);
  Serial_PutString((uint8_t *)buffer);
  Serial_PutString((uint8_t *)") or press 'a' to abort: ");

  // 等待用户输入
  while (1)
  {
    if (HAL_UART_Receive(&UartHandle, &key, 1, RX_TIMEOUT) == HAL_OK)
    {
      if (key == 'a' || key == 'A')
      {
        Serial_PutString((uint8_t *)"\r\nOperation aborted!\r\n");
        lfs_spi_flash_unmount(NULL);
        return;
      }
      else if (key >= '1' && key <= '0' + bin_count)
      {
        break;
      }
      else
      {
        Serial_PutString((uint8_t *)"Invalid input!\r\n");
      }
    }
  }

  // 打开选中的bin文件
  uint8_t file_index = key - '1';
  Serial_PutString((uint8_t *)"\r\nOpening file: ");
  Serial_PutString((uint8_t *)bin_files[file_index]);
  Serial_PutString((uint8_t *)"\r\n");

  err = lfs_file_open(&lfs_instance, &file, bin_files[file_index], LFS_O_RDONLY);
  if (err != LFS_ERR_OK)
  {
    Serial_PutString((uint8_t *)"Failed to open file!\r\n");
    lfs_spi_flash_unmount(NULL);
    return;
  }

  // 获取文件大小
  file_size = lfs_file_size(&lfs_instance, &file);
  Serial_PutString((uint8_t *)"File size: ");
  Int2Str((uint8_t *)buffer, file_size);
  Serial_PutString((uint8_t *)buffer);
  Serial_PutString((uint8_t *)" bytes\r\n");

  // 检查Flash空间是否足够
  if (file_size > USER_FLASH_SIZE)
  {
    Serial_PutString((uint8_t *)"Error: File size exceeds Flash capacity!\r\n");
    lfs_file_close(&lfs_instance, &file);
    lfs_spi_flash_unmount(NULL);
    return;
  }

  // 擦除Flash
  Serial_PutString((uint8_t *)"Erasing Flash...\r\n");
  if (FLASH_If_Erase(APPLICATION_ADDRESS) != FLASHIF_OK)
  {
    Serial_PutString((uint8_t *)"Flash erase failed!\r\n");
    lfs_file_close(&lfs_instance, &file);
    lfs_spi_flash_unmount(NULL);
    return;
  }

  // 读取LFS文件并写入Flash
  Serial_PutString((uint8_t *)"Writing file to Flash...\r\n");
  total_read = 0;
  while (total_read < file_size)
  {
    uint32_t bytes_to_read = (file_size - total_read) > sizeof(buffer) ? sizeof(buffer) : (file_size - total_read);
    err = lfs_file_read(&lfs_instance, &file, buffer, bytes_to_read);
    if (err < 0)
    {
      Serial_PutString((uint8_t *)"File read error!\r\n");
      lfs_file_close(&lfs_instance, &file);
      lfs_spi_flash_unmount(NULL);
      return;
    }
    else if (err == 0)
    {
      break; // 文件读取结束
    }

    // 写入Flash - 计算正确的32位字数，向上取整
    uint32_t word_count = (err + 3) / 4; // 确保非4字节倍数也能正确处理
    if (FLASH_If_Write(flash_address, (uint32_t *)buffer, word_count) != FLASHIF_OK)
    {
      Serial_PutString((uint8_t *)"Flash write failed!\r\n");
      lfs_file_close(&lfs_instance, &file);
      lfs_spi_flash_unmount(NULL);
      return;
    }

    flash_address += err;
    total_read += err;

    // 显示进度
    Serial_PutString((uint8_t *)"Progress: ");
    Int2Str((uint8_t *)buffer, (total_read * 100) / file_size);
    Serial_PutString((uint8_t *)buffer);
    Serial_PutString((uint8_t *)"%\r");
  }

  Serial_PutString((uint8_t *)"\r\nFile written successfully to Flash!\r\n");

  // 关闭文件并卸载文件系统
  lfs_file_close(&lfs_instance, &file);
  lfs_spi_flash_unmount(NULL);

  // 检查应用程序是否有效
  Serial_PutString((uint8_t *)"Checking application validity...\r\n");
  if (header->ih_magic == UBOOT_MAGIC)
  {
    Serial_PutString((uint8_t *)"Valid U-Boot header found!\r\n");
    Serial_PutString((uint8_t *)"Application name: ");
    Serial_PutString((uint8_t *)header->ih_name);
    Serial_PutString((uint8_t *)"\r\n");
  }
  else
  {
    // 检查栈指针
    uint32_t sp = *(uint32_t *)APPLICATION_ADDRESS;
    if ((sp >= 0x20000000U) && (sp <= 0x2002FFFFU))
    {
      Serial_PutString((uint8_t *)"No U-Boot header, but valid stack pointer found!\r\n");
    }
    else
    {
      Serial_PutString((uint8_t *)"Warning: Application may not be valid!\r\n");
    }
  }

  Serial_PutString((uint8_t *)"LittleFS to Flash update completed!\r\n");
}

/**
 * @}
 */
