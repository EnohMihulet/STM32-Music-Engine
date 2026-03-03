import serial

DEVICE_PATH = "/dev/cu.usbmodem2103"
BAUD_RATE = 115200

with serial.Serial(DEVICE_PATH, BAUD_RATE, timeout=1) as s:
    s.write(b"PLAY SONG1\n")
    # print(s.read(2000).decode(errors="replace"))
