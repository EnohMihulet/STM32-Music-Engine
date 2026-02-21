#ifndef APP_BUTTON_H
#define APP_BUTTON_H
#pragma once

#include <stdint.h>

#include "music_engine.h"

static const uint16_t DEBOUNCE_TIME = 50;

static const uint16_t DOUBLE_CLICK_TIME = 200;
static const uint16_t SHORT_CLICK_TIME = 400;
static const uint16_t HOLD_TIME = 700;

typedef enum { ButtonStateIdle=0, ButtonStateDown1, ButtonStateWait, ButtonStateDown2} ButtonState;

typedef struct Button {
	volatile uint8_t isDown;
	volatile uint8_t pressedCount;
	volatile uint8_t releasedCount;

	volatile uint32_t pressedAt;
	volatile uint32_t releasedAt;
	volatile uint32_t changedAt;

	ButtonState buttonState;
} Button;

void Button_Init(Button* b);

void Button_Update(Button* b, MusicEngineController* mec);

#endif /* APP_BUTTON_H */
