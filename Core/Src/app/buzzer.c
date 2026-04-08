#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../Inc/main.h"
#include "../../Inc/tim.h"

#include "../../Inc/app/buzzer.h"

static uint16_t Clamp(uint16_t v, uint16_t lb, uint16_t ub) {
	return v < lb ? lb : v > ub ? ub : v;
}

void Buzzer_Init(BuzzerController* bc) {
	bc->frequencyHz = 0;
	bc->volumePct = 100;
	bc->enabled = false;
}

void Buzzer_Update(BuzzerController* bc) {
	(void)bc;
}

int16_t Buzzer_SetVolume(BuzzerController* bc, uint8_t volumePct) {
	if (bc == NULL) return -1;
	if (volumePct > VOLUME_MAX) return -1;
	bc->volumePct = volumePct;

	uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
	uint32_t ccr = (arr + 1) * (100 - volumePct) / 100;
	if (ccr > arr) ccr = arr;
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr);
	return 0;
}

int16_t Buzzer_SetTone(BuzzerController* bc, uint16_t frequencyHz) {
	if (frequencyHz < 1) return -1;
	bc->frequencyHz = frequencyHz;
	// 1 MHz clk speed / freqHz
	uint32_t arr = Clamp(1000000 / bc->frequencyHz - 1, 16, 65535);
	// duty = .5, crr = duty * (arr + 1)
	uint32_t ccr = (arr + 1) / 2;

	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr);
	__HAL_TIM_SET_AUTORELOAD(&htim3, arr);
	return 0;
}

void Buzzer_On(BuzzerController* bc) {
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void Buzzer_Off(BuzzerController* bc) {
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

int16_t Buzzer_Start(BuzzerController* bc, uint16_t frequencyHz) {
	if (frequencyHz == 0) {
		Buzzer_Stop(bc);
		return 0;
	}
	bc->enabled = true;
	if (Buzzer_SetTone(bc, frequencyHz) == -1) return -1;
	Buzzer_On(bc);
	return 0;
}

void Buzzer_Stop(BuzzerController* bc) {
	bc->enabled = false;
	bc->frequencyHz = 0;
	Buzzer_Off(bc);
}
