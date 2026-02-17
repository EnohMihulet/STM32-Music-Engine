#ifndef APP_UART_CLI_H
#define APP_UART_CLI_H

#include <stdint.h>

#include "music_engine.h"

#define UART_RX_BUFFER_SIZE 256
#define INGEST_BUFFER_SIZE 256

typedef struct UartRxBufferController {
	volatile uint16_t lastPos;
	volatile uint16_t currPos;
	volatile uint8_t pending;

	volatile uint8_t ingestBuffer[INGEST_BUFFER_SIZE];
	volatile uint8_t* rxBuffer;

	char command[32];
	uint16_t commandIndex;

	int16_t number;
	CommandPlaybackControl playbackControl;
	CommandPlayRequest playRequest;
	CommandSettings commandSettings;
} UartRxBufferController;

void UartBufferController_Init(UartRxBufferController* urbc);

void Uart_Update(UartRxBufferController* urbc);

void Handle_Command(UartRxBufferController* urbc, char* command);

#endif /* APP_UART_CLI_H */
