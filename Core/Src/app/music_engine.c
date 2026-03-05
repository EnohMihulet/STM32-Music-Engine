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

	WorkingSong_Init(&mec->ws);
	SongQueue_Init(&mec->songQueue);
	CommandQueue_Init(&mec->commandQueue);

	mec->buzzer = bc;
	mec->songList = sl;
}

void MusicEngine_Update(MusicEngineController* mec, UartCLIController* ucc) {
	Handle_Command(mec, &ucc->responseQueue);

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

void Handle_Command(MusicEngineController* mec, CLIResponseQueue* rq) {
	if (CommandQueue_IsEmpty(&(mec->commandQueue))) return;

	Command c = {Command_None, -1};
	if (CommandQueue_Pop(&(mec->commandQueue), &c) == -1) return;

	int err;
	switch (c.cc) {
		case Command_None:	break;
		case Command_Pause:	err = Handle_Command_Pause(mec, rq, c); break;
		case Command_Resume:	err = Handle_Command_Resume(mec, rq, c); break;
		case Command_Stop:	err = Handle_Command_Stop(mec, rq, c); break;
		case Command_Skip:	err = Handle_Command_Skip(mec, rq, c); break;
		case Command_Clear:	err = Handle_Command_Clear(mec, rq, c); break;
		case Command_Play:	err = Handle_Command_Play(mec, rq, c); break;
		case Command_Queue:	err = Handle_Command_Queue(mec, rq, c); break;
		case Command_Songs:	err = Handle_Command_Songs(mec, rq, c); break;
		case Command_Volume:	err = Handle_Command_Volume(mec, rq, c); break;
		case Command_Status:	err = Handle_Command_Status(mec, rq, c); break;
		case Command_NewSong:	err = Handle_Command_NewSong(mec, rq, c); break;
		case Command_EditSong:	err = Handle_Command_EditSong(mec, rq, c); break;
		case Command_CopySong:	err = Handle_Command_CopySong(mec, rq, c); break;
		case Command_AddNote:	err = Handle_Command_AddNote(mec, rq, c); break;
		case Command_AddRest:	err = Handle_Command_AddRest(mec, rq, c); break;
		case Command_EditNote:	err = Handle_Command_EditNote(mec, rq, c); break;
		case Command_ListSong:	err = Handle_Command_ListSong(mec, rq, c); break;
		case Command_PlaySong:	err = Handle_Command_PlaySong(mec, rq, c); break;
		case Command_ClearSong:	err = Handle_Command_ClearSong(mec, rq, c); break;
		case Command_Save:	err = Handle_Command_Save(mec, rq, c); break;
		case Command_Delete:	err = Handle_Command_Delete(mec, rq, c); break;
		case Command_Quit:	err = Handle_Command_Quit(mec, rq, c); break;
		default: return;
	}

	if (err == 0) {
		CLIResponse_Emit(rq, c.id, RESP_OK, ERR_None, NULL);
	}
}

int Handle_Command_Pause(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->pbState == Stopped) {
		CLIResponse_Emit(rq, c.id, RESP_WARN, ERR_None, "No song playing");
		return -1;
	}
	if (Pause_Song(mec) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_WARN, ERR_None, "Song already paused");
		return -1;
	}
	return 0;
}

int Handle_Command_Resume(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->pbState == Stopped) {
		CLIResponse_Emit(rq, c.id, RESP_WARN, ERR_None, "No song playing");
		return -1;
	}
	if (Resume_Song(mec) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_WARN, ERR_None, "Song already playing");
		return -1;
	}
	return 0;
}

int Handle_Command_Stop(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (Stop_Song(mec) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_WARN, ERR_None, "No song playing");
		return -1;
	}
	return 0;
}

int Handle_Command_Skip(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (Skip_Song(mec) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_WARN, ERR_None, "Nothing to skip");
		return -1;
	}
	return 0;
}

int Handle_Command_Clear(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (SongQueue_IsEmpty(&mec->songQueue)) {
		CLIResponse_Emit(rq, c.id, RESP_WARN, ERR_None, "Song queue already empty");
		return -1;
	}
	else SongQueue_Clear(&mec->songQueue);
	return 0;
}

int Handle_Command_Play(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str1.s, &idx) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "Song titled %s not found", c.u.str1.s);
		return -1;
	}
	
	Play_Song(mec, mec->songList->songs[idx]);

	CLIResponse_Emit(rq, c.id, RESP_OK, ERR_None, NULL);
	CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "Now Playing: %s", c.u.str1.s);
	return -1;
}

int Handle_Command_Queue(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str1.s, &idx) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "Song titled %s not found", c.u.str1.s);
		return -1;
	}

	if (Queue_Song(mec, mec->songList->songs[idx]) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Capacity, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Song queue is full");
		return -1;
	}

	return 0;
}

int Handle_Command_Songs(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	Display_Songs(mec);
	return 0;
}

int Handle_Command_Volume(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (Buzzer_SetVolume(mec->buzzer, c.u.int1.a1) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	return 0;
}

int Handle_Command_Status(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	Display_Status(mec);
	return 0;
}

int Handle_Command_NewSong(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind != WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Already composing a song");
		return -1;
	}

	if (SongList_Contains(mec->songList, c.u.str1.s)) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is already a song", c.u.str1.s);
		return -1;
	}

	if (WorkingSong_NewSong(&mec->ws, c.u.str1.s) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	return 0;
}

int Handle_Command_EditSong(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind != WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Already composing a song");
		return -1;
	}

	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str1.s, &idx) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is not a song", c.u.str1.s);
		return -1;
	}

	Song* s = mec->songList->songs[idx];
	if (WorkingSong_EditSong(&mec->ws, s) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	
	mec->ws.idx = idx;
	return 0;
}

int Handle_Command_CopySong(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind != WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Already composing a song");
		return -1;
	}

	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str2.s1, &idx) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is not a song", c.u.str1.s);
		return -1;
	}
	Song* s = mec->songList->songs[idx];

	if (SongList_Contains(mec->songList, c.u.str2.s2)) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is already a song", c.u.str1.s);
		return -1;
	}

	if (WorkingSong_CopySong(&mec->ws, s, c.u.str2.s2) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	return 0;
}

int Handle_Command_AddNote(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	if (WorkingSong_AddNote(&mec->ws, c.u.int2.a1, c.u.int2.a2) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	return 0;
}

int Handle_Command_AddRest(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	if (WorkingSong_AddNote(&mec->ws, 0, c.u.int1.a1) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	return 0;
}

int Handle_Command_EditNote(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	if (WorkingSong_EditNote(&mec->ws, c.u.int3.a1, c.u.int3.a2, c.u.int3.a3) == -1) {
		char buf[CLI_RESPONSE_MSG_CAPACITY];
		uint16_t frameCount = mec->ws.s->framesSize;
		if (frameCount == 0) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
			snprintf(buf, CLI_RESPONSE_MSG_CAPACITY, "There are no frames to edit");
		}
		else {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_OutOfRange, NULL);
			if (frameCount == 1) snprintf(buf, CLI_RESPONSE_MSG_CAPACITY, "");
			else snprintf(buf, CLI_RESPONSE_MSG_CAPACITY, "Frames valid range: 0-%u", frameCount-1);
		}
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, buf);
		return -1;
	}
	return 0;
}

int Handle_Command_ListSong(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	if (WorkingSong_List(&mec->ws) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	return 0;
}

int Handle_Command_PlaySong(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	if (Play_Song(mec, mec->ws.s) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	return 0;
}

int Handle_Command_ClearSong(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	memset(mec->ws.s->frames, 0, mec->ws.s->framesSize);
	return 0;
}

int Handle_Command_Save(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	size_t songIdx;
	if (mec->ws.kind == WorkingSong_Edit) {
		songIdx = mec->ws.idx;
		if (mec->songList->songs[songIdx]->heapAllocated) free(mec->songList->songs[songIdx]);
	}
	else songIdx = mec->songList->songCount++;

	mec->songList->songs[songIdx] = mec->ws.s;
	mec->ws.s = NULL;
	mec->ws.idx = songIdx;
	if (WorkingSong_EditSong(&mec->ws, mec->songList->songs[songIdx])) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	} 
	return 0;
}

int Handle_Command_Delete(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	uint16_t idx;
	if (SongList_Find(mec->songList, c.u.str1.s, &idx) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is not a song", c.u.str1.s);
		return -1;
	}
	if (mec->ws.idx == idx) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Cannot delete the song you are currently editing");
		return -1;
	}
	if (SongList_Delete(mec->songList, c.u.str1.s) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		return -1;
	}
	return 0;
}

int Handle_Command_Quit(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	free(mec->ws.s);
	mec->ws.s = NULL;
	mec->ws.kind = WorkingSong_None;
	mec->ws.idx = -1;
	return 0;
}

int Play_Song(MusicEngineController* mec, Song* song) {
	if (song == NULL) return -1;

	mec->pbState = Playing;

	mec->currSong = song;
	mec->frameIdx = 0;
	Frame currFrame = song->frames[0];
	mec->remainingTimeMs = currFrame.durationMs;

	if (currFrame.durationMs == 0) {
		mec->updateFrame = true;
		return 0;
	}
	else mec->updateFrame = false;

	Buzzer_Start(mec->buzzer, currFrame.frequencyHz);

	return 0;
}

int Stop_Song(MusicEngineController* mec) {
	if (mec->pbState == Stopped) return -1;

	mec->pbState = Stopped;
	mec->remainingTimeMs = 0;
	mec->updateFrame = false;

	mec->currSong = NULL;
	mec->frameIdx = 0;

	Buzzer_Stop(mec->buzzer);
	return 0;
}

int Queue_Song(MusicEngineController* mec, Song* song) {
	if (song == NULL) return -1;

	if (mec->pbState == Stopped) return Play_Song(mec, song);
	else if (SongQueue_Push(&(mec->songQueue), song) == -1) return -1;

	return 0;
}

int Pause_Song(MusicEngineController* mec) {
	if (mec->pbState == Paused) return -1;
	mec->pbState = Paused;

	Buzzer_Stop(mec->buzzer);
	return 0;
}

int Resume_Song(MusicEngineController* mec) {
	if (mec->pbState != Paused) return -1;
	mec->pbState = Playing;

	Buzzer_Start(mec->buzzer, mec->currSong->frames[mec->frameIdx].frequencyHz);
	return 0;
}

int Skip_Song(MusicEngineController* mec) {
	if (mec->pbState == Stopped) return -1;

	Stop_Song(mec);

	Song* song;
	if (SongQueue_Pop(&(mec->songQueue), &song) == -1) {
		mec->pbState = Stopped;
		return 0;
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
			else len += sprintf(len + (char *)buf, "\r\n");
                }
	}
	else {
		len += sprintf(buf, "Not Playing\r\n");
	}

	len += sprintf((char*)buf + len, "Volume: %u\r\n", mec->buzzer->volumePct);

	echo(buf, len);
	return;
}

void Display_Songs(MusicEngineController* mec) {
	for (uint16_t i = 0; i < mec->songList->songCount; i++) {
		echo((char*)mec->songList->songs[i]->title, strlen(mec->songList->songs[i]->title));
		if (i < mec->songList->songCount - 1) echo(", ", 2);
	}
	echo_newline();
	return;
}

void Display_Volume(MusicEngineController* mec) {
	char buf[32];
	sprintf(buf, "Volume: %u\r\n", mec->buzzer->volumePct);
	echo(buf, strlen(buf));
	return;
}
