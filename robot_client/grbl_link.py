"""Minimal Grbl 1.1 serial link: connect, send G-code lines, wait for 'ok'."""
import re
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
        if self._serial is None:
            raise GrblError("Serial link is not connected")
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

    def read_settings(self):
        """Read numeric Grbl settings from the `$$` response."""
        settings = {}
        for response in self.send_line("$$"):
            match = re.fullmatch(r"\$(\d+)=([-+]?\d+(?:\.\d+)?)", response)
            if match:
                settings[int(match.group(1))] = float(match.group(2))
        return settings

    def get_status(self):
        if self._serial is None or not self._serial.is_open:
            raise GrblError("Serial link is not connected")
        self._serial.reset_input_buffer()
        self._serial.write(b"?")
        deadline = time.time() + self.timeout
        while time.time() < deadline:
            response = self._readline()
            if response.startswith("<") and response.endswith(">"):
                fields = response[1:-1].split("|")
                status = {"state": fields[0]}
                for field in fields[1:]:
                    if ":" in field:
                        key, value = field.split(":", 1)
                        status[key] = value
                return status
        raise TimeoutError("Timed out waiting for Grbl status report")

    def home(self):
        return self.send_line("$H")

    def unlock(self):
        return self.send_line("$X")

    def wait_idle(self, poll_interval=0.2, timeout=60.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            status = self.get_status()
            if status["state"] == "Idle":
                return status
            time.sleep(poll_interval)
        raise TimeoutError("Timed out waiting for Grbl to report Idle")
