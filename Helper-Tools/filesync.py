#!/usr/bin/env python3
"""GUI helper for interacting with LED-Controller-OS over serial."""

from __future__ import annotations

import queue
import re
import threading
import time
import importlib
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Optional

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from tkinter.scrolledtext import ScrolledText

try:
	serial = importlib.import_module("serial")
	list_ports = importlib.import_module("serial.tools.list_ports")
	SerialException = getattr(serial, "SerialException", Exception)
except Exception as exc:  # pragma: no cover - runtime dependency guard
	serial = None
	list_ports = None
	SerialException = Exception
	IMPORT_ERROR = exc
else:
	IMPORT_ERROR = None


DEFAULT_BAUD = 115200

DIR_HEADER_RE = re.compile(r"^\s*Directory of\s+(.+?)\s*$")
DIR_ENTRY_RE = re.compile(r"^\s*<DIR>\s+(.+?)\s+\((\d+)\s+entries\)\s*$")
FILE_ENTRY_RE = re.compile(r"^\s*-\s+v(\d+)\s+-\s+(.+?)\s+\((\d+)\s+bytes\)\s*$")
HEX_LINE_RE = re.compile(r"^[0-9a-fA-F]{2}$")


def escape_device_text(value: str) -> str:
	"""Escape for this console parser so $, ", and \\ stay literal."""
	return value.replace("\\", "\\\\").replace('"', '\\"').replace("$", "\\$")


def quote_device_arg(value: str) -> str:
	return f'"{escape_device_text(value)}"'


@dataclass
class DeviceEntry:
	name: str
	is_dir: bool
	version: Optional[int] = None
	size: Optional[int] = None
	info: str = ""


def parse_device_dir_output(text: str) -> tuple[Optional[str], list[DeviceEntry]]:
	current_path: Optional[str] = None
	entries: list[DeviceEntry] = []

	for raw_line in text.replace("\r", "").split("\n"):
		line = raw_line.strip()
		if not line:
			continue
		if line == "dir" or line.endswith(" >"):
			continue

		header_match = DIR_HEADER_RE.match(line)
		if header_match:
			current_path = header_match.group(1)
			continue

		if line.startswith("Total Files:") or line.startswith("No filesystem loaded"):
			continue

		dir_match = DIR_ENTRY_RE.match(line)
		if dir_match:
			name = dir_match.group(1)
			count = dir_match.group(2)
			entries.append(DeviceEntry(name=name, is_dir=True, info=f"{count} entries"))
			continue

		file_match = FILE_ENTRY_RE.match(line)
		if file_match:
			version = int(file_match.group(1))
			name = file_match.group(2)
			size = int(file_match.group(3))
			entries.append(
				DeviceEntry(
					name=name,
					is_dir=False,
					version=version,
					size=size,
					info=f"v{version}, {size} bytes",
				)
			)

	return current_path, entries


def parse_hex_bytes(text: str) -> bytes:
	out = bytearray()
	for raw_line in text.replace("\r", "").split("\n"):
		tokens = raw_line.strip().split()
		if not tokens:
			continue
		if all(HEX_LINE_RE.match(token) for token in tokens):
			out.extend(int(token, 16) for token in tokens)
	return bytes(out)


class SerialSession:
	def __init__(self, on_chunk: Callable[[str], None]):
		self._on_chunk = on_chunk
		self._serial: Optional[Any] = None
		self._thread: Optional[threading.Thread] = None
		self._running = threading.Event()
		self._write_lock = threading.Lock()

		self._capture_lock = threading.Lock()
		self._capture_active = False
		self._capture_chunks: list[str] = []
		self._capture_last_rx = 0.0

	def connect(self, port: str, baud: int) -> None:
		if serial is None:
			raise RuntimeError(f"pyserial import failed: {IMPORT_ERROR}")
		self.disconnect()
		self._serial = serial.Serial(port=port, baudrate=baud, timeout=0.1)
		self._running.set()
		self._thread = threading.Thread(target=self._reader_loop, daemon=True)
		self._thread.start()

	def disconnect(self) -> None:
		self._running.clear()
		if self._serial is not None:
			try:
				self._serial.close()
			except Exception:
				pass
		if self._thread and self._thread.is_alive():
			self._thread.join(timeout=0.5)
		self._thread = None
		self._serial = None

	def is_connected(self) -> bool:
		return self._serial is not None and self._serial.is_open

	def send_command(self, command: str) -> None:
		if not self.is_connected():
			raise RuntimeError("Not connected")
		with self._write_lock:
			assert self._serial is not None
			self._serial.write((command + "\r").encode("utf-8"))

	def execute_and_capture(self, command: str, timeout: float = 3.0, quiet: float = 0.30) -> str:
		if not self.is_connected():
			raise RuntimeError("Not connected")

		with self._capture_lock:
			self._capture_active = True
			self._capture_chunks = []
			self._capture_last_rx = 0.0

		self.send_command(command)
		start = time.monotonic()
		while time.monotonic() - start < timeout:
			time.sleep(0.05)
			with self._capture_lock:
				if self._capture_chunks and (time.monotonic() - self._capture_last_rx) >= quiet:
					break

		with self._capture_lock:
			out = "".join(self._capture_chunks)
			self._capture_active = False
			self._capture_chunks = []
			self._capture_last_rx = 0.0
		return out

	def _reader_loop(self) -> None:
		while self._running.is_set():
			try:
				if self._serial is None:
					break
				data = self._serial.read(256)
			except SerialException as exc:
				self._on_chunk(f"\n[Serial error] {exc}\n")
				break
			except Exception as exc:
				self._on_chunk(f"\n[Reader error] {exc}\n")
				break

			if not data:
				continue

			chunk = data.decode("utf-8", errors="replace")
			self._on_chunk(chunk)
			with self._capture_lock:
				if self._capture_active:
					self._capture_chunks.append(chunk)
					self._capture_last_rx = time.monotonic()


class FileSyncApp(tk.Tk):
	def __init__(self) -> None:
		super().__init__()
		self.title("LED Controller Serial File Browser")
		self.geometry("1280x820")

		self.console_queue: queue.Queue[str] = queue.Queue()
		self.session = SerialSession(on_chunk=self.console_queue.put)

		self.port_var = tk.StringVar()
		self.baud_var = tk.StringVar(value=str(DEFAULT_BAUD))
		self.command_var = tk.StringVar()
		self.status_var = tk.StringVar(value="Disconnected")
		self.device_path_var = tk.StringVar(value="(unknown)")
		self.download_version_var = tk.StringVar(value="0")

		self.local_cwd = Path.cwd()
		self._device_entries_by_iid: dict[str, DeviceEntry] = {}

		self._build_ui()
		self.refresh_ports()
		self.refresh_local_tree()

		self.after(50, self._poll_console_queue)
		self.protocol("WM_DELETE_WINDOW", self._on_close)

		if IMPORT_ERROR is not None:
			messagebox.showerror(
				"Missing dependency",
				"pyserial is required. Install with:\n\n"
				"  pip install pyserial\n\n"
				f"Import error: {IMPORT_ERROR}",
			)

	def _build_ui(self) -> None:
		root = ttk.Frame(self, padding=8)
		root.pack(fill=tk.BOTH, expand=True)

		top = ttk.Frame(root)
		top.pack(fill=tk.X)

		ttk.Label(top, text="Port:").pack(side=tk.LEFT)
		self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=22, state="readonly")
		self.port_combo.pack(side=tk.LEFT, padx=(4, 6))
		ttk.Button(top, text="Refresh Ports", command=self.refresh_ports).pack(side=tk.LEFT, padx=(0, 6))

		ttk.Label(top, text="Baud:").pack(side=tk.LEFT)
		ttk.Entry(top, textvariable=self.baud_var, width=10).pack(side=tk.LEFT, padx=(4, 6))

		self.connect_btn = ttk.Button(top, text="Connect", command=self.toggle_connection)
		self.connect_btn.pack(side=tk.LEFT, padx=(0, 12))

		ttk.Label(top, text="Status:").pack(side=tk.LEFT)
		ttk.Label(top, textvariable=self.status_var).pack(side=tk.LEFT)

		cmd_frame = ttk.Frame(root)
		cmd_frame.pack(fill=tk.X, pady=(8, 4))
		ttk.Label(cmd_frame, text="Console Command:").pack(side=tk.LEFT)
		cmd_entry = ttk.Entry(cmd_frame, textvariable=self.command_var)
		cmd_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=6)
		cmd_entry.bind("<Return>", lambda _event: self.send_manual_command())
		ttk.Button(cmd_frame, text="Send", command=self.send_manual_command).pack(side=tk.LEFT)

		split_v = ttk.Panedwindow(root, orient=tk.VERTICAL)
		split_v.pack(fill=tk.BOTH, expand=True)

		file_area = ttk.Frame(split_v)
		split_v.add(file_area, weight=3)

		console_area = ttk.Labelframe(split_v, text="Device Console")
		split_v.add(console_area, weight=2)

		split_h = ttk.Panedwindow(file_area, orient=tk.HORIZONTAL)
		split_h.pack(fill=tk.BOTH, expand=True)

		local_panel = ttk.Labelframe(split_h, text="Local Files")
		transfer_panel = ttk.Labelframe(split_h, text="Transfer")
		device_panel = ttk.Labelframe(split_h, text="Device Files")

		split_h.add(local_panel, weight=4)
		split_h.add(transfer_panel, weight=2)
		split_h.add(device_panel, weight=4)

		self._build_local_panel(local_panel)
		self._build_transfer_panel(transfer_panel)
		self._build_device_panel(device_panel)

		self.console = ScrolledText(console_area, wrap=tk.WORD, height=12)
		self.console.pack(fill=tk.BOTH, expand=True)
		self.console.configure(state=tk.DISABLED)

	def _build_local_panel(self, panel: ttk.Labelframe) -> None:
		path_row = ttk.Frame(panel)
		path_row.pack(fill=tk.X, padx=6, pady=6)
		ttk.Label(path_row, text="Path:").pack(side=tk.LEFT)
		self.local_path_label = ttk.Label(path_row, text=str(self.local_cwd))
		self.local_path_label.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=6)

		button_row = ttk.Frame(panel)
		button_row.pack(fill=tk.X, padx=6)
		ttk.Button(button_row, text="Up", command=self.local_up).pack(side=tk.LEFT, padx=(0, 4))
		ttk.Button(button_row, text="Refresh", command=self.refresh_local_tree).pack(side=tk.LEFT, padx=(0, 4))
		ttk.Button(button_row, text="Choose Folder", command=self.choose_local_folder).pack(side=tk.LEFT)

		self.local_tree = ttk.Treeview(
			panel,
			columns=("type", "size", "path"),
			show="headings",
			selectmode="browse",
		)
		self.local_tree.heading("type", text="Type")
		self.local_tree.heading("size", text="Size")
		self.local_tree.heading("path", text="Name")
		self.local_tree.column("type", width=70, anchor=tk.W)
		self.local_tree.column("size", width=90, anchor=tk.E)
		self.local_tree.column("path", width=320, anchor=tk.W)
		self.local_tree.pack(fill=tk.BOTH, expand=True, padx=6, pady=(6, 6))
		self.local_tree.bind("<Double-1>", self.local_double_click)

	def _build_transfer_panel(self, panel: ttk.Labelframe) -> None:
		frame = ttk.Frame(panel, padding=8)
		frame.pack(fill=tk.BOTH, expand=True)

		ttk.Button(frame, text="Upload ->", command=self.upload_selected_local).pack(fill=tk.X, pady=(2, 8))

		ttk.Label(frame, text="Download Version Offset").pack(anchor=tk.W)
		ttk.Spinbox(
			frame,
			from_=0,
			to=999,
			textvariable=self.download_version_var,
			width=8,
		).pack(anchor=tk.W, pady=(0, 8))

		ttk.Button(frame, text="<- Download", command=self.download_selected_device).pack(fill=tk.X, pady=(2, 8))

		ttk.Separator(frame).pack(fill=tk.X, pady=8)
		ttk.Label(frame, text=(
			"Upload uses: store --alloc / --append / --finish\n"
			"Escapes: \\\\, \\$, and \\\""
		)).pack(anchor=tk.W)

	def _build_device_panel(self, panel: ttk.Labelframe) -> None:
		path_row = ttk.Frame(panel)
		path_row.pack(fill=tk.X, padx=6, pady=6)
		ttk.Label(path_row, text="Path:").pack(side=tk.LEFT)
		ttk.Label(path_row, textvariable=self.device_path_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=6)

		nav_row = ttk.Frame(panel)
		nav_row.pack(fill=tk.X, padx=6)
		self.device_nav_var = tk.StringVar(value="")
		ttk.Entry(nav_row, textvariable=self.device_nav_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 4))
		ttk.Button(nav_row, text="cd", command=self.device_cd_from_entry).pack(side=tk.LEFT, padx=(0, 4))
		ttk.Button(nav_row, text="md", command=self.device_md_from_entry).pack(side=tk.LEFT)

		button_row = ttk.Frame(panel)
		button_row.pack(fill=tk.X, padx=6, pady=(6, 0))
		ttk.Button(button_row, text="Up", command=self.device_up).pack(side=tk.LEFT, padx=(0, 4))
		ttk.Button(button_row, text="Refresh", command=self.refresh_device_tree).pack(side=tk.LEFT)

		self.device_tree = ttk.Treeview(
			panel,
			columns=("type", "info", "name"),
			show="headings",
			selectmode="browse",
		)
		self.device_tree.heading("type", text="Type")
		self.device_tree.heading("info", text="Info")
		self.device_tree.heading("name", text="Name")
		self.device_tree.column("type", width=70, anchor=tk.W)
		self.device_tree.column("info", width=140, anchor=tk.W)
		self.device_tree.column("name", width=260, anchor=tk.W)
		self.device_tree.pack(fill=tk.BOTH, expand=True, padx=6, pady=(6, 6))
		self.device_tree.bind("<Double-1>", self.device_double_click)

	def append_console(self, text: str) -> None:
		self.console.configure(state=tk.NORMAL)
		self.console.insert(tk.END, text)
		self.console.see(tk.END)
		self.console.configure(state=tk.DISABLED)

	def set_status(self, text: str) -> None:
		self.status_var.set(text)

	def _poll_console_queue(self) -> None:
		try:
			while True:
				chunk = self.console_queue.get_nowait()
				self.append_console(chunk)
		except queue.Empty:
			pass
		self.after(50, self._poll_console_queue)

	def refresh_ports(self) -> None:
		if list_ports is None:
			self.port_combo["values"] = []
			return
		ports = [port.device for port in list_ports.comports()]
		self.port_combo["values"] = ports
		if ports and not self.port_var.get():
			self.port_var.set(ports[0])

	def toggle_connection(self) -> None:
		if self.session.is_connected():
			self.session.disconnect()
			self.set_status("Disconnected")
			self.connect_btn.configure(text="Connect")
			return

		port = self.port_var.get().strip()
		if not port:
			messagebox.showwarning("Missing port", "Please select a serial port.")
			return
		try:
			baud = int(self.baud_var.get())
		except ValueError:
			messagebox.showwarning("Invalid baud", "Baud must be an integer.")
			return

		try:
			self.session.connect(port, baud)
		except Exception as exc:
			messagebox.showerror("Connection failed", str(exc))
			return

		self.set_status(f"Connected to {port} @ {baud}")
		self.connect_btn.configure(text="Disconnect")
		self.refresh_device_tree()

	def send_manual_command(self) -> None:
		command = self.command_var.get().strip()
		if not command:
			return
		try:
			self.session.send_command(command)
		except Exception as exc:
			messagebox.showerror("Send failed", str(exc))
			return
		self.command_var.set("")

	def refresh_local_tree(self) -> None:
		for item in self.local_tree.get_children():
			self.local_tree.delete(item)

		try:
			entries = list(self.local_cwd.iterdir())
		except Exception as exc:
			messagebox.showerror("Local path error", str(exc))
			return

		entries.sort(key=lambda p: (not p.is_dir(), p.name.lower()))
		self.local_path_label.configure(text=str(self.local_cwd))
		for path in entries:
			kind = "DIR" if path.is_dir() else "FILE"
			size = "" if path.is_dir() else str(path.stat().st_size)
			self.local_tree.insert("", tk.END, values=(kind, size, path.name))

	def local_up(self) -> None:
		parent = self.local_cwd.parent
		if parent != self.local_cwd:
			self.local_cwd = parent
			self.refresh_local_tree()

	def choose_local_folder(self) -> None:
		chosen = filedialog.askdirectory(initialdir=str(self.local_cwd))
		if chosen:
			self.local_cwd = Path(chosen)
			self.refresh_local_tree()

	def local_double_click(self, _event: tk.Event) -> None:
		selected = self._selected_local_path()
		if selected and selected.is_dir():
			self.local_cwd = selected
			self.refresh_local_tree()

	def _selected_local_path(self) -> Optional[Path]:
		item = self.local_tree.focus()
		if not item:
			return None
		values = self.local_tree.item(item, "values")
		if not values:
			return None
		return self.local_cwd / values[2]

	def _run_background(self, work: Callable[[], object], on_done: Callable[[object], None]) -> None:
		def runner() -> None:
			try:
				result = work()
			except Exception as exc:
				self.after(0, lambda: messagebox.showerror("Operation failed", str(exc)))
				return
			self.after(0, lambda: on_done(result))

		threading.Thread(target=runner, daemon=True).start()

	def refresh_device_tree(self) -> None:
		if not self.session.is_connected():
			return

		self.set_status("Refreshing device directory...")

		def work() -> str:
			return self.session.execute_and_capture("dir", timeout=2.5, quiet=0.30)

		def done(output: object) -> None:
			text = str(output)
			path, entries = parse_device_dir_output(text)
			if path:
				self.device_path_var.set(path)

			self._device_entries_by_iid.clear()
			for item in self.device_tree.get_children():
				self.device_tree.delete(item)

			for idx, entry in enumerate(entries):
				iid = f"d{idx}"
				self._device_entries_by_iid[iid] = entry
				kind = "DIR" if entry.is_dir else "FILE"
				self.device_tree.insert("", tk.END, iid=iid, values=(kind, entry.info, entry.name))

			if entries:
				self.set_status(f"Loaded {len(entries)} entries from device")
			else:
				self.set_status("Device directory loaded (no entries parsed)")

		self._run_background(work, done)

	def _selected_device_entry(self) -> Optional[DeviceEntry]:
		item = self.device_tree.focus()
		if not item:
			return None
		return self._device_entries_by_iid.get(item)

	def device_double_click(self, _event: tk.Event) -> None:
		entry = self._selected_device_entry()
		if entry and entry.is_dir:
			self._device_cd(entry.name)

	def device_cd_from_entry(self) -> None:
		target = self.device_nav_var.get().strip()
		if not target:
			entry = self._selected_device_entry()
			if entry and entry.is_dir:
				target = entry.name
		if not target:
			messagebox.showwarning("Missing folder", "Enter a folder name or select a device folder.")
			return
		self._device_cd(target)

	def device_up(self) -> None:
		self._device_cd("..")

	def _device_cd(self, target: str) -> None:
		if not self.session.is_connected():
			return
		self.set_status(f"Changing directory to {target}")

		def work() -> str:
			self.session.execute_and_capture(f"cd {quote_device_arg(target)}", timeout=2.0, quiet=0.25)
			return self.session.execute_and_capture("dir", timeout=2.5, quiet=0.30)

		def done(output: object) -> None:
			self.device_nav_var.set("")
			text = str(output)
			path, entries = parse_device_dir_output(text)
			if path:
				self.device_path_var.set(path)

			self._device_entries_by_iid.clear()
			for item in self.device_tree.get_children():
				self.device_tree.delete(item)
			for idx, entry in enumerate(entries):
				iid = f"d{idx}"
				self._device_entries_by_iid[iid] = entry
				kind = "DIR" if entry.is_dir else "FILE"
				self.device_tree.insert("", tk.END, iid=iid, values=(kind, entry.info, entry.name))
			self.set_status("Directory changed")

		self._run_background(work, done)

	def device_md_from_entry(self) -> None:
		name = self.device_nav_var.get().strip()
		if not name:
			messagebox.showwarning("Missing name", "Enter a new directory name.")
			return
		if not self.session.is_connected():
			return

		self.set_status(f"Creating directory {name}")

		def work() -> str:
			self.session.execute_and_capture(f"md {quote_device_arg(name)}", timeout=2.0, quiet=0.25)
			return self.session.execute_and_capture("dir", timeout=2.5, quiet=0.30)

		def done(output: object) -> None:
			self.device_nav_var.set("")
			text = str(output)
			path, entries = parse_device_dir_output(text)
			if path:
				self.device_path_var.set(path)
			self._device_entries_by_iid.clear()
			for item in self.device_tree.get_children():
				self.device_tree.delete(item)
			for idx, entry in enumerate(entries):
				iid = f"d{idx}"
				self._device_entries_by_iid[iid] = entry
				kind = "DIR" if entry.is_dir else "FILE"
				self.device_tree.insert("", tk.END, iid=iid, values=(kind, entry.info, entry.name))
			self.set_status("Directory created")

		self._run_background(work, done)

	def upload_selected_local(self) -> None:
		local_path = self._selected_local_path()
		if not local_path or not local_path.is_file():
			messagebox.showwarning("Select local file", "Select a local file to upload.")
			return
		if not self.session.is_connected():
			messagebox.showwarning("Not connected", "Connect to your device first.")
			return

		remote_name = local_path.name
		self.set_status(f"Uploading {local_path.name} -> {remote_name}")

		def work() -> str:
			raw = local_path.read_bytes()
			try:
				text = raw.decode("utf-8")
			except UnicodeDecodeError as exc:
				raise RuntimeError(
					"Upload currently supports UTF-8 text files. "
					"For binary files, extend this script to use `store --append -hex`."
				) from exc

			alloc_resp = self.session.execute_and_capture(
				f"store {quote_device_arg(remote_name)} --alloc {len(raw)}",
				timeout=2.5,
				quiet=0.25,
			)
			if "Error:" in alloc_resp:
				raise RuntimeError(alloc_resp.strip())

			segments = text.splitlines(keepends=True)
			if not segments and text == "":
				segments = []

			for segment in segments:
				has_newline = segment.endswith("\n")
				content = segment[:-1] if has_newline else segment
				cmd = f"store --append {'-n ' if not has_newline else ''}\"{escape_device_text(content)}\""
				resp = self.session.execute_and_capture(cmd, timeout=1.5, quiet=0.20)
				if "Error:" in resp:
					raise RuntimeError(resp.strip())

			finish_resp = self.session.execute_and_capture("store --finish", timeout=2.0, quiet=0.20)
			if "Error:" in finish_resp:
				raise RuntimeError(finish_resp.strip())
			return finish_resp

		def done(_output: object) -> None:
			self.set_status(f"Upload complete: {remote_name}")
			self.refresh_device_tree()

		self._run_background(work, done)

	def download_selected_device(self) -> None:
		entry = self._selected_device_entry()
		if not entry or entry.is_dir:
			messagebox.showwarning("Select device file", "Select a file in the device panel.")
			return
		if not self.session.is_connected():
			messagebox.showwarning("Not connected", "Connect to your device first.")
			return

		try:
			version_offset = int(self.download_version_var.get().strip())
			if version_offset < 0:
				raise ValueError
		except ValueError:
			messagebox.showwarning("Invalid version", "Version offset must be a number >= 0.")
			return

		save_path = filedialog.asksaveasfilename(
			initialdir=str(self.local_cwd),
			initialfile=entry.name,
			title="Save downloaded device file",
		)
		if not save_path:
			return

		self.set_status(f"Downloading {entry.name}")

		def work() -> bytes:
			cmd_parts = ["cat", "--hex"]
			if version_offset > 0:
				cmd_parts.append(f"-{version_offset}")
			cmd_parts.append(quote_device_arg(entry.name))
			command = " ".join(cmd_parts)
			output = self.session.execute_and_capture(command, timeout=4.0, quiet=0.30)

			data = parse_hex_bytes(output)
			if not data:
				lowered = output.lower()
				known_errors = (
					"file not found",
					"failed to open file version",
					"requested version",
					"invalid version number",
					"no filesystem loaded",
				)
				if any(marker in lowered for marker in known_errors):
					snippet = output.strip()[:200]
					raise RuntimeError(f"Download failed. Device output: {snippet}")
			return data

		def done(output: object) -> None:
			if not isinstance(output, (bytes, bytearray)):
				messagebox.showerror("Download failed", "Internal error: download did not return bytes")
				return
			data = bytes(output)
			Path(save_path).write_bytes(data)
			self.set_status(f"Downloaded {len(data)} bytes to {save_path}")
			self.refresh_local_tree()

		self._run_background(work, done)

	def _on_close(self) -> None:
		self.session.disconnect()
		self.destroy()


def main() -> None:
	app = FileSyncApp()
	app.mainloop()


if __name__ == "__main__":
	main()
