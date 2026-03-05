import serial
import time
import pytest

DEVICE_PATH = "/dev/cu.usbmodem2103"
BAUD_RATE = 115200
TIMEOUT_S = 1.0
COMMAND_COUNT = 24

@pytest.fixture
def uart():
    ser = serial.Serial(DEVICE_PATH, BAUD_RATE, timeout=1)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    yield ser
    ser.close()

def send_command(ser, cmd:str) -> None:
    ser.write((cmd + "\r\n").encode("ascii"))
    ser.flush()

def get_response(ser, cmd:str) -> str:
    while True:
        line = ser.readline().decode("ascii", errors="replace")

        if not line:
            pytest.fail(f"No response received for command {cmd!r}")

        if line == "OK" or line.startswith("ERR:"):
            return line

def get_all_output(ser) -> str:
    s = ""
    while True:
        line = ser.readline().decode("ascii", errors="replace").strip()

        if not line:
            return s

        s += line

def test_unknown_command_returns_expected_error(uart):
    cmd = "NOTACOMMAND"
    send_command(uart, cmd)
    response = get_response(uart, cmd)
    assert response == "ERR: UNKNOWN COMMAND"

def test_known_command_returns_expected_error(uart):
    cmd = "NOTACOMMAND"
    send_command(uart, "STATUS")
    response = get_response(uart, cmd)
    assert response == "OK"

def test_commands_command(uart):
    send_command(uart, "COMMANDS")
    output = get_all_output(uart)
    assert output.count('\n') == COMMAND_COUNT



