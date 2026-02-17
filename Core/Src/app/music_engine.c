#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../Inc/app/music_engine.h"

void SongQueue_Init(SongQueue* q) {
	q->size = 0;
	memset(q->buffer, 0, SONG_QUEUE_CAPACITY);
}

int16_t SongQueue_Push(SongQueue* q, uint16_t s) {
	if (SongQueue_IsFull(q)) return -1;

	q->buffer[q->size++] = s;
	return 0;
}

int16_t SongQueue_Pop(SongQueue* q, uint16_t* s) {
	if (SongQueue_IsEmpty(q)) return -1;

	*s = q->buffer[0];
	q->size -= 1;

	if (SongQueue_IsEmpty(q)) return 0;

	memcpy(q->buffer, q->buffer + 1, q->size);
	
	return 0;
}

int16_t SongQueue_Front(SongQueue* q, uint16_t* s) {
	if (SongQueue_IsEmpty(q)) return -1;

	*s = q->buffer[0];

	return 0;
}

bool SongQueue_IsFull(SongQueue* q) {
	return q->size == SONG_QUEUE_CAPACITY;
}

bool SongQueue_IsEmpty(SongQueue* q) {
	return q->size == 0;
}

void SongQueue_Clear(SongQueue* q) {
	q->size = 0;
	memset(q->buffer, 0, SONG_QUEUE_CAPACITY);
}

void MusicEngineController_Init(MusicEngineController* mec) {
	mec->playbackControl = CommandPlaybackControlNone;
	mec->playRequest = CommandPlayRequestNone;
	mec->commandSettings = CommandSettingsNone;

	mec->isPlaying = false;;
	mec->isPaused = false;

	mec->songIdx = 0;
	mec->frameIdx = 0;

	mec->number = -1;

	mec->volume = 100;
	mec->tempo = 120;

	BuzzerController_Init(&(mec->buzzer));
	SongQueue_Init(&(mec->queue));
}

void MusicEngine_Update(MusicEngineController* mec) {
	if (mec->playbackControl == CommandPlaybackControlNone && mec->playRequest == CommandPlayRequestNone && mec->commandSettings == CommandSettingsNone) return;

	Handle_PlayCommand(mec, mec->playRequest);
	Handle_PlaybackCommand(mec, mec->playbackControl);
	Handle_SettingsCommand(mec, mec->commandSettings);
	
	if (!mec->isPlaying || mec->isPaused) return;
	if (!mec->buzzer.isPlaying) {
		mec->isPlaying = false;
		return;
	}
	if (mec->buzzer.needNextFrame) {
		const Song* s = &(SONGS[mec->songIdx]);
		const uint16_t nextFrameIdx = mec->frameIdx + 1;
		if (nextFrameIdx == s->framesSize) return;

		mec->frameIdx = nextFrameIdx;
		mec->buzzer.nextFrame = s->frames[nextFrameIdx];
		mec->buzzer.needNextFrame = false;
	}
}

void Handle_PlayCommand(MusicEngineController* mec, CommandPlayRequest pr) {
	switch (mec->playRequest) {
		case CommandPlayRequestNone: break;
		case CommandPlayRequestPlay:
			if (mec->number == -1) {
				Display_Songs(mec);
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else if (1 <= mec->number && mec->number <= 3) {
				Play_Song(mec, mec->number - 1);
			}
			break;
		case CommandPlayRequestQueue:
			if (mec->number == -1) {
				Display_Songs(mec);
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else if (1 <= mec->number && mec->number <= 3) {
				Queue_Song(mec, mec->number - 1);
			}
			break;
		case CommandPlayRequestPlaySeq:
		default: break;
	}
	mec->playRequest = CommandPlayRequestNone;
}

void Handle_PlaybackCommand(MusicEngineController* mec, CommandPlaybackControl pbc) {
	switch (mec->playbackControl) {
		case CommandPlaybackControlNone: break;
		case CommandPlaybackControlPause:
			Pause_Song(mec);
			break;
		case CommandPlaybackControlResume:
			Resume_Song(mec);
			break;
		case CommandPlaybackControlStop:
			Stop_Song(mec);
			break;
		case CommandPlaybackControlSkip:
			Skip_Song(mec);
			break;
		case CommandPlaybackControlClear:
			SongQueue_Clear(&(mec->queue));
			break;
		default: break;
	}
	mec->playbackControl = CommandPlaybackControlNone;
}

void Handle_SettingsCommand(MusicEngineController* mec, CommandSettings s) {
	switch (mec->commandSettings) {
		case CommandSettingsNone: break;
		case CommandSettingsStatus:
			Display_Status(mec);
			break;
		case CommandSettingsTempo:
			if (mec->number == -1) {
				Display_Tempo(mec);
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else {
				Set_Tempo(mec, mec->number);
			}
			break;
		case CommandSettingsVol:
			if (mec->number == -1) {
				Display_Volume(mec);
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else {
				Set_Volume(mec, mec->number);
			}
			break;
		default: break;
	}
	mec->commandSettings = CommandSettingsNone;
}

int16_t Play_Song(MusicEngineController* mec, uint16_t idx) {
	if (idx >= SONG_COUNT) return -1;

	const Song* song = &(SONGS[idx]);

	mec->songIdx = idx;
	mec->frameIdx = 0;
	mec->isPlaying= true;
	mec->isPaused = false;

	mec->buzzer.isPlaying = true;
	mec->buzzer.nextFrame = song->frames[0];
	mec->buzzer.needNextFrame = false;

	return 0;
}

void Stop_Song(MusicEngineController* mec) {
	mec->songIdx = SONG_COUNT;
	mec->frameIdx = 0;
	mec->isPlaying= false;
	mec->isPaused = false;

	BuzzerController_Init(&(mec->buzzer));
	Duty_Update(&(mec->buzzer), mec->volume);
}

int16_t Queue_Song(MusicEngineController* mec, uint16_t idx) {
	if (idx >= SONG_COUNT) return -1;

	if (SongQueue_Push(&(mec->queue), idx) == -1) return -1;

	return 0;
}

void Pause_Song(MusicEngineController* mec) {
	if (mec->isPaused) return;
	mec->isPaused = true;
	Buzzer_Off(&(mec->buzzer));
}

void Resume_Song(MusicEngineController* mec) {
	if (!mec->isPaused) return;
	mec->isPaused = false;

	uint16_t durationMs = Get_Duration(&(mec->buzzer));
	uint16_t freqHz = Get_Frequency(&(mec->buzzer));
	if (freqHz == 0 || durationMs == 0) return;
	Buzzer_On(&(mec->buzzer));
}


void Skip_Song(MusicEngineController* mec) {
	if (!mec->isPlaying) return;

	mec->isPaused = false;
	uint16_t songIdx;
	if (SongQueue_Pop(&(mec->queue), &songIdx) == -1) {
		Stop_Song(mec);
		return;
	}

	Play_Song(mec, songIdx);
}

// TODO:
void Display_Status(MusicEngineController* mec) {
	return;
}

void Display_Songs(MusicEngineController* mec) {
	return;
}

void Display_Volume(MusicEngineController* mec) {
	return;
}

void Display_Tempo(MusicEngineController* mec) {
	return;
}

int16_t Set_Volume(MusicEngineController* mec, uint16_t v) {
	if (v < 0 || v > 100) return -1;
	mec->volume = v;
	Duty_Update(&(mec->buzzer), v);
	return 0;
}

int16_t Set_Tempo(MusicEngineController* mec, uint16_t t) {
	if (t < 40 || t > 200) return -1;
	mec->tempo = t;
	return 0;
}
