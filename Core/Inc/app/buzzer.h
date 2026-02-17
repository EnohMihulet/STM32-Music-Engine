#ifndef APP_BUZZER_H
#define APP_BUZZER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Frame {
	uint16_t frequencyHz;
	uint16_t durationMs;
} Frame;

typedef struct BuzzerController {
	float duty;
	volatile Frame currFrame;
	Frame nextFrame;
	bool isPlaying;
	bool needNextFrame;
} BuzzerController;

void BuzzerController_Init(BuzzerController* bc);

void Buzzer_Update(BuzzerController* bc);

uint16_t Get_Frequency(BuzzerController* bc);
uint16_t Get_Duration(BuzzerController* bc);

int16_t Duty_Update(BuzzerController* bc, uint16_t volume);
int16_t CurrFrame_Update(BuzzerController* bc);
int16_t Set_Frequency(BuzzerController* bc, uint16_t freqHz);

void Buzzer_On(BuzzerController* bc);
void Buzzer_Off(BuzzerController* bc);

#endif /* APP_BUZZER_H */
