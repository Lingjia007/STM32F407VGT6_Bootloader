/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "dma.h"
#include "fatfs.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sdio.h"
#include "dwt_delay.h"
#include <string.h>
#include <stdio.h>
#include "menu.h"
#include "flash_if.h"
#include "common.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef void (*pFunction)(void);
// U-Boot头部定义
#define UBOOT_MAGIC 0x27051956
#define UBOOT_HEADER_SIZE 256
// U-Boot头部结构体
typedef struct image_header
{
  uint32_t ih_magic;   /* Image Header Magic Number  */
  uint32_t ih_hcrc;    /* Image Header CRC Checksum  */
  uint32_t ih_time;    /* Image Creation Timestamp   */
  uint32_t ih_size;    /* Image Data Size            */
  uint32_t ih_load;    /* Data Load Address          */
  uint32_t ih_ep;      /* Entry Point Address        */
  uint32_t ih_dcrc;    /* Image Data CRC Checksum    */
  uint8_t ih_os;       /* Operating System           */
  uint8_t ih_arch;     /* CPU architecture           */
  uint8_t ih_type;     /* Image Type                 */
  uint8_t ih_comp;     /* Compression Type           */
  uint8_t ih_name[32]; /* Image Name                 */
} image_header_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t led_timer_counter = 0; // 定时器计数器 (ms)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint8_t uart_wait_command(uint8_t *cmd, uint32_t timeout);
static uint8_t app_is_valid(void);
static void jump_to_app(void);
static void led_control_task(void);
static void power_on_check(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
  uint8_t data = (uint8_t)ch;
  HAL_UART_Transmit(&huart4, &data, 1, 1000);
  return ch;
}

static uint8_t uart_wait_command(uint8_t *cmd, uint32_t timeout)
{
  return (HAL_UART_Receive(&huart4, cmd, 1, timeout) == HAL_OK);
}

static uint8_t app_is_valid(void)
{
  image_header_t *header = (image_header_t *)APP_ADDRESS;

  // 检查是否有U-Boot头部 (检查魔数)
  if (header->ih_magic == UBOOT_MAGIC)
  {
    printf("Valid U-Boot header found:\r\n");
    printf("  Image name: %.32s\r\n", header->ih_name);
    printf("  Image size: %lu bytes\r\n", header->ih_size);
    printf("  Load address: 0x%08lX\r\n", header->ih_load);
    printf("  Entry point: 0x%08lX\r\n", header->ih_ep);
    printf("  Data CRC: 0x%08lX\r\n", header->ih_dcrc);
    printf("  Header CRC: 0x%08lX\r\n", header->ih_hcrc);

    // 检查加载地址是否有效
    if ((header->ih_load >= 0x08000000U && header->ih_load < 0x08100000U) || // Flash区域
        (header->ih_load >= 0x20000000U && header->ih_load < 0x20030000U))
    { // RAM区域
      return 1;
    }
    else
    {
      printf("Invalid load address\r\n");
    }
  }
  else
  {
    // 兼容没有头部的情况，直接检查栈指针
    uint32_t sp = *(uint32_t *)APP_ADDRESS;
    if ((sp >= 0x20000000U) && (sp <= 0x2002FFFFU))
    {
      printf("No U-Boot header, valid app stack pointer found\r\n");
      return 1;
    }
    else
    {
      printf("No valid application found\r\n");
    }
  }
  return 0;
}

static void jump_to_app(void)
{
  image_header_t *header = (image_header_t *)APP_ADDRESS;
  uint32_t app_addr;
  pFunction jump_fn;

  // 检查是否有U-Boot头部
  if (header->ih_magic == UBOOT_MAGIC)
  {
    // 有头部的情况
    app_addr = APP_ADDRESS + UBOOT_HEADER_SIZE; // 跳过256字节头部，确保中断向量表256字节对齐
    printf("U-Boot header detected\r\n");

    // 如果加载地址在RAM中，需要复制数据到RAM
    if (header->ih_load >= 0x20000000U && header->ih_load <= 0x2002FFFFU)
    {
      printf("Loading application to RAM...\r\n");
      printf("Jumping to application in RAM at 0x%08lX\r\n", (header->ih_load + 4));
      // 复制应用程序数据到指定的RAM地址
      memcpy((void *)header->ih_load, (void *)app_addr, header->ih_size);

      // 设置向量表偏移地址到RAM地址
      SCB->VTOR = header->ih_load;
      __DSB(); // 数据同步屏障
      __ISB(); // 指令同步屏障

      // 设置MSP为RAM中应用程序的栈指针
      __set_MSP(*(uint32_t *)header->ih_load);

      // 设置跳转函数为入口地址
      jump_fn = (pFunction)(*(uint32_t *)(header->ih_load + 4));
    }
    else
    {
      // 从Flash直接运行
      printf("Executing application directly from Flash\r\n");
      printf("Jumping to application in FLASH at 0x%08lX\r\n", (header->ih_load + 4));
      SCB->VTOR = app_addr; // 设置向量表偏移
      __DSB();              // 数据同步屏障
      __ISB();              // 指令同步屏障
      __set_MSP(*(uint32_t *)app_addr);
      jump_fn = (pFunction)(*(uint32_t *)(app_addr + 4));
    }
  }
  else
  {
    // 没有头部的情况，保持原有逻辑
    printf("No U-Boot header, using default boot procedure\r\n");
    SCB->VTOR = APP_ADDRESS; // 设置向量表偏移
    __DSB();                 // 数据同步屏障
    __ISB();                 // 指令同步屏障
    __set_MSP(*(uint32_t *)APP_ADDRESS);
    jump_fn = (pFunction)(*(uint32_t *)(APP_ADDRESS + 4));
  }

  // 禁用中断和外设时钟
  __disable_irq();
  __HAL_RCC_PWR_CLK_DISABLE();
  HAL_RCC_DeInit();

  // 跳转到应用程序
  jump_fn();
}

void show_sdcard_info(void)
{
  HAL_SD_CardInfoTypeDef sdinfo;
  FATFS *fs;
  DWORD fre_clust, fre_sect, tot_sect;
  FRESULT res;

  // 获取SD卡底层信息
  if (HAL_SD_GetCardInfo(&hsd, &sdinfo) == HAL_OK)
  {
    printf("SD Card Info:\r\n");
    printf("  Capacity: %lu MB\r\n", (sdinfo.BlockNbr * sdinfo.BlockSize) / (1024 * 1024));
    printf("  Block Size: %lu bytes\r\n", sdinfo.BlockSize);
    printf("  Block Count: %lu\r\n", sdinfo.BlockNbr);
  }
  else
  {
    printf("Failed to get SD card info\r\n");
    return;
  }

  // 获取FatFs文件系统信息
  fs = &SDFatFS;
  res = f_getfree(SDPath, &fre_clust, &fs);
  if (res == FR_OK)
  {
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;
    printf("FATFS Info:\r\n");
    printf("  FAT type: FAT%u\r\n", (fs->fs_type == 3) ? 32 : 16); // 3=FAT32, 2=FAT16
    printf("  Total Size: %lu MB\r\n", tot_sect / 2048);
    printf("  Free Space: %lu MB\r\n", fre_sect / 2048);
  }
  else
  {
    printf("Failed to get FATFS info: %d\r\n", res);
  }
}

static void power_on_check(void)
{
  uint8_t cmd = 0;

  printf("Checking TF card...\r\n");

  // 先挂载文件系统
  FRESULT fres = f_mount(&SDFatFS, SDPath, 1);
  if (fres != FR_OK)
  {
    printf("f_mount failed: %d\r\n", fres);
  }

  HAL_SD_CardStateTypeDef sd_state;
  uint32_t timeout = 100; // 100ms超时
  uint32_t start_tick = HAL_GetTick();

  do
  {
    sd_state = HAL_SD_GetCardState(&hsd);
    if (HAL_GetTick() - start_tick > timeout)
    {
      printf("TF card detection timeout\r\n");
      sd_state = HAL_SD_CARD_ERROR; // 设置为错误状态
      break;
    }
    HAL_Delay(1);
  } while (sd_state == HAL_SD_CARD_PROGRAMMING || sd_state == HAL_SD_CARD_RECEIVING);

  if (sd_state == HAL_SD_CARD_TRANSFER)
  {
    printf("TF card detected and ready\r\n");
    show_sdcard_info();
    // 这里可以添加从TF卡读取应用程序的逻辑
  }
  else
  {
    printf("No TF card detected or card error (state: %d)\r\n", sd_state);
  }

  // 检查串口命令
  if (uart_wait_command(&cmd, UART_TIMEOUT) && cmd == 'M')
  {
    printf("Enter Bootloader Menu...\r\n");
    Main_Menu();
  }
  else
  {
    if (app_is_valid())
    {
      printf("Jumping to application...\r\n");
      jump_to_app();
    }
    else
    {
      printf("No valid app, entering Bootloader Menu...\r\n");
      Main_Menu();
    }
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_UART4_Init();
  MX_CRC_Init();
  MX_TIM1_Init();
  MX_RTC_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  MX_SDIO_SD_Init_Fix();
  dwt_delay_init();
  printf("Bootloader started...\r\n");
  FLASH_If_Init();
  Common_Init();

  // 启动定时器1
  HAL_TIM_Base_Start_IT(&htim1);

  power_on_check();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // Bootloader_Task();
    // HAL_Delay(10);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * @brief  LED控制任务，实现2秒周期闪烁
 *         前1秒：亮250ms → 灭250ms → 亮250ms → 灭250ms
 *         后1秒：灭1秒
 * @param  None
 * @retval None
 */
static void led_control_task(void)
{
  // 2秒周期 = 2000ms
  uint32_t cycle_time = led_timer_counter % 2000;

  if (cycle_time < 1000) // 前1秒
  {
    uint32_t sub_cycle = cycle_time % 500; // 每500ms一个子周期
    if (sub_cycle < 250)                   // 前250ms亮
    {
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    }
    else // 后250ms灭
    {
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    }
  }
  else // 后1秒全灭
  {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
  }
}

/**
 * @brief  定时器1更新中断回调函数
 * @param  htim: 定时器句柄
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    led_timer_counter++; // 每1ms递增
    led_control_task();  // 在中断中调用LED控制
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
