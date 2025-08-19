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
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
  uint32_t sp = *(uint32_t *)APP_ADDRESS;
  // 栈顶地址必须指向 SRAM F4 = 192KB
  if ((sp >= 0x20000000U) && (sp <= 0x2002FFFFU))
  {
    return 1;
  }
  return 0;
}

static void jump_to_app(void)
{
  uint32_t jump_addr = *(uint32_t *)(APP_ADDRESS + 4);
  pFunction jump_fn = (pFunction)jump_addr;

  __disable_irq(); // 禁用中断
  __HAL_RCC_PWR_CLK_DISABLE();
  HAL_RCC_DeInit(); // 复位RCC

  SCB->VTOR = APP_ADDRESS; // 设置向量表偏移
  __DSB();                 // 数据同步屏障
  __ISB();                 // 指令同步屏障

  __set_MSP(*(uint32_t *)APP_ADDRESS); // 设置主堆栈指针
  jump_fn();                           // 跳转到应用程序
}

void show_sdcard_info(void)
{
  uint64_t CardCap; // SD卡容量
  HAL_SD_CardCIDTypeDef SDCard_CID;
  HAL_SD_CardInfoTypeDef SDCardInfo;
  HAL_StatusTypeDef status;

  // 获取SD卡状态
  HAL_SD_CardStateTypeDef ret = HAL_SD_GetCardState(&hsd);
  printf("SD Card State:%d\r\n", ret);

  // 检查SD卡是否处于可传输状态
  if (ret != HAL_SD_CARD_TRANSFER)
  {
    printf("SD card not ready for transfer\r\n");
    return;
  }

  // 获取SD卡CID信息（卡识别信息）
  status = HAL_SD_GetCardCID(&hsd, &SDCard_CID);
  if (status != HAL_OK)
  {
    printf("Failed to get SD card CID, error: %d\r\n", status);
    return;
  }

  // 获取SD卡基本信息
  status = HAL_SD_GetCardInfo(&hsd, &SDCardInfo);
  if (status != HAL_OK)
  {
    printf("Failed to get SD card info, error: %d\r\n", status);
    return;
  }

  // 根据卡类型显示相关信息
  switch (SDCardInfo.CardType)
  {
  case CARD_SDSC:
  {
    if (SDCardInfo.CardVersion == CARD_V1_X)
      printf("Card Type:SDSC V1\r\n");
    else if (SDCardInfo.CardVersion == CARD_V2_X)
      printf("Card Type:SDSC V2\r\n");
  }
  break;
  case CARD_SDHC_SDXC:
    printf("Card Type:SDHC\r\n");
    break;
  }

  // 计算SD卡总容量
  CardCap = (uint64_t)(SDCardInfo.LogBlockNbr) * (uint64_t)(SDCardInfo.LogBlockSize);

  // 显示详细信息
  printf("Card ManufacturerID:%d\r\n", SDCard_CID.ManufacturerID);     // 制造商ID
  printf("Card RCA:%d\r\n", SDCardInfo.RelCardAdd);                    // 相对卡地址
  printf("LogBlockNbr:%d \r\n", (uint32_t)(SDCardInfo.LogBlockNbr));   // 逻辑块数量
  printf("LogBlockSize:%d \r\n", (uint32_t)(SDCardInfo.LogBlockSize)); // 逻辑块大小
  printf("Card Capacity:%d MB\r\n", (uint32_t)(CardCap >> 20));        // 容量（MB）
  printf("Card BlockSize:%d\r\n\r\n", SDCardInfo.BlockSize);           // 物理块大小
}

static void power_on_check(void)
{
  uint8_t cmd = 0;

  // 安全检查TF卡是否存在并读取
  printf("Checking TF card...\r\n");

  // 使用超时机制检查SD卡状态
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
    show_sdcard_info(); // 显示SD卡信息

    // 这里可以添加从TF卡读取应用程序的逻辑
    // 例如：read_app_from_sdcard();
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
  MX_UART4_Init();
  MX_CRC_Init();
  MX_TIM1_Init();
  MX_SDIO_SD_Init();
  /* USER CODE BEGIN 2 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
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
