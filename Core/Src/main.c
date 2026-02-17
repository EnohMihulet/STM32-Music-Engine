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
#include "../Inc/main.h"
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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
BuzzerController gBuzzerController;
Song gSongs[SONG_COUNT];
UartRxBufferController gUartBufferController;
MusicEngineController gMusicEngineController;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

static void Button_Init(void);
static void BuzzerController_Init(void);
static void Songs_Init(void);
static void UartBufferController_Init(void);
static void MusicEngineController_Init(void);

static void Button_Update(void);
static void Buzzer_Update(void);
static void Uart_Update(void);
static void MusicEngine_Update(void);

static void Handle_Command(char* command);
static ButtonEvent Button_GetEvent(void);

static void Buzzer_On(void);
static void Buzzer_Off(void);

static void Tone_SetFrequency(uint16_t freqHz);
static uint16_t SongQueue_Pop(void);

static void Play_Song(uint16_t songIndex);
static void Queue_Song(uint16_t songIndex);

static void Display_MusicEngine_Status(void);
static void Display_Songs(void);

static void Display_Volume(void);
static void Display_Tempo(void);
static void Set_Volume(uint16_t);
static void Set_Tempo(uint16_t);

static void Song_Pause(void);
static void Song_Resume(void);
static void Song_Stop(void);
static void Song_Skip(void);
static void Queue_Clear(void);

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
  /* USER CODE BEGIN 2 */

	gUartBufferController.rxBuffer = malloc(UART_RX_BUFFER_SIZE);

	Button_Init();
	Songs_Init();
	BuzzerController_Init();
	UartBufferController_Init();
	MusicEngineController_Init();

	HAL_TIM_Base_Start_IT(&htim2);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, gUartBufferController.rxBuffer, UART_RX_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
	__HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_TC);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		Uart_Update();
		Button_Update();
		MusicEngine_Update();

		ButtonEvent e = Button_GetEvent();
		if (e == ButtonEventSingleClick) {
			MusicEngine_Update();
			HAL_UART_Transmit(&huart2, (uint8_t*)pressed_msg, sizeof(pressed_msg)-1, 100);
		}
		if (e == ButtonEventDoubleClick) {
			HAL_UART_Transmit(&huart2, (uint8_t*)double_click_msg, sizeof(double_click_msg)-1, 100);
		}
		if (e == ButtonEventHold) {
			HAL_UART_Transmit(&huart2, (uint8_t*)hold_msg, sizeof(hold_msg)-1, 100);
		}

		Buzzer_Update();
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

	if (!gBuzzerController.isPaused && gBuzzerController.frameRemainingMs > 0) gBuzzerController.frameRemainingMs -= 1;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart2, uint16_t size) {
	if (huart2->Instance != USART2) return;
	if (HAL_UARTEx_GetRxEventType(huart2) != HAL_UART_RXEVENT_IDLE) return;

	uint16_t currPos = size;
	uint16_t lastPos = gUartBufferController.lastPos;

	if (lastPos == currPos) return;

	if (lastPos < currPos) { // Circular buffer has not wrapped
		memcpy((uint8_t*)gUartBufferController.ingestBuffer + lastPos, gUartBufferController.rxBuffer + lastPos, currPos - lastPos);
	}
	else { // Circular buffer wrapped
		memcpy((uint8_t*)gUartBufferController.ingestBuffer + lastPos, gUartBufferController.rxBuffer + lastPos, UART_RX_BUFFER_SIZE - lastPos);
		memcpy((uint8_t*)gUartBufferController.ingestBuffer, gUartBufferController.rxBuffer, currPos);
	}

	gUartBufferController.currPos = currPos;
	gUartBufferController.pending = true;
}

static void Songs_Init(void) {
	gSongs[0] = (Song) {
		.title = "SONG1",
		.framesSize = 1,
		.frames = {{988, 15000}}
	};

	gSongs[1] = (Song) {
		.title = "SONG2",
		.framesSize = 1,
		.frames = {{659, 15000}}
	};

	gSongs[2] = (Song) {
		.title = "SONG3",
		.framesSize = 1,
		.frames = {{392, 15000}}
	};
}

static void UartBufferController_Init() {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	gUartBufferController.lastPos = 0;
	gUartBufferController.currPos = 0;
	gUartBufferController.pending = false;
	gUartBufferController.commandIndex = 0;
	memset(gUartBufferController.command, 0, 16);
	__set_PRIMASK(primask);
}

static void MusicEngineController_Init(void) {
	gMusicEngineController.playbackControl = CommandPlaybackControlNone;
	gMusicEngineController.playRequest = CommandPlayRequestNone;
	gMusicEngineController.commandSettings = CommandSettingsNone;

	gMusicEngineController.number = 0;
}

static void Button_Init(void) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	gButton.isDown = 0;
	gButton.pressedCount = 0;
	gButton.releasedCount = 0;
	gButton.pressedAt = 0;
	gButton.releasedAt = 0;
	gButton.changedAt = 0;
	gButton.buttonState = ButtonStateIdle;
	__set_PRIMASK(primask);
}

static void BuzzerController_Init(void) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	gBuzzerController.size = 0;
	gBuzzerController.index = 0;
	gBuzzerController.isPlaying = 0;
	gBuzzerController.frameRemainingMs = 0;
	__set_PRIMASK(primask);
}

static void MusicEngine_Update(void) {

	MusicEngineController* mec = &gMusicEngineController;
	if (mec->playbackControl == CommandPlaybackControlNone && mec->playRequest == CommandPlayRequestNone && mec->commandSettings == CommandSettingsNone) return;

	switch (mec->playbackControl) {
		case CommandPlaybackControlNone: break;
		case CommandPlaybackControlPause:
			Song_Pause();
			break;
		case CommandPlaybackControlResume:
			Song_Resume();
			break;
		case CommandPlaybackControlStop:
			Song_Stop();
			break;
		case CommandPlaybackControlSkip:
			Song_Skip();
			break;
		case CommandPlaybackControlClear:
			Queue_Clear();
			break;
		default: break;
	}
	mec->playbackControl = CommandPlaybackControlNone;

	switch (mec->playRequest) {
		case CommandPlayRequestNone: break;
		case CommandPlayRequestPlay:
			if (mec->number == -1) {
				Display_Songs();
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else if (1 <= mec->number && mec->number <= 3) {
				Play_Song(mec->number - 1);
			}
			break;
		case CommandPlayRequestQueue:
			if (mec->number == -1) {
				Display_Songs();
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else if (1 <= mec->number && mec->number <= 3) {
				Queue_Song(mec->number - 1);
			}
			break;
		case CommandPlayRequestPlaySeq:
		default: break;
	}
	mec->playRequest = CommandPlayRequestNone;

	switch (mec->commandSettings) {
		case CommandSettingsNone: break;
		case CommandSettingsStatus:
			Display_MusicEngine_Status();
			break;
		case CommandSettingsTempo:
			if (mec->number == -1) {
				Display_Tempo();
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else {
				Set_Tempo(mec->number);
			}
			break;
		case CommandSettingsVol:
			if (mec->number == -1) {
				Display_Volume();
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else {
				Set_Volume(mec->number);
			}
			break;
		default: break;
	}
	mec->commandSettings = CommandSettingsNone;
}

static void Uart_Update(void) {
	if (!gUartBufferController.pending) return;
	
	UartRxBufferController* bc = &gUartBufferController;

	uint16_t start = bc->lastPos;
	uint16_t end = bc->currPos;

	uint16_t commandStart = bc->commandIndex;

	bool gotLine = false;
	uint16_t newLastPos = start;

	if (start < end) {
		for (uint16_t i = start; i < end; i++) {
			char nextChar = bc->ingestBuffer[i];

			if (nextChar == '\b' || nextChar == 0x7F) {
				if (bc->commandIndex == 0) {
					continue;
				}
				bc->command[--(bc->commandIndex)] = 0;
				HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 100);

				newLastPos = i + 1;
				continue;
			}

			if (bc->commandIndex < (sizeof(bc->command) - 1)) {
				char upperChar = toupper((uint8_t)nextChar);
				bc->command[bc->commandIndex++] = upperChar;
				HAL_UART_Transmit(&huart2, (uint8_t*)&upperChar, 1, 100);
			}

			newLastPos = i + 1;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}
	else if (start > end) {
		uint16_t len = INGEST_BUFFER_SIZE - start + end;
		uint16_t i = start;
		for (uint16_t k = 0; k < len; k++) {
			if (i == INGEST_BUFFER_SIZE) i = 0;

			char nextChar = bc->ingestBuffer[i++];

			if (nextChar == '\b' || nextChar == 0x7F) {
				if (bc->commandIndex == 0) {
					continue;
				}
				bc->command[--(bc->commandIndex)] = 0;
				HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 100);

				newLastPos = i;
				continue;
			}

			if (bc->commandIndex < (sizeof(bc->command) - 1)) {
				char upperChar = toupper((uint8_t)nextChar);
				bc->command[bc->commandIndex++] = upperChar;
				HAL_UART_Transmit(&huart2, (uint8_t*)&upperChar, 1, 100);
			}

			newLastPos = i;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}
	else {
		bc->pending = false;
		return;
	}

	bc->lastPos = newLastPos % INGEST_BUFFER_SIZE;

	if (!gotLine) {
		bc->pending = false;
		return;
	}

	while (bc->commandIndex > 0 && (bc->command[bc->commandIndex - 1] == '\n' || bc->command[bc->commandIndex - 1] == '\r')) {
		bc->commandIndex--;
	}
	bc->command[bc->commandIndex] = '\0';

	Handle_Command(bc->command);

	HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);

	bc->pending = false;
	memset(bc->command, 0, sizeof(bc->command));
	bc->commandIndex = 0;
}

static void Button_Update(void) {

	uint8_t pressedCount, releasedCount;
	uint32_t pressedAt, releasedAt;

	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	pressedCount = gButton.pressedCount;
	releasedCount = gButton.releasedCount;
	pressedAt = gButton.pressedAt;
	releasedAt = gButton.releasedAt;

	gButton.pressedCount = 0;
	gButton.releasedCount = 0;
	__set_PRIMASK(primask);

	bool pressed = (pressedCount != 0);
	bool released = (releasedCount != 0);

	switch (gButton.buttonState) {
	case ButtonStateIdle:
		if (pressed) {
			gButton.buttonState = ButtonStateDown1;
		} break;
	case ButtonStateDown1:
		if (released) {
			if ((releasedAt - pressedAt) <= SHORT_CLICK_TIME) gButton.buttonState = ButtonStateWait;
			else gButton.buttonState = ButtonStateSingle;
		} 
		else if ((HAL_GetTick() - pressedAt) >= HOLD_TIME) {
			gButton.buttonState = ButtonStateHold;
		} break;
	case ButtonStateWait:
		if (pressed) {
			if ((pressedAt - releasedAt) <= DOUBLE_CLICK_TIME) gButton.buttonState = ButtonStateDown2;
			else gButton.buttonState = ButtonStateSingle;
		}
		else if ((HAL_GetTick() - releasedAt) > DOUBLE_CLICK_TIME) {
			gButton.buttonState = ButtonStateSingle;
		} break;
	case ButtonStateDown2:
		if (released) {
			if ((releasedAt - pressedAt) <= SHORT_CLICK_TIME) gButton.buttonState = ButtonStateDouble;
			else gButton.buttonState = ButtonStateSingle;
		} break;
	default: break;
	}
}

static void Buzzer_Update(void) {
	if ((!gBuzzerController.isPlaying && gMusicEngineController.queueSize == 0) || gBuzzerController.isPaused) return;

	if (!gBuzzerController.isPlaying && gMusicEngineController.queueSize > 0) {
		uint16_t songIndex = SongQueue_Pop();
		Play_Song(songIndex);
	}

	uint32_t frameRemainingMs;
	uint32_t primask = __get_PRIMASK();

	__disable_irq();
	frameRemainingMs = gBuzzerController.frameRemainingMs;
	__set_PRIMASK(primask);

	if (frameRemainingMs != 0) return;

	if (gBuzzerController.index >= gBuzzerController.size - 1) {
		gBuzzerController.isPlaying = false;

		if (gMusicEngineController.queueSize > 0) {
			uint16_t songIndex = SongQueue_Pop();
			Play_Song(songIndex);
		}
		else {
			Buzzer_Off();
		}
		return;
	}

	gBuzzerController.index += 1;

	Frame* frame = &(gSongs[gBuzzerController.songIndex].frames[gBuzzerController.index]);

	primask = __get_PRIMASK();

	__disable_irq();
	gBuzzerController.frameRemainingMs = frame->durationMs;
	__set_PRIMASK(primask);

	uint16_t freqHz = frame->frequencyHz;

	Buzzer_Off();
	if (freqHz == 0) return;
	if (gMusicEngineController.volume == 0) return;
	Tone_SetFrequency(freqHz);
	Buzzer_On();
}

static void Handle_Command(char* command) {
	UartRxBufferController* bc = &gUartBufferController;

	char* endptr;
	int64_t num = strtol(command, &endptr, 10);
	if (*endptr == '\0') {
		gMusicEngineController.number = num;
		return;
	}

	gMusicEngineController.number = -1;
	if (strcmp(bc->command, "PAUSE") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlPause;
	}
	else if (strcmp(bc->command, "RESUME") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlResume;
	}
	else if (strcmp(bc->command, "STOP") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlStop;
	}
	else if (strcmp(bc->command, "SKIP") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlSkip;
	}
	else if (strcmp(bc->command, "CLEAR") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlClear;
	}
	else if (strcmp(bc->command, "PLAY") == 0) {
		gMusicEngineController.playRequest = CommandPlayRequestPlay;
	}
	else if (strcmp(bc->command, "QUEUE") == 0) {
		gMusicEngineController.playRequest = CommandPlayRequestQueue;
	}
	else if (strcmp(bc->command, "TEMPO") == 0) {
		gMusicEngineController.commandSettings = CommandSettingsTempo;
	}
	else if (strcmp(bc->command, "VOLUME") == 0) {
		gMusicEngineController.commandSettings = CommandSettingsVol;
	}
	else if (strcmp(bc->command, "STATUS") == 0) {
		gMusicEngineController.commandSettings = CommandSettingsStatus;
	}
}

static ButtonEvent Button_GetEvent() {
	switch(gButton.buttonState) {
		case ButtonStateIdle: case ButtonStateDown1: case ButtonStateWait: case ButtonStateDown2:
			return ButtonEventNone;
		case ButtonStateSingle:
			gButton.buttonState = ButtonStateIdle;
			return ButtonEventSingleClick;
		case ButtonStateDouble:
			gButton.buttonState = ButtonStateIdle;
			return ButtonEventDoubleClick;
		case ButtonStateHold:
			gButton.buttonState = ButtonStateIdle;
			return ButtonEventHold;
		default:
			return ButtonEventNone;
	}
}

static void Buzzer_On(void) {
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); 
}

static void Buzzer_Off(void) {
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1); 
}

static uint16_t Clamp(uint16_t v, uint16_t lb, uint16_t ub) {
	return v < lb ? lb : v > ub ? ub : v;
}

static void Tone_SetFrequency(uint16_t freqHz) {
	assert(gMusicEngineController.volume != 0);
	uint16_t arr = Clamp(1000000 / freqHz - 1, 16, 65535);

	float x = 2 * (gMusicEngineController.volume / 100);

	uint16_t ccr1 = (arr + 1) / x;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr1);
	__HAL_TIM_SET_AUTORELOAD(&htim3, arr);
}

static uint16_t SongQueue_Pop() {
	assert(gMusicEngineController.queueSize != 0);

	uint16_t songIndex = gMusicEngineController.queue[0];
	for (uint16_t i = 0; i < gMusicEngineController.queueSize - 1; i++) {
		gMusicEngineController.queue[i] = gMusicEngineController.queue[i+1];
	}
	gMusicEngineController.queueSize -= 1;

	return songIndex;
}

static void Display_MusicEngine_Status(void) {
	HAL_UART_Transmit(&huart2, (uint8_t*)"STATUS: ", 7, 100);

	char buffer[32];
	sprintf(buffer, "Queue Size: %d\r\n", gMusicEngineController.queueSize);
	uint16_t len = strlen(buffer);
	HAL_UART_Transmit(&huart2, buffer, len, 100);
	
	Display_Volume();
	Display_Tempo();
}

static void Display_Songs() {
	HAL_UART_Transmit(&huart2, (uint8_t*)"SONGS: ", 7, 100);
	for (uint16_t i = 0; i < SONG_COUNT; i++) {
		char buffer[32];
		char* title = gSongs[i].title;
		uint16_t len = strlen(title);

		strcpy(buffer, title);
		if (i == SONG_COUNT - 1) {
			buffer[len++] = '\r';
			buffer[len++] = '\n';
		}
		else {
			buffer[len++] = ' ';
		}

		HAL_UART_Transmit(&huart2, buffer, len, 100);
	}
}

static void Display_Volume(void) {
	char buffer[32];
	sprintf(buffer, "Volume: %d%\r\n", gMusicEngineController.volume);
	uint16_t len = strlen(buffer);
	HAL_UART_Transmit(&huart2, buffer, len, 100);
}

static void Display_Tempo(void) {
	char buffer[32];
	sprintf(buffer, "Tempo: %d", gMusicEngineController.tempo);
	uint16_t len = strlen(buffer);
	HAL_UART_Transmit(&huart2, buffer, len, 100);
}

static void Set_Volume(uint16_t newVol) {
	gMusicEngineController.volume = newVol;
}

static void Set_Tempo(uint16_t newTempo) {

}

static void Play_Song(uint16_t songIndex) {
	if (songIndex >= SONG_COUNT) return;

	Song* song = &(gSongs[songIndex]);
	Frame* firstFrame = &(song->frames[0]);

	gBuzzerController.size = song->framesSize;
	gBuzzerController.index = 0;
	gBuzzerController.frameRemainingMs = firstFrame->durationMs;
	gBuzzerController.isPlaying = true;

	Buzzer_Off();
	if (firstFrame->frequencyHz == 0) return;
	if (gMusicEngineController.volume == 0) return;
	Tone_SetFrequency(firstFrame->frequencyHz);
	Buzzer_On();
}

static void Queue_Song(uint16_t songIndex) {
	if (songIndex >= SONG_COUNT) return;

	if (gMusicEngineController.queueSize >= 16) return;

	gMusicEngineController.queue[gMusicEngineController.queueSize++] = songIndex;
}

static void Song_Pause(void) {
	if (gBuzzerController.isPaused) return;
	gBuzzerController.isPaused = true;
	Buzzer_Off();
}

static void Song_Resume(void) {
	if (!gBuzzerController.isPaused) return;
	gBuzzerController.isPaused = false;

	Frame* frame = &(gSongs[gBuzzerController.songIndex].frames[gBuzzerController.index]);
	if (frame->frequencyHz != 0) {
		Buzzer_On();
	}
}

static void Song_Stop(void) {
	Buzzer_Off();
	BuzzerController_Init();
}

static void Song_Skip(void) {
	BuzzerController_Init();

	if (gMusicEngineController.queueSize > 0) {
		uint16_t songIndex = SongQueue_Pop();
		Play_Song(songIndex);
	}
	else {
		Buzzer_Off();
	}
}

static void Queue_Clear(void) {
	gMusicEngineController.queueSize = 0;
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
