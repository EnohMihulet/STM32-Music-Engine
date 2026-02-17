#include "../../Inc/main.h"
#include "../../Inc/app/button.h"
#include <stdbool.h>

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

void Button_Update(Button* b) {
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
			if ((releasedAt - pressedAt) <= SHORT_CLICK_TIME) b->buttonState = ButtonStateWait;
			else b->buttonState = ButtonStateSingle;
		} 
		else if ((HAL_GetTick() - pressedAt) >= HOLD_TIME) {
			b->buttonState = ButtonStateHold;
		} break;
	case ButtonStateWait:
		if (pressed) {
			if ((pressedAt - releasedAt) <= DOUBLE_CLICK_TIME) b->buttonState = ButtonStateDown2;
			else b->buttonState = ButtonStateSingle;
		}
		else if ((HAL_GetTick() - releasedAt) > DOUBLE_CLICK_TIME) {
			b->buttonState = ButtonStateSingle;
		} break;
	case ButtonStateDown2:
		if (released) {
			if ((releasedAt - pressedAt) <= SHORT_CLICK_TIME) b->buttonState = ButtonStateDouble;
			else b->buttonState = ButtonStateSingle;
		} break;
	default: break;
	}

}

ButtonEvent Button_GetEvent(Button* b) {
	switch(b->buttonState) {
		case ButtonStateIdle: case ButtonStateDown1: case ButtonStateWait: case ButtonStateDown2:
			return ButtonEventNone;
		case ButtonStateSingle:
			b->buttonState = ButtonStateIdle;
			return ButtonEventSingleClick;
		case ButtonStateDouble:
			b->buttonState = ButtonStateIdle;
			return ButtonEventDoubleClick;
		case ButtonStateHold:
			b->buttonState = ButtonStateIdle;
			return ButtonEventHold;
		default:
			return ButtonEventNone;
	}
}
