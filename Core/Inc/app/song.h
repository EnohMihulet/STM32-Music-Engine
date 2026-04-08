#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define TITLE_CAPACITY 16
#define FRAMES_CAPACITY 80
#define STATIC_SONGS_COUNT 3
#define SONGLIST_START_CAPACITY 6
#define SONGSTORE_CAPACITY_MAX 16

#define APP_OK 0

typedef struct Frame {
	uint16_t frequencyHz;
	uint16_t durationMs;
} Frame;

typedef struct __attribute__((aligned(4))) Song {
	char title[TITLE_CAPACITY];
	uint16_t framesSize;
	bool isStatic;
	Frame frames[FRAMES_CAPACITY];
} Song;

static Song SONG_1 = {
	.title = "MARIO",
	.framesSize = 24,
	.isStatic = true,
	.frames = {
		{659, 250},   // E5
		{659, 250},   // E5
		{659, 500},   // E5
		{523, 250},   // C5
		{659, 500},   // E5
		{784, 1000},  // G5
		{392, 1000},  // G4
		{523, 750},   // C5
		{392, 750},   // G4
		{330, 750},   // E4
		{440, 500},   // A4
		{494, 500},   // B4
		{466, 250},   // A#4
		{440, 500},   // A4
		{392, 375},   // G4
		{659, 375},   // E5
		{784, 375},   // G5
		{880, 500},   // A5
		{698, 250},   // F5
		{784, 500},   // G5
		{659, 500},   // E5
		{523, 250},   // C5
		{587, 250},   // D5
		{494, 1000},  // B4
	},
};

static Song SONG_2 = {
	.title = "ZELDA",
	.framesSize = 26,
	.isStatic = true,
	.frames = {
		{330, 500},   // E4
		{392, 500},   // G4
		{440, 250},   // A4
		{494, 250},   // B4
		{523, 500},   // C5
		{587, 500},   // D5
		{659, 1000},  // E5
		{659, 250},   // E5
		{659, 250},   // E5
		{698, 250},   // F5
		{784, 250},   // G5
		{880, 500},   // A5
		{988, 500},   // B5
		{1047, 1000}, // C6
		{1047, 250},  // C6
		{988, 250},   // B5
		{880, 250},   // A5
		{784, 250},   // G5
		{698, 500},   // F5
		{784, 500},   // G5
		{880, 1000},  // A5
		{659, 250},   // E5
		{587, 250},   // D5
		{523, 500},   // C5
		{494, 500},   // B4
		{440, 1000},  // A4
	},
};

static Song SONG_3 = {
	.title = "TETRIS",
	.framesSize = 30,
	.isStatic = true,
	.frames = {
		{659, 500},   // E5
		{494, 250},   // B4
		{523, 250},   // C5
		{587, 500},   // D5
		{523, 250},   // C5
		{494, 250},   // B4
		{440, 500},   // A4
		{440, 250},   // A4
		{523, 250},   // C5
		{659, 500},   // E5
		{587, 250},   // D5
		{523, 250},   // C5
		{494, 750},   // B4
		{523, 250},   // C5
		{587, 500},   // D5
		{659, 500},   // E5
		{523, 500},   // C5
		{440, 500},   // A4
		{440, 1000},  // A4
		{587, 500},   // D5
		{698, 250},   // F5
		{880, 500},   // A5
		{784, 250},   // G5
		{698, 250},   // F5
		{659, 750},   // E5
		{523, 250},   // C5
		{659, 500},   // E5
		{587, 250},   // D5
		{523, 250},   // C5
		{494, 1000},  // B4
	},
};

static inline bool SongTitle_IsTooLong(const char* title) {
	if (title == NULL) return false;

	for (uint16_t i = 0; i < TITLE_CAPACITY; i++) {
		if (title[i] == '\0') return false;
	}
	return true;
}
