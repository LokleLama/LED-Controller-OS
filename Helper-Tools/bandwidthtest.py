import threading
import time
import tkinter as tk
from tkinter import messagebox, ttk

import serial
from serial.tools import list_ports


PACKET_SIZE = 64
UI_REFRESH_SECONDS = 0.2
SERIAL_TIMEOUT_SECONDS = 0.0
SERIAL_BAUDRATE = 115200
MAX_IN_FLIGHT_BYTES = PACKET_SIZE * 256


class BandwidthTesterApp:
	def __init__(self, root: tk.Tk) -> None:
		self.root = root
		self.root.title("Serial Loopback Bandwidth Tester")
		self.root.geometry("520x260")

		self._build_ui()

		self._stop_event = threading.Event()
		self._worker_thread = None

		self._start_time = 0.0
		self._tx_total_bytes = 0
		self._rx_total_bytes = 0

		self._refresh_ports()
		self._schedule_ui_update()

	def _build_ui(self) -> None:
		main = ttk.Frame(self.root, padding=12)
		main.pack(fill=tk.BOTH, expand=True)

		port_row = ttk.Frame(main)
		port_row.pack(fill=tk.X)

		ttk.Label(port_row, text="Serial Port:").pack(side=tk.LEFT)

		self.port_combo = ttk.Combobox(port_row, state="readonly", width=35)
		self.port_combo.pack(side=tk.LEFT, padx=(8, 8), fill=tk.X, expand=True)

		ttk.Button(port_row, text="Refresh", command=self._refresh_ports).pack(side=tk.LEFT)

		controls = ttk.Frame(main)
		controls.pack(fill=tk.X, pady=(16, 0))

		self.start_button = ttk.Button(controls, text="Start", command=self._start_test)
		self.start_button.pack(side=tk.LEFT)

		self.stop_button = ttk.Button(controls, text="Stop", command=self._stop_test, state=tk.DISABLED)
		self.stop_button.pack(side=tk.LEFT, padx=(8, 0))

		stats = ttk.LabelFrame(main, text="Throughput", padding=12)
		stats.pack(fill=tk.BOTH, expand=True, pady=(16, 0))

		self.status_var = tk.StringVar(value="Idle")
		self.tx_total_var = tk.StringVar(value="0 B")
		self.rx_total_var = tk.StringVar(value="0 B")
		self.tx_rate_var = tk.StringVar(value="0 B/s")
		self.rx_rate_var = tk.StringVar(value="0 B/s")

		self._add_stat_row(stats, "Status:", self.status_var)
		self._add_stat_row(stats, "TX Total:", self.tx_total_var)
		self._add_stat_row(stats, "RX Total:", self.rx_total_var)
		self._add_stat_row(stats, "TX Rate:", self.tx_rate_var)
		self._add_stat_row(stats, "RX Rate:", self.rx_rate_var)

	@staticmethod
	def _add_stat_row(parent: ttk.Widget, label: str, var: tk.StringVar) -> None:
		row = ttk.Frame(parent)
		row.pack(fill=tk.X, pady=2)
		ttk.Label(row, text=label, width=10).pack(side=tk.LEFT)
		ttk.Label(row, textvariable=var).pack(side=tk.LEFT)

	def _refresh_ports(self) -> None:
		ports = [p.device for p in list_ports.comports()]
		self.port_combo["values"] = ports

		if ports:
			current = self.port_combo.get()
			if current not in ports:
				self.port_combo.set(ports[0])
		else:
			self.port_combo.set("")

	def _start_test(self) -> None:
		port = self.port_combo.get().strip()
		if not port:
			messagebox.showerror("No port selected", "Select a serial port before starting.")
			return

		if self._worker_thread and self._worker_thread.is_alive():
			return

		self._tx_total_bytes = 0
		self._rx_total_bytes = 0
		self._start_time = time.perf_counter()

		self._stop_event.clear()
		self._worker_thread = threading.Thread(target=self._run_test, args=(port,), daemon=True)
		self._worker_thread.start()

		self.status_var.set(f"Running on {port}")
		self.start_button.config(state=tk.DISABLED)
		self.stop_button.config(state=tk.NORMAL)

	def _stop_test(self) -> None:
		self._stop_event.set()
		self.status_var.set("Stopping...")
		self.start_button.config(state=tk.NORMAL)
		self.stop_button.config(state=tk.DISABLED)

	def _run_test(self, port: str) -> None:
		packet = bytes(((i * 37) % 256 for i in range(PACKET_SIZE)))

		try:
			with serial.Serial(
				port=port,
				baudrate=SERIAL_BAUDRATE,
				timeout=SERIAL_TIMEOUT_SECONDS,
				write_timeout=SERIAL_TIMEOUT_SECONDS,
			) as ser:
				in_flight_bytes = 0

				while not self._stop_event.is_set():
					if in_flight_bytes < MAX_IN_FLIGHT_BYTES:
						try:
							sent = ser.write(packet) or 0
							self._tx_total_bytes += sent
							in_flight_bytes += sent
						except serial.SerialTimeoutException:
							pass

					waiting = ser.in_waiting
					if waiting:
						data = ser.read(waiting)
						read_size = len(data)
						self._rx_total_bytes += read_size
						in_flight_bytes = max(0, in_flight_bytes - read_size)
					else:
						time.sleep(0.0005)

		except Exception as exc:
			self.root.after(0, lambda: self._on_worker_error(str(exc)))
			return

		self.root.after(0, self._on_worker_stopped)

	def _on_worker_error(self, error_message: str) -> None:
		self._stop_event.set()
		self.status_var.set(f"Error: {error_message}")
		self.start_button.config(state=tk.NORMAL)
		self.stop_button.config(state=tk.DISABLED)

	def _on_worker_stopped(self) -> None:
		self.status_var.set("Stopped")
		self.start_button.config(state=tk.NORMAL)
		self.stop_button.config(state=tk.DISABLED)

	def _schedule_ui_update(self) -> None:
		self._update_stats()
		self.root.after(int(UI_REFRESH_SECONDS * 1000), self._schedule_ui_update)

	def _update_stats(self) -> None:
		elapsed = max(time.perf_counter() - self._start_time, 1e-9)
		tx_rate = self._tx_total_bytes / elapsed
		rx_rate = self._rx_total_bytes / elapsed

		self.tx_total_var.set(self._format_bytes(self._tx_total_bytes))
		self.rx_total_var.set(self._format_bytes(self._rx_total_bytes))
		self.tx_rate_var.set(f"{self._format_rate(tx_rate)}B/s")
		self.rx_rate_var.set(f"{self._format_rate(rx_rate)}B/s")

	@staticmethod
	def _format_bytes(value: int) -> str:
		units = ["B", "kB", "MB", "GB"]
		number = float(value)
		for unit in units:
			if number < 1024.0 or unit == units[-1]:
				return f"{number:.2f} {unit}" if unit != "B" else f"{int(number)} B"
			number /= 1024.0
		return f"{int(value)} B"

	@staticmethod
	def _format_rate(value: float) -> str:
		units = ["", "k", "M", "G"]
		number = float(value)
		for unit in units:
			if number < 1024.0 or unit == units[-1]:
				return f"{number:.2f} {unit}".strip()
			number /= 1024.0
		return f"{value:.2f}"


def main() -> None:
	root = tk.Tk()
	BandwidthTesterApp(root)
	root.mainloop()


if __name__ == "__main__":
	main()
