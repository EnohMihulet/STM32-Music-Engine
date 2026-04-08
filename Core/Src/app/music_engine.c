#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../Inc/usart.h"

#include "app/song_storage.h"
#include "app/uart_cli.h"
#include "app/song.h"
#include "app/working_song.h"
#include "app/music_engine.h"

static bool Frame_IsPlayable(const Frame* f) {
	if (f == NULL) return false;
	if (f->durationMs == 0) return true;
	if (f->frequencyHz == 0) return true;
	if (f->frequencyHz < FREQUENCY_MIN_HZ || f->frequencyHz > FREQUENCY_MAX_HZ) return false;
	return true;
}

void MusicEngineController_Init(MusicEngineController* mec, BuzzerController* bc, SongList* sl, SongStoreHeader* ssh) {
	mec->pbState = Stopped;
	mec->remainingTimeMs = 0;
	mec->updateFrame = false;

	mec->currSong = NULL;
	mec->frameIdx = 0;

	WorkingSong_Init(&mec->ws);
	SongQueue_Init(&mec->songQueue);
	CommandQueue_Init(&mec->commandQueue);

	mec->songList = sl;
	mec->songStoreHeader = ssh;

	mec->buzzer = bc;
}

void MusicEngine_Update(MusicEngineController* mec, UartCLIController* ucc) {
	Handle_Command(mec, ucc);

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
			if (!Frame_IsPlayable(&mec->currSong->frames[mec->frameIdx])) {
				Stop_Song(mec);
				return;
			}
			mec->remainingTimeMs = mec->currSong->frames[mec->frameIdx].durationMs;

			if (mec->remainingTimeMs == 0) {
				mec->updateFrame = true;
				return;
			}
			else mec->updateFrame = false;

			if (Buzzer_Start(mec->buzzer, mec->currSong->frames[mec->frameIdx].frequencyHz) == -1) {
				Stop_Song(mec);
				return;
			}
		}
		else if (mec->pbState == Stopped && !SongQueue_IsEmpty(&mec->songQueue)) {
			Song* song;
			if (SongQueue_Pop(&(mec->songQueue), &song) == -1) return;
			
			Play_Song(mec, song);

			return;
		}
	}
}

void Handle_Command(MusicEngineController* mec, UartCLIController* ucc) {
	if (CommandQueue_IsEmpty(&(mec->commandQueue))) return;

	Command c = {Command_None, -1};
	if (CommandQueue_Pop(&(mec->commandQueue), &c) == -1) return;

	int err = 0;
	CLIResponseQueue* rq = &ucc->responseQueue;
	switch (c.cc) {
		case Command_None:	break;
		case Command_Pause:	err = Handle_Command_Pause(mec, rq, c); break;
		case Command_Resume:	err = Handle_Command_Resume(mec, rq, c); break;
		case Command_Stop:	err = Handle_Command_Stop(mec, rq, c); break;
		case Command_Skip:	err = Handle_Command_Skip(mec, rq, c); break;
		case Command_Clear:	err = Handle_Command_Clear(mec, ucc, c); break;
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
		case Command_EditTitle:	err = Handle_Command_EditTitle(mec, rq, c); break;
		case Command_ListSong:	err = Handle_Command_ListSong(mec, rq, c); break;
		case Command_PlaySong:	err = Handle_Command_PlaySong(mec, rq, c); break;
		case Command_ClearSong:	err = Handle_Command_ClearSong(mec, ucc, c); break;
		case Command_Save:	err = Handle_Command_Save(mec, rq, c); break;
		case Command_Delete:	err = Handle_Command_Delete(mec, ucc, c); break;
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

int Handle_Command_Clear(MusicEngineController* mec, UartCLIController* ucc, Command c) {
	if (SongQueue_IsEmpty(&mec->songQueue)) {
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_WARN, ERR_None, "Song queue already empty");
		return -1;
	}
	else if (!c.confirmed) {
		ucc->confirmationPending = true;
		ucc->needsConfirmed = c;
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_WARN, ERR_None, "This cannot be undone. Are you sure? Y/N");
		return -1;
	} 
	SongQueue_Clear(&mec->songQueue);
	return 0;
}

int Handle_Command_Play(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	uint16_t idx = SongList_Find(mec->songList, c.u.str1.s);
	if (idx == mec->songList->songCount) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is not a song", c.u.str1.s);
		return -1;
	}

	if (Play_Song(mec, mec->songList->songs[idx]) == -1) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Song cannot be played");
		return -1;
	}

	CLIResponse_Emit(rq, c.id, RESP_OK, ERR_None, NULL);
	CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "Now Playing: %s", c.u.str1.s);
	return -1;
}

int Handle_Command_Queue(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	uint16_t idx = SongList_Find(mec->songList, c.u.str1.s);
	if (idx == mec->songList->songCount) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is not a song", c.u.str1.s);
		return -1;
	}

	bool wasStopped = (mec->pbState == Stopped);

	if (Queue_Song(mec, mec->songList->songs[idx]) == -1) {
		if (wasStopped) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
			CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Song cannot be played");
		}
		else {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Capacity, NULL);
			CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Song queue is full");
		}
		return -1;
	}

	if (wasStopped) {
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "Now Playing: %s", c.u.str1.s);
	}
	else {
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "Queued: %s", c.u.str1.s);
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

	WorkingSong_Codes ws = WorkingSong_NewSong(mec->songList, mec->songStoreHeader, &mec->ws, c.u.str1.s);
	if (ws == APP_OK) return 0;

	if (ws == WORKINGSONG_ERR_EXISTS) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is already a song", c.u.str1.s);
		return -1;
	}

	if (ws == WORKINGSONG_ERR_BADINPUT) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Invalid song title");
		return -1;
	}

	if (ws == WORKINGSONG_ERR_STATE) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Already composing a song");
		return -1;
	}

	CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
	return -1;
}

int Handle_Command_EditSong(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind != WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Already composing a song");
		return -1;
	}

	WorkingSong_Codes ws = WorkingSong_EditSong(mec->songList, mec->songStoreHeader, &mec->ws, c.u.str1.s);
	if (ws == APP_OK) return 0;

	if (ws == WORKINGSONG_ERR_NOTFOUND) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is not a song", c.u.str1.s);
		return -1;
	}

	if (ws == WORKINGSONG_ERR_BADINPUT) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Invalid song title");
		return -1;
	}

	if (ws == WORKINGSONG_ERR_STATE) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Already composing a song");
		return -1;
	}

	CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
	return -1;
}

int Handle_Command_CopySong(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind != WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Already composing a song");
		return -1;
	}

	WorkingSong_Codes ws = WorkingSong_CopySong(mec->songList, mec->songStoreHeader, &mec->ws, c.u.str2.s1, c.u.str2.s2);
	if (ws == APP_OK) return 0;

	if (ws == WORKINGSONG_ERR_EXISTS) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is already a song", c.u.str2.s2);
		return -1;
	}

	if (ws == WORKINGSONG_ERR_BADINPUT) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Invalid song title");
		return -1;
	}

	if (ws == WORKINGSONG_ERR_STATE) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Already composing a song");
		return -1;
	}

	CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
	return -1;
}

int Handle_Command_AddNote(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	WorkingSong_Codes ws = WorkingSong_AddNote(&mec->ws, c.u.int2.a1, c.u.int2.a2);
	if (ws == APP_OK) return 0;

	if (ws == WORKINGSONG_ERR_CAPACITY) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Capacity, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "Max frames: %u", FRAMES_CAPACITY);
		return -1;
	}

	if (ws == WORKINGSONG_ERR_STATE) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
	return -1;
}

int Handle_Command_AddRest(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	WorkingSong_Codes ws = WorkingSong_AddNote(&mec->ws, 0, c.u.int1.a1);
	if (ws == APP_OK) return 0;

	if (ws == WORKINGSONG_ERR_CAPACITY) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Capacity, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "Max frames: %u", FRAMES_CAPACITY);
		return -1;
	}

	if (ws == WORKINGSONG_ERR_STATE) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
	return -1;
}

int Handle_Command_EditNote(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	WorkingSong_Codes ws = WorkingSong_EditNote(&mec->ws, c.u.int3.a1, c.u.int3.a2, c.u.int3.a3);
	if (ws != APP_OK) {
		char buf[CLI_RESPONSE_MSG_CAPACITY];
		uint16_t frameCount = mec->ws.s == NULL ? 0 : mec->ws.s->framesSize;
		if (ws == WORKINGSONG_ERR_STATE || frameCount == 0) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
			snprintf(buf, CLI_RESPONSE_MSG_CAPACITY, "There are no frames to edit");
		}
		else {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_OutOfRange, NULL);
			if (frameCount == 1) snprintf(buf, CLI_RESPONSE_MSG_CAPACITY, "Frames valid range: 0");
			else snprintf(buf, CLI_RESPONSE_MSG_CAPACITY, "Frames valid range: 0-%u", frameCount-1);
		}
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, buf);
		return -1;
	}
	return 0;
}

int Handle_Command_EditTitle(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None || mec->ws.s == NULL) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	if (strcmp(mec->ws.s->title, c.u.str1.s) != 0 && SongList_Contains(mec->songList, c.u.str1.s)) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is already a song", c.u.str1.s);
		return -1;
	}

	if (WorkingSong_SetTitle(&mec->ws, c.u.str1.s) != APP_OK) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Invalid song title");
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

	WorkingSong_Codes ws = WorkingSong_List(&mec->ws);
	if (ws != APP_OK) {
		if (ws == WORKINGSONG_ERR_STATE) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
			CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		}
		else {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
		}
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
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Song cannot be played");
		return -1;
	}
	return 0;
}

int Handle_Command_ClearSong(MusicEngineController* mec, UartCLIController* ucc, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	if (!c.confirmed) {
		ucc->confirmationPending = true;
		ucc->needsConfirmed = c;
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_WARN, ERR_None, "This cannot be undone. Are you sure? Y/N");
		return -1;
	}

	memset(mec->ws.s->frames, 0, mec->ws.s->framesSize * sizeof(Frame));
	mec->ws.s->framesSize = 0;
	return 0;
}

int Handle_Command_Save(MusicEngineController* mec, CLIResponseQueue* rq, Command c) {
	if (mec->ws.kind == WorkingSong_None) {
		CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
		CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Must be composing a song");
		return -1;
	}

	if (mec->ws.kind == WorkingSong_Edit && mec->ws.originalTitle[0] != '\0'
		&& strncmp(mec->ws.originalTitle, mec->ws.s->title, TITLE_CAPACITY) != 0) {
		if (SongList_Contains(mec->songList, mec->ws.s->title)) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_BadArgument, NULL);
			CLIResponse_Emitf(rq, c.id, RESP_INFO, ERR_None, "%s is already a song", mec->ws.s->title);
			return -1;
		}

		if (SongList_Delete(mec->songList, mec->songStoreHeader, mec->ws.originalTitle) != APP_OK) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
			CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Failed to save the song");
			return -1;
		}

		if (SongList_Add(mec->songList, mec->songStoreHeader, mec->ws.s) != APP_OK) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
			CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Failed to save the song");
			return -1;
		}

		memset(mec->ws.originalTitle, 0, TITLE_CAPACITY);
		strncpy(mec->ws.originalTitle, mec->ws.s->title, TITLE_CAPACITY - 1);
		return 0;
	}

	if (SongList_Contains(mec->songList, mec->ws.s->title)) {
		if (SongList_Replace(mec->songList, mec->songStoreHeader, mec->ws.s) != APP_OK) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Invalid_State, NULL);
			CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Failed to save the song");
			return -1;
		}
	}
	else {
		if (SongList_Add(mec->songList, mec->songStoreHeader, mec->ws.s) != APP_OK) {
			CLIResponse_Emit(rq, c.id, RESP_ERR, ERR_Internal, NULL);
			CLIResponse_Emit(rq, c.id, RESP_INFO, ERR_None, "Failed to save the song");
			return -1;
		}
	}	

	if (mec->ws.kind == WorkingSong_Edit) {
		memset(mec->ws.originalTitle, 0, TITLE_CAPACITY);
		strncpy(mec->ws.originalTitle, mec->ws.s->title, TITLE_CAPACITY - 1);
	}

	return 0;
}

int Handle_Command_Delete(MusicEngineController* mec, UartCLIController* ucc, Command c) {
	if (!SongList_Contains(mec->songList, c.u.str1.s)) {
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(&ucc->responseQueue, c.id, RESP_INFO, ERR_None, "%s is not a song", c.u.str1.s);
		return -1;
	}

	if (SongList_IsSongStatic(mec->songList, c.u.str1.s)) {
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emitf(&ucc->responseQueue, c.id, RESP_INFO, ERR_None, "Cannot delete this song", c.u.str1.s);
		return -1;
	}

	if (mec->ws.kind != WorkingSong_None && mec->ws.s != NULL && strcmp(mec->ws.s->title, c.u.str1.s) == 0) {
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_ERR, ERR_BadArgument, NULL);
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_INFO, ERR_None, "Cannot delete the song you are currently editing");
		return -1;
	}

	if (!c.confirmed) {
		ucc->confirmationPending = true;
		ucc->needsConfirmed = c;
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_WARN, ERR_None, "This cannot be undone. Are you sure? Y/N");
		return -1;
	}

	if (SongList_Delete(mec->songList, mec->songStoreHeader, c.u.str1.s) != APP_OK) {
		CLIResponse_Emit(&ucc->responseQueue, c.id, RESP_ERR, ERR_Internal, NULL);
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

	bool ownedBySongList = false;
	if (mec->ws.s != NULL) {
		uint16_t idx = SongList_Find(mec->songList, mec->ws.s->title);
		if (idx < mec->songList->songCount && mec->songList->songs[idx] == mec->ws.s) {
			ownedBySongList = true;
		}
	}

	if (!ownedBySongList) {
		free(mec->ws.s);
	}
	mec->ws.s = NULL;
	mec->ws.kind = WorkingSong_None;
	mec->ws.changed = false;
	memset(mec->ws.originalTitle, 0, TITLE_CAPACITY);
	return 0;
}

int Play_Song(MusicEngineController* mec, Song* song) {
	if (song == NULL || song->framesSize == 0) return -1;
	if (!Frame_IsPlayable(&song->frames[0])) return -1;

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

	if (Buzzer_Start(mec->buzzer, currFrame.frequencyHz) == -1) {
		Stop_Song(mec);
		return -1;
	}

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

	if (Buzzer_Start(mec->buzzer, mec->currSong->frames[mec->frameIdx].frequencyHz) == -1) {
		Stop_Song(mec);
		return -1;
	}
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
	size_t len = 0;

#define APPEND_STATUS(...) do { \
	if (len < sizeof(buf)) { \
		int written = snprintf(buf + len, sizeof(buf) - len, __VA_ARGS__); \
		if (written > 0) { \
			size_t remaining = sizeof(buf) - len; \
			if ((size_t)written >= remaining) len = sizeof(buf) - 1; \
			else len += (size_t)written; \
		} \
	} \
} while (0)

	if (mec->pbState == Playing || mec->pbState == Paused) {
		const char* songTitle = (mec->currSong != NULL) ? mec->currSong->title : "<none>";
		if (mec->pbState == Playing)
			APPEND_STATUS("Playing: %s\r\n", songTitle);
		else if (mec->pbState == Paused)
			APPEND_STATUS("Paused: %s\r\n", songTitle);

		uint16_t qSize = mec->songQueue.size;
		if (qSize != 0) {
			uint16_t songCount = qSize > 3 ? 3 : qSize;
			APPEND_STATUS("In Queue: ");
			for (uint16_t i = 0; i < songCount; i++) {
				Song* song = 0;
				SongQueue_At(&mec->songQueue, i, &song);
				APPEND_STATUS("%s ", song->title);
			}
			if (qSize > 3) APPEND_STATUS("...\r\n");
			else APPEND_STATUS("\r\n");
                }
	}
	else {
		APPEND_STATUS("Not Playing\r\n");
	}

	APPEND_STATUS("Volume: %u\r\n", mec->buzzer->volumePct);

	echo(buf, (uint16_t)len);

#undef APPEND_STATUS
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
