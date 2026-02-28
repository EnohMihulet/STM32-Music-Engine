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

#define COMMAND_SEGMENT_COUNT 5

#define STR(x) #x
#define XSTR(x) STR(x)

#define RANGE_STR(min, max) XSTR(min) "-" XSTR(max)

#define VOLUME_RANGE_STR RANGE_STR(VOLUME_MIN, VOLUME_MAX)
#define FREQHZ_RANGE_STR RANGE_STR(FREQUENCY_MIN_HZ, FREQUENCY_MAX_HZ)
#define DURMS_RANGE_STR RANGE_STR(DURATION_MIN_MS, DURATION_MAX_MS)

#define LIST_OF_COMMANDS \
	X(Command_None,		0, "NONE",	"")   \
	X(Command_Commands,	0, "COMMANDS",	"COMMANDS") \
	X(Command_Pause,	0, "PAUSE",	"PAUSE")  \
	X(Command_Resume,	0, "RESUME", 	"RESUME") \
	X(Command_Stop,		0, "STOP", 	"STOP")   \
	X(Command_Skip,		0, "SKIP", 	"SKIP")   \
	X(Command_Clear,	0, "CLEAR", 	"CLEAR")  \
	X(Command_Songs,	0, "SONGS", 	"SONGS")  \
	X(Command_Play,		1, "PLAY", 	"PLAY <SONG_TITLE>")   \
	X(Command_Queue,	1, "QUEUE", 	"QUEUE <SONG_TITLE")  \
	X(Command_Tempo,	1, "TEMPO", 	"TEMPO <>")  \
	X(Command_Volume,	1, "VOLUME", 	"VOLUME <" VOLUME_RANGE_STR ">") \
	X(Command_Status,	0, "STATUS", 	"STATUS") \
	X(Command_NewSong,	1, "NEWSONG", 	"NEWSONG <NEW_SONG_TITLE>") \
	X(Command_EditSong,	1, "EDITSONG", 	"EDITSONG <SONG_TITLE>") \
	X(Command_CopySong,	2, "COPYSONG", 	"COPYSONG <SONG_TITLE> <NEW_SONG_TITLE>") \
	X(Command_AddNote,	2, "ADDNOTE", 	"ADDNOTE <" FREQHZ_RANGE_STR "> <" DURMS_RANGE_STR ">") \
	X(Command_AddRest,	1, "ADDREST", 	"ADDREST <" DURMS_RANGE_STR ">") \
	X(Command_EditNote,	3, "EDITNOTE", "EDITNOTE <FRAME_NUMBER> <" FREQHZ_RANGE_STR "> <" DURMS_RANGE_STR ">") \
	X(Command_EditTitle,	1, "EDITTITLE","EDITTITLE <NEW_TITLE>") \
	X(Command_ListSong,	0, "LISTSONG", 	"LISTSONG") \
	X(Command_PlaySong,	0, "PLAYSONG", 	"PLAYSONG") \
	X(Command_ClearSong,	0, "CLEARSONG", "CLEARSONG") \
	X(Command_Save,		0, "SAVE", 	"SAVE") \

#define X(COMMAND, COMMAND_ARG_COUNT, COMMAND_STR, COMMAND_USAGE) COMMAND,
typedef enum CommandCode {
	LIST_OF_COMMANDS
	COMMAND_COUNT
} CommandCode;
#undef X

#define X(COMMAND, COMMAND_ARG_COUNT, COMMAND_STR, COMMAND_USAGE) COMMAND_ARG_COUNT,
static const int8_t COMMAND_ARG_COUNTS[COMMAND_COUNT] = {
	LIST_OF_COMMANDS
};
#undef X

#define X(COMMAND, COMMAND_ARG_COUNT, COMMAND_STR, COMMAND_USAGE) COMMAND_STR,
static const char* const COMMAND_STRINGS[COMMAND_COUNT] = {
	LIST_OF_COMMANDS
};
#undef X

#define X(COMMAND, COMMAND_ARG_COUNT, COMMAND_STR, COMMAND_USAGE) COMMAND_USAGE,
static const char* const COMMAND_USAGE[COMMAND_COUNT] = {
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
	Command_Int1,
	Command_Int2,
	Command_Int3,
	Command_Str1,
	Command_Str2
} CommandPayloadKind;


typedef struct Command {
	CommandCode cc;
	CommandPayloadKind kind;

	union {
		struct {} args0;
		struct { int32_t a1; } int1;
		struct { int32_t a1, a2; } int2;
		struct { int32_t a1, a2, a3; } int3;
		struct { char s[COMMAND_STRING_CAPACITY]; } str1;
		struct {
			char s1[COMMAND_STRING_CAPACITY];
			char s2[COMMAND_STRING_CAPACITY];
		} str2;
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

void Print_Commands();

int16_t echo(const char* s, uint16_t len);

#endif
