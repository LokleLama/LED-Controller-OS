# LED Controller FileSync GUI

This folder contains `filesync.py`, a desktop GUI for:
- connecting to your LED-Controller-OS device over serial,
- viewing the live device console,
- browsing local files and device files,
- uploading/downloading files.

## Prerequisites

- Python 3.10+ recommended
- `tkinter` available in your Python install

On Debian/Ubuntu, if `tkinter` is missing:

```bash
sudo apt-get update
sudo apt-get install -y python3-tk
```

## Setup and Run with venv (Linux)

From the repository root:

```bash
cd /root/LED-Controller-OS/Helper-Tools
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python filesync.py
```

## Next Runs

When opening a new terminal later:

```bash
cd /root/LED-Controller-OS/Helper-Tools
source .venv/bin/activate
python filesync.py
```

## Exit

To leave the virtual environment:

```bash
deactivate
```

## Notes

- The app auto-detects serial ports using `pyserial`.
- Default baud is `115200`.
- Upload uses the device commands: `store --alloc`, `store --append`, `store --finish`.
- Download uses `cat --hex` and supports version offsets via `-N`.
