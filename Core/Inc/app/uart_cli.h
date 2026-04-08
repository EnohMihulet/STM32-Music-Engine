#ifndef APP_UART_CLI_H
#define APP_UART_CLI_H
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "buzzer.h"
#include "queue.h"

#define UART_RX_BUFFER_SIZE 256
#define COMMAND_CAPACITY 64
#define COMMAND_QUEUE_CAPACITY 16
#define COMMAND_STRING_CAPACITY 16
#define UART_OUTPUT_QUEUE_CAPACITY 16
#define CLI_RESPONSE_MSG_CAPACITY 64

#define COMMAND_SEGMENT_COUNT 4

#define VOLUME_RANGE_STR "0-100"
#define FREQHZ_RANGE_STR "16-65535"
#define DURMS_RANGE_STR "1-65535"

#define LIST_OF_COMMANDS \
	X(Command_None,		0, "",	"")   \
	X(Command_Commands,	0, "COMMANDS",	"COMMANDS") \
	X(Command_Pause,	0, "PAUSE",	"PAUSE")  \
	X(Command_Resume,	0, "RESUME", 	"RESUME") \
	X(Command_Stop,		0, "STOP", 	"STOP")   \
	X(Command_Skip,		0, "SKIP", 	"SKIP")   \
	X(Command_Clear,	0, "CLEAR", 	"CLEAR")  \
	X(Command_Songs,	0, "SONGS", 	"SONGS")  \
	X(Command_Play,		1, "PLAY", 	"PLAY <SONG_TITLE>")   \
	X(Command_Queue,	1, "QUEUE", 	"QUEUE <SONG_TITLE>")  \
	X(Command_Volume,	1, "VOLUME", 	"VOLUME <" VOLUME_RANGE_STR ">") \
	X(Command_Status,	0, "STATUS", 	"STATUS") \
	X(Command_NewSong,	1, "NEWSONG", 	"NEWSONG <NEW_SONG_TITLE>") \
	X(Command_EditSong,	1, "EDITSONG", 	"EDITSONG <SONG_TITLE>") \
	X(Command_CopySong,	2, "COPYSONG", 	"COPYSONG <SONG_TITLE> <NEW_SONG_TITLE>") \
	X(Command_AddNote,	2, "ADDNOTE", 	"ADDNOTE <" FREQHZ_RANGE_STR "> <" DURMS_RANGE_STR ">") \
	X(Command_AddRest,	1, "ADDREST", 	"ADDREST <" DURMS_RANGE_STR ">") \
	X(Command_EditNote,	3, "EDITNOTE", 	"EDITNOTE <FRAME_NUMBER> <" FREQHZ_RANGE_STR "> <" DURMS_RANGE_STR ">") \
	X(Command_EditTitle,	1, "EDITTITLE",	"EDITTITLE <NEW_TITLE>") \
	X(Command_ListSong,	0, "LISTSONG", 	"LISTSONG") \
	X(Command_PlaySong,	0, "PLAYSONG", 	"PLAYSONG") \
	X(Command_ClearSong,	0, "CLEARSONG", "CLEARSONG") \
	X(Command_Save,		0, "SAVE", 	"SAVE") \
	X(Command_Delete,	1, "DELETE", 	"DELETE <SONG_TITLE>") \
	X(Command_Quit,		0, "QUIT",	"QUIT") \

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

#define LIST_OF_ERRORS                                     \
	X(ERR_None,	      "")                          \
	X(ERR_UnknownCommand, "UnknownCommand")            \
	X(ERR_ArgumentCount,  "WrongArgumentCount")        \
	X(ERR_BadNumber,      "NumericArgNotParsable")     \
	X(ERR_OutOfRange,     "OutsideAllowedRange")       \
	X(ERR_BadArgument,    "UnexpectedArgument")        \
	X(ERR_Invalid_State,  "CommandNotAllowedRightNow") \
	X(ERR_Capacity,       "RanOutOfSpace")             \
	X(ERR_Empty,          "RequiredThingMissing")      \
	X(ERR_Busy,           "TryAgainLater")             \
	X(ERR_Internal,       "UnexpectedFailure")

#define X(ERR, ERR_STR) ERR,
typedef enum ErrCode {
	LIST_OF_ERRORS
	ERR_CODE_COUNT
} ErrCode;
#undef X

#define X(ERR, ERR_STR) ERR_STR,
static const char* const ERR_CODES[ERR_CODE_COUNT] = {
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
	uint16_t id;
	CommandCode cc;
	CommandPayloadKind kind;
	bool confirmed;

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

typedef enum ResponseKind {
	RESP_OK, RESP_ERR, RESP_INFO, RESP_WARN
} ResponseKind;

typedef struct CLIResponse {
	uint16_t id;
	ResponseKind kind;
	ErrCode code;
	char msg[CLI_RESPONSE_MSG_CAPACITY];
} CLIResponse;

typedef struct CommandStrSegments {
	size_t count;
	char s1[COMMAND_STRING_CAPACITY];
	char s2[COMMAND_STRING_CAPACITY];
	char s3[COMMAND_STRING_CAPACITY];
	char s4[COMMAND_STRING_CAPACITY];
} CommandStrSegments;

QUEUE_DECLARE(CommandQueue, Command, COMMAND_QUEUE_CAPACITY)
QUEUE_DECLARE(CLIResponseQueue, CLIResponse, UART_OUTPUT_QUEUE_CAPACITY)

int CLIResponse_Emit(CLIResponseQueue* q, uint16_t id, ResponseKind kind, ErrCode code, const char* msg);
int CLIResponse_Emitf(CLIResponseQueue* q, uint16_t id, ResponseKind kind, ErrCode code, const char* fmt, ...);

typedef struct UartCLIController {
	volatile uint16_t lastPos;
	volatile uint16_t currPos;
	uint16_t nextCommandId;

	bool promptPending;
	bool confirmationPending;
	Command needsConfirmed;

	uint8_t* rxBuffer;
	CLIResponseQueue responseQueue;
	char command[COMMAND_CAPACITY];
	uint16_t commandIndex;
} UartCLIController;

void UartCLIController_Init(UartCLIController* ucc);
void Uart_Update(UartCLIController* ucc, CommandQueue* cq);

void Maybe_PrintPrompt(UartCLIController* ucc, CommandQueue* cq);

int Read_From_RXBuffer(UartCLIController* ucc);
void Append_To_CommandBuffer(UartCLIController* ucc, char c);

int Parse_ConfirmationInput(UartCLIController*ucc);
int Parse_CommandString(UartCLIController* ucc, Command* out);
int Break_Up_CommandString(UartCLIController* ucc, CommandStrSegments* css, uint16_t commandId);
int Parse_ZeroArgCommand(UartCLIController* ucc, CommandStrSegments* css, Command* out);
int Parse_OneArgCommand(UartCLIController* ucc, CommandStrSegments* css, Command* out);
int Parse_TwoArgCommand(UartCLIController* ucc, CommandStrSegments* css, Command* out);
int Parse_ThreeArgCommand(UartCLIController* ucc, CommandStrSegments* css, Command* out);

int CommandCode_From_String(char* commandStr, CommandCode* cc);
int CommandArg_From_String(char* commandStr, int64_t* arg);

int Validate_ArgCount();

bool Print_CLIResponses(UartCLIController* ucc);
void Print_ErrCode(uint16_t id, ErrCode code, const char* detail);
void Print_Commands();
void echo_newline();
int echo(const char* s, uint16_t len);
int echos(const char* s);

#endif
