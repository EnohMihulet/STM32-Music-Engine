#ifndef APP_UART_CLI_H
#define APP_UART_CLI_H
#pragma once

#include <stdint.h>

#include "buzzer.h"
#include "queue.h"

#define UART_RX_BUFFER_SIZE 256
#define COMMAND_CAPACITY 64
#define COMMAND_QUEUE_CAPACITY 15
#define COMMAND_STRING_CAPACITY 16

#define COMMAND_SEGMENT_COUNT 4

#define LIST_OF_COMMANDS \
	X(Command_None,		0, "NONE")   \
	X(Command_Pause,	0, "PAUSE")  \
	X(Command_Resume,	0, "RESUME") \
	X(Command_Stop,		0, "STOP")   \
	X(Command_Skip,		0, "SKIP")   \
	X(Command_Clear,	0, "CLEAR")  \
	X(Command_Songs,	0, "SONGS")  \
	X(Command_Play,		1, "PLAY")   \
	X(Command_Queue,	1, "QUEUE")  \
	X(Command_Tempo,	1, "TEMPO")  \
	X(Command_Volume,	1, "VOLUME") \
	X(Command_Status,	0, "STATUS") \
	X(Command_NewSong,	1, "NEWSONG") \
	X(Command_AddNote,	2, "ADD NOTE") \
	X(Command_AddRest,	1, "ADD REST") \
	X(Command_ListSong,	0, "LISTSONG") \
	X(Command_PlaySong,	0, "PLAYSONG") \
	X(Command_ClearSong,	0, "CLEARSONG") \
	X(Command_Save,		0, "SAVE") \
	X(Command_Load,		0, "LOAD")

#define X(COMMAND, COMMAND_ARG_COUNT, COMMAND_STR) COMMAND,
typedef enum CommandCode {
	LIST_OF_COMMANDS
	COMMAND_COUNT
} CommandCode;
#undef X

#define X(COMMAND, COMMAND_ARG_COUNT, COMMAND_STR) COMMAND_ARG_COUNT,
static const int8_t COMMAND_ARG_COUNTS[COMMAND_COUNT] = {
	LIST_OF_COMMANDS
};
#undef X

#define X(COMMAND, COMMAND_ARG_COUNT, COMMAND_STR) COMMAND_STR,
static const char* const COMMAND_STRINGS[COMMAND_COUNT] = {
	LIST_OF_COMMANDS
};
#undef X

#define LIST_OF_ERRORS  \
	X(ERR_UnknownCommand, "UNKNOWN COMMAND") 		\
	X(ERR_ArgumentCount,  "TOO FEW/MANY ARGUMENTS") 	\
	X(ERR_BadNumber,      "NUMERIC ARG NOT PARSEABLE")	\
	X(ERR_OutOfRange,     "ARG OUTSIDE ALLOWED RANGE")	\
	X(ERR_BadArgument,    "UNEXPECTED ARGUMENT")		\
	X(ERR_Invalid_State,  "COMMAND NOT ALLOWED RIGHT NOW")	\
	X(ERR_Capacity,       "RAN OUT OF SPACE")		\
	X(ERR_Empty,          "REQUIRED THING MISSING")		\
	X(ERR_Busy,           "TRY AGAIN LATER")		\
	X(ERR_Internal,       "UNEXPECTED FAILURE")

#define X(ERR, ERR_STR) ERR,
typedef enum CommandReturnCode {
	OK,
	LIST_OF_ERRORS
	COMMAND_RETURN_CODE_COUNT
} CommandReturnCode;
#undef X

#define X(ERR, ERR_STR) ERR_STR,
static const char* const COMMAND_RETURN_CODES[COMMAND_RETURN_CODE_COUNT] = {
	"OK",
	LIST_OF_ERRORS
};
#undef X


typedef enum CommandPayloadKind {
	Command_Args0,
	Command_Args1,
	Command_Args2,
	Command_Str
} CommandPayloadKind;


typedef struct Command {
	CommandCode cc;
	CommandPayloadKind kind;

	union {
		struct {} args0;
		struct { int32_t a1; } args1;
		struct { int32_t a1, a2; } args2;
		char str[COMMAND_STRING_CAPACITY];
	} u;
} Command;

QUEUE_DECLARE(CommandQueue, Command, COMMAND_QUEUE_CAPACITY)

typedef struct UartCLIController {
	volatile uint16_t lastPos;
	volatile uint16_t currPos;
	uint8_t* rxBuffer;

	char command[COMMAND_CAPACITY];
	uint16_t commandIndex;
} UartCLIController;

void UartCLIController_Init(UartCLIController* ucc);

void Uart_Update(UartCLIController* ucc, CommandQueue* cq);

void Append_To_CommandBuffer(UartCLIController* ucc, char c);

CommandReturnCode Parse_CommandString(UartCLIController* ucc, Command* out);

int16_t CommandCode_From_String(char* commandStr, CommandCode* cc);

int16_t CommandArg_From_String(char* commandStr, int32_t* arg);

CommandReturnCode CommandArg_Is_Valid(Command* c);

void Print_CommandReturnCode(CommandReturnCode crc);

int16_t echo(const char* s, uint16_t len);

#endif
