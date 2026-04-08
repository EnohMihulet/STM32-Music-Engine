#ifndef APP_MUSIC_ENGINE_H
#define APP_MUSIC_ENGINE_H
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "song.h"
#include "buzzer.h"
#include "queue.h"
#include "uart_cli.h"
#include "working_song.h"

#define SONG_QUEUE_CAPACITY 16

QUEUE_DECLARE(SongQueue, Song*, SONG_QUEUE_CAPACITY)

typedef enum PlaybackState {Stopped, Playing, Paused} PlaybackState;

typedef struct MusicEngineController {
	volatile PlaybackState pbState;
	volatile uint16_t remainingTimeMs;
	volatile bool updateFrame;

	Song* currSong;
	uint16_t frameIdx;

	WorkingSong ws;
	CommandQueue commandQueue;
	SongQueue songQueue;

	BuzzerController* buzzer;
	SongList* songList;
	SongStoreHeader* songStoreHeader;
} MusicEngineController;

void MusicEngineController_Init(MusicEngineController* mec, BuzzerController* bc, SongList* sl, SongStoreHeader* ssh);

void MusicEngine_Update(MusicEngineController* mec, UartCLIController* ucc);

void Handle_Command(MusicEngineController* mec, UartCLIController* ucc);

int Handle_Command_Pause(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Resume(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Stop(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Skip(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Clear(MusicEngineController* mec, UartCLIController* ucc, Command c);
int Handle_Command_Play(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Queue(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Songs(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Volume(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Status(MusicEngineController* mec, CLIResponseQueue* rq, Command c);

int Handle_Command_NewSong(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_EditSong(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_CopySong(MusicEngineController* mec, CLIResponseQueue* rq, Command c);

int Handle_Command_AddNote(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_AddRest(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_EditNote(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_EditTitle(MusicEngineController* mec, CLIResponseQueue* rq, Command c);

int Handle_Command_ListSong(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_PlaySong(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_ClearSong(MusicEngineController* mec, UartCLIController* ucc, Command c);
int Handle_Command_Save(MusicEngineController* mec, CLIResponseQueue* rq, Command c);
int Handle_Command_Delete(MusicEngineController* mec, UartCLIController* ucc, Command c);
int Handle_Command_Quit(MusicEngineController* mec, CLIResponseQueue* rq, Command c);

int Play_Song(MusicEngineController* mec, Song* song);
int Queue_Song(MusicEngineController* mec, Song* song);
int Stop_Song(MusicEngineController* mec);

int Pause_Song(MusicEngineController* mec);
int Resume_Song(MusicEngineController* mec);
int Skip_Song(MusicEngineController* mec);

void Display_Status(MusicEngineController* mec);
void Display_Songs(MusicEngineController* mec);
void Display_Volume(MusicEngineController* mec);

#endif
