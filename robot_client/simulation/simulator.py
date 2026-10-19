"""Local browser simulator and Grbl-compatible link for CRPT."""
import argparse
import json
import math
import re
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Optional, cast
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

from .. import config
from ..crpt_link import CrptError

DEFAULT_URL = "http://127.0.0.1:8765"
_SETTINGS = {
    28: 7,
    130: config.SAFE_TRAVEL_X_MM + 30.0,
    131: config.SAFE_TRAVEL_Y_MM + 30.0,
    132: 200.0,
    133: 360.0,
    140: 30.0,
    141: 30.0,
    142: 30.0,
    143: 0.0,
}
_PAGE = Path(__file__).with_name("simulator.html")


class SimulatorState:
    """Small animated machine model shared by the HTTP API and browser view."""

    def __init__(self):
        self.lock = threading.RLock()
        self.axes = [0.0, 0.0, 0.0, 0.0]
        self.motion = None
        self.carrying = False
        self.plate = {
            "x": config.TABLE_SIDE_MM / 2.0,
            "y": config.TABLE_SIDE_MM + config.ARM_REACH_MM / 2.0,
        }
        self.dropoff = None
        self.events = []
        self.last_command = "Ready"
        self.demo_running = False

    def _position_locked(self):
        if self.motion is None:
            return list(self.axes)
        fraction = min(1.0, (time.monotonic() - self.motion["started"]) / self.motion["duration"])
        start, target = self.motion["start"], self.motion["target"]
        self.axes = [a + (b - a) * fraction for a, b in zip(start, target)]
        if fraction >= 1.0:
            self.axes = list(target)
            self.motion = None
        return list(self.axes)

    def _tip_locked(self, axes):
        a_angle = axes[3] % 360.0
        if not math.isclose(a_angle, 0.0, abs_tol=1e-6):
            a_angle -= config.ROTATION_OFFSET_DEG
        angle = math.radians(a_angle)
        reach = config.ARM_REACH_MM
        return {
            "x": axes[0] + math.cos(angle) * reach,
            "y": axes[1] + math.sin(angle) * reach,
        }

    def snapshot(self):
        with self.lock:
            axes = self._position_locked()
            offset = self._table_offset()
            if self.carrying:
                tip = self._tip_locked(axes)
                self.plate = {"x": tip["x"] + offset["x"], "y": tip["y"] + offset["y"]}
            return {
                "axes": axes,
                "state": "Cycle" if self.motion is not None else "Idle",
                "bounds": {
                    "x": [0.0, _SETTINGS[130] - _SETTINGS[140]],
                    "y": [0.0, _SETTINGS[131] - _SETTINGS[141]],
                    "z": [0.0, _SETTINGS[132] - _SETTINGS[142]],
                },
                "travel": {"x": _SETTINGS[130], "y": _SETTINGS[131]},
                "table": config.TABLE_SIDE_MM,
                "table_offset": offset,
                "reach": config.ARM_REACH_MM,
                "rotation_offset": config.ROTATION_OFFSET_DEG,
                "scene_extent": config.TABLE_SIDE_MM + config.ARM_REACH_MM,
                "side_tables": [
                    {
                        "name": "PICKUP TABLE",
                        "direction": "+Y",
                        "x": (config.TABLE_SIDE_MM - config.ARM_REACH_MM) / 2.0,
                        "y": config.TABLE_SIDE_MM,
                        "width": config.ARM_REACH_MM,
                        "height": config.ARM_REACH_MM,
                    },
                    {
                        "name": "DESTINATION TABLE",
                        "direction": "+X",
                        "x": config.TABLE_SIDE_MM,
                        "y": (config.TABLE_SIDE_MM - config.ARM_REACH_MM) / 2.0,
                        "width": config.ARM_REACH_MM,
                        "height": config.ARM_REACH_MM,
                    },
                ],
                "pullback": {"x": _SETTINGS[140], "y": _SETTINGS[141]},
                "plate": dict(self.plate),
                "carrying": self.carrying,
                "dropoff": self.dropoff,
                "last_command": self.last_command,
                "demo_running": self.demo_running,
                "events": list(self.events[-8:]),
            }

    @staticmethod
    def _table_offset():
        return {
            "x": config.PLACEMENT_OFFSET_SIDE3_MM,
            "y": config.PLACEMENT_OFFSET_SIDE4_MM,
        }

    def _event_locked(self, label):
        self.events.append({"time": time.strftime("%H:%M:%S"), "label": label})
        self.events = self.events[-20:]

    def command(self, line):
        line = line.strip()
        with self.lock:
            axes = self._position_locked()
            self.last_command = line
            if line == "$H":
                self.axes = [0.0, 0.0, 0.0, 0.0]
                self.motion = None
                self.carrying = False
                self.plate = {
                    "x": config.TABLE_SIDE_MM / 2.0,
                    "y": config.TABLE_SIDE_MM + config.ARM_REACH_MM / 2.0,
                }
                self.dropoff = None
                self._event_locked("Homing cycle")
                return ["ok"]
            if line in ("$X", "G21 G90", "G21", "G90"):
                if line == "$X":
                    self._event_locked("Alarm unlocked")
                return ["ok"]
            if line == "$$":
                return [f"${number}={value:g}" for number, value in _SETTINGS.items()] + ["ok"]

            words = re.findall(r"([XYZAF])([-+]?(?:\d+\.?\d*|\.\d+))", line.upper())
            if not words:
                return ["ok"]
            target = list(axes)
            indexes = {"X": 0, "Y": 1, "Z": 2, "A": 3}
            feed = config.DEFAULT_FEED_RATE_XY
            for axis, raw_value in words:
                value = float(raw_value)
                if axis == "F":
                    feed = max(value, 1.0)
                    continue
                index = indexes[axis]
                target[index] = value
                lower, upper = (0.0, _SETTINGS[133]) if axis == "A" else (0.0, _SETTINGS[{"X": 130, "Y": 131, "Z": 132}[axis]])
                if value < lower or value > upper:
                    return [f"error: target {axis}{value:g} outside simulator travel"]

            distance = max(abs(a - b) for a, b in zip(axes, target))
            duration = max(0.22, min(1.2, distance / feed * 60.0))
            if distance:
                self.motion = {
                    "start": axes,
                    "target": target,
                    "started": time.monotonic(),
                    "duration": duration,
                }
                self._event_locked(f"Move {line}")
            return ["ok"]

    def set_transport(self, pickup, dropoff):
        with self.lock:
            offset = self._table_offset()
            self.plate = {"x": float(pickup[0]) + offset["x"], "y": float(pickup[1]) + offset["y"]}
            self.dropoff = {"x": float(dropoff[0]) + offset["x"], "y": float(dropoff[1]) + offset["y"]}
            self.carrying = False
            self._event_locked("Plate staged at pickup")

    def set_gripper(self, closed):
        with self.lock:
            axes = self._position_locked()
            if closed:
                self.carrying = True
                self._event_locked("Gripper closed · plate secured")
            else:
                if self.carrying:
                    tip = self._tip_locked(axes)
                    offset = self._table_offset()
                    self.plate = self.dropoff or {
                        "x": tip["x"] + offset["x"],
                        "y": tip["y"] + offset["y"],
                    }
                self.carrying = False
                self._event_locked("Gripper opened · plate placed")

    def run_demo(self, simulator_url, from_side, to_side, pick_xyz, place_xyz):
        with self.lock:
            if self.demo_running:
                return False
            self.demo_running = True

        def sequence():
            try:
                controller = _attach_controller(simulator_url)
                if controller.orientation is None:
                    controller.home()
                controller.transport(
                    from_orientation=from_side,
                    to_orientation=to_side,
                    pick_xyz=pick_xyz,
                    place_xyz=place_xyz,
                )
            except Exception as error:
                with self.lock:
                    self._event_locked(f"Transfer failed: {error}")
            finally:
                with self.lock:
                    self.demo_running = False

        threading.Thread(target=sequence, daemon=True).start()
        return True

    def _wait_motion(self):
        while True:
            with self.lock:
                self._position_locked()
                if self.motion is None:
                    return
            time.sleep(0.025)


_STATE = SimulatorState()
_SERVER = None
_SERVER_THREAD = None
_SERVER_LOCK = threading.Lock()


_CONTROLLER = None


def _attach_controller(simulator_url):
    """Return the shared controller so orientation state persists across requests."""
    global _CONTROLLER
    from ..robot import RobotController

    with _SERVER_LOCK:
        if _CONTROLLER is None:
            controller = RobotController(
                sim=True,
                simulator_url=simulator_url,
                open_simulator=False,
            )
            controller.link.connect()
            controller._load_machine_configuration()
            controller._update_machine_position()
            _CONTROLLER = controller
        return _CONTROLLER


class _Handler(BaseHTTPRequestHandler):
    def log_message(self, format_string, *args):
        return

    def _json(self, payload, status=200):
        data = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.end_headers()

    def _simulator_url(self) -> str:
        server = cast(_ReusableHTTPServer, self.server)
        host = server.server_name
        port = server.server_port
        if host in ("0.0.0.0", "::"):
            host = "127.0.0.1"
        return f"http://{host}:{port}"

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            try:
                content = _PAGE.read_bytes()
            except OSError:
                self.send_error(404, "simulator.html is missing")
                return
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(content)))
            self.end_headers()
            self.wfile.write(content)
        elif self.path == "/api/state":
            self._json(_STATE.snapshot())
        elif self.path == "/api/settings":
            self._json({"settings": _SETTINGS})
        elif self.path == "/api/ping":
            self._json({"ok": True})
        else:
            self.send_error(404)

    def do_POST(self):
        try:
            length = int(self.headers.get("Content-Length", "0"))
            payload: dict[str, Any] = json.loads(self.rfile.read(length) or b"{}")
            if self.path == "/api/command":
                responses = _STATE.command(payload.get("line", ""))
                error = next((row for row in responses if row.startswith("error")), None)
                self._json({"responses": responses}, 400 if error else 200)
            elif self.path == "/api/gripper":
                _STATE.set_gripper(bool(payload.get("closed")))
                self._json({"ok": True})
            elif self.path == "/api/transport":
                _STATE.set_transport(payload["pickup"], payload["dropoff"])
                self._json({"ok": True})
            elif self.path == "/api/demo":
                def xyz(key):
                    row = payload[key]
                    return (float(row["x"]), float(row["y"]), float(row["z"]))

                self._json(
                    {
                        "started": _STATE.run_demo(
                            self._simulator_url(),
                            int(payload["from_side"]),
                            int(payload["to_side"]),
                            xyz("pick"),
                            xyz("place"),
                        )
                    }
                )
            elif self.path == "/api/control":
                controller = _attach_controller(self._simulator_url())
                action = payload.get("action")
                if action == "home":
                    controller.home()
                elif action == "orient":
                    controller.set_orientation(int(payload["side"]))
                elif action == "move_xy":
                    axis_bounds = controller.axis_bounds
                    if axis_bounds is None:
                        raise RuntimeError("Controller travel bounds are unavailable")
                    controller.move_machine_to(z=axis_bounds["z"][1])
                    controller.move_machine_to(
                        x=float(payload["x"]),
                        y=float(payload["y"]),
                    )
                elif action == "move_table":
                    controller.set_orientation(int(payload["side"]))
                    controller.move_to(x=float(payload["x"]), y=float(payload["y"]))
                else:
                    raise ValueError(f"Unknown controller action {action!r}")
                self._json({"ok": True, "status": controller.link.get_status()})
            else:
                self.send_error(404)
        except (KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
            self._json({"error": str(error)}, 400)


class _ReusableHTTPServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def serve(host="127.0.0.1", port=8765, open_browser=False):
    server = _ReusableHTTPServer((host, port), _Handler)
    address = f"http://{host}:{server.server_port}"
    if open_browser:
        threading.Timer(0.4, webbrowser.open, args=(address,)).start()
    print(f"Robot simulator ready at {address}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


def _start_embedded(url):
    global _SERVER, _SERVER_THREAD
    from urllib.parse import urlparse

    parsed = urlparse(url)
    host = parsed.hostname or "127.0.0.1"
    port = parsed.port or 80
    with _SERVER_LOCK:
        if _SERVER is None:
            _SERVER = _ReusableHTTPServer((host, port), _Handler)
            _SERVER_THREAD = threading.Thread(target=_SERVER.serve_forever, daemon=True)
            _SERVER_THREAD.start()


class SimulatorLink:
    """HTTP-backed link matching the controller's serial-link API."""

    def __init__(self, url: Optional[str] = DEFAULT_URL, open_browser=True, timeout=5.0):
        self.url = (url or DEFAULT_URL).rstrip("/")
        self.open_browser = open_browser
        self.timeout = timeout
        self.connected = False

    def _request(self, path, payload=None):
        data = None if payload is None else json.dumps(payload).encode("utf-8")
        request = Request(
            self.url + path,
            data=data,
            headers={"Content-Type": "application/json"},
            method="GET" if data is None else "POST",
        )
        try:
            with urlopen(request, timeout=self.timeout) as response:
                return json.loads(response.read().decode("utf-8"))
        except HTTPError as error:
            message = error.read().decode("utf-8", errors="replace")
            raise CrptError(f"Simulator error: {message}") from error
        except (URLError, TimeoutError) as error:
            raise CrptError(f"Cannot reach simulator at {self.url}: {error}") from error

    def connect(self):
        try:
            self._request("/api/ping")
        except CrptError:
            _start_embedded(self.url)
            deadline = time.monotonic() + 5.0
            while True:
                try:
                    self._request("/api/ping")
                    break
                except CrptError:
                    if time.monotonic() >= deadline:
                        raise
                    time.sleep(0.05)
        self.connected = True
        if self.open_browser:
            webbrowser.open(self.url)
        return self

    def close(self):
        self.connected = False

    def send_line(self, line, wait_ok=True):
        if not self.connected:
            raise CrptError("Simulator link is not connected")
        result = self._request("/api/command", {"line": line})
        return result["responses"] if wait_ok else []

    def read_settings(self):
        return {int(number): float(value) for number, value in self._request("/api/settings")["settings"].items()}

    def get_status(self):
        snapshot = self._request("/api/state")
        mpos = ",".join(f"{value:.3f}" for value in snapshot["axes"])
        return {"state": snapshot["state"], "MPos": mpos}

    def home(self):
        return self.send_line("$H")

    def unlock(self):
        return self.send_line("$X")

    def wait_idle(self, poll_interval=0.05, timeout=60.0):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            status = self.get_status()
            if status["state"] == "Idle":
                return status
            time.sleep(poll_interval)
        raise TimeoutError("Timed out waiting for simulator to report Idle")

    def prepare_transport(self, pick_xy, place_xy):
        self._request("/api/transport", {"pickup": pick_xy, "dropoff": place_xy})

    def gripper_close(self):
        self._request("/api/gripper", {"closed": True})

    def gripper_open(self):
        self._request("/api/gripper", {"closed": False})


def main(argv=None):
    parser = argparse.ArgumentParser(description="Run the CRPT top-down simulator")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args(argv)
    serve(host=args.host, port=args.port, open_browser=not args.no_browser)


if __name__ == "__main__":
    main()
