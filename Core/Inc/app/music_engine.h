#ifndef APP_MUSIC_ENGINE_H
#define APP_MUSIC_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#include "buzzer.h"

#define TITLE_MAX_CAPACITY 16
#define FRAMES_MAX_CAPACITY 80

#define SONG_COUNT 3
#define SONG_QUEUE_CAPACITY 16


typedef enum { CommandPlaybackControlNone=0, CommandPlaybackControlPause, CommandPlaybackControlResume, CommandPlaybackControlStop, CommandPlaybackControlSkip, CommandPlaybackControlClear } CommandPlaybackControl;
typedef enum { CommandPlayRequestNone=0, CommandPlayRequestPlay, CommandPlayRequestQueue, CommandPlayRequestPlaySeq } CommandPlayRequest;
typedef enum { CommandSettingsNone=0, CommandSettingsTempo, CommandSettingsVol, CommandSettingsStatus } CommandSettings;


typedef struct Song {
	char title[TITLE_MAX_CAPACITY];
	uint16_t framesSize;
	Frame frames[FRAMES_MAX_CAPACITY];
} Song;

static const Song SONG_1 = {
	.title = "SONG1",
	.framesSize = 1,
	.frames = {{988, 15000}}
};

static const Song SONG_2 = {
	.title = "SONG2",
	.framesSize = 1,
	.frames = {{659, 15000}}
};

static const Song SONG_3 = {
	.title = "SONG3",
	.framesSize = 1,
	.frames = {{392, 15000}}
};

static const Song SONGS[SONG_COUNT] = {SONG_1, SONG_2, SONG_3};

typedef struct SongQueue {
	uint16_t size;
	uint16_t buffer[SONG_QUEUE_CAPACITY];
} SongQueue;

typedef struct MusicEngineController {
	CommandPlaybackControl playbackControl;
	CommandPlayRequest playRequest;
	CommandSettings commandSettings;

	bool isPlaying;
	bool isPaused;

	uint16_t songIdx;
	int16_t frameIdx;

	int16_t number;

	uint16_t volume;
	uint16_t tempo;
	
	BuzzerController buzzer;
	SongQueue queue;
} MusicEngineController;

void SongQueue_Init(SongQueue* q);

int16_t SongQueue_Push(SongQueue* q, uint16_t s);
int16_t SongQueue_Pop(SongQueue* q, uint16_t* s);
int16_t SongQueue_Front(SongQueue* q, uint16_t* s);
bool SongQueue_IsFull(SongQueue* q);
bool SongQueue_IsEmpty(SongQueue* q);
void SongQueue_Clear(SongQueue* q);

void MusicEngineController_Init(MusicEngineController* mec);

void MusicEngine_Update(MusicEngineController* mec);

void Handle_PlayCommand(MusicEngineController* mec, CommandPlayRequest pr);
void Handle_PlaybackCommand(MusicEngineController* mec, CommandPlaybackControl pbc);
void Handle_SettingsCommand(MusicEngineController* mec, CommandSettings s);

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
