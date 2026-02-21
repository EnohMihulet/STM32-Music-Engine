#ifndef APP_UART_CLI_H
#define APP_UART_CLI_H

#include <stdint.h>

#include "music_engine.h"

#define UART_RX_BUFFER_SIZE 256

typedef struct UartCLIController {
	volatile uint16_t lastPos;
	volatile uint16_t currPos;
	uint8_t* rxBuffer;

	char command[32];
	uint16_t commandIndex;
} UartCLIController;

void UartCLIController_Init(UartCLIController* ucc);

void Uart_Update(UartCLIController* ucc, MusicEngineController* mec);

void Append_To_CommandBuffer(UartCLIController* ucc, char c);

int16_t Parse_CommandString(UartCLIController* ucc, Command* out);

uint16_t echo(char* s, uint16_t len);

#endif
