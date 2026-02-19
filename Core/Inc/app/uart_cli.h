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
	CommandCode lastCC;
} UartRxBufferController;

void UartBufferController_Init(UartRxBufferController* urbc);

void Uart_Update(UartRxBufferController* urbc, MusicEngineController* mec);

void Update_MusicEngine_Command(UartRxBufferController* urbc, MusicEngineController* mec, char* command);

void Update_LastCommandCode(UartRxBufferController* urbc, CommandCode cc);

uint16_t echo(char* s, uint16_t len);

#endif
