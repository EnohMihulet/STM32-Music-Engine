/* USER CODE BEGIN Header */
/**
	******************************************************************************
	* @file			 : main.c
	* @brief			: Main program body
	******************************************************************************
	* @attention
	*
	* Copyright (c) 2026 STMicroelectronics.
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
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../Inc/app/button.h"
#include "../Inc/app/buzzer.h"
#include "../Inc/app/uart_cli.h"
#include "../Inc/app/music_engine.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
Button gButton;
UartCLIController gUartCLIController;
MusicEngineController gMusicEngineController;

static const char pressed_msg[] = "Pressed\r\n";
static const char double_click_msg[] = "Double click\r\n";
static const char hold_msg[] = "Hold\r\n";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

	gUartCLIController.rxBuffer = malloc(UART_RX_BUFFER_SIZE);

	Button_Init(&gButton);
	UartCLIController_Init(&gUartCLIController);
	MusicEngineController_Init(&gMusicEngineController);

	HAL_TIM_Base_Start_IT(&htim2);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, gUartCLIController.rxBuffer, UART_RX_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
	__HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_TC);

	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		Uart_Update(&gUartCLIController, &gMusicEngineController);
		Button_Update(&gButton);
		MusicEngine_Update(&gMusicEngineController);

		ButtonEvent e = Button_GetEvent(&gButton);
		if (e == ButtonEventSingleClick) {
			HAL_UART_Transmit(&huart2, (uint8_t*)pressed_msg, sizeof(pressed_msg)-1, 100);

			HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
		}
		if (e == ButtonEventDoubleClick) {
			HAL_UART_Transmit(&huart2, (uint8_t*)double_click_msg, sizeof(double_click_msg)-1, 100);
		}
		if (e == ButtonEventHold) {
			HAL_UART_Transmit(&huart2, (uint8_t*)hold_msg, sizeof(hold_msg)-1, 100);
		}

		Buzzer_Update(&(gMusicEngineController.buzzer));
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin != GPIO_PIN_13) return;

	uint32_t now = HAL_GetTick();

	if (now - gButton.changedAt < DEBOUNCE_TIME) return;

	uint8_t state = (HAL_GPIO_ReadPin(GPIOC, GPIO_Pin) == GPIO_PIN_RESET);
	gButton.changedAt = now;

	if (state && !gButton.isDown) {
		gButton.isDown = 1;
		if (gButton.pressedCount < 255) gButton.pressedCount++;
		gButton.pressedAt = now;
	} 
	else if (!state && gButton.isDown) {
		gButton.isDown = 0;
		if (gButton.releasedCount < 255) gButton.releasedCount++;
		gButton.releasedAt = now;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
	if (htim->Instance != TIM2) return;
	if (gMusicEngineController.pbState != Playing) return;

	if (gMusicEngineController.remainingTimeMs != 0) {
		gMusicEngineController.remainingTimeMs -= 1;

		if (gMusicEngineController.remainingTimeMs == 0) gMusicEngineController.updateFrame = true;
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart2, uint16_t size) {
	if (huart2->Instance != USART2) return;
	if (HAL_UARTEx_GetRxEventType(huart2) != HAL_UART_RXEVENT_IDLE) return;

	uint16_t currPos = size;
	uint16_t lastPos = gUartCLIController.lastPos;

	if (lastPos == currPos) return;

	if (lastPos < currPos) {
		memcpy((uint8_t*)gUartCLIController.ingestBuffer + lastPos, gUartCLIController.rxBuffer + lastPos, currPos - lastPos);
	}
	else {
		memcpy((uint8_t*)gUartCLIController.ingestBuffer + lastPos, gUartCLIController.rxBuffer + lastPos, UART_RX_BUFFER_SIZE - lastPos);
		memcpy((uint8_t*)gUartCLIController.ingestBuffer, gUartCLIController.rxBuffer, currPos);
	}

	gUartCLIController.currPos = currPos;
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
