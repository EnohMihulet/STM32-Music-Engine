#ifndef APP_MUSIC_ENGINE_H
#define APP_MUSIC_ENGINE_H
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "song.h"
#include "buzzer.h"
#include "queue.h"
#include "uart_cli.h"

#define SONG_QUEUE_CAPACITY 16


QUEUE_DECLARE(SongQueue, uint16_t, SONG_QUEUE_CAPACITY)

typedef enum PlaybackState {Stopped, Playing, Paused} PlaybackState;

typedef struct MusicEngineController {
	volatile PlaybackState pbState;
	volatile uint16_t remainingTimeMs;
	volatile bool updateFrame;

	uint16_t songIdx;
	uint16_t frameIdx;
	Frame currFrame;

	uint16_t tempo;

	bool composingSong;
	WorkingSong* ws;
	
	CommandQueue commandQueue;
	SongQueue songQueue;
	BuzzerController buzzer;
	SongList* songList;
} MusicEngineController;

void MusicEngineController_Init(MusicEngineController* mec, SongList* sl);

void MusicEngine_Update(MusicEngineController* mec);

void Handle_Command(MusicEngineController* mec);

CommandReturnCode Handle_Command_NewSong(MusicEngineController* mec, Command c);
CommandReturnCode Handle_Command_AddNote(MusicEngineController* mec, Command c);
CommandReturnCode Handle_Command_AddRest(MusicEngineController* mec, Command c);
CommandReturnCode Handle_Command_ListSong(MusicEngineController* mec, Command c);
CommandReturnCode Handle_Command_PlaySong(MusicEngineController* mec, Command c);
CommandReturnCode Handle_Command_ClearSong(MusicEngineController* mec, Command c);
CommandReturnCode Handle_Command_Save(MusicEngineController* mec, Command c);
CommandReturnCode Handle_Command_Load(MusicEngineController* mec, Command c);

void Handle_Err_Code(CommandReturnCode crc);

CommandReturnCode Play_Song(MusicEngineController* mec, uint16_t idx);
CommandReturnCode Queue_Song(MusicEngineController* mec, uint16_t idx);
CommandReturnCode Stop_Song(MusicEngineController* mec);

CommandReturnCode Pause_Song(MusicEngineController* mec);
CommandReturnCode Resume_Song(MusicEngineController* mec);
CommandReturnCode Skip_Song(MusicEngineController* mec);

void Display_Status(MusicEngineController* mec);
void Display_Songs(MusicEngineController* mec);
void Display_Volume(MusicEngineController* mec);
void Display_Tempo(MusicEngineController* mec);

int16_t Set_Volume(MusicEngineController* mec, uint16_t v);
int16_t Set_Tempo(MusicEngineController* mec, uint16_t t);

#endif
