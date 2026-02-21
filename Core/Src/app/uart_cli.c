#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../../Inc/usart.h"
#include "../../Inc/app/uart_cli.h"

void UartCLIController_Init(UartCLIController* ucc) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	ucc->lastPos = 0;
	ucc->currPos = 0;
	ucc->commandIndex = 0;
	memset(ucc->command, 0, sizeof(ucc->command));
	__set_PRIMASK(primask);
}

void Uart_Update(UartCLIController* ucc, MusicEngineController* mec) {
	uint32_t primask = __get_PRIMASK();
	uint16_t start, end;
	__disable_irq();
	start = ucc->lastPos;
	end = ucc->currPos;
	__set_PRIMASK(primask);

	if (start == end) return;

	bool gotLine = false;
	uint16_t newLastPos = start;

	if (start < end) {
		for (uint16_t i = start; i < end; i++) {

			char nextChar = toupper((uint8_t)ucc->ingestBuffer[i++]);
			Append_To_CommandBuffer(ucc, nextChar);
			if (nextChar != '\b' && nextChar != 0x7F) echo(&nextChar, 1);

			newLastPos = i;

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

			char nextChar = toupper((uint8_t)ucc->ingestBuffer[i++]);
			Append_To_CommandBuffer(ucc, nextChar);
			if (nextChar != '\b' && nextChar != 0x7F) echo(&nextChar, 1);

			newLastPos = i;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}

	primask = __get_PRIMASK();
	__disable_irq();
	ucc->lastPos = newLastPos;
	__set_PRIMASK(primask);

	if (!gotLine) {
		return;
	}

	while (ucc->commandIndex > 0 && (ucc->command[ucc->commandIndex - 1] == '\n' || ucc->command[ucc->commandIndex - 1] == '\r')) {
		ucc->commandIndex--;
	}
	ucc->command[ucc->commandIndex] = '\0';

	Command c;
	if (Parse_CommandString(ucc, &c) == 0) CommandQueue_Push(&mec->commandQueue, c);

	echo("\r\n", 2);

	memset(ucc->command, 0, sizeof(ucc->command));
	ucc->commandIndex = 0;
}

void Append_To_CommandBuffer(UartCLIController* ucc, char c) {
	if (c == '\b' || c == 0x7F) {
		if (ucc->commandIndex == 0) {
			return;
		}
		ucc->command[--(ucc->commandIndex)] = 0;
		echo("\b \b", 3);
		return;
	}

	if (ucc->commandIndex < (sizeof(ucc->command) - 1)) {
		c = toupper((uint8_t)c);
		ucc->command[ucc->commandIndex++] = c;
	}
}

int16_t Parse_CommandString(UartCLIController* ucc, Command* out) {
	CommandCode cc = COMMAND_COUNT;
	int32_t arg = -1;
	for (uint16_t i = 0; i < ucc->commandIndex; i++) {
		char c = ucc->command[i];
		if (c == ' ') {
			ucc->command[i] = '\0';
			for (uint16_t i = 0; i < COMMAND_COUNT; i++) {
				if (strcmp(ucc->command, COMMAND_STRINGS[i]) == 0) {
					cc = (CommandCode)i;
					break;
				}
			}
			if (cc == COMMAND_COUNT) return -1;

			char* endptr;
			int64_t num = strtol((char*)ucc->command + i + 1, &endptr, 10);
			if (*endptr == '\0') arg = num;
			break;
		}
		else if (i == ucc->commandIndex - 1) {
			for (uint16_t i = 0; i < COMMAND_COUNT; i++) {
				if (strcmp(ucc->command, COMMAND_STRINGS[i]) == 0) {
					cc = (CommandCode)i;
					break;
				}
			}
			if (cc == COMMAND_COUNT) return -1;
		}
	}
	out->cc = cc;
	out->arg = arg;

	return 0;
}

uint16_t echo(char* s, uint16_t len) {
	if (len == 0) return -1;

	HAL_UART_Transmit(&huart2, (uint8_t*)s, len, 100);
	return 0;
}
