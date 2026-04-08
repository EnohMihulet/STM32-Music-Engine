#ifndef APP_BUZZER_H
#define APP_BUZZER_H

#include <stdint.h>
#include <stdbool.h>

#define FREQUENCY_MAX_HZ 65535
#define FREQUENCY_MIN_HZ 16
#define DURATION_MIN_MS 1
#define DURATION_MAX_MS 65535
#define VOLUME_MAX 100
#define VOLUME_MIN 0

typedef struct BuzzerController {
	uint16_t frequencyHz;
	uint8_t volumePct;
	bool enabled;
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
