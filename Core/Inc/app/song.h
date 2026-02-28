#pragma once
#include <stdint.h>

#define TITLE_CAPACITY 16
#define FRAMES_CAPACITY 80
#define START_SONG_COUNT 3
#define SONGLIST_START_CAPACITY 6

typedef struct Frame {
	uint16_t frequencyHz;
	uint16_t durationMs;
} Frame;


typedef struct Song {
	char title[TITLE_CAPACITY];
	uint16_t framesSize;
	Frame frames[FRAMES_CAPACITY];
} Song;

typedef enum WorkingSong_Kind {
	WorkingSong_None, WorkingSong_New, WorkingSong_Edit, WorkingSong_Copy
} WorkingSong_Kind;

typedef struct WorkingSong {
	WorkingSong_Kind kind;
	int16_t idx;
	Song* s;
} WorkingSong;

typedef struct SongList {
	uint16_t songCount;
	uint16_t songCapacity; Song** songs;
} SongList;

static Song SONG_1 = {
	.title = "SONG1",
	.framesSize = 3,
	.frames = {{988, 1000}, {0, 1000}, {988, 1000}}
};

static Song SONG_2 = {
	.title = "SONG2",
	.framesSize = 1,
	.frames = {{659, 1000}}
};

static Song SONG_3 = {
	.title = "SONG3",
	.framesSize = 1,
	.frames = {{392, 10000}}
};

void WorkingSong_Init(WorkingSong* ws);

int16_t WorkingSong_NewSong(WorkingSong* ws, char* title);
int16_t WorkingSong_EditSong(WorkingSong* ws, Song* s);
int16_t WorkingSong_CopySong(WorkingSong* ws, Song* s, char* title);

int16_t WorkingSong_SetTitle(WorkingSong* ws, char* title);
int16_t WorkingSong_AddNote(WorkingSong* ws, uint16_t frequencyHz, uint16_t durationMs);
int16_t WorkingSong_EditNote(WorkingSong* ws, uint16_t frameIdx, uint16_t frequencyHz, uint16_t durationMs);

int16_t WorkingSong_List(WorkingSong* ws);

void SongList_Init(SongList* sl);

int16_t SongList_Add(SongList* sl, WorkingSong* ws);

int16_t SongList_Grow(SongList* sl);

int16_t SongList_Find(SongList* sl, const char* title, uint16_t* idx);
