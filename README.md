Music Engine Project

Project using STM32CubeIDE

Board / MCU:
    MCU: STM32F446RETx
    Board: Nucleo-F446RE


Build:
    Open project in STM32CubeIDE
    Build in IDE
    Flash using ST-LINK

Controls:
    Blue Button (B1 / PC13)
        The blue USER button is used to control playback:
            -Single press: Pause / Resume
            -Double press: Skip to next song
            -Hold: Clear the song queue
    USART Command Interface
        Commands can be sent over USART2 as ASCII text. Each command must be terminated with newline (\n) or carriage return (\r). Input is case-insensitive.
        Commands:
            -PAUSE - pause playback
            -RESUME - resume playback
            -STOP - stop playback
            -SKIP - skip to the next song
            -CLEAR - clear the song queue
            -SONGS - list available songs
            -PLAY x - play song index x
            -QUEUE x - enqueue song index x
            -TEMPO x - set tempo (Not implemented)
            -VOLUME x - set volume as %
            -STATUS - print current playback/engine status

Wiring: 
    Pins used:
        -PA6 (TIM3_CH1) = Tone output
        -PB6 (TIM4_CH1) = Volume PWM output
        -GND
        -3.3V (or 5V)
    Parts:
        -Passive buzzer (piezo)
        -Q2: NPN transistor (main buzzer switch)
        -Q1: NPN transistor (volume clamp)
        -Resistors:
           Rtone = 2.2kΩ
           Rb = 1.0kΩ
           Rvol = 2.2kΩ
           Rpd2 = 100kΩ (Q2 base pulldown)
           Rpd1 = 100kΩ (Q1 base pulldown)
    Connections:
        -Buzzer + main switch:
            Buzzer + -> 3.3V (or 5V) -> Node N
            Node N -> Rb (1.0k) -> Q2 Base
            Q2 Base -> Rpd2 (100k) -> GND
        -Volume Clamp:
            Q1 Emitter -> GND
            Q1 Collector -> Node N
            PB6 (TIM4_CH1) -> Rvol (2.2k) -> Q1 Base
            Q1 Base -> Rpd1 (100k) -> GND

Notes:
    TIM4 duty is inverted:
        TIM4 HIGH turns Q1 On, muting the buzzer
        Use mute duty = 100% - volume%
