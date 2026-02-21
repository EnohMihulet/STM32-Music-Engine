#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "../../Inc/usart.h"

#include "../../Inc/app/uart_cli.h"
#include "../../Inc/app/music_engine.h"


void MusicEngineController_Init(MusicEngineController* mec) {
	mec->pbState = Stopped;

	mec->songIdx = SONG_COUNT;
	mec->frameIdx = 0;
	mec->currFrame = (Frame){0,0};
	mec->remainingTimeMs = 0;

	mec->tempo = 0;

	Buzzer_Init(&(mec->buzzer));
	SongQueue_Init(&(mec->songQueue));
	CommandQueue_Init(&(mec->commandQueue));
}

void MusicEngine_Update(MusicEngineController* mec) {
	Handle_Command(mec);

	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	uint16_t remainingTimeMs = mec->remainingTimeMs;
	__set_PRIMASK(primask);


	if (remainingTimeMs == 0) {
		if (mec->frameIdx == SONGS[mec->songIdx].framesSize) {
			Stop_Song(mec);

			uint16_t songIdx;
			if (SongQueue_Pop(&(mec->songQueue), &songIdx) == -1) return;
		
			Play_Song(mec, songIdx);
			return;
		}
		
		if (mec->pbState == Playing) { 
			mec->frameIdx += 1;
			mec->currFrame = SONGS[mec->songIdx].frames[mec->frameIdx];

			uint32_t primask = __get_PRIMASK();
			__disable_irq();
			mec->remainingTimeMs = mec->currFrame.durationMs;
			__set_PRIMASK(primask);

			Buzzer_Start(&mec->buzzer, mec->currFrame.frequencyHz);
		}
		else if (mec->pbState == Stopped && !SongQueue_IsEmpty(&mec->songQueue)) {
			uint16_t songIdx;
			if (SongQueue_Pop(&(mec->songQueue), &songIdx) == -1) return;
		
			Play_Song(mec, songIdx);
			return;
		}
	}
}

void Handle_Command(MusicEngineController* mec) {
	if (CommandQueue_IsEmpty(&(mec->commandQueue))) return;

	Command c = {Command_None, -1};
	if (CommandQueue_Pop(&(mec->commandQueue), &c) == -1) return;

	switch (c.cc) {
		case Command_None: break;
		case Command_Pause: Handle_Command_Pause(mec, c); break;
		case Command_Resume: Handle_Command_Resume(mec, c); break;
		case Command_Stop: Handle_Command_Stop(mec, c); break;
		case Command_Skip: Handle_Command_Skip(mec, c); break;
		case Command_Clear: Handle_Command_Clear(mec, c); break;
		case Command_Play: Handle_Command_Play(mec, c); break;
		case Command_Queue: Handle_Command_Queue(mec, c); break;
		case Command_Songs: Display_Songs(mec); break;
		case Command_Tempo: Handle_Command_Tempo(mec, c); break;
		case Command_Volume: Handle_Command_Volume(mec, c); break;
		case Command_Status: Display_Status(mec); break;
		default: break;
	}
}

void Handle_Command_Pause(MusicEngineController* mec, Command c) {
	Pause_Song(mec);
}

void Handle_Command_Resume(MusicEngineController* mec, Command c) {
	Resume_Song(mec);
}

void Handle_Command_Stop(MusicEngineController* mec, Command c) {
	Stop_Song(mec);
}

void Handle_Command_Skip(MusicEngineController* mec, Command c) {
	Skip_Song(mec);
}

void Handle_Command_Clear(MusicEngineController* mec, Command c) {
	SongQueue_Clear(&(mec->songQueue));
}

void Handle_Command_Play(MusicEngineController* mec, Command c) {
	if (c.arg == -1)
		Display_Songs(mec);
	else
		Play_Song(mec, c.arg);
}

void Handle_Command_Queue(MusicEngineController* mec, Command c) {
	if (c.arg == -1)
		Display_Songs(mec);
	else
		Queue_Song(mec, c.arg);}

void Handle_Command_Tempo(MusicEngineController* mec, Command c) {
	mec->tempo = c.arg;
}

void Handle_Command_Volume(MusicEngineController* mec, Command c) {
	if (c.arg < 0 || c.arg > 100) return;
	Buzzer_SetVolume(&mec->buzzer, c.arg);
}

int16_t Play_Song(MusicEngineController* mec, uint16_t idx) {
	if (idx >= SONG_COUNT) return -1;

	const Song* song = &(SONGS[idx]);

	mec->pbState = Playing;
	mec->songIdx = idx;
	mec->frameIdx = 0;
	mec->currFrame = song->frames[0];

	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	mec->remainingTimeMs = mec->currFrame.durationMs;
	__set_PRIMASK(primask);

	Buzzer_Start(&mec->buzzer, mec->currFrame.frequencyHz);

	return 0;
}

void Stop_Song(MusicEngineController* mec) {
	mec->pbState = Stopped;

	mec->songIdx = SONG_COUNT;
	mec->frameIdx = 0;
	mec->currFrame = (Frame){0,0};
	mec->remainingTimeMs = 0;

	Buzzer_Stop(&mec->buzzer);
}

int16_t Queue_Song(MusicEngineController* mec, uint16_t idx) {
	if (idx >= SONG_COUNT) return -1;

	if (SongQueue_Push(&(mec->songQueue), idx) == -1) return -1;

	return 0;
}

void Pause_Song(MusicEngineController* mec) {
	if (mec->pbState == Paused) return;
	mec->pbState = Paused;
	Buzzer_Stop(&mec->buzzer);
}

void Resume_Song(MusicEngineController* mec) {
	if (mec->pbState != Paused) return;
	mec->pbState = Playing;

	Buzzer_Start(&mec->buzzer, mec->currFrame.frequencyHz);
}


void Skip_Song(MusicEngineController* mec) {
	if (mec->pbState == Stopped) return;

	Stop_Song(mec);

	uint16_t songIdx;
	if (SongQueue_Pop(&(mec->songQueue), &songIdx) == -1) {
		mec->pbState = Stopped;
		return;
	}

	mec->pbState = Playing;
	Play_Song(mec, songIdx);
}

void Display_Status(MusicEngineController* mec) {
	// Playing: Song Title
	// In Queue: song 1, song 2, song 3, ...
	// Volume: 10%, Tempo: 120
	char buf[128];
	uint8_t len = 0;

	char* songTitle = (char*)SONGS[mec->songIdx].title;

	if (mec->pbState == Playing || mec->pbState == Paused) {
		if (mec->pbState == Playing)
			len += sprintf(buf, "Playing: %s\r\n", songTitle);
		else if (mec->pbState == Paused)
			len += sprintf(buf, "Paused: %s\r\n", songTitle);

		uint16_t qSize = mec->songQueue.size;
		if (qSize != 0) {
			uint16_t songCount = qSize > 3 ? 3 : qSize;
			len += sprintf((char*)buf + len, "In Queue: ");
			for (uint16_t i = 0; i < songCount; i++) {
				uint16_t songIdx = 0;
				SongQueue_At(&mec->songQueue, i, &songIdx);
				len += sprintf((char*)buf + len, "%s ", SONGS[songIdx].title);
			}
			if (qSize > 3) len += sprintf((char*)buf + len, "...\r\n");
			else len += sprintf((char*)buf + len, "\r\n");
		}
	}
	else {
		len += sprintf(buf, "Not Playing\r\n");
	}

	len += sprintf((char*)buf + len, "Volume: %u ", mec->buzzer.volumePct);
	len += sprintf((char*)buf + len, "Tempo: %u\r\n", mec->tempo);

	echo(buf, len + 1);
	return;
}

void Display_Songs(MusicEngineController* mec) {
	for (uint16_t i = 0; i < SONG_COUNT; i++) {
		echo((char*)SONGS[i].title, strlen(SONGS[i].title));
		echo(", ", 2);
	}
	echo("\r\n", 2);
	return;
}

void Display_Volume(MusicEngineController* mec) {
	char buf[32];
	sprintf(buf, "Volume: %u\r\n", mec->buzzer.volumePct);
	echo(buf, strlen(buf));
	return;
}

void Display_Tempo(MusicEngineController* mec) {
	char buf[32];
	sprintf(buf, "Tempo: %u\r\n", mec->buzzer.volumePct);
	echo(buf, strlen(buf));
	return;
}

int16_t Set_Tempo(MusicEngineController* mec, uint16_t t) {
	if (t < 40 || t > 200) return -1;
	mec->tempo = t;
	return 0;
}
