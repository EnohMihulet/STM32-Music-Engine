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

	uint16_t start = urbc->lastPos;
	uint16_t end = urbc->currPos;

	bool gotLine = false;
	uint16_t newLastPos = start;

	if (start < end) {
		for (uint16_t i = start; i < end; i++) {
			char nextChar = urbc->ingestBuffer[i];

			if (nextChar == '\b' || nextChar == 0x7F) {
				if (urbc->commandIndex == 0) {
					continue;
				}
				urbc->command[--(urbc->commandIndex)] = 0;
				echo("\b \b", 3);

				newLastPos = i + 1;
				continue;
			}

			if (urbc->commandIndex < (sizeof(urbc->command) - 1)) {
				char upperChar = toupper((uint8_t)nextChar);
				urbc->command[urbc->commandIndex++] = upperChar;
				echo(&upperChar, 1);
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

			char nextChar = urbc->ingestBuffer[i++];

			if (nextChar == '\b' || nextChar == 0x7F) {
				if (urbc->commandIndex == 0) {
					continue;
				}
				urbc->command[--(urbc->commandIndex)] = 0;
				echo("\b \b", 3);

				newLastPos = i;
				continue;
			}

			if (urbc->commandIndex < (sizeof(urbc->command) - 1)) {
				char upperChar = toupper((uint8_t)nextChar);
				urbc->command[urbc->commandIndex++] = upperChar;
				echo(&upperChar, 1);
			}

			newLastPos = i;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}
	else {
		urbc->pending = false;
		return;
	}

	urbc->lastPos = newLastPos % INGEST_BUFFER_SIZE;

	if (!gotLine) {
		urbc->pending = false;
		return;
	}

	while (urbc->commandIndex > 0 && (urbc->command[urbc->commandIndex - 1] == '\n' || urbc->command[urbc->commandIndex - 1] == '\r')) {
		urbc->commandIndex--;
	}

	urbc->command[urbc->commandIndex] = '\0';

	Update_MusicEngine_Command(urbc, mec, urbc->command);

	echo("\r\n", 2);

	urbc->pending = false;
	memset(urbc->command, 0, sizeof(urbc->command));
	urbc->commandIndex = 0;
}

void Update_MusicEngine_Command(UartRxBufferController* urbc, MusicEngineController* mec, char* command) {
	if (mec == NULL) return;

	char* endptr;
	int64_t num = strtol(command, &endptr, 10);
	if (*endptr == '\0') {
		Command c = {urbc->lastCC, num};
		CommandQueue_Push(&(mec->commandQueue), c);
		Update_LastCommandCode(urbc, Command_None);
	}

	for (uint16_t i = 0; i < COMMAND_COUNT; i++) {
		if (strcmp(command, COMMAND_STRINGS[i]) == 0) {
			Command c = {(CommandCode)i, -1};

			if (c.cc == Command_Play || c.cc == Command_Queue) c.cc = Command_Songs;

			CommandQueue_Push(&(mec->commandQueue), c);
			Update_LastCommandCode(urbc, (CommandCode)i);
			return;
		}
	}

	Update_LastCommandCode(urbc, Command_None);
}

void Update_LastCommandCode(UartRxBufferController* urbc, CommandCode cc) {
	urbc->lastCC = cc;
}

uint16_t echo(char* s, uint16_t len) {
	if (len == 0) return -1;

	HAL_UART_Transmit(&huart2, (uint8_t*)s, len, 100);
	return 0;
}
