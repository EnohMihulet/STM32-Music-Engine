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

void Uart_Update(UartCLIController* ucc, CommandQueue* cq) {
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
			char nextChar = toupper((uint8_t)ucc->rxBuffer[i]);
			Append_To_CommandBuffer(ucc, nextChar);
			if (nextChar != '\b' && nextChar != 0x7F) echo(&nextChar, 1);

			newLastPos = i + 1;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}
	else if (start > end) {
		uint16_t len = UART_RX_BUFFER_SIZE - start + end;
		uint16_t i = start;
		for (uint16_t k = 0; k < len; k++) {
			if (i == UART_RX_BUFFER_SIZE) i = 0;

			char nextChar = toupper((uint8_t)ucc->rxBuffer[i++]);
			Append_To_CommandBuffer(ucc, nextChar);
			if (nextChar != '\b' && nextChar != 0x7F) echo(&nextChar, 1);

			newLastPos = i;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}

	ucc->lastPos = newLastPos % UART_RX_BUFFER_SIZE;

	if (!gotLine) {
		return;
	}

	while (ucc->commandIndex > 0 && (ucc->command[ucc->commandIndex - 1] == '\n' || ucc->command[ucc->commandIndex - 1] == '\r')) {
		ucc->commandIndex--;
	}
	ucc->command[ucc->commandIndex] = '\0';

	echo("\r\n", 2);

	Command c;
	CommandReturnCode crc;
	crc = Parse_CommandString(ucc, &c);
	if (crc == OK) CommandQueue_Push(cq, c);
	else Print_CommandReturnCode(crc);

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

CommandReturnCode Parse_CommandString(UartCLIController* ucc, Command* out) {
	char buffer[COMMAND_SEGMENT_COUNT][COMMAND_STRING_CAPACITY];
	uint16_t segmentCount = 0;
	uint16_t segmentStart = 0;

	for (uint16_t i = 0; i <= ucc->commandIndex; i++) {
		char c = ucc->command[i];
		if (i - segmentStart  >= COMMAND_STRING_CAPACITY) return ERR_Capacity;
		if (c == ' ' || c == '\0') {
			ucc->command[i] = '\0';
			strcpy(buffer[segmentCount], ucc->command + segmentStart);
			segmentCount += 1;
			segmentStart = i + 1;
		}
	}

	if (segmentCount == 1) {
		if (CommandCode_From_String(buffer[0], &out->cc) != 0) return ERR_UnknownCommand;
		if (COMMAND_ARG_COUNTS[out->cc] != 0) return ERR_ArgumentCount;
		out->kind = Command_Args0;
	}
	else if (segmentCount == 2) {
		if (CommandCode_From_String(buffer[0], &out->cc) != 0) return ERR_UnknownCommand;
		if (COMMAND_ARG_COUNTS[out->cc] != 1) return ERR_ArgumentCount;

		if (out->cc == Command_Play || out->cc == Command_Queue || out->cc == Command_NewSong) {
			out->kind = Command_Str;
			strcpy(out->u.str, buffer[1]);
			return OK;
		}

		char* endptr;
		int64_t num = strtol((char*)buffer[1], &endptr, 10);
		if (*endptr == '\0') {
			out->kind = Command_Args1;
			out->u.args1.a1 = num;
			if (out->cc == Command_Volume && (out->u.args1.a1 < VOLUME_MIN || out->u.args1.a1 > VOLUME_MAX)) return ERR_OutOfRange;
			if (out->cc == Command_Tempo && (out->u.args1.a1 < VOLUME_MIN || out->u.args1.a1 > VOLUME_MAX)) return ERR_OutOfRange;
		}
		else return ERR_BadArgument;
	}
	else if (segmentCount == 3 || segmentCount == 4) {
		if (strcmp(buffer[0], "ADD") != 0) return ERR_UnknownCommand;

		if (strcmp(buffer[1], "NOTE") == 0) {
			if (segmentCount != 4) return ERR_ArgumentCount;
			out->cc = Command_AddNote;
			out->kind = Command_Args2;
			if (CommandArg_From_String(buffer[2], &out->u.args2.a1) == -1) return ERR_BadNumber;
			if (CommandArg_From_String(buffer[3], &out->u.args2.a2) == -1) return ERR_BadNumber;
			if (out->u.args2.a1 > FREQUENCY_MAX_HZ) return ERR_OutOfRange;
			if (out->u.args2.a1 < FREQUENCY_MIN_HZ) return ERR_OutOfRange;
			if (out->u.args2.a2 < 0) return ERR_OutOfRange;
		}
		else if (strcmp(buffer[1], "REST") == 0) {
			if (segmentCount != 3) return ERR_ArgumentCount;
			out->cc = Command_AddRest;
			out->kind = Command_Args1;
			if (CommandArg_From_String(buffer[2], &out->u.args1.a1) == -1) return ERR_BadNumber;
			if (out->u.args1.a1 < 0) return ERR_OutOfRange;
		}
		else return ERR_UnknownCommand;
	}
	else return ERR_UnknownCommand;

	return OK;
}

int16_t CommandCode_From_String(char* commandStr, CommandCode* cc) {
	for (uint16_t i = 0; i < COMMAND_COUNT; i++) {
		if (strcmp(commandStr, COMMAND_STRINGS[i]) == 0) {
			*cc = (CommandCode)i;
			return 0;
		}
	}
	return -1;
}

int16_t CommandArg_From_String(char* commandStr, int32_t* arg) {
	char* endptr;
	int64_t num = strtol(commandStr, &endptr, 10);
	if (*endptr == '\0') *arg = num;
	else return -1;
	return 0;
}

void Print_CommandReturnCode(CommandReturnCode crc) {
	if (crc == COMMAND_RETURN_CODE_COUNT) return;
	echo(COMMAND_RETURN_CODES[crc], strlen(COMMAND_RETURN_CODES[crc]));
	echo("\r\n", 2);
}

int16_t echo(const char* s, uint16_t len) {
	if (len == 0) return -1;

	HAL_UART_Transmit(&huart2, (uint8_t*)s, len, 100);
	return 0;
}
