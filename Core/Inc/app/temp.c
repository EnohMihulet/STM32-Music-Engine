/*

static void Songs_Init(void) {
	gSongs[0] = (Song) {
		.title = "SONG1",
		.framesSize = 1,
		.frames = {{988, 15000}}
	};

	gSongs[1] = (Song) {
		.title = "SONG2",
		.framesSize = 1,
		.frames = {{659, 15000}}
	};

	gSongs[2] = (Song) {
		.title = "SONG3",
		.framesSize = 1,
		.frames = {{392, 15000}}
	};
}

static void UartBufferController_Init() {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	gUartBufferController.lastPos = 0;
	gUartBufferController.currPos = 0;
	gUartBufferController.pending = false;
	gUartBufferController.commandIndex = 0;
	memset(gUartBufferController.command, 0, 16);
	__set_PRIMASK(primask);
}

static void MusicEngineController_Init(void) {
	gMusicEngineController.playbackControl = CommandPlaybackControlNone;
	gMusicEngineController.playRequest = CommandPlayRequestNone;
	gMusicEngineController.commandSettings = CommandSettingsNone;

	gMusicEngineController.number = 0;
}

static void Button_Init(void) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	gButton.isDown = 0;
	gButton.pressedCount = 0;
	gButton.releasedCount = 0;
	gButton.pressedAt = 0;
	gButton.releasedAt = 0;
	gButton.changedAt = 0;
	gButton.buttonState = ButtonStateIdle;
	__set_PRIMASK(primask);
}

static void BuzzerController_Init(void) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	gBuzzerController.size = 0;
	gBuzzerController.index = 0;
	gBuzzerController.isPlaying = 0;
	gBuzzerController.frameRemainingMs = 0;
	__set_PRIMASK(primask);
}

static void MusicEngine_Update(void) {

	MusicEngineController* mec = &gMusicEngineController;
	if (mec->playbackControl == CommandPlaybackControlNone && mec->playRequest == CommandPlayRequestNone && mec->commandSettings == CommandSettingsNone) return;

	switch (mec->playbackControl) {
		case CommandPlaybackControlNone: break;
		case CommandPlaybackControlPause:
			Song_Pause();
			break;
		case CommandPlaybackControlResume:
			Song_Resume();
			break;
		case CommandPlaybackControlStop:
			Song_Stop();
			break;
		case CommandPlaybackControlSkip:
			Song_Skip();
			break;
		case CommandPlaybackControlClear:
			Queue_Clear();
			break;
		default: break;
	}
	mec->playbackControl = CommandPlaybackControlNone;

	switch (mec->playRequest) {
		case CommandPlayRequestNone: break;
		case CommandPlayRequestPlay:
			if (mec->number == -1) {
				Display_Songs();
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else if (1 <= mec->number && mec->number <= 3) {
				Play_Song(mec->number - 1);
			}
			break;
		case CommandPlayRequestQueue:
			if (mec->number == -1) {
				Display_Songs();
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else if (1 <= mec->number && mec->number <= 3) {
				Queue_Song(mec->number - 1);
			}
			break;
		case CommandPlayRequestPlaySeq:
		default: break;
	}
	mec->playRequest = CommandPlayRequestNone;

	switch (mec->commandSettings) {
		case CommandSettingsNone: break;
		case CommandSettingsStatus:
			Display_MusicEngine_Status();
			break;
		case CommandSettingsTempo:
			if (mec->number == -1) {
				Display_Tempo();
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else {
				Set_Tempo(mec->number);
			}
			break;
		case CommandSettingsVol:
			if (mec->number == -1) {
				Display_Volume();
				mec->number = 0;
				return;
			}
			else if (mec->number == 0) {
				return;
			}
			else {
				Set_Volume(mec->number);
			}
			break;
		default: break;
	}
	mec->commandSettings = CommandSettingsNone;
}

static void Uart_Update(void) {
	if (!gUartBufferController.pending) return;
	
	UartRxBufferController* bc = &gUartBufferController;

	uint16_t start = bc->lastPos;
	uint16_t end = bc->currPos;

	uint16_t commandStart = bc->commandIndex;

	bool gotLine = false;
	uint16_t newLastPos = start;

	if (start < end) {
		for (uint16_t i = start; i < end; i++) {
			char nextChar = bc->ingestBuffer[i];

			if (nextChar == '\b' || nextChar == 0x7F) {
				if (bc->commandIndex == 0) {
					continue;
				}
				bc->command[--(bc->commandIndex)] = 0;
				HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 100);

				newLastPos = i + 1;
				continue;
			}

			if (bc->commandIndex < (sizeof(bc->command) - 1)) {
				char upperChar = toupper((uint8_t)nextChar);
				bc->command[bc->commandIndex++] = upperChar;
				HAL_UART_Transmit(&huart2, (uint8_t*)&upperChar, 1, 100);
			}

			newLastPos = i + 1;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}
	else if (start > end) {
		uint16_t len = INGEST_BUFFER_SIZE - start + end;
		uint16_t i = start;
		for (uint16_t k = 0; k < len; k++) {
			if (i == INGEST_BUFFER_SIZE) i = 0;

			char nextChar = bc->ingestBuffer[i++];

			if (nextChar == '\b' || nextChar == 0x7F) {
				if (bc->commandIndex == 0) {
					continue;
				}
				bc->command[--(bc->commandIndex)] = 0;
				HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 100);

				newLastPos = i;
				continue;
			}

			if (bc->commandIndex < (sizeof(bc->command) - 1)) {
				char upperChar = toupper((uint8_t)nextChar);
				bc->command[bc->commandIndex++] = upperChar;
				HAL_UART_Transmit(&huart2, (uint8_t*)&upperChar, 1, 100);
			}

			newLastPos = i;

			if (nextChar == '\n' || nextChar == '\r') {
				gotLine = true;
				break;
			}
		}
	}
	else {
		bc->pending = false;
		return;
	}

	bc->lastPos = newLastPos % INGEST_BUFFER_SIZE;

	if (!gotLine) {
		bc->pending = false;
		return;
	}

	while (bc->commandIndex > 0 && (bc->command[bc->commandIndex - 1] == '\n' || bc->command[bc->commandIndex - 1] == '\r')) {
		bc->commandIndex--;
	}
	bc->command[bc->commandIndex] = '\0';

	Handle_Command(bc->command);

	HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);

	bc->pending = false;
	memset(bc->command, 0, sizeof(bc->command));
	bc->commandIndex = 0;
}

static void Button_Update(void) {

	uint8_t pressedCount, releasedCount;
	uint32_t pressedAt, releasedAt;

	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	pressedCount = gButton.pressedCount;
	releasedCount = gButton.releasedCount;
	pressedAt = gButton.pressedAt;
	releasedAt = gButton.releasedAt;

	gButton.pressedCount = 0;
	gButton.releasedCount = 0;
	__set_PRIMASK(primask);

	bool pressed = (pressedCount != 0);
	bool released = (releasedCount != 0);

	switch (gButton.buttonState) {
	case ButtonStateIdle:
		if (pressed) {
			gButton.buttonState = ButtonStateDown1;
		} break;
	case ButtonStateDown1:
		if (released) {
			if ((releasedAt - pressedAt) <= SHORT_CLICK_TIME) gButton.buttonState = ButtonStateWait;
			else gButton.buttonState = ButtonStateSingle;
		} 
		else if ((HAL_GetTick() - pressedAt) >= HOLD_TIME) {
			gButton.buttonState = ButtonStateHold;
		} break;
	case ButtonStateWait:
		if (pressed) {
			if ((pressedAt - releasedAt) <= DOUBLE_CLICK_TIME) gButton.buttonState = ButtonStateDown2;
			else gButton.buttonState = ButtonStateSingle;
		}
		else if ((HAL_GetTick() - releasedAt) > DOUBLE_CLICK_TIME) {
			gButton.buttonState = ButtonStateSingle;
		} break;
	case ButtonStateDown2:
		if (released) {
			if ((releasedAt - pressedAt) <= SHORT_CLICK_TIME) gButton.buttonState = ButtonStateDouble;
			else gButton.buttonState = ButtonStateSingle;
		} break;
	default: break;
	}
}

static void Buzzer_Update(void) {
	if ((!gBuzzerController.isPlaying && gMusicEngineController.queueSize == 0) || gBuzzerController.isPaused) return;

	if (!gBuzzerController.isPlaying && gMusicEngineController.queueSize > 0) {
		uint16_t songIndex = SongQueue_Pop();
		Play_Song(songIndex);
	}

	uint32_t frameRemainingMs;
	uint32_t primask = __get_PRIMASK();

	__disable_irq();
	frameRemainingMs = gBuzzerController.frameRemainingMs;
	__set_PRIMASK(primask);

	if (frameRemainingMs != 0) return;

	if (gBuzzerController.index >= gBuzzerController.size - 1) {
		gBuzzerController.isPlaying = false;

		if (gMusicEngineController.queueSize > 0) {
			uint16_t songIndex = SongQueue_Pop();
			Play_Song(songIndex);
		}
		else {
			Buzzer_Off();
		}
		return;
	}

	gBuzzerController.index += 1;

	Frame* frame = &(gSongs[gBuzzerController.songIndex].frames[gBuzzerController.index]);

	primask = __get_PRIMASK();

	__disable_irq();
	gBuzzerController.frameRemainingMs = frame->durationMs;
	__set_PRIMASK(primask);

	uint16_t freqHz = frame->frequencyHz;

	Buzzer_Off();
	if (freqHz == 0) return;
	if (gMusicEngineController.volume == 0) return;
	Tone_SetFrequency(freqHz);
	Buzzer_On();
}

static void Handle_Command(char* command) {
	UartRxBufferController* bc = &gUartBufferController;

	char* endptr;
	int64_t num = strtol(command, &endptr, 10);
	if (*endptr == '\0') {
		gMusicEngineController.number = num;
		return;
	}

	gMusicEngineController.number = -1;
	if (strcmp(bc->command, "PAUSE") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlPause;
	}
	else if (strcmp(bc->command, "RESUME") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlResume;
	}
	else if (strcmp(bc->command, "STOP") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlStop;
	}
	else if (strcmp(bc->command, "SKIP") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlSkip;
	}
	else if (strcmp(bc->command, "CLEAR") == 0) {
		gMusicEngineController.playbackControl = CommandPlaybackControlClear;
	}
	else if (strcmp(bc->command, "PLAY") == 0) {
		gMusicEngineController.playRequest = CommandPlayRequestPlay;
	}
	else if (strcmp(bc->command, "QUEUE") == 0) {
		gMusicEngineController.playRequest = CommandPlayRequestQueue;
	}
	else if (strcmp(bc->command, "TEMPO") == 0) {
		gMusicEngineController.commandSettings = CommandSettingsTempo;
	}
	else if (strcmp(bc->command, "VOLUME") == 0) {
		gMusicEngineController.commandSettings = CommandSettingsVol;
	}
	else if (strcmp(bc->command, "STATUS") == 0) {
		gMusicEngineController.commandSettings = CommandSettingsStatus;
	}
}

static ButtonEvent Button_GetEvent() {
	switch(gButton.buttonState) {
		case ButtonStateIdle: case ButtonStateDown1: case ButtonStateWait: case ButtonStateDown2:
			return ButtonEventNone;
		case ButtonStateSingle:
			gButton.buttonState = ButtonStateIdle;
			return ButtonEventSingleClick;
		case ButtonStateDouble:
			gButton.buttonState = ButtonStateIdle;
			return ButtonEventDoubleClick;
		case ButtonStateHold:
			gButton.buttonState = ButtonStateIdle;
			return ButtonEventHold;
		default:
			return ButtonEventNone;
	}
}

static void Buzzer_On(void) {
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); 
}

static void Buzzer_Off(void) {
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1); 
}

static uint16_t Clamp(uint16_t v, uint16_t lb, uint16_t ub) {
	return v < lb ? lb : v > ub ? ub : v;
}

static void Tone_SetFrequency(uint16_t freqHz) {
	assert(gMusicEngineController.volume != 0);
	uint16_t arr = Clamp(1000000 / freqHz - 1, 16, 65535);

	uint16_t ccr1 = (arr + 1) / x;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr1);
	__HAL_TIM_SET_AUTORELOAD(&htim3, arr);
}

static uint16_t SongQueue_Pop() {
	assert(gMusicEngineController.queueSize != 0);

	uint16_t songIndex = gMusicEngineController.queue[0];
	for (uint16_t i = 0; i < gMusicEngineController.queueSize - 1; i++) {
		gMusicEngineController.queue[i] = gMusicEngineController.queue[i+1];
	}
	gMusicEngineController.queueSize -= 1;

	return songIndex;
}

static void Display_MusicEngine_Status(void) {
	HAL_UART_Transmit(&huart2, (uint8_t*)"STATUS: ", 7, 100);

	char buffer[32];
	sprintf(buffer, "Queue Size: %d\r\n", gMusicEngineController.queueSize);
	uint16_t len = strlen(buffer);
	HAL_UART_Transmit(&huart2, buffer, len, 100);
	
	Display_Volume();
	Display_Tempo();
}

static void Display_Songs() {
	HAL_UART_Transmit(&huart2, (uint8_t*)"SONGS: ", 7, 100);
	for (uint16_t i = 0; i < SONG_COUNT; i++) {
		char buffer[32];
		char* title = gSongs[i].title;
		uint16_t len = strlen(title);

		strcpy(buffer, title);
		if (i == SONG_COUNT - 1) {
			buffer[len++] = '\r';
			buffer[len++] = '\n';
		}
		else {
			buffer[len++] = ' ';
		}

		HAL_UART_Transmit(&huart2, buffer, len, 100);
	}
}

static void Display_Volume(void) {
	char buffer[32];
	sprintf(buffer, "Volume: %d%\r\n", gMusicEngineController.volume);
	uint16_t len = strlen(buffer);
	HAL_UART_Transmit(&huart2, buffer, len, 100);
}

static void Display_Tempo(void) {
	char buffer[32];
	sprintf(buffer, "Tempo: %d", gMusicEngineController.tempo);
	uint16_t len = strlen(buffer);
	HAL_UART_Transmit(&huart2, buffer, len, 100);
}

static void Set_Volume(uint16_t newVol) {
	gMusicEngineController.volume = newVol;
}

static void Set_Tempo(uint16_t newTempo) {

}

static void Play_Song(uint16_t songIndex) {
	if (songIndex >= SONG_COUNT) return;

	Song* song = &(gSongs[songIndex]);
	Frame* firstFrame = &(song->frames[0]);

	gBuzzerController.size = song->framesSize;
	gBuzzerController.index = 0;
	gBuzzerController.frameRemainingMs = firstFrame->durationMs;
	gBuzzerController.isPlaying = true;

	Buzzer_Off();
	if (firstFrame->frequencyHz == 0) return;
	if (gMusicEngineController.volume == 0) return;
	Tone_SetFrequency(firstFrame->frequencyHz);
	Buzzer_On();
}

static void Queue_Song(uint16_t songIndex) {
	if (songIndex >= SONG_COUNT) return;

	if (gMusicEngineController.queueSize >= 16) return;

	gMusicEngineController.queue[gMusicEngineController.queueSize++] = songIndex;
}

static void Song_Pause(void) {
	if (gBuzzerController.isPaused) return;
	gBuzzerController.isPaused = true;
	Buzzer_Off();
}

static void Song_Resume(void) {
	if (!gBuzzerController.isPaused) return;
	gBuzzerController.isPaused = false;

	Frame* frame = &(gSongs[gBuzzerController.songIndex].frames[gBuzzerController.index]);
	if (frame->frequencyHz != 0) {
		Buzzer_On();
	}
}

static void Song_Stop(void) {
	Buzzer_Off();
	BuzzerController_Init();
}

static void Song_Skip(void) {
	BuzzerController_Init();

	if (gMusicEngineController.queueSize > 0) {
		uint16_t songIndex = SongQueue_Pop();
		Play_Song(songIndex);
	}
	else {
		Buzzer_Off();
	}
}

static void Queue_Clear(void) {
	gMusicEngineController.queueSize = 0;
}

*/
