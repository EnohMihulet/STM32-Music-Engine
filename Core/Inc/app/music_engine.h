#ifndef APP_MUSIC_ENGINE_H
#define APP_MUSIC_ENGINE_H
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "buzzer.h"
#include "queue.h"

#define TITLE_CAPACITY 16
#define FRAMES_CAPACITY 80
#define SONG_QUEUE_CAPACITY 16
#define COMMAND_QUEUE_CAPACITY 15

#define SONG_COUNT 3

#define LIST_OF_COMMANDS \
	X(Command_None,   "NONE")   \
	X(Command_Pause,  "PAUSE")  \
	X(Command_Resume, "RESUME") \
	X(Command_Stop,   "STOP")   \
	X(Command_Skip,   "SKIP")   \
	X(Command_Clear,  "CLEAR")  \
	X(Command_Songs,  "SONGS")  \
	X(Command_Play,   "PLAY")   \
	X(Command_Queue,  "QUEUE")  \
	X(Command_Tempo,  "TEMPO")  \
	X(Command_Volume, "VOLUME") \
	X(Command_Status, "STATUS")

#define X(COMMAND, COMMAND_STR) COMMAND,
typedef enum CommandCode {
	LIST_OF_COMMANDS
	COMMAND_COUNT
} CommandCode;
#undef X

#define X(COMMAND, COMMAND_STR) COMMAND_STR,
static const char* const COMMAND_STRINGS[COMMAND_COUNT] = {
	LIST_OF_COMMANDS
};
#undef X

typedef struct Frame {
	uint16_t frequencyHz;
	uint16_t durationMs;
} Frame;


typedef struct Song {
	char title[TITLE_CAPACITY];
	uint16_t framesSize;
	Frame frames[FRAMES_CAPACITY];
} Song;

static const Song SONG_1 = {
	.title = "SONG1",
	.framesSize = 1,
	.frames = {{988, 1000}}
};

static const Song SONG_2 = {
	.title = "SONG2",
	.framesSize = 1,
	.frames = {{659, 1000}}
};

static const Song SONG_3 = {
	.title = "SONG3",
	.framesSize = 1,
	.frames = {{392, 10000}}
};

static const Song SONGS[SONG_COUNT] = {SONG_1, SONG_2, SONG_3};

typedef struct Command {
	CommandCode cc;
	int32_t arg;
} Command;

QUEUE_DECLARE(SongQueue, uint16_t, SONG_QUEUE_CAPACITY)

QUEUE_DECLARE(CommandQueue, Command, COMMAND_QUEUE_CAPACITY)

typedef enum PlaybackState {Stopped, Playing, Paused} PlaybackState;

typedef struct MusicEngineController {
	volatile PlaybackState pbState;
	volatile uint16_t remainingTimeMs;
	volatile bool updateFrame;

	uint16_t songIdx;
	uint16_t frameIdx;
	Frame currFrame;

	uint16_t tempo;
	
	BuzzerController buzzer;
	CommandQueue commandQueue;
	SongQueue songQueue;
} MusicEngineController;

void MusicEngineController_Init(MusicEngineController* mec);

void MusicEngine_Update(MusicEngineController* mec);

void Handle_Command(MusicEngineController* mec);

void Handle_PlayCommand(MusicEngineController* mec);

void Handle_Command_Pause(MusicEngineController* mec, Command c);
void Handle_Command_Resume(MusicEngineController* mec, Command c);
void Handle_Command_Stop(MusicEngineController* mec, Command c);
void Handle_Command_Skip(MusicEngineController* mec, Command c);
void Handle_Command_Clear(MusicEngineController* mec, Command c);
void Handle_Command_Play(MusicEngineController* mec, Command c);
void Handle_Command_Queue(MusicEngineController* mec, Command c);
void Handle_Command_Tempo(MusicEngineController* mec, Command c);
void Handle_Command_Volume(MusicEngineController* mec, Command c);

int16_t Play_Song(MusicEngineController* mec, uint16_t idx);
int16_t Queue_Song(MusicEngineController* mec, uint16_t idx);
void Stop_Song(MusicEngineController* mec);

void Pause_Song(MusicEngineController* mec);
void Resume_Song(MusicEngineController* mec);
void Skip_Song(MusicEngineController* mec);

void Display_Status(MusicEngineController* mec);
void Display_Songs(MusicEngineController* mec);
void Display_Volume(MusicEngineController* mec);
void Display_Tempo(MusicEngineController* mec);

int16_t Set_Volume(MusicEngineController* mec, uint16_t v);
int16_t Set_Tempo(MusicEngineController* mec, uint16_t t);

#endif /* APP_MUSIC_ENGINE_H */
