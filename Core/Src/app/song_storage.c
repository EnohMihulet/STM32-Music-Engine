#include <string.h>

#include "app/song.h"
#include "app/song_storage.h"
#include "app/uart_cli.h"

static Song g_songCache[SONGSTORE_CAPACITY_MAX];

Storage_Codes SongStore_ReadROM_SongAt(uint16_t idx, Song* out) {
	if (out == NULL) return STORAGE_ERR_NULL;
	if (idx >= SONGSTORE_CAPACITY_MAX) return STORAGE_ERR_BADINPUT;

	uint8_t* addr = (uint8_t*)FLASH_SONGSTORAGE_SONGS_START_ADDR;
	addr += FLASH_SONG_BYTES * idx;

	memcpy(out->title, addr, TITLE_CAPACITY);
	addr += TITLE_CAPACITY;

	memcpy(&out->framesSize, addr, sizeof(uint16_t));
	addr += sizeof(uint16_t);

	memcpy(&out->isStatic, addr, sizeof(uint8_t));
	addr += sizeof(uint8_t);

	memcpy(out->frames, addr, FRAMES_CAPACITY * sizeof(Frame));
	return APP_OK;
}

Storage_Codes SongStore_WriteROM_Magic(uint32_t newMagic) {
	HAL_StatusTypeDef hal = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_SONGSTORAGE_START_ADDR, newMagic);
	if (hal != HAL_OK) return STORAGE_ERR_PROGRAM;
	return APP_OK;
}

Storage_Codes SongStore_WriteROM_Count(uint16_t newCount) {
	uint32_t addr = FLASH_SONGSTORAGE_START_ADDR + sizeof(uint32_t);
	HAL_StatusTypeDef hal = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, newCount);
	if (hal != HAL_OK) return STORAGE_ERR_PROGRAM;
	return APP_OK;
}

Storage_Codes SongStore_WriteROM_SongTitles(char songs[][TITLE_CAPACITY], uint16_t count) {
	if (songs == NULL) return STORAGE_ERR_NULL;
	if (count > SONGSTORE_CAPACITY_MAX) return STORAGE_ERR_BADINPUT;

	HAL_StatusTypeDef hal;
	uint8_t* addr = (uint8_t*)FLASH_SONGSTORAGE_START_ADDR + sizeof(uint32_t) + sizeof(uint16_t);

	for (uint16_t i = 0; i < count; i++) {
		for (uint16_t j = 0; j < TITLE_CAPACITY; j++) {
			hal = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)addr, songs[i][j]);
			addr += 1;
			if (hal != HAL_OK) return STORAGE_ERR_PROGRAM;
		}
	}
	return APP_OK;
}

Storage_Codes SongStore_WriteROM_Song(Song* s, uint16_t idx) {
	if (s == NULL) return STORAGE_ERR_NULL;
	if (idx >= SONGSTORE_CAPACITY_MAX) return STORAGE_ERR_BADINPUT;

	HAL_StatusTypeDef hal;
	uint8_t* addr = (uint8_t*)FLASH_SONGSTORAGE_SONGS_START_ADDR + (FLASH_SONG_BYTES * idx);

	for (uint16_t i = 0; i < TITLE_CAPACITY; i++) {
		hal = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)addr, s->title[i]);
		addr += 1;
		if (hal != HAL_OK) return STORAGE_ERR_PROGRAM;
	}

	uint8_t* fs = (uint8_t*)&s->framesSize;
	for (uint16_t i = 0; i < sizeof(uint16_t); i++) {
		hal = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)addr, fs[i]);
		addr += 1;
		if (hal != HAL_OK) return STORAGE_ERR_PROGRAM;
	}

	hal = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)addr, s->isStatic);
	addr += sizeof(uint8_t);
	if (hal != HAL_OK) return STORAGE_ERR_PROGRAM;

	uint8_t* f = (uint8_t*)s->frames;
	for (uint16_t i = 0; i < (FRAMES_CAPACITY * sizeof(Frame)); i++) {
		hal = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)addr, f[i]);
		addr += 1;
		if (hal != HAL_OK) return STORAGE_ERR_PROGRAM;
	}

	return APP_OK;
}

Storage_Codes SongStoreHeader_Init(SongStoreHeader* ssh) {
	if (ssh == NULL) return STORAGE_ERR_NULL;

	uint32_t magic = *((uint32_t*)FLASH_SONGSTORAGE_START_ADDR);
	if (magic != FLASH_SONGSTORAGE_MAGIC) {
		ssh->magic = FLASH_SONGSTORAGE_MAGIC;
		ssh->count = 0;
		memset(ssh->songTitles, 0, SONGSTORE_CAPACITY_MAX * TITLE_CAPACITY);

		HAL_StatusTypeDef hal = HAL_FLASH_Unlock();
		if (hal != HAL_OK) return STORAGE_ERR_UNLOCK;

		FLASH_EraseInitTypeDef eraseInit = {
			.TypeErase = FLASH_TYPEERASE_SECTORS,
			.Sector = FLASH_SECTOR_7,
			.NbSectors = 1,
			.VoltageRange = FLASH_VOLTAGE_RANGE_3,
		};
		uint32_t sectorErr = 0;

		hal = HAL_FLASHEx_Erase(&eraseInit, &sectorErr);
		if (hal != HAL_OK) {
			HAL_FLASH_Lock();
			return STORAGE_ERR_ERASE;
		}

		Storage_Codes ss = SongStore_WriteROM_Magic(FLASH_SONGSTORAGE_MAGIC);
		if (ss != APP_OK) {
			HAL_FLASH_Lock();
			return ss;
		}

		ss = SongStore_WriteROM_Count(0);
		if (ss != APP_OK) {
			HAL_FLASH_Lock();
			return ss;
		}

		ss = SongStore_WriteROM_SongTitles(ssh->songTitles, SONGSTORE_CAPACITY_MAX);
		if (ss != APP_OK) {
			HAL_FLASH_Lock();
			return ss;
		}

		hal = HAL_FLASH_Lock();
		if (hal != HAL_OK) return STORAGE_ERR_LOCK;
		return APP_OK;
	}

	ssh->magic = FLASH_SONGSTORAGE_MAGIC;
	ssh->count = *((uint16_t*)(FLASH_SONGSTORAGE_START_ADDR + sizeof(uint32_t)));
	if (ssh->count > SONGSTORE_CAPACITY_MAX) {
		ssh->count = 0;
		return STORAGE_ERR_BADINPUT;
	}

	uint8_t* addr = (uint8_t*)(FLASH_SONGSTORAGE_START_ADDR + sizeof(uint32_t) + sizeof(uint16_t));
	for (uint16_t titleNum = 0; titleNum < ssh->count; titleNum++) {
		for (uint16_t charNum = 0; charNum < TITLE_CAPACITY; charNum++) {
			ssh->songTitles[titleNum][charNum] = *addr;
			addr += 1;
		}
	}

	for (uint16_t titleNum = ssh->count; titleNum < SONGSTORE_CAPACITY_MAX; titleNum++) {
		memset(ssh->songTitles[titleNum], 0, TITLE_CAPACITY);
	}

	return APP_OK;
}

bool SongStorage_Contains(SongStoreHeader* ssh, const char* title) {
	if (ssh == NULL || title == NULL) return false;
	if (SongTitle_IsTooLong(title)) return false;

	for (uint16_t i = 0; i < ssh->count; i++) {
		if (strncmp(ssh->songTitles[i], title, TITLE_CAPACITY) == 0) return true;
	}
	return false;
}

uint16_t SongStorage_Find(SongStoreHeader* ssh, const char* title) {
	if (ssh == NULL || title == NULL) return SONGSTORE_CAPACITY_MAX;
	if (SongTitle_IsTooLong(title)) return ssh->count;

	for (uint16_t i = 0; i < ssh->count; i++) {
		if (strncmp(ssh->songTitles[i], title, TITLE_CAPACITY) == 0) return i;
	}
	return ssh->count;
}

void SongStorage_List(SongStoreHeader* ssh) {
	if (ssh == NULL) return;

	for (uint16_t i = 0; i < ssh->count; i++) {
		echos(ssh->songTitles[i]);
		echo_newline();
	}
}

static Storage_Codes SongStorage_SaveTo(SongStoreHeader* ssh, Song* s, uint16_t idx, bool isNewSong) {
	if (ssh == NULL || s == NULL) return STORAGE_ERR_NULL;
	if (ssh->count > SONGSTORE_CAPACITY_MAX) return STORAGE_ERR_BADINPUT;
	if (idx >= ssh->count) return STORAGE_ERR_BADINPUT;

	uint16_t readCount = isNewSong ? (ssh->count - 1) : ssh->count;
	if (readCount > SONGSTORE_CAPACITY_MAX) return STORAGE_ERR_BADINPUT;
	for (uint16_t i = 0; i < readCount; i++) {
		Storage_Codes readStatus = SongStore_ReadROM_SongAt(i, &g_songCache[i]);
		if (readStatus != APP_OK) return readStatus;
	}

	HAL_StatusTypeDef hal = HAL_FLASH_Unlock();
	if (hal != HAL_OK) return STORAGE_ERR_UNLOCK;

	FLASH_EraseInitTypeDef eraseInit = {
		.TypeErase = FLASH_TYPEERASE_SECTORS,
		.Sector = FLASH_SECTOR_7,
		.NbSectors = 1,
		.VoltageRange = FLASH_VOLTAGE_RANGE_3,
	};
	uint32_t sectorErr = 0;

	hal = HAL_FLASHEx_Erase(&eraseInit, &sectorErr);
	if (hal != HAL_OK) {
		HAL_FLASH_Lock();
		return STORAGE_ERR_ERASE;
	}

	Storage_Codes ss = SongStore_WriteROM_Magic(FLASH_SONGSTORAGE_MAGIC);
	if (ss != APP_OK) {
		HAL_FLASH_Lock();
		return ss;
	}

	ss = SongStore_WriteROM_Count(ssh->count);
	if (ss != APP_OK) {
		HAL_FLASH_Lock();
		return ss;
	}

	ss = SongStore_WriteROM_SongTitles(ssh->songTitles, SONGSTORE_CAPACITY_MAX);
	if (ss != APP_OK) {
		HAL_FLASH_Lock();
		return ss;
	}

	for (uint16_t i = 0; i < ssh->count; i++) {
		if (i == idx) ss = SongStore_WriteROM_Song(s, i);
		else ss = SongStore_WriteROM_Song(&g_songCache[i], i);

		if (ss != APP_OK) {
			HAL_FLASH_Lock();
			return ss;
		}
	}

	hal = HAL_FLASH_Lock();
	if (hal != HAL_OK) return STORAGE_ERR_LOCK;

	return APP_OK;
}

static Storage_Codes SongStorage_SaveNew(SongStoreHeader* ssh, Song* s) {
	if (ssh == NULL || s == NULL) return STORAGE_ERR_NULL;
	if (ssh->count >= SONGSTORE_CAPACITY_MAX) return STORAGE_ERR_FULL;

	uint16_t idx = ssh->count;
	memset(ssh->songTitles[idx], 0, TITLE_CAPACITY);
	strncpy(ssh->songTitles[idx], s->title, TITLE_CAPACITY - 1);
	ssh->count += 1;

	Storage_Codes ss = SongStorage_SaveTo(ssh, s, idx, true);
	if (ss != APP_OK) {
		ssh->count -= 1;
		memset(ssh->songTitles[ssh->count], 0, TITLE_CAPACITY);
	}

	return ss;
}

static Storage_Codes SongStorage_SaveExisting(SongStoreHeader* ssh, Song* s, uint16_t idx) {
	return SongStorage_SaveTo(ssh, s, idx, false);
}

Storage_Codes SongStorage_Save(SongStoreHeader* ssh, Song* s) {
	if (ssh == NULL || s == NULL) return STORAGE_ERR_NULL;
	if (ssh->count > SONGSTORE_CAPACITY_MAX) return STORAGE_ERR_BADINPUT;
	if (SongTitle_IsTooLong(s->title)) return STORAGE_ERR_BADINPUT;

	uint16_t idx = SongStorage_Find(ssh, s->title);
	if (idx == ssh->count) {
		if (ssh->count >= SONGSTORE_CAPACITY_MAX) return STORAGE_ERR_FULL;
		return SongStorage_SaveNew(ssh, s);
	}
	else return SongStorage_SaveExisting(ssh, s, idx);
}

Storage_Codes SongStorage_LoadByTitle(SongStoreHeader* ssh, const char* title, Song* out) {
	if (ssh == NULL || title == NULL || out == NULL) return STORAGE_ERR_NULL;
	if (SongTitle_IsTooLong(title)) return STORAGE_ERR_BADINPUT;

	uint16_t idx = SongStorage_Find(ssh, title);
	if (idx == ssh->count) return STORAGE_ERR_NOTFOUND;

	return SongStore_ReadROM_SongAt(idx, out);
}

Storage_Codes SongStorage_LoadByIdx(SongStoreHeader* ssh, uint16_t idx, Song* out) {
	if (ssh == NULL || out == NULL) return STORAGE_ERR_NULL;
	if (idx >= ssh->count) return STORAGE_ERR_BADINPUT;

	return SongStore_ReadROM_SongAt(idx, out);
}

Storage_Codes SongStorage_DeleteByTitle(SongStoreHeader* ssh, const char* title) {
	if (ssh == NULL || title == NULL) return STORAGE_ERR_NULL;
	if (SongTitle_IsTooLong(title)) return STORAGE_ERR_BADINPUT;

	uint16_t idx = SongStorage_Find(ssh, title);
	if (idx == ssh->count) return STORAGE_ERR_NOTFOUND;

	return SongStorage_DeleteByIdx(ssh, idx);
}

Storage_Codes SongStorage_DeleteByIdx(SongStoreHeader* ssh, uint16_t idx) {
	if (ssh == NULL) return STORAGE_ERR_NULL;
	if (idx >= ssh->count) return STORAGE_ERR_BADINPUT;

	uint16_t oldCount = ssh->count;
	uint16_t newCount = oldCount - 1;

	uint16_t out = 0;
	for (uint16_t i = 0; i < oldCount; i++) {
		if (i == idx) continue;
		Storage_Codes readStatus = SongStore_ReadROM_SongAt(i, &g_songCache[out]);
		if (readStatus != APP_OK) return readStatus;
		out += 1;
	}

	for (uint16_t i = idx; i + 1 < oldCount; i++) {
		memcpy(ssh->songTitles[i], ssh->songTitles[i + 1], TITLE_CAPACITY);
	}
	memset(ssh->songTitles[newCount], 0, TITLE_CAPACITY);
	ssh->count = newCount;

	HAL_StatusTypeDef hal = HAL_FLASH_Unlock();
	if (hal != HAL_OK) return STORAGE_ERR_UNLOCK;

	FLASH_EraseInitTypeDef eraseInit = {
		.TypeErase = FLASH_TYPEERASE_SECTORS,
		.Sector = FLASH_SECTOR_7,
		.NbSectors = 1,
		.VoltageRange = FLASH_VOLTAGE_RANGE_3,
	};
	uint32_t sectorErr = 0;

	hal = HAL_FLASHEx_Erase(&eraseInit, &sectorErr);
	if (hal != HAL_OK) {
		HAL_FLASH_Lock();
		return STORAGE_ERR_ERASE;
	}

	Storage_Codes ss = SongStore_WriteROM_Magic(FLASH_SONGSTORAGE_MAGIC);
	if (ss != APP_OK) {
		HAL_FLASH_Lock();
		return ss;
	}

	ss = SongStore_WriteROM_Count(ssh->count);
	if (ss != APP_OK) {
		HAL_FLASH_Lock();
		return ss;
	}

	ss = SongStore_WriteROM_SongTitles(ssh->songTitles, SONGSTORE_CAPACITY_MAX);
	if (ss != APP_OK) {
		HAL_FLASH_Lock();
		return ss;
	}

	for (uint16_t i = 0; i < ssh->count; i++) {
		ss = SongStore_WriteROM_Song(&g_songCache[i], i);
		if (ss != APP_OK) {
			HAL_FLASH_Lock();
			return ss;
		}
	}

	hal = HAL_FLASH_Lock();
	if (hal != HAL_OK) return STORAGE_ERR_LOCK;

	return APP_OK;
}

bool EraseFlashSector7() {
	HAL_SuspendTick();
	__disable_irq();

	HAL_StatusTypeDef hal = HAL_FLASH_Unlock();
	if (hal != HAL_OK) {
		__enable_irq();
		HAL_ResumeTick();
		return false;
	}

	FLASH_EraseInitTypeDef eraseInit = {
		.TypeErase	= FLASH_TYPEERASE_SECTORS,
		.Sector	   = FLASH_SECTOR_7,
		.NbSectors	= 1,
		.VoltageRange = FLASH_VOLTAGE_RANGE_3,
	};

	uint32_t sectorErr = 0xFFFFFFFF;
	hal = HAL_FLASHEx_Erase(&eraseInit, &sectorErr);

	HAL_FLASH_Lock();

	__enable_irq();
	HAL_ResumeTick();

	return (hal == HAL_OK);
}
