"""Minimal GRBL-CRPT 1.1 serial link: connect, send G-code lines, wait for 'ok'."""
import re
import time
from typing import Any, Dict, Protocol, Tuple


class CrptError(RuntimeError):
    """Raised when GRBL-CRPT reports an error or alarm for a sent command."""


class MachineLink(Protocol):
    """Transport the RobotController talks to: real serial GRBL-CRPT or the simulator."""

    def connect(self) -> Any: ...
    def close(self) -> Any: ...
    def send_line(self, line: str, wait_ok: bool = True) -> Any: ...
    def read_settings(self) -> Dict[int, float]: ...
    def get_status(self) -> Dict[str, str]: ...
    def home(self) -> Any: ...
    def unlock(self) -> Any: ...
    def wait_idle(self) -> Dict[str, str]: ...
    def prepare_transport(self, pick_xy: Tuple[float, float], place_xy: Tuple[float, float]) -> Any: ...
    def gripper_close(self) -> Any: ...
    def gripper_open(self) -> Any: ...


class CrptLink:
    def __init__(self, port, baud, timeout=2.0):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self._serial = None

    def connect(self):
        import serial

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

    def _ensure_open(self):
        if self._serial is None or not self._serial.is_open:
            raise CrptError("Serial link is not connected")
        return self._serial

    def _readline(self):
        return self._ensure_open().readline().decode(errors="replace").strip()

    def send_line(self, line, wait_ok=True):
        self._ensure_open().write((line.strip() + "\n").encode())
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
                raise CrptError(f"GRBL-CRPT error for '{line}': {resp}")
            if resp.startswith("ALARM"):
                raise CrptError(f"GRBL-CRPT alarm for '{line}': {resp}")
        return responses

    def read_settings(self):
        """Read numeric GRBL-CRPT settings from the `$$` response."""
        settings = {}
        for response in self.send_line("$$"):
            match = re.fullmatch(r"\$(\d+)=([-+]?\d+(?:\.\d+)?)", response)
            if match:
                settings[int(match.group(1))] = float(match.group(2))
        return settings

    def get_status(self):
        serial_port = self._ensure_open()
        serial_port.reset_input_buffer()
        serial_port.write(b"?")
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
        raise TimeoutError("Timed out waiting for GRBL-CRPT status report")

    def home(self):
        return self.send_line("$H")

    def unlock(self):
        return self.send_line("$X")

    def prepare_transport(self, pick_xy, place_xy):
        """Hook for simulated plate staging; real gripper hardware is not wired yet."""

    def gripper_close(self):
        """Hook for the future physical gripper close command."""

    def gripper_open(self):
        """Hook for the future physical gripper open command."""

    def wait_idle(self, poll_interval=0.2, timeout=60.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            status = self.get_status()
            if status["state"] == "Idle":
                return status
            time.sleep(poll_interval)
        raise TimeoutError("Timed out waiting for GRBL-CRPT to report Idle")
