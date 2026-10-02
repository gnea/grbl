"""Minimal Grbl 1.1 serial link: connect, send G-code lines, wait for 'ok'."""
import time

import serial


class GrblError(RuntimeError):
    """Raised when Grbl reports an error or alarm for a sent command."""


class GrblLink:
    def __init__(self, port, baud, timeout=2.0):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self._serial = None

    def connect(self):
        self._serial = serial.Serial(self.port, self.baud, timeout=self.timeout)
        time.sleep(2.0)  # Arduino resets when the serial port opens
        self._serial.reset_input_buffer()
        self._serial.write(b"\r\n\r\n")
        time.sleep(0.5)
        self._serial.reset_input_buffer()
        return self

    def close(self):
        if self._serial is not None and self._serial.is_open:
            self._serial.close()

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()

    def _readline(self):
        return self._serial.readline().decode(errors="replace").strip()

    def send_line(self, line, wait_ok=True):
        if self._serial is None or not self._serial.is_open:
            raise GrblError("Serial link is not connected")
        self._serial.write((line.strip() + "\n").encode())
        if not wait_ok:
            return []
        responses = []
        while True:
            resp = self._readline()
            if resp == "":
                continue
            responses.append(resp)
            if resp == "ok":
                break
            if resp.startswith("error"):
                raise GrblError(f"Grbl error for '{line}': {resp}")
            if resp.startswith("ALARM"):
                raise GrblError(f"Grbl alarm for '{line}': {resp}")
        return responses

    def home(self):
        return self.send_line("$H")

    def unlock(self):
        return self.send_line("$X")

    def wait_idle(self, poll_interval=0.2, timeout=60.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            self._serial.reset_input_buffer()
            self._serial.write(b"?")
            time.sleep(poll_interval)
            raw = self._serial.read_all().decode(errors="replace")
            for line in raw.splitlines():
                line = line.strip()
                if line.startswith("<") and line.endswith(">"):
                    state = line[1:-1].split("|")[0]
                    if state == "Idle":
                        return
        raise TimeoutError("Timed out waiting for Grbl to report Idle")
