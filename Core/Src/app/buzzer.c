#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../Inc/main.h"
#include "../../Inc/tim.h"

#include "../../Inc/app/buzzer.h"

void BuzzerController_Init(BuzzerController* bc) {
	bc->duty = .5;
	bc->isPlaying = false;
	bc->needNextFrame = false;

	bc->currFrame = (Frame){0, 0};
	bc->nextFrame = (Frame){0, 0};
}

void Buzzer_Update(BuzzerController* bc) {
	if (!bc->isPlaying) return;

	uint16_t frequencyHz = Get_Frequency(bc);
	uint16_t durationMs = Get_Duration(bc);

	if (durationMs == 0 && !bc->needNextFrame) {
		CurrFrame_Update(bc);
		bc->needNextFrame = true;
		frequencyHz = Get_Frequency(bc);
		durationMs = Get_Duration(bc);
	
		Buzzer_Off(bc);
		Set_Frequency(bc, frequencyHz);
		if (frequencyHz == 0) return;
		Buzzer_On(bc);
	}
	else if (durationMs == 0 && bc->needNextFrame) {
		Buzzer_Off(bc);
		bc->isPlaying = false;
	}
}

static uint16_t Clamp(uint16_t v, uint16_t lb, uint16_t ub) {
	return v < lb ? lb : v > ub ? ub : v;
}

uint16_t Get_Frequency(BuzzerController* bc) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	uint16_t freq = bc->currFrame.frequencyHz;
	__set_PRIMASK(primask);
	return freq;
}
uint16_t Get_Duration(BuzzerController* bc) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	uint16_t duration = bc->currFrame.durationMs;
	__set_PRIMASK(primask);
	return duration;
}

int16_t Duty_Update(BuzzerController* bc, uint16_t volume) {
	(void)bc;
	(void)volume;
	return 0;
}

int16_t Set_Frequency(BuzzerController* bc, uint16_t freqHz) {
	if (freqHz < 1) return -1;

	// 1 MHz clk speed / freqHz
	uint16_t arr = Clamp(1000000 / freqHz - 1, 16, 65535);
	// duty = .5, crr = duty * (arr + 1)
	uint16_t ccr = (arr + 1) * bc->duty;

	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr);
	__HAL_TIM_SET_AUTORELOAD(&htim3, arr);
	return 0;
}

int16_t CurrFrame_Update(BuzzerController* bc) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	bc->currFrame = bc->nextFrame;
	__set_PRIMASK(primask);
	return 0;
}

void Buzzer_On(BuzzerController* bc) {
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void Buzzer_Off(BuzzerController* bc) {
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

