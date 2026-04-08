#pragma once
#include "song_list.h"
#include "song_storage.h"

typedef enum WorkingSong_Kind {
	WorkingSong_None, WorkingSong_New, WorkingSong_Edit, WorkingSong_Copy
} WorkingSong_Kind;

typedef enum {
	WORKINGSONG_ERR_NULL = 1,
	WORKINGSONG_ERR_BADINPUT,
	WORKINGSONG_ERR_NOTFOUND,
	WORKINGSONG_ERR_EXISTS,
	WORKINGSONG_ERR_INTERNAL,
	WORKINGSONG_ERR_STATE,
	WORKINGSONG_ERR_CAPACITY,
	WORKINGSONG_ERR_STORAGE,
} WorkingSong_Codes;

typedef struct WorkingSong {
	WorkingSong_Kind kind;
	bool changed;
	char originalTitle[TITLE_CAPACITY];
	Song* s;
} WorkingSong;

void WorkingSong_Init(WorkingSong* ws);

WorkingSong_Codes WorkingSong_NewSong(SongList* sl, SongStoreHeader* ssh, WorkingSong* ws, char* title);
WorkingSong_Codes WorkingSong_EditSong(SongList* sl, SongStoreHeader* ssh, WorkingSong* ws, char* title);
WorkingSong_Codes WorkingSong_CopySong(SongList* sl, SongStoreHeader* ssh, WorkingSong* ws, char* copiedTitle, char* newTitle);

WorkingSong_Codes WorkingSong_SetTitle(WorkingSong* ws, char* title);
WorkingSong_Codes WorkingSong_AddNote(WorkingSong* ws, uint16_t frequencyHz, uint16_t durationMs);
WorkingSong_Codes WorkingSong_EditNote(WorkingSong* ws, uint16_t frameIdx, uint16_t frequencyHz, uint16_t durationMs);

WorkingSong_Codes WorkingSong_List(WorkingSong* ws);
