#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "../../Inc/usart.h"

#include "../../Inc/app/uart_cli.h"
#include "../../Inc/app/music_engine.h"

void MusicEngineController_Init(MusicEngineController* mec, SongList* sl) {
	mec->pbState = Stopped;
	mec->remainingTimeMs = 0;
	mec->updateFrame = false;

	mec->songIdx = 0;
	mec->frameIdx = 0;
	mec->currFrame = (Frame){0,0};

	mec->tempo = 0;

	mec->composingSong = false;
	memset(mec->ws, 0, sizeof(WorkingSong));
	mec->songList = sl;

	Buzzer_Init(&(mec->buzzer));
	SongQueue_Init(&(mec->songQueue));
	CommandQueue_Init(&(mec->commandQueue));
}

void MusicEngine_Update(MusicEngineController* mec) {
	Handle_Command(mec);

	if (mec->updateFrame) {
		if (mec->pbState == Playing) {
			uint16_t nextFrameIdx = mec->frameIdx + 1;
			if (nextFrameIdx >= mec->songList->songs[mec->songIdx]->framesSize) {
				Stop_Song(mec);

				uint16_t songIdx;
				if (SongQueue_Pop(&(mec->songQueue), &songIdx) == -1) return;
			
				Play_Song(mec, songIdx);
				return;
			}

			mec->frameIdx = nextFrameIdx;
			mec->currFrame = mec->songList->songs[mec->songIdx]->frames[mec->frameIdx];
			mec->remainingTimeMs = mec->currFrame.durationMs;

			if (mec->currFrame.durationMs == 0) {
				mec->updateFrame = true;
				return;
			}
			else mec->updateFrame = false;

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

	CommandReturnCode crc = OK;

	switch (c.cc) {
		case Command_None: break;
		case Command_Pause:	crc = Pause_Song(mec); break;
		case Command_Resume:	crc = Resume_Song(mec); break;
		case Command_Stop:	crc = Stop_Song(mec); break;
		case Command_Skip:	crc = Skip_Song(mec); break;
		case Command_Clear:	SongQueue_Clear(&mec->songQueue); break;
		case Command_Play:	crc = Play_Song(mec, (uint16_t)c.u.args1.a1); break;
		case Command_Queue:	crc = Queue_Song(mec, (uint16_t)c.u.args1.a1); break;
		case Command_Songs:	Display_Songs(mec); break;
		case Command_Tempo:	crc = Set_Tempo(mec, c.u.args1.a1); break;
		case Command_Volume:	Buzzer_SetVolume(&mec->buzzer, c.u.args1.a1); break;
		case Command_Status:	Display_Status(mec); break;
		case Command_NewSong:	crc = Handle_Command_NewSong(mec, c); break;
		case Command_AddNote:	crc = Handle_Command_AddNote(mec, c); break;
		case Command_AddRest:	crc = Handle_Command_AddRest(mec, c); break;
		case Command_ListSong:	crc = Handle_Command_ListSong(mec, c); break;
		case Command_PlaySong:	crc = Handle_Command_PlaySong(mec, c); break;
		case Command_ClearSong:	crc = Handle_Command_ClearSong(mec, c); break;
		case Command_Save:	crc = Handle_Command_Save(mec, c); break;
		case Command_Load:	crc = Handle_Command_Load(mec, c); break;
		default: break;
	}

	Handle_Err_Code(crc);
}

void Handle_Err_Code(CommandReturnCode crc) {
	if (crc != OK)
	Print_CommandReturnCode(crc);
}

CommandReturnCode Handle_Command_NewSong(MusicEngineController* mec, Command c) {
	if (mec->composingSong) return ERR_Invalid_State;
	mec->composingSong = true;
	strcpy(mec->ws->s.title, c.u.str);
	return OK;
}

CommandReturnCode Handle_Command_AddNote(MusicEngineController* mec, Command c) {
	if (!mec->composingSong) return ERR_Invalid_State;
	Song* s = &mec->ws->s;
	s->frames[s->framesSize++] = (Frame){c.u.args2.a1, c.u.args2.a2};
	return OK;
}

CommandReturnCode Handle_Command_AddRest(MusicEngineController* mec, Command c) {
	if (!mec->composingSong) return ERR_Invalid_State;
	Song* s = &mec->ws->s;
	s->frames[s->framesSize++] = (Frame){0, c.u.args1.a1};
	return OK;
}

CommandReturnCode Handle_Command_ListSong(MusicEngineController* mec, Command c) {
	echo("HERE 4", 6);
	return OK;
}

CommandReturnCode Handle_Command_PlaySong(MusicEngineController* mec, Command c) {
	echo("HERE 5", 6);
	return OK;
}

CommandReturnCode Handle_Command_ClearSong(MusicEngineController* mec, Command c) {
	if (!mec->composingSong) return ERR_Invalid_State;
	memset(mec->ws->s.frames, 0, mec->ws->s.framesSize);
	return OK;
}

CommandReturnCode Handle_Command_Save(MusicEngineController* mec, Command c) {
	echo("HERE 7", 6);
	// mec->songList->songs[mec->songList->songCount++] = mec->
	return OK;
}

CommandReturnCode Handle_Command_Load(MusicEngineController* mec, Command c) {
	echo("HERE 8", 6);
	return OK;
}

// TODO: Change form idx to song title
CommandReturnCode Play_Song(MusicEngineController* mec, uint16_t idx) {
	if (idx >= mec->songList->songCount) return ERR_BadArgument;

	const Song* song = mec->songList->songs[idx];

	mec->pbState = Playing;
	mec->songIdx = idx;
	mec->frameIdx = 0;
	mec->currFrame = song->frames[0];
	mec->remainingTimeMs = mec->currFrame.durationMs;

	if (mec->currFrame.durationMs == 0) {
		mec->updateFrame = true;
		return OK;
	}
	else mec->updateFrame = false;

	Buzzer_Start(&mec->buzzer, mec->currFrame.frequencyHz);

	return OK;
}

CommandReturnCode Stop_Song(MusicEngineController* mec) {
	if (mec->pbState == Stopped) return ERR_Invalid_State;

	mec->pbState = Stopped;
	mec->remainingTimeMs = 0;
	mec->updateFrame = false;

	mec->songIdx = 0;
	mec->frameIdx = 0;
	mec->currFrame = (Frame){0,0};

	Buzzer_Stop(&mec->buzzer);
	return OK;
}

// TODO: Change form idx to song title
CommandReturnCode Queue_Song(MusicEngineController* mec, uint16_t idx) {
	if (idx >= mec->songList->songCount) return ERR_BadArgument;

	if (mec->pbState == Stopped) return Play_Song(mec, idx);
	else if (SongQueue_Push(&(mec->songQueue), idx) == -1) return ERR_Capacity;

	return OK;
}

CommandReturnCode Pause_Song(MusicEngineController* mec) {
	if (mec->pbState == Paused) return ERR_Invalid_State;
	mec->pbState = Paused;

	Buzzer_Stop(&mec->buzzer);
	return OK;
}

CommandReturnCode Resume_Song(MusicEngineController* mec) {
	if (mec->pbState != Paused) return ERR_Invalid_State;
	mec->pbState = Playing;

	Buzzer_Start(&mec->buzzer, mec->currFrame.frequencyHz);
	return OK;
}

CommandReturnCode Skip_Song(MusicEngineController* mec) {
	if (mec->pbState == Stopped) return ERR_Invalid_State;

	Stop_Song(mec);

	uint16_t songIdx;
	if (SongQueue_Pop(&(mec->songQueue), &songIdx) == -1) {
		mec->pbState = Stopped;
		return OK;
	}

	mec->pbState = Playing;
	return Play_Song(mec, songIdx);
}

void Display_Status(MusicEngineController* mec) {

	char buf[128];
	uint8_t len = 0;

	char* songTitle = (char*)mec->songList->songs[mec->songIdx]->title;

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
				len += sprintf((char*)buf + len, "%s ", mec->songList->songs[songIdx]->title);
			}
			if (qSize > 3) len += sprintf((char*)buf + len, "...\r\n");
			else
                          len += sprintf(len + (char *)buf, "\r\n");
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
	for (uint16_t i = 0; i < mec->songList->songCount; i++) {
		echo((char*)mec->songList->songs[i]->title, strlen(mec->songList->songs[i]->title));
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
