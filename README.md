# Music Engine Project

Project using STM32CubeIDE.

## Board / MCU

- MCU: STM32F446RETx
- Board: Nucleo-F446RE

## Build

1. Open project in STM32CubeIDE.
2. Build in IDE.
3. Flash using ST-LINK.

## Controls

### Blue Button (B1 / PC13)

The blue USER button is used to control playback:

- Single press: Pause / Resume
- Double press: Skip to next song
- Hold: Clear the song queue

### USART Command Interface

Commands can be sent over USART2 as ASCII text.

## Commands

`x` is used as a numeric argument (for example, song index or percentage).

| Command | Description | Example usage |
| --- | --- | --- |
| `PAUSE` | Pause playback | `PAUSE` |
| `RESUME` | Resume playback | `RESUME` |
| `STOP` | Stop playback | `STOP` |
| `SKIP` | Skip to the next song | `SKIP` |
| `CLEAR` | Clear the song queue | `CLEAR` |
| `SONGS` | List available songs | `SONGS` |
| `PLAY x` | Play song index `x` | `PLAY 2` |
| `QUEUE x` | Enqueue song index `x` | `QUEUE 4` |
| `TEMPO x` | Set tempo (not implemented) | `TEMPO 120` |
| `VOLUME x` | Set volume as `%` | `VOLUME 70` |
| `STATUS` | Print current playback/engine status | `STATUS` |

## Wiring

### Pins used

- PA6 (TIM3_CH1) = Tone output
- PB6 (TIM4_CH1) = Volume PWM output
- GND
- 3.3V (or 5V)

### Parts

- Passive buzzer (piezo)
- Q2: NPN transistor (main buzzer switch)
- Q1: NPN transistor (volume clamp)
- Resistors:
  - Rtone = 2.2kOhm
  - Rb = 1.0kOhm
  - Rvol = 2.2kOhm
  - Rpd2 = 100kOhm (Q2 base pulldown)
  - Rpd1 = 100kOhm (Q1 base pulldown)

### Connections

#### Buzzer + main switch

- Buzzer + -> 3.3V (or 5V) -> Node N
- Node N -> Rb (1.0k) -> Q2 Base
- Q2 Base -> Rpd2 (100k) -> GND

#### Volume clamp

- Q1 Emitter -> GND
- Q1 Collector -> Node N
- PB6 (TIM4_CH1) -> Rvol (2.2k) -> Q1 Base
- Q1 Base -> Rpd1 (100k) -> GND

## Notes

TIM4 duty is inverted:

- TIM4 HIGH turns Q1 on, muting the buzzer.
- Use mute duty = 100% - volume%.
