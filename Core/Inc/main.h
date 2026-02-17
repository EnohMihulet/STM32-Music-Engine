/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define FRAMES_MAX_CAPACITY 80
#define UART_RX_BUFFER_SIZE 256
#define INGEST_BUFFER_SIZE 256
#define SONG_COUNT 3
#define SONG_QUEUE_CAPACITY 16

static const uint16_t DEBOUNCE_TIME = 50;

static const uint16_t DOUBLE_CLICK_TIME = 200;
static const uint16_t SHORT_CLICK_TIME = 400;
static const uint16_t HOLD_TIME = 700;



static const char pressed_msg[] = "Button pressed.\r\n";
static const char double_click_msg[] = "Button double clicked.\r\n";
static const char hold_msg[] = "Button held.\r\n";

typedef enum { ButtonStateIdle=0, ButtonStateDown1, ButtonStateWait, ButtonStateDown2, ButtonStateSingle=4, ButtonStateDouble=5, ButtonStateHold=6} ButtonState;
typedef enum { ButtonEventNone=3, ButtonEventSingleClick=4, ButtonEventDoubleClick=5, ButtonEventHold=6 } ButtonEvent;

typedef enum { BuzzerPriorityPreempt=0, BuzzerPriorityQueue} BuzzerPriorityType;

typedef enum { CommandPlaybackControlNone=0, CommandPlaybackControlPause, CommandPlaybackControlResume, CommandPlaybackControlStop, CommandPlaybackControlSkip, CommandPlaybackControlClear } CommandPlaybackControl;
typedef enum { CommandPlayRequestNone=0, CommandPlayRequestPlay, CommandPlayRequestQueue, CommandPlayRequestPlaySeq } CommandPlayRequest;
typedef enum { CommandSettingsNone=0, CommandSettingsTempo, CommandSettingsVol, CommandSettingsStatus } CommandSettings;

typedef struct Button {
	volatile uint8_t isDown;
	volatile uint8_t pressedCount;
	volatile uint8_t releasedCount;

	volatile uint32_t pressedAt;
	volatile uint32_t releasedAt;
	volatile uint32_t changedAt;

	ButtonState buttonState;
} Button;

typedef struct Frame {
	uint16_t frequencyHz;
	uint16_t durationMs;
} Frame;

typedef struct Song {
	char title[16];
	uint16_t framesSize;
	Frame frames[FRAMES_MAX_CAPACITY];
} Song;

typedef struct BuzzerController {
	uint8_t size;
	uint8_t index;
	uint8_t songIndex;
	uint8_t isPlaying;
	uint8_t isPaused;

	volatile uint16_t frameRemainingMs;
} BuzzerController;

typedef struct UartRxBufferController {
	volatile uint16_t lastPos;
	volatile uint16_t currPos;
	volatile uint8_t pending;

	volatile uint8_t ingestBuffer[INGEST_BUFFER_SIZE];
	volatile uint8_t* rxBuffer;

	char command[32];
	uint16_t commandIndex;
} UartRxBufferController;

typedef struct MusicEngineController {
	CommandPlaybackControl playbackControl;
	CommandPlayRequest playRequest;
	CommandSettings commandSettings;

	int16_t number;
	uint16_t volume;
	uint16_t tempo;
	uint16_t queueSize;
	uint16_t queue[16];
} MusicEngineController;


/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
