#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "../main.h"

#include "../../Inc/app/song.h"

#define FLASH_SONGSTORAGE_MAGIC 33441230
#define FLASH_SONGSTORAGE_START_ADDR 0x08060000U
#define FLASH_SONGSTORAGE_SONGS_START_ADDR (FLASH_SONGSTORAGE_START_ADDR + sizeof(uint32_t) + sizeof(uint16_t) + (SONGSTORE_CAPACITY_MAX * TITLE_CAPACITY))
#define FLASH_SONGSTORAGE_END_ADDR   0x0807FFFFU

#define FLASH_SONG_BYTES (TITLE_CAPACITY + sizeof(uint16_t) + sizeof(uint8_t) + (FRAMES_CAPACITY * sizeof(Frame)))

typedef struct SongStoreHeader {
	uint32_t magic;
	uint16_t count;
	char songTitles[SONGSTORE_CAPACITY_MAX][TITLE_CAPACITY];
} SongStoreHeader;

typedef enum {
    STORAGE_ERR_NULL = 1,
    STORAGE_ERR_BADINPUT,
    STORAGE_ERR_NOTFOUND,
    STORAGE_ERR_FULL,
    STORAGE_ERR_UNLOCK,
    STORAGE_ERR_ERASE,
    STORAGE_ERR_PROGRAM,
    STORAGE_ERR_LOCK,
} Storage_Codes;

Storage_Codes SongStoreHeader_Init(SongStoreHeader* ssh);
bool SongStorage_Contains(SongStoreHeader* ssh, const char* title);
uint16_t SongStorage_Find(SongStoreHeader* ssh, const char* title);
void SongStorage_List(SongStoreHeader* ssh);

Storage_Codes SongStorage_Save(SongStoreHeader* ssh, Song* s);

Storage_Codes SongStorage_LoadByTitle(SongStoreHeader* ssh, const char* title, Song* out);
Storage_Codes SongStorage_LoadByIdx(SongStoreHeader* ssh, uint16_t idx, Song* out);

Storage_Codes SongStorage_DeleteByTitle(SongStoreHeader* ssh, const char* title);
Storage_Codes SongStorage_DeleteByIdx(SongStoreHeader* ssh, uint16_t idx);

bool EraseFlashSector7();
