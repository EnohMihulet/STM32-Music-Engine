#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../../Inc/app/uart_cli.h"
#include "../../Inc/app/song.h"


void WorkingSong_Init(WorkingSong* ws) {
	ws->kind = WorkingSong_None;
	ws->idx = -1;
}

int16_t WorkingSong_NewSong(WorkingSong* ws, char* title) {
	if (ws->s != NULL) return -1;
	ws->kind = WorkingSong_New;
	ws->s = malloc(sizeof(Song));
	ws->s->framesSize = 0;
	ws->s->heapAllocated = true;
	return WorkingSong_SetTitle(ws, title);
}

int16_t WorkingSong_EditSong(WorkingSong* ws, Song* s) {
	if (ws->s != NULL || s == NULL) return -1;
	ws->kind = WorkingSong_Edit;
	ws->s = malloc(sizeof(Song));
	ws->s->heapAllocated = true;
	memcpy(ws->s, s, sizeof(Song));
	return 0;
}

int16_t WorkingSong_CopySong(WorkingSong* ws, Song* s, char* title) {
	if (ws->s != NULL || s == NULL) return -1;
	ws->kind = WorkingSong_Copy;
	ws->s = malloc(sizeof(Song));
	ws->s->heapAllocated = true;
	memcpy(ws->s, s, sizeof(Song));
	return WorkingSong_SetTitle(ws, title);
}

int16_t WorkingSong_SetTitle(WorkingSong* ws, char* title) {
	if (ws->s == NULL) return -1;
	if (strlen(title) >= TITLE_CAPACITY) return -1;
	strcpy(ws->s->title, title);
	return 0;
}

int16_t WorkingSong_AddNote(WorkingSong* ws, uint16_t frequencyHz, uint16_t durationMs) {
	if (ws->s == NULL) return -1;
	ws->s->frames[ws->s->framesSize++] = (Frame){frequencyHz, durationMs};
	return 0;
}

int16_t WorkingSong_EditNote(WorkingSong* ws, uint16_t frameIdx, uint16_t frequencyHz, uint16_t durationMs) {
	if (ws->s == NULL) return -1;
	if (frameIdx >= ws->s->framesSize) return -1;
	ws->s->frames[frameIdx] = (Frame){frequencyHz, durationMs};
	return 0;
}

int16_t WorkingSong_List(WorkingSong* ws) {
	if (ws == NULL) return -1;
	echo(ws->s->title, strlen(ws->s->title));
	echo("\r\n", 2);
	for (uint16_t i = 0; i < ws->s->framesSize; i++) {
		char buf[64];
		uint16_t len = sprintf(buf, "%d: {%d, %d}\r\n", i, ws->s->frames[i].frequencyHz, ws->s->frames[i].durationMs);
		echo(buf, len);
	}
	return 0;
}

void SongList_Init(SongList* sl) {
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

	sl->songs[sl->songCount++] = ws->s;
	return 0;
}

int16_t SongList_Delete(SongList* sl, const char* title) {
	uint16_t idx;
	if (SongList_Find(sl, title, &idx) == -1) return -1;

	if (sl->songs[idx]->heapAllocated) free(sl->songs[idx]);
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

int16_t SongList_Replace(SongList* sl, WorkingSong* ws) {
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

bool SongList_Contains(SongList* sl, const char* title) {
	for (uint16_t i = 0; i < sl->songCount; i++) {
		if (strcmp(sl->songs[i]->title, title) == 0) return true;
	}
	return false;
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
