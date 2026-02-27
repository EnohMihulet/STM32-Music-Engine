#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../../Inc/app/song.h"

uint16_t WorkingSong_Init(WorkingSong* ws) {
	if (ws == NULL) return -1;
	ws->s.framesSize = 0;
	memset(ws->s.title, 0, TITLE_CAPACITY);
	memset(ws->s.frames, 0, FRAMES_CAPACITY);
	return 0;
}

uint16_t WorkingSong_SetTitle(WorkingSong* ws, char* title) {
	if (strlen(title) >= TITLE_CAPACITY) return -1;
	strcpy(ws->s.title, title);
	return 0;
}

uint16_t WorkingSong_AddNote(WorkingSong* ws, uint16_t frequencyHz, uint16_t durationMs) {
	if (ws == NULL) return -1;
	ws->s.frames[ws->s.framesSize++] = (Frame){frequencyHz, durationMs};
	return 0;
}

uint16_t WorkingSong_List(WorkingSong* ws) {
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

uint16_t SongList_Add(SongList* sl, WorkingSong* ws) {
	if (sl->songCount >= sl->songCapacity) SongList_Grow(sl);

	sl->songs[sl->songCount++] = &ws->s;
	return 0;
}

uint16_t SongList_Grow(SongList* sl) {

	Song** temp = (Song**)malloc(sizeof(SongList*) * (sl->songCapacity * 2));
	memcpy(temp, sl->songs, sl->songCount);
	free(sl->songs);

	sl->songs = temp;
	sl->songCapacity *= 2;
	return 0;
}
