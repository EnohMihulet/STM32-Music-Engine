import serial
import time
import pytest

DEVICE_PATH = "/dev/cu.usbmodem2103"
BAUD_RATE = 115200
TIMEOUT_S = 1.0

@pytest.fixture
def uart():
    ser = None
    try:
        ser = serial.Serial(DEVICE_PATH, BAUD_RATE, timeout=TIMEOUT_S)
    except (serial.SerialException, OSError) as exc:
        pytest.skip(f"UART device unavailable at {DEVICE_PATH}: {exc}")

    assert ser is not None

    ser.reset_input_buffer()
    ser.reset_output_buffer()
    yield ser
    ser.close()

def send_command(ser, cmd: str) -> None:
    ser.reset_input_buffer()
    ser.write((cmd + "\r\n").encode("ascii"))
    ser.flush()

def get_response(ser, cmd: str) -> str:
    deadline = time.monotonic() + TIMEOUT_S
    while time.monotonic() < deadline:
        line = ser.readline().decode("ascii", errors="replace").strip()

        if not line:
            continue

        if line == "OK" or line.startswith("ERR:"):
            return line

    pytest.fail(f"No terminal response received for command {cmd!r}")
    raise AssertionError("unreachable")


@pytest.mark.parametrize(
    ("cmd", "expected"),
    [
        ("NOTACOMMAND", "ERR: UnknownCommand"),
        ("VOLUME", "ERR: WrongArgumentCount"),
        ("VOLUME ABC", "ERR: NumericArgNotParsable"),
        ("VOLUME -1", "ERR: NumericArgNotParsable"),
        ("VOLUME 2147483648", "ERR: NumericArgNotParsable"),
        ("VOLUME 101", "ERR: OutsideAllowedRange"),
        ("ADDNOTE 15 100", "ERR: OutsideAllowedRange"),
        ("ADDNOTE 440 100 1", "ERR: WrongArgumentCount"),
        ("PLAY NOSUCHSONG", "ERR: UnexpectedArgument"),
        ("QUEUE NOSUCHSONG", "ERR: UnexpectedArgument"),
        ("VOLUME 1 2 3 4", "ERR: WrongArgumentCount"),
        ("VOLUME 1234567890123456", "ERR: RanOutOfSpace"),
    ],
)
def test_invalid_commands_return_expected_error(uart, cmd, expected):
    send_command(uart, cmd)
    response = get_response(uart, cmd)
    assert response == expected


def test_valid_volume_command_returns_ok(uart):
    cmd = "VOLUME 50"
    send_command(uart, cmd)
    response = get_response(uart, cmd)
    assert response == "OK"


def test_songs_command_returns_ok(uart):
    cmd = "SONGS"
    send_command(uart, cmd)
    response = get_response(uart, cmd)
    assert response == "OK"


def test_commands_are_case_insensitive(uart):
    cmd = "volume 25"
    send_command(uart, cmd)
    response = get_response(uart, cmd)
    assert response == "OK"
