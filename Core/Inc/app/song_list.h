#pragma once
#include "song.h"
#include "song_storage.h"

typedef struct SongList {
	uint16_t songCount;
	uint16_t songCapacity;
	Song** songs;
} SongList;

void SongList_Init(SongList* sl, SongStoreHeader* ssh);

int16_t SongList_Add(SongList* sl, SongStoreHeader* ssh, Song* s);
int16_t SongList_Delete(SongList* sl, SongStoreHeader* ssh, const char* title);
int16_t SongList_Replace(SongList* sl, SongStoreHeader* ssh, Song* s);

bool SongList_Contains(SongList* sl, const char* title);
uint16_t SongList_Find(SongList* sl, const char* title);
bool SongList_IsSongStatic(SongList* sl, const char* title);
