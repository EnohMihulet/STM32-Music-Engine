#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../Inc/usart.h"

#include "../../Inc/app/uart_cli.h"
#include "../../Inc/app/music_engine.h"


void MusicEngineController_Init(MusicEngineController* mec) {
	mec->isPlaying = false;;
	mec->isPaused = false;

	mec->songIdx = 0;
	mec->frameIdx = 0;

	mec->volume = 100;
	mec->tempo = 120;

	BuzzerController_Init(&(mec->buzzer));
	SongQueue_Init(&(mec->songQueue));
	CommandQueue_Init(&(mec->commandQueue));
}

void MusicEngine_Update(MusicEngineController* mec) {
	Handle_Command(mec);
	
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
		case Command_Status: Handle_Command_Status(mec,c ); break;
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
	
}

void Handle_Command_Volume(MusicEngineController* mec, Command c) {

}

void Handle_Command_Status(MusicEngineController* mec, Command c) {

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

	if (SongQueue_Push(&(mec->songQueue), idx) == -1) return -1;

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
	if (SongQueue_Pop(&(mec->songQueue), &songIdx) == -1) {
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
	for (uint16_t i = 0; i < SONG_COUNT; i++) {
		echo((char*)SONGS[i].title, strlen(SONGS[i].title));
		echo(", ", 2);
	}
	echo("\r\n", 2);
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
