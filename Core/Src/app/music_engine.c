#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../Inc/usart.h"

#include "../../Inc/app/uart_cli.h"
#include "../../Inc/app/music_engine.h"

void MusicEngineController_Init(MusicEngineController* mec, BuzzerController* bc, SongList* sl) {
	mec->pbState = Stopped;
	mec->remainingTimeMs = 0;
	mec->updateFrame = false;

	mec->currSong = NULL;
	mec->frameIdx = 0;

	mec->tempo = 0;

	WorkingSong_Init(&mec->ws);
	SongQueue_Init(&mec->songQueue);
	CommandQueue_Init(&mec->commandQueue);

	mec->buzzer = bc;
	mec->songList = sl;
}

void MusicEngine_Update(MusicEngineController* mec) {
	Handle_Command(mec);

	if (mec->updateFrame) {
		if (mec->pbState == Playing) {
			uint16_t nextFrameIdx = mec->frameIdx + 1;
			if (nextFrameIdx >= mec->currSong->framesSize) {
				Stop_Song(mec);

				Song *song;
				if (SongQueue_Pop(&(mec->songQueue), &song)== -1) return;
			
				Play_Song(mec, song);
				return;
			}

			mec->frameIdx = nextFrameIdx;
			mec->remainingTimeMs = mec->currSong->frames[mec->frameIdx].durationMs;

			if (mec->remainingTimeMs == 0) {
				mec->updateFrame = true;
				return;
			}
			else mec->updateFrame = false;

			Buzzer_Start(mec->buzzer, mec->currSong->frames[mec->frameIdx].frequencyHz);
		}
		else if (mec->pbState == Stopped && !SongQueue_IsEmpty(&mec->songQueue)) {
			Song* song;
			if (SongQueue_Pop(&(mec->songQueue), &song) == -1) return;
		
			Play_Song(mec, song);
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
		case Command_Play:	crc = Handle_Command_Play(mec, c); break;
		case Command_Queue:	crc = Handle_Command_Queue(mec, c); break;
		case Command_Songs:	Display_Songs(mec); break;
		case Command_Tempo:	crc = Set_Tempo(mec, c.u.int1.a1); break;
		case Command_Volume:	Buzzer_SetVolume(mec->buzzer, c.u.int1.a1); break;
		case Command_Status:	Display_Status(mec); break;

		case Command_NewSong:	crc = Handle_Command_NewSong(mec, c); break;
		case Command_EditSong:	crc = Handle_Command_EditSong(mec, c); break;
		case Command_CopySong:	crc = Handle_Command_CopySong(mec, c); break;

		case Command_AddNote:	crc = Handle_Command_AddNote(mec, c); break;
		case Command_AddRest:	crc = Handle_Command_AddRest(mec, c); break;
		case Command_EditNote:	crc = Handle_Command_EditNote(mec, c); break;

		case Command_ListSong:	crc = Handle_Command_ListSong(mec, c); break;
		case Command_PlaySong:	crc = Handle_Command_PlaySong(mec, c); break;
		case Command_ClearSong:	crc = Handle_Command_ClearSong(mec, c); break;
		case Command_Save:	crc = Handle_Command_Save(mec, c); break;
		default: break;
	}

	Handle_Err_Code(crc);
}

void Handle_Err_Code(CommandReturnCode crc) {
	if (crc != OK)
	Print_CommandReturnCode(crc);
}

CommandReturnCode Handle_Command_Play(MusicEngineController* mec, Command c) {
	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str1.s, &idx) == -1) return ERR_BadArgument;
	return Play_Song(mec, mec->songList->songs[idx]);
}

CommandReturnCode Handle_Command_Queue(MusicEngineController* mec, Command c) {
	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str1.s, &idx) == -1) return ERR_BadArgument;
	return Queue_Song(mec, mec->songList->songs[idx]);
}

CommandReturnCode Handle_Command_NewSong(MusicEngineController* mec, Command c) {
	if (mec->ws.kind != WorkingSong_None) return ERR_Invalid_State;
	if (WorkingSong_NewSong(&mec->ws, c.u.str1.s) == -1) return ERR_BadArgument; // TODO: ERr handling
	return OK;
}

CommandReturnCode Handle_Command_EditSong(MusicEngineController* mec, Command c) {
	if (mec->ws.kind != WorkingSong_None) return ERR_Invalid_State;
	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str1.s, &idx) == -1) return ERR_BadArgument;
	Song* s = mec->songList->songs[idx];
	if (WorkingSong_EditSong(&mec->ws, s) == -1) return ERR_BadArgument; // TODO: ERr handling
	mec->ws.idx = idx;
	return OK;
}

CommandReturnCode Handle_Command_CopySong(MusicEngineController* mec, Command c) {
	if (mec->ws.kind != WorkingSong_None) return ERR_Invalid_State;
	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str2.s1, &idx) == -1) return ERR_BadArgument;
	Song* s = mec->songList->songs[idx];
	if (WorkingSong_CopySong(&mec->ws, s, c.u.str2.s2) == -1) return ERR_BadArgument; // TODO: ERr handling
	return OK;
}

CommandReturnCode Handle_Command_AddNote(MusicEngineController* mec, Command c) {
	if (mec->ws.kind == WorkingSong_None) return ERR_Invalid_State;
	if (WorkingSong_AddNote(&mec->ws, c.u.int2.a1, c.u.int2.a2) == -1) return ERR_Internal;
	return OK;
}

CommandReturnCode Handle_Command_AddRest(MusicEngineController* mec, Command c) {
	if (mec->ws.kind == WorkingSong_None) return ERR_Invalid_State;
	if (WorkingSong_AddNote(&mec->ws, 0, c.u.int1.a1) == -1) return ERR_Internal;
	return OK;
}

CommandReturnCode Handle_Command_EditNote(MusicEngineController* mec, Command c) {
	if (mec->ws.kind == WorkingSong_None) return ERR_Invalid_State;
	if (WorkingSong_EditNote(&mec->ws, c.u.int3.a1, c.u.int3.a2, c.u.int3.a3) == -1) return ERR_BadArgument;
	return OK;
}

CommandReturnCode Handle_Command_ListSong(MusicEngineController* mec, Command c) {
	if (mec->ws.kind == WorkingSong_None) return ERR_Invalid_State;
	if (WorkingSong_List(&mec->ws) == -1) return ERR_Internal;
	return OK;
}

CommandReturnCode Handle_Command_PlaySong(MusicEngineController* mec, Command c) {
	return Play_Song(mec, mec->ws.s);
}

CommandReturnCode Handle_Command_ClearSong(MusicEngineController* mec, Command c) {
	if (mec->ws.kind == WorkingSong_None) return ERR_Invalid_State;
	// TODO: Should it stop composing or just clear the frames
	return OK;
}

CommandReturnCode Handle_Command_Save(MusicEngineController* mec, Command c) {
	switch (mec->ws.kind) {
		case WorkingSong_None: return ERR_Invalid_State;
		case WorkingSong_New: case WorkingSong_Copy:
			mec->songList->songs[mec->songList->songCount++] = mec->ws.s;
			mec->ws.s = NULL;
			break;
		case WorkingSong_Edit: break;
	}
	mec->ws.kind = WorkingSong_None;
	mec->ws.idx = 0;
	mec->ws.s = NULL;
	return OK;
}

CommandReturnCode Play_Song(MusicEngineController* mec, Song* song) {
	if (song == NULL) return ERR_BadArgument;

	mec->pbState = Playing;

	mec->currSong = song;
	mec->frameIdx = 0;
	Frame currFrame = song->frames[0];
	mec->remainingTimeMs = currFrame.durationMs;

	if (currFrame.durationMs == 0) {
		mec->updateFrame = true;
		return OK;
	}
	else mec->updateFrame = false;

	Buzzer_Start(mec->buzzer, currFrame.frequencyHz);

	return OK;
}

CommandReturnCode Stop_Song(MusicEngineController* mec) {
	if (mec->pbState == Stopped) return ERR_Invalid_State;

	mec->pbState = Stopped;
	mec->remainingTimeMs = 0;
	mec->updateFrame = false;

	mec->currSong = NULL;
	mec->frameIdx = 0;

	Buzzer_Stop(mec->buzzer);
	return OK;
}

CommandReturnCode Queue_Song(MusicEngineController* mec, Song* song) {
	if (song == NULL) return ERR_BadArgument;

	if (mec->pbState == Stopped) return Play_Song(mec, song);
	else if (SongQueue_Push(&(mec->songQueue), song) == -1) return ERR_Capacity;

	return OK;
}

CommandReturnCode Pause_Song(MusicEngineController* mec) {
	if (mec->pbState == Paused) return ERR_Invalid_State;
	mec->pbState = Paused;

	Buzzer_Stop(mec->buzzer);
	return OK;
}

CommandReturnCode Resume_Song(MusicEngineController* mec) {
	if (mec->pbState != Paused) return ERR_Invalid_State;
	mec->pbState = Playing;

	Buzzer_Start(mec->buzzer, mec->currSong->frames[mec->frameIdx].frequencyHz);
	return OK;
}

CommandReturnCode Skip_Song(MusicEngineController* mec) {
	if (mec->pbState == Stopped) return ERR_Invalid_State;

	Stop_Song(mec);

	Song* song;
	if (SongQueue_Pop(&(mec->songQueue), &song) == -1) {
		mec->pbState = Stopped;
		return OK;
	}

	mec->pbState = Playing;
	return Play_Song(mec, song);
}

void Display_Status(MusicEngineController* mec) {

	char buf[128];
	uint8_t len = 0;

	char* songTitle = (char*)mec->currSong->title;

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
				Song* song = 0;
				SongQueue_At(&mec->songQueue, i, &song);
				len += sprintf((char*)buf + len, "%s ", song->title);
			}
			if (qSize > 3) len += sprintf((char*)buf + len, "...\r\n");
			else
                          len += sprintf(len + (char *)buf, "\r\n");
                }
	}
	else {
		len += sprintf(buf, "Not Playing\r\n");
	}

	len += sprintf((char*)buf + len, "Volume: %u ", mec->buzzer->volumePct);
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
	sprintf(buf, "Volume: %u\r\n", mec->buzzer->volumePct);
	echo(buf, strlen(buf));
	return;
}

void Display_Tempo(MusicEngineController* mec) {
	char buf[32];
	sprintf(buf, "Tempo: %u\r\n", mec->buzzer->volumePct);
	echo(buf, strlen(buf));
	return;
}

int16_t Set_Tempo(MusicEngineController* mec, uint16_t t) {
	if (t < 40 || t > 200) return -1;
	mec->tempo = t;
	return 0;
}
