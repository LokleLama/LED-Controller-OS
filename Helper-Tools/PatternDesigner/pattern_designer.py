#!/usr/bin/env python3
"""LED pattern designer and visualizer for WS2812 LED strips."""

from __future__ import annotations

import subprocess
import json
import socket
import struct
import threading
import time
import base64
import shlex
import tempfile
from colorsys import hsv_to_rgb, rgb_to_hsv
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Optional
from enum import Enum

import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, ttk


DEFAULT_UPLOAD_HOST = "localhost"
DEFAULT_UPLOAD_PORT = 8822
UPLOAD_CHUNK_SIZE = 48
COMMAND_TERMINATOR = "\r\n"


class LEDFormat(Enum):
    """Supported LED color formats."""
    RGB = (0, 1, 2, None)    # R, G, B, no alpha
    RBG = (0, 2, 1, None)
    BGR = (2, 1, 0, None)
    BRG = (2, 0, 1, None)
    GRB = (1, 0, 2, None)
    GBR = (1, 2, 0, None)
    RGBW = (0, 1, 2, 3)      # R, G, B, W
    GRBW = (1, 0, 2, 3)
    WRGB = (3, 0, 1, 2)
    WRBG = (3, 0, 2, 1)
    WGRB = (3, 1, 0, 2)
    WBGR = (3, 2, 1, 0)

    def apply(self, r: int, g: int, b: int, w: int = 0) -> int:
        """Convert RGB(W) to 32-bit value according to format."""
        components = [r, g, b, w]
        result = 0
        
        if self.value[3] is None:
            # RGB format (3 bytes)
            result = (components[self.value[0]] << 16) | \
                    (components[self.value[1]] << 8) | \
                    components[self.value[2]]
        else:
            # RGBW format (4 bytes)
            result = (components[self.value[0]] << 24) | \
                    (components[self.value[1]] << 16) | \
                    (components[self.value[2]] << 8) | \
                    components[self.value[3]]
        
        return result

    @staticmethod
    def from_string(s: str) -> LEDFormat:
        """Parse format string like 'RGB', 'RGBW', etc."""
        try:
            return LEDFormat[s.upper()]
        except KeyError:
            raise ValueError(f"Unknown LED format: {s}")


class LEDDensity(Enum):
    """LED density (LEDs per meter)."""
    DENSITY_30 = 30
    DENSITY_60 = 60
    DENSITY_120 = 120


@dataclass
class PatternSettings:
    """Pattern configuration."""
    led_format: LEDFormat = LEDFormat.RGB
    density: LEDDensity = LEDDensity.DENSITY_60
    strip_length_cm: int = 100  # Total physical length in cm
    frame_delay_ms: int = 50    # Milliseconds per frame
    offset_jump: int = 1        # Pixels to advance per frame


class PatternData:
    """Manages the pattern data (LED values)."""
    
    def __init__(self, num_leds: int = 300):
        self.num_leds = num_leds
        self.leds: list[tuple[int, int, int, int]] = [(0, 0, 0, 0)] * num_leds  # (r, g, b, w) tuples
        self.keypoints: dict[int, tuple[int, int, int, int]] = {}  # index -> (r, g, b, w)
    
    def set_led(self, index: int, r: int, g: int, b: int, w: int = 0) -> None:
        """Set LED color at index."""
        if 0 <= index < self.num_leds:
            self.leds[index] = (r, g, b, w)
            self.keypoints[index] = (r, g, b, w)
    
    def get_led(self, index: int) -> tuple[int, int, int, int]:
        """Get LED color (r, g, b, w) at index."""
        if 0 <= index < self.num_leds:
            return self.leds[index]
        return (0, 0, 0, 0)
    
    def _catmull_rom(self, p0: float, p1: float, p2: float, p3: float, t: float) -> float:
        """Catmull-Rom cubic spline interpolation."""
        t2 = t * t
        t3 = t2 * t
        
        return 0.5 * (
            2.0 * p1 +
            (-p0 + p2) * t +
            (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
            (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3
        )
    
    def interpolate_between_keypoints(self) -> None:
        """Interpolate colors between keypoints using Catmull-Rom splines."""
        if not self.keypoints:
            return
        
        sorted_indices = sorted(self.keypoints.keys())
        
        if len(sorted_indices) < 2:
            return
        
        # For each segment between consecutive keypoints
        for i in range(len(sorted_indices) - 1):
            start_idx = sorted_indices[i]
            end_idx = sorted_indices[i + 1]
            
            # Get control points for spline (extended for endpoints)
            p0_idx = sorted_indices[max(0, i - 1)] if i > 0 else start_idx
            p1_idx = start_idx
            p2_idx = end_idx
            p3_idx = sorted_indices[min(len(sorted_indices) - 1, i + 2)] if i < len(sorted_indices) - 2 else end_idx
            
            p0_color = self.keypoints[p0_idx]
            p1_color = self.keypoints[p1_idx]
            p2_color = self.keypoints[p2_idx]
            p3_color = self.keypoints[p3_idx]
            
            num_steps = end_idx - start_idx + 1
            
            for step in range(num_steps):
                t = step / (num_steps - 1) if num_steps > 1 else 0
                
                r = int(self._catmull_rom(p0_color[0], p1_color[0], p2_color[0], p3_color[0], t))
                g = int(self._catmull_rom(p0_color[1], p1_color[1], p2_color[1], p3_color[1], t))
                b = int(self._catmull_rom(p0_color[2], p1_color[2], p2_color[2], p3_color[2], t))
                w = int(self._catmull_rom(p0_color[3], p1_color[3], p2_color[3], p3_color[3], t))
                
                # Clamp values to 0-255
                r = max(0, min(255, r))
                g = max(0, min(255, g))
                b = max(0, min(255, b))
                w = max(0, min(255, w))
                
                idx = start_idx + step
                if 0 <= idx < self.num_leds:
                    self.leds[idx] = (r, g, b, w)
    
    def to_bytes(self, led_format: LEDFormat) -> bytes:
        """Convert pattern to bytes using specified format."""
        data = bytearray()
        for r, g, b, w in self.leds:
            value = led_format.apply(r, g, b, w)
            data.extend(struct.pack('>I', value))  # Big-endian 32-bit
        return bytes(data)


class LEDVisualizer:
    """Handles visualization of LED strips."""
    
    @staticmethod
    def draw_leds(canvas: tk.Canvas, leds: list[tuple[int, int, int, int]], 
                  strip_width: int = 30, cell_width: int = 20, 
                  cell_height: int = 20) -> None:
        """Draw LEDs on canvas showing their colors."""
        canvas.delete("all")
        
        for idx, (r, g, b, w) in enumerate(leds):
            # Blend with white if W component exists
            if w > 0:
                r = min(255, r + w)
                g = min(255, g + w)
                b = min(255, b + w)
            
            # Convert to hex color
            color = f"#{r:02x}{g:02x}{b:02x}"
            
            # Calculate position
            x = (idx % strip_width) * cell_width
            y = (idx // strip_width) * cell_height
            
            # Draw rectangle
            canvas.create_rectangle(
                x, y, x + cell_width - 1, y + cell_height - 1,
                fill=color, outline="gray"
            )
            
            # Optionally draw LED index
            if cell_width > 30:
                canvas.create_text(
                    x + cell_width // 2, y + cell_height // 2,
                    text=str(idx), fill="white", font=("Arial", 8)
                )


class PatternDesignerApp(tk.Tk):
    """Main application for designing LED patterns."""
    
    def __init__(self) -> None:
        super().__init__()
        self.title("LED Pattern Designer")
        self.geometry("1400x700")
        
        self.settings = PatternSettings()
        self.strip_led_count = self._calculate_strip_led_count()
        self.pattern = PatternData(max(self.strip_led_count + 1, 600))
        
        self.is_animating = False
        self.animation_thread: Optional[threading.Thread] = None
        self.current_frame = 0

        self.strip_bar_height = 15
        self.editor_cell_size = 15
        self.live_update_var = tk.BooleanVar(value=False)
        self.preview_brightness_var = tk.DoubleVar(value=1.0)
        self.preview_brightness_label_var = tk.StringVar(value="1.00x")
        
        self._build_ui()
        self._bind_variable_traces()
        self._on_color_change()
        self.apply_timeline_settings(refresh=True)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _bind_variable_traces(self) -> None:
        """Bind traces for optional live updates only."""
        self.length_var.trace_add("write", lambda *_: self._on_settings_change())
        self.delay_var.trace_add("write", lambda *_: self._on_settings_change())
        self.jump_var.trace_add("write", lambda *_: self._on_settings_change())
    
    def _build_ui(self) -> None:
        """Build the user interface."""
        root = ttk.Frame(self, padding=8)
        root.pack(fill=tk.BOTH, expand=True)
        
        # Top control panel
        self._build_control_panel(root)

        settings_frame = ttk.Labelframe(root, text="Timeline Settings", padding=8)
        settings_frame.pack(fill=tk.X, pady=(6, 6))
        self._build_settings_panel(settings_frame)

        tools_frame = ttk.Labelframe(root, text="Color And Editing Tools", padding=8)
        tools_frame.pack(fill=tk.X, pady=(0, 6))
        self._build_tools_panel(tools_frame)

        timeline_frame = ttk.Frame(root)
        timeline_frame.pack(fill=tk.BOTH, expand=True)

        strip_frame = ttk.Labelframe(timeline_frame, text="Actual Strip Output (15 px high)", padding=6)
        strip_frame.pack(fill=tk.X, pady=(0, 6))
        self.strip_canvas = tk.Canvas(strip_frame, height=26, bg="#111111", highlightthickness=1, highlightbackground="#444444")
        self.strip_canvas.pack(fill=tk.X)
        self.strip_canvas.bind("<Configure>", lambda _evt: self.refresh_visualization())

        editor_frame = ttk.Labelframe(timeline_frame, text="Pattern Timeline Editor (5x5 cells)", padding=6)
        editor_frame.pack(fill=tk.BOTH, expand=True)

        self.editor_canvas = tk.Canvas(editor_frame, height=46, bg="#1b1b1b", highlightthickness=1, highlightbackground="#444444")
        self.editor_canvas.pack(fill=tk.X, expand=False)
        self.editor_canvas.bind("<Button-1>", self._on_editor_click)
        self.editor_canvas.bind("<B1-Motion>", self._on_editor_click)

        self.editor_scroll = ttk.Scrollbar(editor_frame, orient=tk.HORIZONTAL, command=self.editor_canvas.xview)
        self.editor_scroll.pack(fill=tk.X, pady=(4, 0))
        self.editor_canvas.configure(xscrollcommand=self.editor_scroll.set)

        self.info_var = tk.StringVar(value="Frame: 0")
        ttk.Label(editor_frame, textvariable=self.info_var).pack(fill=tk.X, pady=(6, 0))

        console_frame = ttk.Labelframe(root, text="Command Console", padding=6)
        console_frame.pack(fill=tk.BOTH, expand=False, pady=(6, 0))
        self._build_console_panel(console_frame)

    def _build_console_panel(self, parent: ttk.Frame) -> None:
        """Build bottom console output panel for command diagnostics."""
        controls = ttk.Frame(parent)
        controls.pack(fill=tk.X, pady=(0, 6))

        ttk.Button(controls, text="Clear Console", command=self._clear_console).pack(side=tk.LEFT)

        console_wrap = ttk.Frame(parent)
        console_wrap.pack(fill=tk.BOTH, expand=True)

        self.console_text = tk.Text(
            console_wrap,
            height=10,
            wrap=tk.WORD,
            bg="#101010",
            fg="#d8d8d8",
            insertbackground="#d8d8d8",
            relief=tk.FLAT,
            font=("TkFixedFont", 9),
        )
        self.console_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        console_scroll = ttk.Scrollbar(console_wrap, orient=tk.VERTICAL, command=self.console_text.yview)
        console_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.console_text.configure(yscrollcommand=console_scroll.set)

        self.console_text.tag_configure("cmd", foreground="#7ec8ff")
        self.console_text.tag_configure("stdout", foreground="#d8d8d8")
        self.console_text.tag_configure("stderr", foreground="#ff8a8a")
        self.console_text.tag_configure("meta", foreground="#a8a8a8")

        self._log_console("Console ready. Export commands and dfile output will appear here.", tag="meta")

    def _clear_console(self) -> None:
        """Clear command console output."""
        self.console_text.configure(state=tk.NORMAL)
        self.console_text.delete("1.0", tk.END)
        self.console_text.configure(state=tk.DISABLED)

    def _log_console(self, message: str, tag: str = "stdout") -> None:
        """Append a line to the command console."""
        if not hasattr(self, "console_text"):
            return

        self.console_text.configure(state=tk.NORMAL)
        self.console_text.insert(tk.END, message + "\n", tag)
        self.console_text.see(tk.END)
        self.console_text.configure(state=tk.DISABLED)

    @staticmethod
    def _short_arg(value: str, max_len: int = 100) -> str:
        """Shorten long argument strings for readable console output."""
        if len(value) <= max_len:
            return value
        hidden = len(value) - max_len
        return f"{value[:max_len]}...<+{hidden} chars>"

    def _format_command_for_log(self, cmd: list[str]) -> str:
        """Render command line text safely for log output."""
        shortened = [self._short_arg(arg) for arg in cmd]
        try:
            return shlex.join(shortened)
        except Exception:
            return " ".join(shortened)

    def _run_logged_command(self, cmd: list[str], cwd: Optional[Path] = None) -> subprocess.CompletedProcess[str]:
        """Run a command and emit command/stdout/stderr/exit to the console."""
        cwd_str = str(cwd) if cwd is not None else None
        command_text = self._format_command_for_log(cmd)
        if cwd_str:
            self._log_console(f"$ {command_text}  (cwd: {cwd_str})", tag="cmd")
        else:
            self._log_console(f"$ {command_text}", tag="cmd")

        try:
            result = subprocess.run(
                cmd,
                check=True,
                capture_output=True,
                text=True,
                cwd=cwd_str,
            )
            if result.stdout:
                self._log_console(result.stdout.rstrip("\n"), tag="stdout")
            if result.stderr:
                self._log_console(result.stderr.rstrip("\n"), tag="stderr")
            self._log_console(f"[exit {result.returncode}]", tag="meta")
            return result
        except subprocess.CalledProcessError as e:
            if e.stdout:
                self._log_console(e.stdout.rstrip("\n"), tag="stdout")
            if e.stderr:
                self._log_console(e.stderr.rstrip("\n"), tag="stderr")
            self._log_console(f"[exit {e.returncode}]", tag="meta")
            raise
    
    def _build_tools_panel(self, parent: ttk.Frame) -> None:
        """Build color and editing tools panel."""
        # Create color display frame that shows current color
        self.color_display = tk.Frame(parent, bg="#FF0000", width=40, height=20)
        self.color_display.pack(side=tk.LEFT, padx=(0, 8))
        self.color_display.pack_propagate(False)
        self.color_display.bind("<Button-1>", self._on_color_canvas_click)
        
        ttk.Button(parent, text="Paint Picker", command=self._open_paint_color_picker).pack(side=tk.LEFT, padx=2)
        
        # RGB sliders
        ttk.Label(parent, text="R:").pack(side=tk.LEFT, padx=(8, 2))
        self.r_var = tk.StringVar(value="255")
        ttk.Scale(parent, from_=0, to=255, variable=self.r_var, orient=tk.HORIZONTAL,
                 command=self._on_color_change, length=80).pack(side=tk.LEFT, padx=2)
        ttk.Entry(parent, textvariable=self.r_var, width=4).pack(side=tk.LEFT, padx=2)

        ttk.Label(parent, text="G:").pack(side=tk.LEFT, padx=(8, 2))
        self.g_var = tk.StringVar(value="0")
        ttk.Scale(parent, from_=0, to=255, variable=self.g_var, orient=tk.HORIZONTAL,
                 command=self._on_color_change, length=80).pack(side=tk.LEFT, padx=2)
        ttk.Entry(parent, textvariable=self.g_var, width=4).pack(side=tk.LEFT, padx=2)

        ttk.Label(parent, text="B:").pack(side=tk.LEFT, padx=(8, 2))
        self.b_var = tk.StringVar(value="0")
        ttk.Scale(parent, from_=0, to=255, variable=self.b_var, orient=tk.HORIZONTAL,
                 command=self._on_color_change, length=80).pack(side=tk.LEFT, padx=2)
        ttk.Entry(parent, textvariable=self.b_var, width=4).pack(side=tk.LEFT, padx=2)

        ttk.Separator(parent, orient=tk.VERTICAL).pack(side=tk.LEFT, padx=8, fill=tk.Y)
        ttk.Label(parent, text="Preview Brightness:").pack(side=tk.LEFT, padx=(2, 2))
        ttk.Scale(
            parent,
            from_=1.0,
            to=4.0,
            variable=self.preview_brightness_var,
            orient=tk.HORIZONTAL,
            command=self._on_preview_brightness_change,
            length=120,
        ).pack(side=tk.LEFT, padx=2)
        ttk.Label(parent, textvariable=self.preview_brightness_label_var, width=6).pack(side=tk.LEFT, padx=(2, 2))
        
        ttk.Separator(parent, orient=tk.VERTICAL).pack(side=tk.LEFT, padx=8, fill=tk.Y)
        ttk.Button(parent, text="Clear All", command=self.clear_pattern).pack(side=tk.LEFT, padx=2)
        ttk.Button(parent, text="Interpolate", command=self.interpolate_pattern).pack(side=tk.LEFT, padx=2)
        ttk.Button(parent, text="Gradient", command=self.create_gradient).pack(side=tk.LEFT, padx=2)
        ttk.Label(parent, text="(Paint LEDs by clicking/dragging in the timeline bar below)").pack(side=tk.LEFT, padx=(8, 0), fill=tk.X, expand=True)

    def _on_preview_brightness_change(self, _value: str) -> None:
        """Update preview brightness text and redraw timeline bars."""
        brightness = max(1.0, float(self.preview_brightness_var.get()))
        self.preview_brightness_label_var.set(f"{brightness:.2f}x")
        self.refresh_visualization()
    
    def _build_control_panel(self, parent: ttk.Frame) -> None:
        """Build top control panel with buttons."""
        panel = ttk.Frame(parent)
        panel.pack(fill=tk.X, pady=(0, 8))
        
        ttk.Button(panel, text="Load Pattern", command=self.load_pattern).pack(side=tk.LEFT, padx=2)
        ttk.Button(panel, text="Save Pattern", command=self.save_pattern).pack(side=tk.LEFT, padx=2)
        ttk.Separator(panel, orient=tk.VERTICAL).pack(side=tk.LEFT, padx=8, fill=tk.Y)
        ttk.Button(panel, text="Export to Binary", command=self.export_binary).pack(side=tk.LEFT, padx=2)
        ttk.Button(panel, text="Export + Send To Device", command=self.export_and_send_network).pack(side=tk.LEFT, padx=2)
        ttk.Separator(panel, orient=tk.VERTICAL).pack(side=tk.LEFT, padx=8, fill=tk.Y)
        
        self.anim_btn = ttk.Button(panel, text="Start Animation", command=self.toggle_animation)
        self.anim_btn.pack(side=tk.LEFT, padx=2)
        ttk.Button(panel, text="Reset Frame", command=self.reset_frame).pack(side=tk.LEFT, padx=2)
    
    def _build_settings_panel(self, parent: ttk.Frame) -> None:
        """Build settings panel."""
        ttk.Label(parent, text="LED Format:").grid(row=0, column=0, sticky=tk.W, pady=4)
        self.format_var = tk.StringVar(value="RGB")
        format_combo = ttk.Combobox(
            parent, textvariable=self.format_var,
            values=[fmt.name for fmt in LEDFormat], state="readonly"
        )
        format_combo.grid(row=0, column=1, sticky=tk.EW, pady=4, padx=(4, 10))
        format_combo.bind("<<ComboboxSelected>>", self._on_settings_change)

        ttk.Label(parent, text="Density (LED/m):").grid(row=0, column=2, sticky=tk.W, pady=4)
        self.density_var = tk.StringVar(value="60")
        density_combo = ttk.Combobox(
            parent, textvariable=self.density_var,
            values=["30", "60", "120"], state="readonly"
        )
        density_combo.grid(row=0, column=3, sticky=tk.EW, pady=4, padx=(4, 10))
        density_combo.bind("<<ComboboxSelected>>", self._on_settings_change)

        ttk.Label(parent, text="Strip Length (cm):").grid(row=0, column=4, sticky=tk.W, pady=4)
        self.length_var = tk.StringVar(value="100")
        ttk.Spinbox(parent, from_=10, to=5000, textvariable=self.length_var,
                   command=self._on_settings_change, width=8).grid(row=0, column=5, sticky=tk.EW, pady=4, padx=(4, 10))

        ttk.Label(parent, text="Strip LEDs:").grid(row=0, column=6, sticky=tk.W, pady=4)
        self.total_leds_var = tk.StringVar(value=str(self.strip_led_count))
        ttk.Label(parent, textvariable=self.total_leds_var).grid(row=0, column=7, sticky=tk.W, pady=4, padx=(4, 10))

        ttk.Label(parent, text="Pattern LEDs:").grid(row=1, column=0, sticky=tk.W, pady=4)
        self.pattern_length_var = tk.StringVar(value=str(self.pattern.num_leds))
        ttk.Spinbox(parent, from_=2, to=200000, textvariable=self.pattern_length_var,
                   command=self._on_pattern_length_change, width=10).grid(row=1, column=1, sticky=tk.EW, pady=4, padx=(4, 10))

        ttk.Label(parent, text="Frame Delay (ms):").grid(row=1, column=2, sticky=tk.W, pady=4)
        self.delay_var = tk.StringVar(value="50")
        ttk.Spinbox(parent, from_=10, to=1000, textvariable=self.delay_var,
                   command=self._on_settings_change, width=8).grid(row=1, column=3, sticky=tk.EW, pady=4, padx=(4, 10))

        ttk.Label(parent, text="Offset Jump (px):").grid(row=1, column=4, sticky=tk.W, pady=4)
        self.jump_var = tk.StringVar(value="1")
        ttk.Spinbox(parent, from_=1, to=50, textvariable=self.jump_var,
                   command=self._on_settings_change, width=8).grid(row=1, column=5, sticky=tk.EW, pady=4, padx=(4, 10))

        ttk.Checkbutton(parent, text="Live Update", variable=self.live_update_var).grid(row=1, column=6, sticky=tk.W, pady=4)
        ttk.Button(parent, text="Apply Timeline Settings", command=lambda: self.apply_timeline_settings(refresh=True)).grid(
            row=1, column=7, sticky=tk.EW, pady=4
        )

        parent.columnconfigure(1, weight=1)
        parent.columnconfigure(3, weight=1)
        parent.columnconfigure(5, weight=1)
        parent.columnconfigure(7, weight=1)
    
    def _on_color_change(self, *args: Any) -> None:
        """Update color display when sliders change."""
        try:
            r = int(float(self.r_var.get()))
            g = int(float(self.g_var.get()))
            b = int(float(self.b_var.get()))
        except ValueError:
            return
        
        color = f"#{r:02x}{g:02x}{b:02x}"
        self.color_display.config(bg=color)
    
    def _on_color_canvas_click(self, event: tk.Event) -> None:
        """Open color picker dialog."""
        from tkinter.colorchooser import askcolor
        
        try:
            r = int(float(self.r_var.get()))
            g = int(float(self.g_var.get()))
            b = int(float(self.b_var.get()))
        except ValueError:
            r = g = b = 0
        
        color, _ = askcolor((r, g, b))
        if color:
            self.r_var.set(str(int(color[0])))
            self.g_var.set(str(int(color[1])))
            self.b_var.set(str(int(color[2])))
            self._on_color_change()

    def _open_paint_color_picker(self) -> None:
        """Open a Paint-style HSV color picker dialog."""
        try:
            r = int(float(self.r_var.get()))
            g = int(float(self.g_var.get()))
            b = int(float(self.b_var.get()))
        except ValueError:
            r, g, b = 255, 0, 0

        h_init, s_init, v_init = rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
        state = {"h": h_init, "s": s_init, "v": v_init}

        picker = tk.Toplevel(self)
        picker.title("Paint Color Picker")
        picker.resizable(False, False)
        picker.transient(self)

        main = ttk.Frame(picker, padding=8)
        main.pack(fill=tk.BOTH, expand=True)

        sv_w, sv_h = 220, 180
        hue_w, hue_h = 24, 180

        sv_canvas = tk.Canvas(main, width=sv_w, height=sv_h, highlightthickness=1, highlightbackground="#666666")
        sv_canvas.grid(row=0, column=0, rowspan=3, sticky=tk.NSEW)

        hue_canvas = tk.Canvas(main, width=hue_w, height=hue_h, highlightthickness=1, highlightbackground="#666666")
        hue_canvas.grid(row=0, column=1, rowspan=3, sticky=tk.NS, padx=(8, 8))

        preview = tk.Canvas(main, width=60, height=40, highlightthickness=1, highlightbackground="#666666")
        preview.grid(row=0, column=2, sticky=tk.W)

        rgb_label = ttk.Label(main, text="RGB(0, 0, 0)")
        rgb_label.grid(row=1, column=2, sticky=tk.W, pady=(6, 0))

        btn_row = ttk.Frame(main)
        btn_row.grid(row=2, column=2, sticky=tk.SW, pady=(8, 0))

        sv_img = tk.PhotoImage(width=sv_w, height=sv_h)
        hue_img = tk.PhotoImage(width=hue_w, height=hue_h)
        sv_canvas.create_image(0, 0, anchor=tk.NW, image=sv_img)
        hue_canvas.create_image(0, 0, anchor=tk.NW, image=hue_img)
        picker._sv_img = sv_img
        picker._hue_img = hue_img

        def redraw_hue() -> None:
            for y in range(hue_h):
                h = y / max(1, hue_h - 1)
                rr, gg, bb = hsv_to_rgb(h, 1.0, 1.0)
                color = f"#{int(rr * 255):02x}{int(gg * 255):02x}{int(bb * 255):02x}"
                row = "{" + " ".join([color] * hue_w) + "}"
                hue_img.put(row, to=(0, y))

        def redraw_sv() -> None:
            h = state["h"]
            for y in range(sv_h):
                v = 1.0 - (y / max(1, sv_h - 1))
                row_colors: list[str] = []
                for x in range(sv_w):
                    s = x / max(1, sv_w - 1)
                    rr, gg, bb = hsv_to_rgb(h, s, v)
                    row_colors.append(f"#{int(rr * 255):02x}{int(gg * 255):02x}{int(bb * 255):02x}")
                sv_img.put("{" + " ".join(row_colors) + "}", to=(0, y))

        def draw_markers() -> None:
            sv_canvas.delete("marker")
            hue_canvas.delete("marker")

            sx = int(state["s"] * (sv_w - 1))
            sy = int((1.0 - state["v"]) * (sv_h - 1))
            sv_canvas.create_oval(sx - 4, sy - 4, sx + 4, sy + 4, outline="#ffffff", width=2, tag="marker")

            hy = int(state["h"] * (hue_h - 1))
            hue_canvas.create_line(0, hy, hue_w, hy, fill="#ffffff", width=2, tag="marker")

        def update_preview() -> None:
            rr, gg, bb = hsv_to_rgb(state["h"], state["s"], state["v"])
            r_i = int(rr * 255)
            g_i = int(gg * 255)
            b_i = int(bb * 255)
            preview.configure(bg=f"#{r_i:02x}{g_i:02x}{b_i:02x}")
            rgb_label.configure(text=f"RGB({r_i}, {g_i}, {b_i})")

        def apply_color(close: bool) -> None:
            rr, gg, bb = hsv_to_rgb(state["h"], state["s"], state["v"])
            self.r_var.set(str(int(rr * 255)))
            self.g_var.set(str(int(gg * 255)))
            self.b_var.set(str(int(bb * 255)))
            self._on_color_change()
            if close:
                picker.destroy()

        def on_sv(event: tk.Event) -> None:
            x = min(max(int(event.x), 0), sv_w - 1)
            y = min(max(int(event.y), 0), sv_h - 1)
            state["s"] = x / max(1, sv_w - 1)
            state["v"] = 1.0 - (y / max(1, sv_h - 1))
            draw_markers()
            update_preview()

        def on_hue(event: tk.Event) -> None:
            y = min(max(int(event.y), 0), hue_h - 1)
            state["h"] = y / max(1, hue_h - 1)
            redraw_sv()
            draw_markers()
            update_preview()

        ttk.Button(btn_row, text="Apply", command=lambda: apply_color(close=False)).pack(side=tk.LEFT)
        ttk.Button(btn_row, text="OK", command=lambda: apply_color(close=True)).pack(side=tk.LEFT, padx=(6, 0))
        ttk.Button(btn_row, text="Cancel", command=picker.destroy).pack(side=tk.LEFT, padx=(6, 0))

        sv_canvas.bind("<Button-1>", on_sv)
        sv_canvas.bind("<B1-Motion>", on_sv)
        hue_canvas.bind("<Button-1>", on_hue)
        hue_canvas.bind("<B1-Motion>", on_hue)

        redraw_hue()
        redraw_sv()
        draw_markers()
        update_preview()
    
    def _on_editor_click(self, event: tk.Event) -> None:
        """Handle click and drag painting on the editor timeline."""
        try:
            r = int(float(self.r_var.get()))
            g = int(float(self.g_var.get()))
            b = int(float(self.b_var.get()))
        except ValueError:
            return

        idx = int(self.editor_canvas.canvasx(event.x) // self.editor_cell_size)
        if 0 <= idx < self.pattern.num_leds:
            self.pattern.set_led(idx, r, g, b, 0)
            self.refresh_visualization()

    def _calculate_strip_led_count(self) -> int:
        """Calculate actual strip length in LEDs from density and physical length."""
        return max(1, int(self.settings.density.value * self.settings.strip_length_cm / 100))

    def _ensure_pattern_length(self, requested_length: int) -> int:
        """Ensure pattern length is always larger than actual strip length."""
        minimum = self.strip_led_count + 1
        return max(minimum, requested_length)

    def _resize_pattern(self, new_length: int) -> None:
        """Resize pattern data while preserving existing values."""
        old_leds = list(self.pattern.leds)
        self.pattern.num_leds = new_length
        self.pattern.leds = old_leds[:new_length]
        if len(self.pattern.leds) < new_length:
            self.pattern.leds.extend([(0, 0, 0, 0)] * (new_length - len(self.pattern.leds)))
        self.pattern.keypoints = {
            idx: value for idx, value in self.pattern.keypoints.items() if idx < new_length
        }
        if self.current_frame >= self.pattern.num_leds:
            self.current_frame = 0

    def _on_pattern_length_change(self) -> None:
        """Handle user change of editable pattern LED count."""
        if not self.live_update_var.get():
            return
        self.apply_timeline_settings(refresh=True)
    
    def _on_settings_change(self, *args: Any) -> None:
        """Handle settings change."""
        if not self.live_update_var.get():
            return
        self.apply_timeline_settings(refresh=True)

    def apply_timeline_settings(self, refresh: bool = True) -> None:
        """Apply timeline settings from UI controls."""
        try:
            self.settings.led_format = LEDFormat.from_string(self.format_var.get())
            self.settings.density = LEDDensity(int(self.density_var.get()))
            self.settings.strip_length_cm = int(self.length_var.get())
            self.settings.frame_delay_ms = int(self.delay_var.get())
            self.settings.offset_jump = int(self.jump_var.get())

            self.strip_led_count = self._calculate_strip_led_count()
            self.total_leds_var.set(str(self.strip_led_count))

            try:
                requested = int(self.pattern_length_var.get())
            except ValueError:
                requested = self.pattern.num_leds
            enforced = self._ensure_pattern_length(requested)
            if enforced != requested:
                self.pattern_length_var.set(str(enforced))
            if enforced != self.pattern.num_leds:
                self._resize_pattern(enforced)

            if refresh:
                self.refresh_visualization()
        except ValueError:
            pass
    
    def refresh_visualization(self) -> None:
        """Refresh both timeline bars."""
        self._draw_strip_bar()
        self._draw_editor_bar()

    def _display_color(self, r: int, g: int, b: int, w: int) -> str:
        """Convert an RGBW value to displayable hex color."""
        if w > 0:
            r = min(255, r + w)
            g = min(255, g + w)
            b = min(255, b + w)

        # Display-only brightness boost for dark colors in preview/editor bars.
        brightness = max(1.0, float(self.preview_brightness_var.get()))
        r = min(255, int(r * brightness))
        g = min(255, int(g * brightness))
        b = min(255, int(b * brightness))
        return f"#{r:02x}{g:02x}{b:02x}"

    def _draw_strip_bar(self) -> None:
        """Draw the actual strip output bar filling the full visible width."""
        self.strip_canvas.delete("all")
        if self.strip_led_count <= 0 or self.pattern.num_leds <= 0:
            return

        canvas_width = max(1, self.strip_canvas.winfo_width())
        led_width = canvas_width / self.strip_led_count
        y0 = 5
        y1 = y0 + self.strip_bar_height

        for strip_idx in range(self.strip_led_count):
            pattern_idx = (self.current_frame + strip_idx) % self.pattern.num_leds
            r, g, b, w = self.pattern.get_led(pattern_idx)
            color = self._display_color(r, g, b, w)
            x0 = strip_idx * led_width
            x1 = (strip_idx + 1) * led_width
            self.strip_canvas.create_rectangle(x0, y0, x1, y1, fill=color, outline=color)

    def _draw_editor_bar(self) -> None:
        """Draw the editable timeline with bordered LED cells and orientation markers."""
        self.editor_canvas.delete("all")
        if self.pattern.num_leds <= 0:
            return

        size = self.editor_cell_size
        y0 = 18
        y1 = y0 + size
        for idx, (r, g, b, w) in enumerate(self.pattern.leds):
            x0 = idx * size
            x1 = x0 + size
            color = self._display_color(r, g, b, w)

            # Orientation aids: mark every 10th LED with a tick, label every 20th LED.
            if idx % 10 == 0:
                tick_x = x0 + (size / 2)
                self.editor_canvas.create_line(tick_x, y0 - 8, tick_x, y0 - 2, fill="#bbbbbb", width=1)
            if idx % 20 == 0:
                self.editor_canvas.create_text(x0 + 1, y0 - 10, text=str(idx), fill="#d0d0d0", anchor=tk.SW, font=("TkDefaultFont", 8))

            self.editor_canvas.create_rectangle(
                x0,
                y0,
                x1,
                y1,
                fill=color,
                outline="#4f4f4f",
                width=1,
            )

        self.editor_canvas.configure(scrollregion=(0, 0, self.pattern.num_leds * size, y1 + 2))
    
    def clear_pattern(self) -> None:
        """Clear all LEDs."""
        self.pattern.leds = [(0, 0, 0, 0)] * self.pattern.num_leds
        self.pattern.keypoints.clear()
        self.refresh_visualization()
    
    def interpolate_pattern(self) -> None:
        """Interpolate colors between keypoints."""
        if len(self.pattern.keypoints) < 2:
            messagebox.showwarning("Interpolate", "Set at least 2 keypoints first (by clicking on the grid)")
            return
        
        self.pattern.interpolate_between_keypoints()
        self.refresh_visualization()
    
    def create_gradient(self) -> None:
        """Create a color gradient across the strip."""
        if self.pattern.num_leds < 2:
            return
        
        try:
            r = int(float(self.r_var.get()))
            g = int(float(self.g_var.get()))
            b = int(float(self.b_var.get()))
        except ValueError:
            return
        
        # Create gradient from black to current color
        for i in range(self.pattern.num_leds):
            t = i / (self.pattern.num_leds - 1)
            gr = int(r * t)
            gg = int(g * t)
            gb = int(b * t)
            self.pattern.leds[i] = (gr, gg, gb, 0)
        
        self.refresh_visualization()
    
    def toggle_animation(self) -> None:
        """Start or stop animation preview."""
        if self.is_animating:
            self.is_animating = False
            self.anim_btn.config(text="Start Animation")
        else:
            self.is_animating = True
            self.anim_btn.config(text="Stop Animation")
            self.animation_thread = threading.Thread(target=self._animation_loop, daemon=True)
            self.animation_thread.start()
    
    def _animation_loop(self) -> None:
        """Animation loop running in background thread."""
        while self.is_animating:
            try:
                frame_delay = int(self.delay_var.get()) / 1000.0
                time.sleep(frame_delay)
                
                self.current_frame = (self.current_frame + int(self.jump_var.get())) % self.pattern.num_leds
                self.info_var.set(f"Frame: {self.current_frame}")
                
                # Draw animated preview
                self.after(0, self._draw_animation_frame)
            except Exception:
                break
    
    def _draw_animation_frame(self) -> None:
        """Draw current animation frame."""
        if not self.is_animating:
            return

        self._draw_strip_bar()
    
    def reset_frame(self) -> None:
        """Reset animation frame to 0."""
        self.current_frame = 0
        self.info_var.set("Frame: 0")
        self._draw_animation_frame()
    
    def load_pattern(self) -> None:
        """Load pattern from JSON file."""
        filepath = filedialog.askopenfilename(
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if not filepath:
            return
        
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)

            raw_leds = data.get("leds", [(0, 0, 0, 0)] * self.pattern.num_leds)
            normalized_leds: list[tuple[int, int, int, int]] = []
            for item in raw_leds:
                if isinstance(item, (list, tuple)) and len(item) >= 3:
                    r = int(item[0])
                    g = int(item[1])
                    b = int(item[2])
                    w = int(item[3]) if len(item) > 3 else 0
                    normalized_leds.append((r, g, b, w))

            requested_len = len(normalized_leds) if normalized_leds else self.pattern.num_leds
            new_len = self._ensure_pattern_length(requested_len)
            self._resize_pattern(new_len)
            self.pattern.leds[:len(normalized_leds)] = normalized_leds[:self.pattern.num_leds]
            self.pattern.keypoints = {int(k): tuple(v) for k, v in data.get("keypoints", {}).items()}

            self.pattern_length_var.set(str(self.pattern.num_leds))
            self.refresh_visualization()
            messagebox.showinfo("Load Pattern", "Pattern loaded successfully")
        except Exception as e:
            messagebox.showerror("Load Error", str(e))
    
    def save_pattern(self) -> None:
        """Save pattern to JSON file."""
        filepath = filedialog.asksaveasfilename(
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if not filepath:
            return
        
        try:
            data = {
                "leds": self.pattern.leds,
                "keypoints": {str(k): v for k, v in self.pattern.keypoints.items()},
                "settings": {
                    "format": self.settings.led_format.name,
                    "density": self.settings.density.value,
                    "strip_length_cm": self.settings.strip_length_cm,
                }
            }
            
            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2)
            
            messagebox.showinfo("Save Pattern", "Pattern saved successfully")
        except Exception as e:
            messagebox.showerror("Save Error", str(e))

    @staticmethod
    def _escape_device_text(value: str) -> str:
        """Escape text for the device parser."""
        return value.replace("\\", "\\\\").replace('"', '\\"').replace("$", "\\$")

    @classmethod
    def _quote_device_arg(cls, value: str) -> str:
        return f'"{cls._escape_device_text(value)}"'

    def _find_dfile_binary(self) -> tuple[Path, Path]:
        """Locate dfile executable and return (dfile_dir, dfile_binary)."""
        repo_root = Path(__file__).resolve().parents[2]
        dfile_dir = repo_root / "TestPrograms" / "DataFile"
        candidates = [
            dfile_dir / "build" / "dfile",
            dfile_dir / "dfile",
            Path("/root/LED-Controller-OS/TestPrograms/DataFile/build/dfile"),
        ]
        for candidate in candidates:
            if candidate.exists():
                return dfile_dir, candidate
        raise FileNotFoundError(
            "Could not find dfile tool. Build it first with:\n"
            "cd TestPrograms/DataFile && make"
        )

    def _export_pattern_to_binary_path(self, output_path: Path) -> None:
        """Export the current pattern to a binary DataFile path."""
        dfile_dir, dfile_binary = self._find_dfile_binary()
        self._log_console(f"Using dfile: {dfile_binary}", tag="meta")

        self._run_logged_command(
            [str(dfile_binary), "create", "LEDP", str(output_path)],
            cwd=dfile_dir,
        )

        frame_delay_bytes = struct.pack('<H', self.settings.frame_delay_ms)
        frame_delay_b64 = base64.b64encode(frame_delay_bytes).decode('ascii')
        self._run_logged_command(
            [str(dfile_binary), "append", "tim", "-b64", frame_delay_b64, str(output_path)],
            cwd=dfile_dir,
        )

        offset_jump_bytes = struct.pack('<H', self.settings.offset_jump)
        offset_jump_b64 = base64.b64encode(offset_jump_bytes).decode('ascii')
        self._run_logged_command(
            [str(dfile_binary), "append", "jmp", "-b64", offset_jump_b64, str(output_path)],
            cwd=dfile_dir,
        )

        pattern_length_bytes = struct.pack('<H', self.pattern.num_leds)
        pattern_length_b64 = base64.b64encode(pattern_length_bytes).decode('ascii')
        self._run_logged_command(
            [str(dfile_binary), "append", "len", "-b64", pattern_length_b64, str(output_path)],
            cwd=dfile_dir,
        )

        pattern_bytes = self.pattern.to_bytes(self.settings.led_format)
        pattern_b64 = base64.b64encode(pattern_bytes).decode('ascii')
        self._log_console(f"Pattern payload: {len(pattern_bytes)} bytes ({len(pattern_b64)} b64 chars)", tag="meta")
        self._run_logged_command(
            [str(dfile_binary), "append", "dat", "-b64", pattern_b64, str(output_path)],
            cwd=dfile_dir,
        )

    def _send_command_and_capture(
        self,
        sock: socket.socket,
        command: str,
        timeout_s: float = 4.0,
        quiet_s: float = 0.25,
    ) -> str:
        """Send one command over TCP and capture response until quiet period."""
        sock.sendall((command + COMMAND_TERMINATOR).encode("utf-8"))

        chunks: list[bytes] = []
        start = time.monotonic()
        last_rx: Optional[float] = None
        while time.monotonic() - start < timeout_s:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                data = b""
            if data:
                chunks.append(data)
                last_rx = time.monotonic()
                continue
            if last_rx is not None and (time.monotonic() - last_rx) >= quiet_s:
                break

        return b"".join(chunks).decode("utf-8", errors="replace")

    def _upload_binary_over_network(self, host: str, port: int, remote_name: str, payload: bytes) -> None:
        """Upload binary payload to device filesystem through SerialServer."""
        self._log_console(f"Opening TCP connection to {host}:{port}", tag="meta")
        with socket.create_connection((host, port), timeout=3.0) as sock:
            sock.settimeout(0.10)

            alloc_cmd = f"store {self._quote_device_arg(remote_name)} --alloc {len(payload)}"
            alloc_resp = self._send_command_and_capture(sock, alloc_cmd, timeout_s=3.0)
            if "Error:" in alloc_resp or "<device not connected>" in alloc_resp:
                raise RuntimeError(f"Upload allocate failed: {alloc_resp.strip()}")

            for start in range(0, len(payload), UPLOAD_CHUNK_SIZE):
                chunk = payload[start:start + UPLOAD_CHUNK_SIZE]
                encoded = base64.b64encode(chunk).decode("ascii")
                append_cmd = f"store --append -b64 {self._quote_device_arg(encoded)}"
                append_resp = self._send_command_and_capture(sock, append_cmd, timeout_s=2.0)
                if "Error:" in append_resp or "<device not connected>" in append_resp:
                    raise RuntimeError(f"Upload append failed: {append_resp.strip()}")

            finish_resp = self._send_command_and_capture(sock, "store --finish", timeout_s=3.0)
            if "Error:" in finish_resp or "<device not connected>" in finish_resp:
                raise RuntimeError(f"Upload finish failed: {finish_resp.strip()}")
    
    def export_binary(self) -> None:
        """Export pattern to binary file using dataFile tool."""
        filepath = filedialog.asksaveasfilename(
            defaultextension=".bin",
            filetypes=[("Binary files", "*.bin"), ("All files", "*.*")]
        )
        if not filepath:
            return
        
        try:
            self._log_console("--- Export started ---", tag="meta")
            self._log_console(f"Target file: {filepath}", tag="meta")

            output_path = Path(filepath)
            self._export_pattern_to_binary_path(output_path)
            pattern_bytes = self.pattern.to_bytes(self.settings.led_format)

            self._log_console("--- Export completed successfully ---", tag="meta")
            
            messagebox.showinfo(
                "Export Successful",
                f"Pattern exported to:\n{filepath}\n\n"
                f"LED count: {self.pattern.num_leds}\n"
                f"Pattern size: {len(pattern_bytes)} bytes\n"
                f"Frame delay: {self.settings.frame_delay_ms} ms\n"
                f"Offset jump: {self.settings.offset_jump} pixels\n"
                f"Format: {self.settings.led_format.name}"
            )
        except FileNotFoundError as e:
            self._log_console("dfile binary not found.", tag="stderr")
            messagebox.showerror("Export Error", str(e))
        except subprocess.CalledProcessError as e:
            error_msg = e.stderr if e.stderr else str(e)
            self._log_console("--- Export failed ---", tag="stderr")
            messagebox.showerror("Export Error", f"dfile tool error:\n{error_msg}\n\nSee Command Console for full output.")
        except Exception as e:
            self._log_console(f"Unexpected export error: {e}", tag="stderr")
            messagebox.showerror("Export Error", str(e))

    def export_and_send_network(self) -> None:
        """Export pattern and upload to device over SerialServer TCP."""
        remote_name = simpledialog.askstring(
            "Remote File Name",
            "Filename on device:",
            initialvalue="pattern.bin",
            parent=self,
        )
        if not remote_name:
            return

        host = simpledialog.askstring(
            "SerialServer Host",
            "Host:",
            initialvalue=DEFAULT_UPLOAD_HOST,
            parent=self,
        )
        if host is None:
            return
        host = host.strip()
        if not host:
            messagebox.showwarning("Invalid host", "Host cannot be empty.")
            return

        port = simpledialog.askinteger(
            "SerialServer Port",
            "TCP port:",
            initialvalue=DEFAULT_UPLOAD_PORT,
            minvalue=1,
            maxvalue=65535,
            parent=self,
        )
        if port is None:
            return

        try:
            self._log_console("--- Export + Network send started ---", tag="meta")
            self._log_console(f"Target: {host}:{port} -> {remote_name}", tag="meta")

            with tempfile.TemporaryDirectory(prefix="pattern_export_") as temp_dir:
                temp_path = Path(temp_dir) / "pattern.bin"
                self._export_pattern_to_binary_path(temp_path)
                payload = temp_path.read_bytes()
                self._log_console(f"Upload payload: {len(payload)} bytes", tag="meta")
                self._upload_binary_over_network(host, port, remote_name, payload)

            self._log_console("--- Export + Network send completed successfully ---", tag="meta")
            messagebox.showinfo(
                "Upload Successful",
                f"Pattern exported and uploaded to {host}:{port}\n"
                f"Remote file: {remote_name}",
            )
        except FileNotFoundError as e:
            self._log_console("dfile binary not found.", tag="stderr")
            messagebox.showerror("Export And Send Error", str(e))
        except subprocess.CalledProcessError as e:
            error_msg = e.stderr if e.stderr else str(e)
            self._log_console("--- Export + Network send failed ---", tag="stderr")
            messagebox.showerror("Export And Send Error", f"dfile tool error:\n{error_msg}\n\nSee Command Console for full output.")
        except Exception as e:
            self._log_console(f"Export/send error: {e}", tag="stderr")
            messagebox.showerror("Export And Send Error", str(e))
    
    def _on_close(self) -> None:
        """Handle window close."""
        self.is_animating = False
        if self.animation_thread and self.animation_thread.is_alive():
            self.animation_thread.join(timeout=1)
        self.destroy()


def main() -> None:
    app = PatternDesignerApp()
    app.mainloop()


if __name__ == "__main__":
    main()
