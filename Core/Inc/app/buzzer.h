#ifndef APP_BUZZER_H
#define APP_BUZZER_H

#include <stdint.h>
#include <stdbool.h>

#define GATE_FREQUENCY_HZ 500
#define GATE_PERIOD_MS 1 / GATE_FREQUENCY_HZ

typedef struct BuzzerController {
	volatile uint16_t frequencyHz;
	volatile uint8_t volumePct;
	volatile bool enabled;
	volatile uint16_t accumulator; // For volume gating
} BuzzerController;

void Buzzer_Init(BuzzerController* bc);

void Buzzer_Update(BuzzerController* bc);

int16_t Buzzer_SetVolume(BuzzerController* bc, uint8_t volumePct);
int16_t Buzzer_SetTone(BuzzerController* bc, uint16_t frequencyHz);

void Buzzer_On(BuzzerController* bc);
void Buzzer_Off(BuzzerController* bc);

int16_t Buzzer_Start(BuzzerController* bc, uint16_t frequencyHz);
void Buzzer_Stop(BuzzerController* bc);

#endif /* APP_BUZZER_H */
