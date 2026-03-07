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


| Command          | Description                                        | Example usage                |
| ---------------- | -------------------------------------------------- | ---------------------------- |
| `COMMANDS`       | List all available commands                        | `COMMANDS`                   |
| `PAUSE`          | Pause playback                                     | `PAUSE`                      |
| `RESUME`         | Resume playback                                    | `RESUME`                     |
| `STOP`           | Stop playback                                      | `STOP`                       |
| `SKIP`           | Skip to the next song                              | `SKIP`                       |
| `CLEAR`          | Clear the song queue                               | `CLEAR`                      |
| `SONGS`          | List available songs                               | `SONGS`                      |
| `PLAY x`         | Play the song titled `x`                           | `PLAY MySong`                |
| `QUEUE x`        | Add the song titled `x` to the queue               | `QUEUE MySong`               |
| `VOLUME x`       | Set the playback volume to `x`                     | `VOLUME 70`                  |
| `STATUS`         | Print current playback or engine status            | `STATUS`                     |
| `NEWSONG x`      | Create a new song with title `x`                   | `NEWSONG MyNewSong`          |
| `EDITSONG x`     | Open song `x` for editing                          | `EDITSONG MySong`            |
| `COPYSONG x y`   | Copy song `x` to a new song titled `y`             | `COPYSONG MySong MySongCopy` |
| `ADDNOTE x y`    | Add a note with frequency `x` and duration `y`     | `ADDNOTE 440 500`            |
| `ADDREST x`      | Add a rest with duration `x`                       | `ADDREST 250`                |
| `EDITNOTE x y z` | Edit frame `x` with frequency `y` and duration `z` | `EDITNOTE 3 440 500`         |
| `EDITTITLE x`    | Change the current song title to `x`               | `EDITTITLE BetterSongName`   |
| `LISTSONG`       | List notes or contents of the current song         | `LISTSONG`                   |
| `PLAYSONG`       | Play the currently edited song                     | `PLAYSONG`                   |
| `CLEARSONG`      | Remove all notes from the current song             | `CLEARSONG`                  |
| `SAVE`           | Save the current song                              | `SAVE`                       |
| `DELETE x`       | Delete the song titled `x`                         | `DELETE MySong`              |
| `QUIT`           | Quit the program                                   | `QUIT`                       |


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
