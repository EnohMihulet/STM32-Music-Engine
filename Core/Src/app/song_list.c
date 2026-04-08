#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../../Inc/main.h"

#include "app/song.h"
#include "app/song_storage.h"
#include "app/song_list.h"

extern Song SONG_1;
extern Song SONG_2;
extern Song SONG_3;

static int16_t SongList_Grow(SongList* sl) {
	if (sl == NULL) return -1;

	uint16_t newCapacity = sl->songCapacity * 2;
	Song** temp = (Song**)malloc(sizeof(Song*) * newCapacity);
	if (temp == NULL) return -1;

	memcpy(temp, sl->songs, sizeof(Song*) * sl->songCount);
	free(sl->songs);

	sl->songs = temp;
	sl->songCapacity = newCapacity;
	return 0;
}

void SongList_Init(SongList* sl, SongStoreHeader* ssh) {
	if (sl == NULL || ssh == NULL) return;

	uint16_t initialCount = STATIC_SONGS_COUNT + ssh->count;
	sl->songCapacity = initialCount;
	if (sl->songCapacity < SONGLIST_START_CAPACITY) {
		sl->songCapacity = SONGLIST_START_CAPACITY;
	}

	sl->songs = (Song**)malloc(sizeof(Song*) * sl->songCapacity);
	if (sl->songs == NULL) {
		sl->songCount = 0;
		sl->songCapacity = 0;
		return;
	}
	memset(sl->songs, 0, sizeof(Song*) * sl->songCapacity);

	uint16_t loadedCount = 0;
	sl->songs[0] = &SONG_1;
	sl->songs[1] = &SONG_2;
	sl->songs[2] = &SONG_3;
	loadedCount = STATIC_SONGS_COUNT;

	for (uint16_t i = 0; i < ssh->count; i++) {
		Song* s = (Song*)malloc(sizeof(Song));
		if (s == NULL) break;

		Storage_Codes sc = SongStorage_LoadByIdx(ssh, i, s);
		if (sc != APP_OK) {
			free(s);
			continue;
		}

		sl->songs[loadedCount++] = s;
	}

	sl->songCount = loadedCount;
}

int16_t SongList_Add(SongList* sl, SongStoreHeader* ssh, Song* s) {
	if (sl->songCount >= sl->songCapacity) {
		if (SongList_Grow(sl) != APP_OK) return -1;
	}

	if (SongStorage_Save(ssh, s) != APP_OK) return -1;

	sl->songs[sl->songCount++] = s;
	return APP_OK;
}

int16_t SongList_Delete(SongList* sl, SongStoreHeader* ssh, const char* title) {
	uint16_t idx = SongList_Find(sl, title);
	if (idx == sl->songCount) return -1;

	if (idx < STATIC_SONGS_COUNT) return -1;

	uint16_t storageIdx = idx - STATIC_SONGS_COUNT;
	if (SongStorage_DeleteByIdx(ssh, storageIdx) != APP_OK) return -1;
	free(sl->songs[idx]);
	sl->songs[idx] = NULL;

	if (idx == sl->songCount - 1) {
		sl->songCount -= 1;
		return 0;
	}
	for (uint16_t i = idx; i < sl->songCount - 1; i++) {
		sl->songs[i] = sl->songs[i+1];
	}
	sl->songCount -= 1;
	return 0;
}

int16_t SongList_Replace(SongList* sl, SongStoreHeader* ssh, Song* s) {
	uint16_t idx = SongList_Find(sl, s->title);
	if (idx == sl->songCount) {
		return -1;
	}

	if (SongStorage_Save(ssh, s) != APP_OK) {
		return -1;
	}
	memcpy(sl->songs[idx], s, sizeof(Song));
	return APP_OK;
}

bool SongList_Contains(SongList* sl, const char* title) {
	for (uint16_t i = 0; i < sl->songCount; i++) {
		if (strcmp(sl->songs[i]->title, title) == 0) return true;
	}
	return false;
}

uint16_t SongList_Find(SongList* sl, const char* title) {
	for (uint16_t i = 0; i < sl->songCount; i++) {
		if (strcmp(sl->songs[i]->title, title) == 0) {
			return i;
		}
	}
	return sl->songCount;
}

bool SongList_IsSongStatic(SongList* sl, const char* title) {
	uint16_t idx = SongList_Find(sl, title);
	if (idx == sl->songCount) return false;
	return idx < STATIC_SONGS_COUNT;
}
