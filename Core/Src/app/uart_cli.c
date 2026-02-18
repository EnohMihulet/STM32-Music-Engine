#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../../Inc/usart.h"
#include "../../Inc/app/uart_cli.h"

void UartBufferController_Init(UartRxBufferController* urbc) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	urbc->lastPos = 0;
	urbc->currPos = 0;
	urbc->pending = false;
	urbc->commandIndex = 0;
	memset(urbc->command, 0, sizeof(urbc->command));
	__set_PRIMASK(primask);
}

void Uart_Update(UartRxBufferController* urbc, MusicEngineController* mec) {
	if (!urbc->pending) return;
	
	UartRxBufferController* bc = urbc;

	uint16_t start = bc->lastPos;
	uint16_t end = bc->currPos;

	uint16_t commandStart = bc->commandIndex;

	bool gotLine = false;
	uint16_t newLastPos = start;

	if (start < end) {
		for (uint16_t i = start; i < end; i++) {
			char nextChar = bc->ingestBuffer[i];

			if (nextChar == '\b' || nextChar == 0x7F) {
				if (bc->commandIndex == 0) {
					continue;
				}
				bc->command[--(bc->commandIndex)] = 0;
				HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 100);

				newLastPos = i + 1;
				continue;
			}

			if (bc->commandIndex < (sizeof(bc->command) - 1)) {
				char upperChar = toupper((uint8_t)nextChar);
				bc->command[bc->commandIndex++] = upperChar;
				HAL_UART_Transmit(&huart2, (uint8_t*)&upperChar, 1, 100);
			}

			newLastPos = i + 1;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}
	else if (start > end) {
		uint16_t len = INGEST_BUFFER_SIZE - start + end;
		uint16_t i = start;
		for (uint16_t k = 0; k < len; k++) {
			if (i == INGEST_BUFFER_SIZE) i = 0;

			char nextChar = bc->ingestBuffer[i++];

			if (nextChar == '\b' || nextChar == 0x7F) {
				if (bc->commandIndex == 0) {
					continue;
				}
				bc->command[--(bc->commandIndex)] = 0;
				HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 100);

				newLastPos = i;
				continue;
			}

			if (bc->commandIndex < (sizeof(bc->command) - 1)) {
				char upperChar = toupper((uint8_t)nextChar);
				bc->command[bc->commandIndex++] = upperChar;
				HAL_UART_Transmit(&huart2, (uint8_t*)&upperChar, 1, 100);
			}

			newLastPos = i;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}
	else {
		bc->pending = false;
		return;
	}

	bc->lastPos = newLastPos % INGEST_BUFFER_SIZE;

	if (!gotLine) {
		bc->pending = false;
		return;
	}

	while (bc->commandIndex > 0 && (bc->command[bc->commandIndex - 1] == '\n' || bc->command[bc->commandIndex - 1] == '\r')) {
		bc->commandIndex--;
	}
	bc->command[bc->commandIndex] = '\0';

	Update_MusicEngine_Command(bc, mec, bc->command);

	HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);

	bc->pending = false;
	memset(bc->command, 0, sizeof(bc->command));
	bc->commandIndex = 0;
}

void Update_MusicEngine_Command(UartRxBufferController* urbc, MusicEngineController* mec, char* command) {
	HAL_UART_Transmit(&huart2, (uint8_t*)urbc->command, strlen(urbc->command), 100);
	HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);
	if (mec == NULL) return;

	char* endptr;
	int64_t num = strtol(command, &endptr, 10);
	if (*endptr == '\0') {
		mec->number = (int16_t)num;
		return;
	}

	mec->number = -1;
	for (uint16_t i = 0; i < COMMAND_COUNT; i++) {
		if (strcmp(command, COMMAND_STRINGS[i]) == 0) {
			mec->command = i;
		}

	}
}
