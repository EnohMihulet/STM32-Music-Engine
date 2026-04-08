import serial
import time
import pytest
import re

DEVICE_PATH = "/dev/cu.usbmodem2103"
BAUD_RATE = 115200
TIMEOUT_S = 1.0
RESULT_LINE_RE = re.compile(
    r"^#(?P<id>\d+)\s+(?P<kind>OK|ERR|INFO|WARN)(?:\s+(?P<code>[A-Za-z0-9_]+))?(?::\s*(?P<detail>.*))?$"
)

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

def get_response(ser, cmd: str) -> tuple[str, str | None]:
    deadline = time.monotonic() + TIMEOUT_S
    while time.monotonic() < deadline:
        line = ser.readline().decode("ascii", errors="replace").strip()

        if not line:
            continue

        match = RESULT_LINE_RE.match(line)
        if not match:
            continue

        kind = match.group("kind")
        if kind not in ("OK", "ERR"):
            continue

        code = match.group("code") if kind == "ERR" else None
        return kind, code

    pytest.fail(f"No terminal response received for command {cmd!r}")
    raise AssertionError("unreachable")


@pytest.mark.parametrize(
    ("cmd", "expected_code"),
    [
        ("NOTACOMMAND", "UnknownCommand"),
        ("VOLUME", "WrongArgumentCount"),
        ("VOLUME ABC", "NumericArgNotParsable"),
        ("VOLUME -1", "NumericArgNotParsable"),
        ("VOLUME 2147483648", "NumericArgNotParsable"),
        ("VOLUME 101", "OutsideAllowedRange"),
        ("ADDNOTE 15 100", "OutsideAllowedRange"),
        ("ADDNOTE 440 100 1", "WrongArgumentCount"),
        ("PLAY NOTASONG", "UnexpectedArgument"),
        ("QUEUE NOTASONG", "UnexpectedArgument"),
        ("VOLUME 1 2 3 4", "WrongArgumentCount"),
        ("VOLUME 1234567890123456", "RanOutOfSpace"),
    ],
)
def test_invalid_commands_return_expected_error(uart, cmd, expected_code):
    send_command(uart, cmd)
    kind, code = get_response(uart, cmd)
    assert kind == "ERR"
    assert code == expected_code


def test_valid_volume_command_returns_ok(uart):
    cmd = "VOLUME 50"
    send_command(uart, cmd)
    kind, code = get_response(uart, cmd)
    assert kind == "OK"
    assert code is None


def test_songs_command_returns_ok(uart):
    cmd = "SONGS"
    send_command(uart, cmd)
    kind, code = get_response(uart, cmd)
    assert kind == "OK"
    assert code is None


def test_commands_are_case_insensitive(uart):
    cmd = "volume 25"
    send_command(uart, cmd)
    kind, code = get_response(uart, cmd)
    assert kind == "OK"
    assert code is None
