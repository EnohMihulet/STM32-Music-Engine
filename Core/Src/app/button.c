#include "../../Inc/main.h"

#include <stdbool.h>

#include "../../Inc/app/button.h"
#include "../../Inc/app/music_engine.h"

void Button_Init(Button* b) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	b->isDown = 0;
	b->pressedCount = 0;
	b->releasedCount = 0;
	b->pressedAt = 0;
	b->releasedAt = 0;
	b->changedAt = 0;
	b->buttonState = ButtonStateIdle;
	__set_PRIMASK(primask);

}

void Button_Update(Button* b, MusicEngineController* mec) {
	uint8_t pressedCount, releasedCount;
	uint32_t pressedAt, releasedAt;

	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	pressedCount = b->pressedCount;
	releasedCount = b->releasedCount;
	pressedAt = b->pressedAt;
	releasedAt = b->releasedAt;

	b->pressedCount = 0;
	b->releasedCount = 0;
	__set_PRIMASK(primask);

	bool pressed = (pressedCount != 0);
	bool released = (releasedCount != 0);

	switch (b->buttonState) {
	case ButtonStateIdle:
		if (pressed) {
			b->buttonState = ButtonStateDown1;
		} break;
	case ButtonStateDown1:
		if (released) {
			if ((releasedAt - pressedAt) > SHORT_CLICK_TIME) {
				// Confirmed single click
				b->buttonState = ButtonStateIdle;
				CommandCode cc = mec->pbState == Playing ? Command_Pause : Command_Resume;
				CommandQueue_Push(&mec->commandQueue, (Command){cc, 0});
			}
			else b->buttonState = ButtonStateWait;
		} 
		else if ((HAL_GetTick() - pressedAt) >= HOLD_TIME) {
			// Confirmed hold
			b->buttonState = ButtonStateIdle;
			CommandQueue_Push(&mec->commandQueue, (Command){Command_Clear, 0});
		} break;
	case ButtonStateWait:
		if (pressed) {
			if ((pressedAt - releasedAt) > DOUBLE_CLICK_TIME) {
				// Confirmed single click
				b->buttonState = ButtonStateIdle;
				CommandCode cc = mec->pbState == Playing ? Command_Pause : Command_Resume;
				CommandQueue_Push(&mec->commandQueue, (Command){cc, 0});
			}
			else b->buttonState = ButtonStateDown2;
		}
		else if ((HAL_GetTick() - releasedAt) > DOUBLE_CLICK_TIME) {
			// Confirmed single click
			b->buttonState = ButtonStateIdle;
			CommandCode cc = mec->pbState == Playing ? Command_Pause : Command_Resume;
			CommandQueue_Push(&mec->commandQueue, (Command){cc, 0});
		} break;
	case ButtonStateDown2:
		if (released) {
			if ((releasedAt - pressedAt) <= SHORT_CLICK_TIME) {
				// Confirmed double click
				b->buttonState = ButtonStateIdle;
				CommandQueue_Push(&mec->commandQueue, (Command){Command_Skip, 0});
			}
			else {
				// Confirmed single click
				b->buttonState = ButtonStateIdle;
				CommandCode cc = mec->pbState == Playing ? Command_Pause : Command_Resume;
				CommandQueue_Push(&mec->commandQueue, (Command){cc, 0});
			}
		} break;
	default: break;
	}
}
