#!/usr/bin/env python3
"""
Simple Serial-to-Network multiplexer with a tiny GUI.

- User selects a serial port and presses Connect.
- The app opens serial at 115200 baud.
- A TCP server listens on port 5522.
- Each client gets line-buffered command input, so mixed partial lines
  from multiple clients cannot interfere.
- Only one command is in-flight on the serial device at a time.
- The response is routed back only to the client that sent the command.
- End of response is detected when a received line ends with " >".
- If the serial device disconnects, the app keeps the TCP server alive,
    waits for the same serial port to reappear, and reconnects automatically.
- While serial is offline, each incoming command is answered with
    "<device not connected>".

Protocol for TCP clients:
- Send commands terminated by '\n'.
- Receive raw response bytes for the respective command.
"""

from __future__ import annotations

import queue
import re
import socket
import threading
import time
import tkinter as tk
from dataclasses import dataclass
from datetime import datetime
from tkinter import scrolledtext
from tkinter import messagebox, ttk

import serial
from serial.tools import list_ports

SERVER_HOST = "0.0.0.0"
SERVER_PORT = 8822
BAUDRATE = 115200
READ_CHUNK_SIZE = 1024
RECONNECT_INTERVAL_S = 1.0
OFFLINE_RESPONSE = b"<device not connected>\r\n"


@dataclass
class CommandJob:
    client: socket.socket
    payload: bytes


class SerialNetworkBridge:
    def __init__(self, port_name: str, log_fn=None):
        self.port_name = port_name
        self.log_fn = log_fn
        self.serial_port: serial.Serial | None = None

        self.server_socket: socket.socket | None = None
        self.clients: set[socket.socket] = set()
        self.client_buffers: dict[socket.socket, bytearray] = {}

        self.command_queue: queue.Queue[CommandJob] = queue.Queue()
        self.active_job: CommandJob | None = None
        self.active_response = bytearray()

        self._serial_reader_thread: threading.Thread | None = None
        self._serial_manager_thread: threading.Thread | None = None
        self._accept_thread: threading.Thread | None = None
        self._dispatcher_thread: threading.Thread | None = None
        self._stop_event = threading.Event()
        self._lock = threading.Lock()
        self._serial_lock = threading.Lock()

    def _log(self, message: str) -> None:
        line = f"[{datetime.now().strftime('%H:%M:%S')}] {message}"
        print(line)
        if self.log_fn is not None:
            self.log_fn(line)

    @staticmethod
    def _client_name(client: socket.socket) -> str:
        try:
            host, port = client.getpeername()
            return f"{host}:{port}"
        except OSError:
            return "<disconnected-client>"

    @staticmethod
    def _preview_bytes(data: bytes, limit: int = 200) -> str:
        text = data.decode("utf-8", errors="replace")
        text = text.replace("\r", "\\r").replace("\n", "\\n")
        if len(text) > limit:
            return text[:limit] + "..."
        return text

    @staticmethod
    def _is_response_complete(buffer: bytes) -> bool:
        # Detect a prompt at end of stream, e.g. "root >" or "root > ",
        # even when there is no trailing newline after the prompt.
        text = buffer.decode("utf-8", errors="ignore").replace("\r", "")
        return re.search(r"(?:^|\n)[^\n]*\s>\s?$", text) is not None

    def start(self) -> None:
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((SERVER_HOST, SERVER_PORT))
        self.server_socket.listen(8)
        self.server_socket.settimeout(0.5)

        self._stop_event.clear()

        self._serial_manager_thread = threading.Thread(target=self._serial_manager_loop, daemon=True)
        self._serial_reader_thread = threading.Thread(target=self._serial_reader_loop, daemon=True)
        self._accept_thread = threading.Thread(target=self._accept_loop, daemon=True)
        self._dispatcher_thread = threading.Thread(target=self._dispatch_loop, daemon=True)

        self._serial_manager_thread.start()
        self._serial_reader_thread.start()
        self._accept_thread.start()
        self._dispatcher_thread.start()
        self._log(f"Server listening on {SERVER_HOST}:{SERVER_PORT}, selected serial port {self.port_name}")

    def is_serial_connected(self) -> bool:
        with self._serial_lock:
            return self.serial_port is not None

    def stop(self) -> None:
        self._stop_event.set()
        self._log("Stopping server")

        if self.server_socket is not None:
            try:
                self.server_socket.close()
            except OSError:
                pass
            self.server_socket = None

        with self._lock:
            clients = list(self.clients)

        for client in clients:
            self._remove_client(client)

        self._mark_serial_disconnected()

    def _serial_manager_loop(self) -> None:
        while not self._stop_event.is_set():
            with self._serial_lock:
                already_connected = self.serial_port is not None

            if already_connected:
                time.sleep(RECONNECT_INTERVAL_S)
                continue

            try:
                new_port = serial.Serial(self.port_name, BAUDRATE, timeout=0.1)
            except (serial.SerialException, OSError):
                time.sleep(RECONNECT_INTERVAL_S)
                continue

            with self._serial_lock:
                if self._stop_event.is_set():
                    try:
                        new_port.close()
                    except OSError:
                        pass
                    return
                self.serial_port = new_port
            self._log(f"Serial connected: {self.port_name}")

    def _accept_loop(self) -> None:
        assert self.server_socket is not None
        while not self._stop_event.is_set():
            try:
                client, _addr = self.server_socket.accept()
            except socket.timeout:
                continue
            except OSError:
                break

            client.settimeout(0.5)
            with self._lock:
                self.clients.add(client)
                self.client_buffers[client] = bytearray()
            self._log(f"Client connected: {self._client_name(client)}")

            threading.Thread(target=self._client_loop, args=(client,), daemon=True).start()

    def _client_loop(self, client: socket.socket) -> None:
        while not self._stop_event.is_set():
            try:
                data = client.recv(READ_CHUNK_SIZE)
            except socket.timeout:
                continue
            except (ConnectionError, OSError):
                break

            if not data:
                break

            jobs = []
            with self._lock:
                buffer = self.client_buffers.get(client)
                if buffer is None:
                    break

                buffer.extend(data)
                while True:
                    pos = buffer.find(b"\n")
                    if pos < 0:
                        break
                    line = bytes(buffer[: pos + 1])
                    del buffer[: pos + 1]
                    if line.strip(b"\r\n"):
                        self._log(
                            f"JOB queued from {self._client_name(client)}: {self._preview_bytes(line)}"
                        )
                        jobs.append(CommandJob(client=client, payload=line))

            for job in jobs:
                self.command_queue.put(job)

        self._remove_client(client)

    def _dispatch_loop(self) -> None:
        while not self._stop_event.is_set():
            if self.active_job is not None:
                self._stop_event.wait(0.01)
                continue

            try:
                job = self.command_queue.get(timeout=0.2)
            except queue.Empty:
                continue

            # Skip jobs from clients that are already gone.
            with self._lock:
                if job.client not in self.clients:
                    continue

            with self._serial_lock:
                current_port = self.serial_port

            if current_port is None:
                self._send_offline_response(job.client)
                continue

            try:
                self._log(
                    f"TX -> device (from {self._client_name(job.client)}): {self._preview_bytes(job.payload)}"
                )
                current_port.write(job.payload)
                current_port.flush()
            except (serial.SerialException, OSError):
                self._mark_serial_disconnected()
                self._send_offline_response(job.client)
                continue

            self.active_job = job
            self.active_response = bytearray()

    def _serial_reader_loop(self) -> None:
        while not self._stop_event.is_set():
            with self._serial_lock:
                current_port = self.serial_port

            if current_port is None:
                time.sleep(0.05)
                continue

            try:
                incoming = current_port.read(READ_CHUNK_SIZE)
            except (serial.SerialException, OSError):
                self._mark_serial_disconnected()
                continue

            if not incoming:
                continue

            if self.active_job is None:
                # Ignore unsolicited device messages when no command is active.
                continue

            self.active_response.extend(incoming)
            if self._is_response_complete(bytes(self.active_response)):
                self._finish_active_response()

    def _send_offline_response(self, client: socket.socket) -> None:
        try:
            client.sendall(OFFLINE_RESPONSE)
            self._log(f"TX -> {self._client_name(client)}: {self._preview_bytes(OFFLINE_RESPONSE)}")
        except (ConnectionError, OSError):
            self._remove_client(client)

    def _finish_active_response(self) -> None:
        job = self.active_job
        if job is None:
            return

        response = bytes(self.active_response)
        self.active_job = None
        self.active_response = bytearray()

        try:
            job.client.sendall(response)
            self._log(
                f"RX <- device response to {self._client_name(job.client)}: {self._preview_bytes(response)}"
            )
        except (ConnectionError, OSError):
            self._remove_client(job.client)

    def _mark_serial_disconnected(self) -> None:
        with self._serial_lock:
            port = self.serial_port
            self.serial_port = None

        if port is not None:
            try:
                port.close()
            except OSError:
                pass
            self._log("Serial disconnected")

        # If a command was in-flight during disconnect, fail it immediately.
        active = self.active_job
        self.active_job = None
        self.active_response = bytearray()
        if active is not None:
            self._send_offline_response(active.client)

    def _remove_client(self, client: socket.socket) -> None:
        with self._lock:
            existed = client in self.clients
            if existed:
                self.clients.remove(client)
            self.client_buffers.pop(client, None)

        try:
            client.close()
        except OSError:
            pass

        if existed:
            self._log(f"Client disconnected: {self._client_name(client)}")

        if not existed:
            return

        # Drop pending queue items for this client by rebuilding the queue.
        retained: list[CommandJob] = []
        while True:
            try:
                item = self.command_queue.get_nowait()
            except queue.Empty:
                break
            if item.client is not client:
                retained.append(item)

        for item in retained:
            self.command_queue.put(item)

        # If this client had the in-flight command, drop it and continue.
        if self.active_job is not None and self.active_job.client is client:
            self.active_job = None
            self.active_response = bytearray()


class App:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Serial Network Server")
        self.root.resizable(True, True)

        self.bridge: SerialNetworkBridge | None = None
        self.log_queue: queue.Queue[str] = queue.Queue()

        frame = ttk.Frame(root, padding=12)
        frame.grid(row=0, column=0, sticky="nsew")

        ttk.Label(frame, text="Serial Port:").grid(row=0, column=0, sticky="w")

        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(frame, textvariable=self.port_var, width=30, state="readonly")
        self.port_combo.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(4, 8))

        self.refresh_button = ttk.Button(frame, text="Refresh", command=self.refresh_ports)
        self.refresh_button.grid(row=2, column=0, sticky="ew")

        self.connect_button = ttk.Button(frame, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=2, column=1, sticky="ew", padx=(8, 0))

        self.status_var = tk.StringVar(value="Stopped")
        ttk.Label(frame, textvariable=self.status_var).grid(row=3, column=0, columnspan=2, sticky="w", pady=(10, 0))
        ttk.Label(frame, text=f"Server: {SERVER_HOST}:{SERVER_PORT}").grid(
            row=4, column=0, columnspan=2, sticky="w", pady=(2, 0)
        )

        ttk.Label(frame, text="Communication Log:").grid(row=5, column=0, columnspan=2, sticky="w", pady=(10, 4))
        self.log_text = scrolledtext.ScrolledText(frame, height=14, width=90, state="disabled", wrap="word")
        self.log_text.grid(row=6, column=0, columnspan=2, sticky="nsew")

        frame.columnconfigure(0, weight=1)
        frame.columnconfigure(1, weight=1)
        frame.rowconfigure(6, weight=1)

        self.refresh_ports()
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.root.after(300, self._update_runtime_status)
        self.root.after(120, self._drain_log_queue)

    def refresh_ports(self) -> None:
        ports = [p.device for p in list_ports.comports()]
        self.port_combo["values"] = ports

        if ports:
            if self.port_var.get() not in ports:
                self.port_var.set(ports[0])
        else:
            self.port_var.set("")

    def toggle_connection(self) -> None:
        if self.bridge is None:
            self._start_bridge()
        else:
            self._stop_bridge()

    def _start_bridge(self) -> None:
        selected_port = self.port_var.get().strip()
        if not selected_port:
            messagebox.showerror("No serial port", "Please select a serial port.")
            return

        bridge = SerialNetworkBridge(selected_port, log_fn=self.enqueue_log)
        try:
            bridge.start()
        except Exception as exc:
            messagebox.showerror("Failed to start", str(exc))
            return

        self.bridge = bridge
        self.status_var.set(f"Running on {selected_port} @ {BAUDRATE} | Serial: connecting...")
        self.connect_button.configure(text="Disconnect")
        self.port_combo.configure(state="disabled")
        self.refresh_button.configure(state="disabled")

    def _stop_bridge(self) -> None:
        assert self.bridge is not None
        self.bridge.stop()
        self.bridge = None

        self.status_var.set("Stopped")
        self.connect_button.configure(text="Connect")
        self.port_combo.configure(state="readonly")
        self.refresh_button.configure(state="normal")

    def on_close(self) -> None:
        if self.bridge is not None:
            self.bridge.stop()
        self.root.destroy()

    def _update_runtime_status(self) -> None:
        if self.bridge is not None:
            serial_state = "connected" if self.bridge.is_serial_connected() else "disconnected"
            self.status_var.set(f"Running on {self.bridge.port_name} @ {BAUDRATE} | Serial: {serial_state}")
        self.root.after(300, self._update_runtime_status)

    def enqueue_log(self, message: str) -> None:
        self.log_queue.put(message)

    def _append_log_ui(self, line: str) -> None:
        self.log_text.configure(state="normal")
        self.log_text.insert("end", line + "\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def _drain_log_queue(self) -> None:
        for _ in range(200):
            try:
                line = self.log_queue.get_nowait()
            except queue.Empty:
                break
            self._append_log_ui(line)
        self.root.after(120, self._drain_log_queue)


def main() -> None:
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
