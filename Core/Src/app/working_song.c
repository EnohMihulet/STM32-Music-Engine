#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/song.h"
#include "app/song_list.h"
#include "app/working_song.h"
#include "app/uart_cli.h"

static WorkingSong_Codes WorkingSong_MapStorageErr(Storage_Codes code) {
	if (code == STORAGE_ERR_NOTFOUND) return WORKINGSONG_ERR_NOTFOUND;
	if (code == STORAGE_ERR_BADINPUT || code == STORAGE_ERR_NULL) return WORKINGSONG_ERR_BADINPUT;
	return WORKINGSONG_ERR_STORAGE;
}

void WorkingSong_Init(WorkingSong* ws) {
	if (ws == NULL) return;
	ws->kind = WorkingSong_None;
	ws->changed = false;
	memset(ws->originalTitle, 0, TITLE_CAPACITY);
	ws->s = NULL;
}

WorkingSong_Codes WorkingSong_NewSong(SongList* sl, SongStoreHeader* ssh, WorkingSong* ws, char* title) {
	if (ssh == NULL || ws == NULL || title == NULL) return WORKINGSONG_ERR_NULL;
	if (ws->s != NULL) return WORKINGSONG_ERR_STATE;
	if (SongTitle_IsTooLong(title)) return WORKINGSONG_ERR_BADINPUT;

	if (SongList_Contains(sl, title)) {
		return WORKINGSONG_ERR_EXISTS;
	}

	ws->kind = WorkingSong_New;
	ws->changed = false;
	memset(ws->originalTitle, 0, TITLE_CAPACITY);
	ws->s = malloc(sizeof(Song));
	if (ws->s == NULL) return WORKINGSONG_ERR_INTERNAL;

	ws->s->isStatic = false;
	ws->s->framesSize = 0;

	if (WorkingSong_SetTitle(ws, title) != APP_OK) {
		free(ws->s);
		ws->kind = WorkingSong_None;
		ws->s = NULL;
		return WORKINGSONG_ERR_INTERNAL;
	}
	return APP_OK;
}

WorkingSong_Codes WorkingSong_EditSong(SongList* sl, SongStoreHeader* ssh, WorkingSong* ws, char* title) {
	if (ssh == NULL || ws == NULL || title == NULL) return WORKINGSONG_ERR_NULL;
	if (ws->s != NULL) return WORKINGSONG_ERR_STATE;
	if (SongTitle_IsTooLong(title)) return WORKINGSONG_ERR_BADINPUT;

	if (!SongList_Contains(sl, title)) {
		return WORKINGSONG_ERR_NOTFOUND;
	}

	if (SongList_IsSongStatic(sl, title)) {
		return WORKINGSONG_ERR_BADINPUT;
	}

	ws->kind = WorkingSong_Edit;
	ws->changed = false;
	ws->s = malloc(sizeof(Song));
	if (ws->s == NULL) {
		ws->kind = WorkingSong_None;
		return WORKINGSONG_ERR_INTERNAL;
	}

	Storage_Codes ss = SongStorage_LoadByTitle(ssh, title, ws->s);
	if (ss != APP_OK) {
		free(ws->s);
		ws->s = NULL;
		ws->kind = WorkingSong_None;
		return WorkingSong_MapStorageErr(ss);
	}

	memset(ws->originalTitle, 0, TITLE_CAPACITY);
	strncpy(ws->originalTitle, ws->s->title, TITLE_CAPACITY - 1);

	return APP_OK;
}

WorkingSong_Codes WorkingSong_CopySong(SongList* sl, SongStoreHeader* ssh, WorkingSong* ws, char* copiedTitle, char* newTitle) {
	if (ssh == NULL || ws == NULL || copiedTitle == NULL || newTitle == NULL) return WORKINGSONG_ERR_NULL;
	if (ws->s != NULL) return WORKINGSONG_ERR_STATE;

	if (SongTitle_IsTooLong(copiedTitle) || SongTitle_IsTooLong(newTitle)) {
		return WORKINGSONG_ERR_BADINPUT;
	}

	if (SongList_Contains(sl, newTitle)) {
		return WORKINGSONG_ERR_EXISTS;
	}

	uint16_t idx = SongList_Find(sl, copiedTitle);
	if (idx == sl->songCount) {
		return WORKINGSONG_ERR_BADINPUT;
	}

	ws->kind = WorkingSong_Copy;
	ws->changed = false;
	memset(ws->originalTitle, 0, TITLE_CAPACITY);
	ws->s = malloc(sizeof(Song));
	if (ws->s == NULL) {
		ws->kind = WorkingSong_None;
		return WORKINGSONG_ERR_INTERNAL;
	}

	memcpy(ws->s, sl->songs[idx], sizeof(Song));
	ws->s->isStatic = false;

	if (WorkingSong_SetTitle(ws, newTitle) != APP_OK) {
		free(ws->s);
		ws->s = NULL;
		ws->kind = WorkingSong_None;
		return WORKINGSONG_ERR_BADINPUT;
	}
	return APP_OK;
}

WorkingSong_Codes WorkingSong_SetTitle(WorkingSong* ws, char* title) {
	if (ws == NULL || ws->s == NULL || title == NULL) return WORKINGSONG_ERR_NULL;
	if (SongTitle_IsTooLong(title)) return WORKINGSONG_ERR_BADINPUT;
	memset(ws->s->title, 0, TITLE_CAPACITY);
	strncpy(ws->s->title, title, TITLE_CAPACITY - 1);
	return APP_OK;
}

WorkingSong_Codes WorkingSong_AddNote(WorkingSong* ws, uint16_t frequencyHz, uint16_t durationMs) {
	if (ws == NULL || ws->s == NULL) return WORKINGSONG_ERR_STATE;
	if (ws->s->framesSize >= FRAMES_CAPACITY) return WORKINGSONG_ERR_CAPACITY;
	ws->s->frames[ws->s->framesSize++] = (Frame){frequencyHz, durationMs};
	return APP_OK;
}

WorkingSong_Codes WorkingSong_EditNote(WorkingSong* ws, uint16_t frameIdx, uint16_t frequencyHz, uint16_t durationMs) {
	if (ws == NULL || ws->s == NULL) return WORKINGSONG_ERR_STATE;
	if (frameIdx >= ws->s->framesSize) return WORKINGSONG_ERR_BADINPUT;
	ws->s->frames[frameIdx] = (Frame){frequencyHz, durationMs};
	return APP_OK;
}

WorkingSong_Codes WorkingSong_List(WorkingSong* ws) {
	if (ws == NULL || ws->s == NULL) return WORKINGSONG_ERR_STATE;
	echo(ws->s->title, strlen(ws->s->title));
	echo("\r\n", 2);
	for (uint16_t i = 0; i < ws->s->framesSize; i++) {
		char buf[64];
		uint16_t len = sprintf(buf, "%d: {%d, %d}\r\n", i, ws->s->frames[i].frequencyHz, ws->s->frames[i].durationMs);
		echo(buf, len);
	}
	return APP_OK;
}
