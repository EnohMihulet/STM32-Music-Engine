#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../../Inc/app/song.h"

uint16_t WorkingSong_Init(WorkingSong* ws) {
	ws = malloc(sizeof(WorkingSong));
	if (ws == NULL) return -1;
	ws->s.framesSize = FRAMES_CAPACITY;
	memset(ws->s.title, 0, TITLE_CAPACITY);
	memset(ws->s.frames, 0, FRAMES_CAPACITY);
	return 0;
}

void WorkingSong_SetTitle(WorkingSong* ws, char* title) {
	if (strlen(title) >= TITLE_CAPACITY) return;
	strcpy(ws->s.title, title);
}

void WorkingSong_AddNote(WorkingSong* ws, uint16_t frequencyHz, uint16_t durationMs) {
	ws->s.frames[ws->s.framesSize++] = (Frame){frequencyHz, durationMs};
}

void WorkingSong_List(WorkingSong* ws) {

}

void SongList_Init(SongList* sl) {
	sl->songs = (Song**)malloc(sizeof(SongList*) * SONGLIST_START_CAPACITY);
	if (sl->songs == NULL) return;
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
