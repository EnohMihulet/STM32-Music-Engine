#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../Inc/usart.h"
#include "../../Inc/app/uart_cli.h"


void UartCLIController_Init(UartCLIController* ucc) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	ucc->lastPos = 0;
	ucc->currPos = 0;
	ucc->nextCommandId = 0;
	ucc->commandIndex = 0;
	memset(ucc->command, 0, sizeof(ucc->command));
	__set_PRIMASK(primask);
}

int CLIResponse_Emit(CLIResponseQueue* q, uint16_t id, ResponseKind kind, ErrCode code, const char* msg) {
	CLIResponse resp = { .id = id, .kind = kind, .code = code };
	if (msg != NULL) {
		strncpy(resp.msg, msg, CLI_RESPONSE_MSG_CAPACITY - 1);
		resp.msg[CLI_RESPONSE_MSG_CAPACITY - 1] = '\0';
	}
	else resp.msg[0] = '\0';

	return CLIResponseQueue_Push(q, resp);
}

int CLIResponse_Emitf(CLIResponseQueue* q, uint16_t id, ResponseKind kind, ErrCode code, const char* fmt, ...) {
	CLIResponse resp = { .id = id, .kind = kind, .code = code };
	if (fmt != NULL) {
		va_list args;
		va_start(args, fmt);
		vsnprintf(resp.msg, CLI_RESPONSE_MSG_CAPACITY, fmt, args);
		va_end(args);
	}
	else resp.msg[0] = '\0';

	return CLIResponseQueue_Push(q, resp);
}

void Uart_Update(UartCLIController* ucc, CommandQueue* cq) {
	Print_CLIResponses(ucc);

	if (Read_From_RXBuffer(ucc) == -1) return;

	Command c;
	c.id = ucc->nextCommandId++;

	if (Parse_CommandString(ucc, &c) == 0) {
		if (c.cc == Command_Commands) Print_Commands();
		else CommandQueue_Push(cq, c);
	}

	memset(ucc->command, 0, sizeof(ucc->command));
	ucc->commandIndex = 0;
}

void Print_CLIReponses(UartCLIController* ucc) {
	CLIResponse resp;
	while (CLIResponseQueue_Pop(&ucc->responseQueue, &resp) != -1) {
	}
}

int Read_From_RXBuffer(UartCLIController* ucc) {
	uint32_t primask = __get_PRIMASK();
	uint16_t start, end;
	__disable_irq();
	start = ucc->lastPos;
	end = ucc->currPos;
	__set_PRIMASK(primask);

	if (start == end) return -1;

	bool gotLine = false;
	uint16_t newLastPos = start;

	if (start < end) {
		for (uint16_t i = start; i < end; i++) {
			char nextChar = toupper((uint8_t)ucc->rxBuffer[i]);
			Append_To_CommandBuffer(ucc, nextChar);
			if (nextChar != '\b' && nextChar != 0x7F) echo(&nextChar, 1);

			newLastPos = i + 1;

			if (nextChar == '\n' || nextChar == '\r') {
				echo_newline();
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
				echo_newline();
				gotLine = true;
				break;
			}
		}
	}

	ucc->lastPos = newLastPos % UART_RX_BUFFER_SIZE;

	if (!gotLine) return -1;

	while (ucc->commandIndex > 0 && (ucc->command[ucc->commandIndex - 1] == '\n' || ucc->command[ucc->commandIndex - 1] == '\r')) {
		ucc->commandIndex--;
	}
	ucc->command[ucc->commandIndex] = '\0';

	return 0;
}

void Append_To_CommandBuffer(UartCLIController* ucc, char c) {
	if (c == '\b' || c == 0x7F) {
		if (ucc->commandIndex == 0) return;
		ucc->command[--(ucc->commandIndex)] = 0;
		echo("\b \b", 3);
		return;
	}

	if (ucc->commandIndex < (sizeof(ucc->command) - 1)) {
		c = toupper((uint8_t)c);
		ucc->command[ucc->commandIndex++] = c;
	}
}

int Parse_CommandString(UartCLIController* ucc, Command* out) {
	CommandStrSegments css;
	if (Break_Up_CommandString(ucc, &css, out->id) == -1) return -1;

	if (CommandCode_From_String(css.s1, &out->cc) == -1) {
		CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_UnknownCommand, NULL);
		CLIResponse_Emitf(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Unknown command: %s", css.s1);
		return -1;
	}

	if (out->cc == Command_None)
		return -1;

	switch (css.count) {
		case 1: return Parse_ZeroArgCommand(ucc, &css, out);
		case 2: return Parse_OneArgCommand(ucc, &css, out);
		case 3: return Parse_TwoArgCommand(ucc, &css, out);
		case 4: return Parse_ThreeArgCommand(ucc, &css, out);
		default: break;
	}

	return 0;
}

int Parse_ZeroArgCommand(UartCLIController* ucc, CommandStrSegments* css, Command* out) {
	switch (out->cc) {
		case Command_Pause: case Command_Resume: case Command_Stop: case Command_Skip: case Command_Clear: 
		case Command_Commands: case Command_Songs: case Command_Status: 
		case Command_ListSong: case Command_PlaySong: case Command_ClearSong: case Command_Save: case Command_Quit:
			out->kind = Command_Args0;
			return 0;

		default: {
			CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_ArgumentCount, NULL);
			CLIResponse_Emitf(
				&ucc->responseQueue,
				out->id,
				RESP_INFO,
				ERR_None,
				"Command %s expects %d args",
				COMMAND_STRINGS[out->cc],
				COMMAND_ARG_COUNTS[out->cc]
			);
			return -1;
		}
	}
	return 0;
}

int Parse_OneArgCommand(UartCLIController* ucc, CommandStrSegments* css, Command* out) {
	switch (out->cc) {
		// ARGS 1 STR
		case Command_Play: case Command_Queue: 
		case Command_NewSong: case Command_EditSong: case Command_EditTitle: case Command_Delete:
			out->kind = Command_Str1;
			strcpy(out->u.str1.s, css->s2);
			return 0;

		// ARGS 1 NUMBER
		case Command_Volume: case Command_AddRest: {
			int64_t num;
			if (CommandArg_From_String(css->s2, &num)) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Did you mistype the number?");
				return -1;
			}

			if (num < 0) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Argument must be positive");
				return -1;
			}
			else if (num > INT32_MAX) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Argument is greater than int32_t max");
				return -1;
			}

			if (out->cc == Command_Volume && (num < VOLUME_MIN || num > VOLUME_MAX)) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_OutOfRange, NULL);
				CLIResponse_Emitf(
					&ucc->responseQueue,
					out->id,
					RESP_INFO,
					ERR_None,
					"Frequency valid range: %d-%d",
					VOLUME_MIN,
					VOLUME_MAX
				);
				return -1;
			}
			out->kind = Command_Int1;
			out->u.int1.a1 = num;
			return 0;
		}
		default: {
			CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_ArgumentCount, NULL);
			CLIResponse_Emitf(
				&ucc->responseQueue,
				out->id,
				RESP_INFO,
				ERR_None,
				"Command %s expects %d args",
				COMMAND_STRINGS[out->cc],
				COMMAND_ARG_COUNTS[out->cc]
			);
			return -1;
		}
	}
	return 0;
}

int Parse_TwoArgCommand(UartCLIController* ucc, CommandStrSegments* css, Command* out) {
	switch (out->cc) {
		case Command_CopySong:
			out->kind = Command_Str2;
			strcpy(out->u.str2.s1, css->s2);
			strcpy(out->u.str2.s2, css->s3);
			return 0;
		case Command_AddNote:
			out->kind = Command_Int2;
			int64_t num1;
			int64_t num2;
			int err1 = CommandArg_From_String(css->s2, &num1);
			int err2 = CommandArg_From_String(css->s3, &num2);
			if (err1 == -1 || err2 == -1) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Did you mistype the number?");
				return -1;			
			}

			if (num1 < 0 || num2 < 0) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Arguments must be positive");
				return -1;
			}
			else if (num1 > INT32_MAX || num2 > INT32_MAX) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Arguments is greater than int32_t max");
				return -1;
			}

			if (num1 < FREQUENCY_MIN_HZ || num1 > FREQUENCY_MAX_HZ) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_OutOfRange, NULL);
				CLIResponse_Emitf(
					&ucc->responseQueue,
					out->id,
					RESP_INFO,
					ERR_None,
					"Frequency valid range: %d-%d",
					FREQUENCY_MIN_HZ,
					FREQUENCY_MAX_HZ
				);
				return -1;
			}
			out->kind = Command_Int2;
			out->u.int2.a1 = num1;
			out->u.int2.a2 = num2;
			return 0;
		default: {
			CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_ArgumentCount, NULL);
			CLIResponse_Emitf(
				&ucc->responseQueue,
				out->id,
				RESP_INFO,
				ERR_None,
				"Command %s expects %d args",
				COMMAND_STRINGS[out->cc],
				COMMAND_ARG_COUNTS[out->cc]
			);
			return -1;
		}
	}
	return 0;
}

int Parse_ThreeArgCommand(UartCLIController* ucc, CommandStrSegments* css, Command* out) {
	switch (out->cc) {
		case Command_EditNote:
			out->kind = Command_Int3;
			int64_t num1;
			int64_t num2;
			int64_t num3;
			int err1 = CommandArg_From_String(css->s2, &num1);
			int err2 = CommandArg_From_String(css->s3, &num2);
			int err3 = CommandArg_From_String(css->s4, &num3);
			if (err1 == -1 || err2 == -1 || err3 == -1) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Did you mistype the number?");
				return -1;
			}

			if (num1 < 0 || num2 < 0 || num3 < 0) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Arguments must be positive");
				return -1;
			}
			else if (num1 > INT32_MAX || num2 > INT32_MAX || num3 > INT32_MAX) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_BadNumber, NULL);
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_INFO, ERR_None, "Arguments is greater than int32_t max");
				return -1;
			}

			if (num2 < FREQUENCY_MIN_HZ || num2 > FREQUENCY_MAX_HZ) {
				CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_OutOfRange, NULL);
				CLIResponse_Emitf(
					&ucc->responseQueue,
					out->id,
					RESP_INFO,
					ERR_None,
					"Frequency valid range: %d-%d",
					FREQUENCY_MIN_HZ,
					FREQUENCY_MAX_HZ
				);
				return -1;
			}
			out->kind = Command_Int3;
			out->u.int3.a1 = num1;
			out->u.int3.a2 = num2;
			out->u.int3.a3 = num3;
			return 0;
		default: {
			CLIResponse_Emit(&ucc->responseQueue, out->id, RESP_ERR, ERR_ArgumentCount, NULL);
			CLIResponse_Emitf(
				&ucc->responseQueue,
				out->id,
				RESP_INFO,
				ERR_None,
				"Command %s expects %d args",
				COMMAND_STRINGS[out->cc],
				COMMAND_ARG_COUNTS[out->cc]
			);
			return -1;
		}
	}
	return 0;
}

// TODO: Should probably rewrite this
int Break_Up_CommandString(UartCLIController *ucc, CommandStrSegments *css, uint16_t commandId) {
	css->count = 0;
	char* currStr = css->s1;

	int start = 0;
	for (uint16_t i = 0; i <= ucc->commandIndex; i++) {
		char c = ucc->command[i];

		if (i - start >= COMMAND_STRING_CAPACITY) {
			CLIResponse_Emit(&ucc->responseQueue, commandId, RESP_ERR, ERR_Capacity, NULL);
			return -1;
		}

		if (c == ' ' || c == '\0') {
			if (css->count >= COMMAND_SEGMENT_COUNT) {
				CLIResponse_Emit(&ucc->responseQueue, commandId, RESP_ERR, ERR_ArgumentCount, NULL);
				return -1;
			}
			ucc->command[i] = '\0';
			strcpy(currStr, ucc->command + start);

			currStr = currStr + COMMAND_STRING_CAPACITY;
			css->count += 1;
			start = i + 1;
		}
	}

	return 0;
}

int CommandCode_From_String(char* commandStr, CommandCode* cc) {
	for (uint16_t i = 0; i < COMMAND_COUNT; i++) {
		if (strcmp(commandStr, COMMAND_STRINGS[i]) == 0) {
			*cc = (CommandCode)i;
			return 0;
		}
	}
	return -1;
}

int CommandArg_From_String(char* commandStr, int64_t* arg) {
	char* endptr;
	int64_t num = strtol(commandStr, &endptr, 10);
	if (*endptr == '\0') *arg = num;
	else return -1;
	return 0;
}

void Print_CLIResponses(UartCLIController *ucc) {
	while (!CLIResponseQueue_IsEmpty(&ucc->responseQueue)) {
		CLIResponse resp;
		CLIResponseQueue_Pop(&ucc->responseQueue, &resp);
		if (resp.kind == RESP_OK || resp.kind == RESP_ERR) {
			Print_ErrCode(resp.code);
		}
		else if (resp.kind == RESP_INFO) {
			char line[64];
			uint16_t len = snprintf(line, 64, "INFO %s\r\n", resp.msg);
			echo(line, len);
		}
		else {
			char line[64];
			uint16_t len = snprintf(line, 64, "WARNING %s\r\n", resp.msg);
			echo(line, len);
		}
	}
}

void Print_ErrCode(ErrCode code) {
	if (code == ERR_CODE_COUNT) return;
	if (code != ERR_None) {
		char buffer[64];
		size_t len = snprintf(buffer, 64, "ERR: %s\r\n", ERR_CODES[code]);
		echo(buffer, len);
	}
	else echo("OK\r\n", 4);
}

void Print_Commands() {
	for (uint16_t i = 2; i < COMMAND_COUNT; i++) {
		const char* c = COMMAND_USAGE[i];
		echo(c, strlen(c));
		echo_newline();
	}
}

int echo(const char* s, uint16_t len) {
	if (len == 0) return -1;
	HAL_UART_Transmit(&huart2, (uint8_t*)s, len, 100);
	return 0;
}

void echo_newline() {
	HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);
}
