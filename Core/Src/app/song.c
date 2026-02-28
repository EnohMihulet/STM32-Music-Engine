#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../../Inc/app/song.h"

void WorkingSong_Init(WorkingSong** ws) {
	*ws = malloc(sizeof(WorkingSong));
	assert((*ws));
	(*ws)->s.framesSize = 0;
}

int16_t WorkingSong_SetTitle(WorkingSong* ws, char* title) {
	if (strlen(title) >= TITLE_CAPACITY) return -1;
	strcpy(ws->s.title, title);
	return 0;
}

int16_t WorkingSong_AddNote(WorkingSong* ws, uint16_t frequencyHz, uint16_t durationMs) {
	if (ws == NULL) return -1;
	ws->s.frames[ws->s.framesSize++] = (Frame){frequencyHz, durationMs};
	return 0;
}

int16_t WorkingSong_List(WorkingSong* ws) {
	return 0;
}

void SongList_Init(SongList* sl) {
	if (sl == NULL) return;
	sl->songs = (Song**)malloc(sizeof(SongList*) * SONGLIST_START_CAPACITY);
	assert(sl->songs);
	sl->songs[0] = &SONG_1;
	sl->songs[1] = &SONG_2;
	sl->songs[2] = &SONG_3;
	sl->songCount = START_SONG_COUNT;
	sl->songCapacity = SONGLIST_START_CAPACITY;
}

int16_t SongList_Add(SongList* sl, WorkingSong* ws) {
	if (sl->songCount >= sl->songCapacity) SongList_Grow(sl);

	sl->songs[sl->songCount++] = &ws->s;
	return 0;
}

int16_t SongList_Grow(SongList* sl) {

	Song** temp = (Song**)malloc(sizeof(SongList*) * (sl->songCapacity * 2));
	memcpy(temp, sl->songs, sl->songCount);
	free(sl->songs);

	sl->songs = temp;
	sl->songCapacity *= 2;
	return 0;
}

int16_t SongList_Find(SongList* sl, const char* title, uint16_t* idx) {
	for (uint16_t i = 0; i < sl->songCount; i++) {
		if (strcmp(sl->songs[i]->title, title) == 0) {
			*idx = i;
			return 0;
		}
	}
	return -1;
}
