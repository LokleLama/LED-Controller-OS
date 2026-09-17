# SPFS — Simple Pico File System

## Overview

SPFS (Simple Pico File System) is a purpose-built flash file system for the Raspberry Pi Pico / Pico 2 (RP2040 / RP2350). It stores files and directories directly in the upper portion of the on-chip flash memory, leaving the lower portion for the application firmware.

### Flash Layout

| Region | Origin | Size |
|---|---|---|
| Firmware (program) | `0x10000000` | Total flash − 256 KB |
| SPFS filesystem | End of firmware | 256 KB |

For a 2 MB board (RP2040 default), the filesystem starts at offset `0x1E0000`; for a 4 MB board (RP2350 default) it starts at offset `0x3C0000`. These values are injected at compile time via `SPFS_FLASH_OFFSET` and `SPFS_FLASH_SIZE`.

---

## Key Features

- **Hierarchical directories** — create nested subdirectories to organise files.
- **File versioning** — every `write` to an existing file creates a new version while keeping all previous versions intact (append-only flash semantics).
- **Chunked / streaming writes** — large files can be written in multiple `--append` passes after pre-allocating the content block; this avoids buffering the whole file in RAM.
- **Version tags** — attach a human-readable description string to any specific version of a file.
- **Script execution** — text files containing console commands can be executed line-by-line with `exec`.
- **Hardlinks** — a file or directory can be referenced under multiple names.
- **Block-level integrity** — CRC32/CRC16 is used on block headers; the block usage map distinguishes free, used, reserved, directory, tag, and bad blocks.
- **Auto-mount at boot** — on start-up `Main.cpp` calls `searchFileSystem()` and, if no filesystem is found, automatically calls `createNewFileSystem()`.

---

## Console Commands Reference

The commands below are available via the USB-serial console. Each command returns `0` on success or a negative value on error.

---

### `mkfs` — Create a New Filesystem

Formats the SPFS flash region and creates a fresh filesystem. The root directory is named `root`.

> **WARNING:** This erases all existing data in the filesystem area.

```
mkfs
```

**Example:**
```
> mkfs
Creating filesystem at 3932160 with size 262144
Filesystem loaded into console.
```

The new filesystem is immediately active in the console — no reboot is required.

---

### `fsinfo` — Display Filesystem Information

Shows the filesystem name, version, sizes, and a block usage map.

```
fsinfo [--no-map]
```

| Option | Description |
|---|---|
| *(none)* | Full output including the block map |
| `--no-map` | Suppress the block map for shorter output |

**Block map symbols:**

| Symbol | Meaning |
|---|---|
| `.` | Free |
| `U` | Used (generic) |
| `F` | Used by a file content block |
| `R` | Reserved / pre-allocated for a file (write in progress) |
| `D` | Used by a directory block |
| `T` | Used by a version tag |
| `B` | Bad block |

**Example:**
```
> fsinfo
 File System Information: LEDControllerFS (version 1.2.0.0)
  - Total Size: 256 kB
  - Block Size: 256 bytes
  - Free Space: 252 kB
  - Block Usage:
     * Free Blocks: 1008
     * Used Blocks: 16
        - Used Blocks for Files      : 4
        - Reserved Blocks for Files  : 0
        - Used Blocks for Directories: 10
        - Used Blocks for Tags       : 2
     * Bad Blocks: 0
Block Usage Map (.=Free, U=Used, F=Used by File, R=Reserved for File, D=Used by Directory, T=Used by Tag, B=Bad):
DDDDDDDDDDDD.... FFFF............
...
```

---

### `dir` — List Directory Contents

Lists all subdirectories and files in the current directory.

```
dir
```

**Example:**
```
> dir
 Directory of /root
 Total Files: 2 Size: 512 bytes, Size on disk: 768 bytes
 <DIR>  scripts (3 entries)
 - v3 - config.txt (256 bytes)
 - v1 - boot.sh (128 bytes)
```

The `v3` prefix in the file listing indicates the file has 3 versions.

---

### `cd` — Change Directory

Changes the current working directory. Use `..` to navigate to the parent directory.

```
cd <directory>
cd ..
```

**Examples:**
```
> cd scripts
> cd ..
```

If the directory does not exist, an error is printed and the current directory is unchanged.

---

### `md` — Make Directory

Creates a new subdirectory inside the current directory and immediately changes into it.

```
md <directory>
```

**Example:**
```
> md configs
> dir
 Directory of /root/configs
 Total Files: 0 Size: 0 bytes, Size on disk: 256 bytes
```

If the directory already exists, the console changes into it (no error is produced for the existing case, but the return code is `-1`).

---

### `cat` — Display File Contents

Reads and prints the contents of a file. By default the latest version is shown.

```
cat [--hex] [-<version_offset>] <filename>
```

| Option | Description |
|---|---|
| `--hex` | Dump content as hexadecimal bytes (16 per line) |
| `-<N>` | Show version `(current − N)`, e.g. `-1` shows the previous version |

**Examples:**
```
> cat config.txt
brightness=100
color=FF0000

> cat -1 config.txt
brightness=80
color=00FF00

> cat --hex config.txt
62 72 69 67 68 74 6e 65 73 73 3d 31 30 30 0a 63
6f 6c 6f 72 3d 46 46 30 30 30 30 0a
```

---

### `store` — Write / Append to a File

Creates or updates a file with the provided content, creating a new version each time.

#### Simple (single-write) mode

```
store <file> [<content>]
store <file> [-b64 <base64-content>]
store <file> [-hex <hex-content>]
```

Each call to `store <file>` with a content argument writes that content as a single new version. Multiple content arguments are concatenated.

**Examples:**
```
> store config.txt brightness=100
> store config.txt -b64 YnJpZ2h0bmVzcz0xMDA=
> store data.bin -hex 48656c6c6f
```

#### Chunked (streaming) mode

Use this when the content is too large to fit in a single command or must be streamed from the host in multiple transfers.

**Step 1 — Allocate** a content block of the exact final size (in bytes):
```
store <file> --alloc <size>
```

**Step 2 — Append** data chunk by chunk:
```
store --append [-n] [-b64 <data> | -hex <data> | <data>]
```

| Option | Description |
|---|---|
| `-n` | Do not add a newline after the appended text |
| `-b64 <data>` | Decode the next argument from Base64 before appending |
| `-hex <data>` | Decode the next argument from hexadecimal before appending |

**Step 3 — Finalize** the content block:
```
store --finish
```

If the write is interrupted before `--finish`, the reserved blocks remain marked `R` in the block map. Use [`fix`](#fix--repair-an-unfinished-file) to recover.

**Full chunked example** — writing a 26-byte file in two passes:
```
> store greeting.txt --alloc 26
Allocated content size of 26 bytes.
> store --append -n Hello,
Content appended successfully.
> store --append -n " World!"
Content appended successfully.
> store --finish
Content finalized successfully.
> cat greeting.txt
Hello, World!
```

---

### `tag` — Manage Version Tags

Attaches, reads, or deletes a human-readable description tag on a specific file version.

```
tag <file> -r
tag <file> -c <version> <description>
tag <file> -d <version>
```

| Sub-command | Description |
|---|---|
| `-r` | Read all tags for every version of the file |
| `-c <version> <description>` | Create a tag on the given version |
| `-d <version>` | Delete the tag on the given version |

**Examples:**
```
> tag config.txt -c 3 "increased brightness limit"
> tag config.txt -r
Version 1: initial config
Version 2: [no tag]
Version 3: increased brightness limit

> tag config.txt -d 2
```

---

### `fix` — Repair an Unfinished File

If a chunked write was interrupted (e.g. by a power loss between `--alloc` and `--finish`), the reserved blocks remain allocated but the content version is incomplete. `fix` finalises the unfinished content block so those blocks are properly accounted for.

```
fix <file>
```

**Example:**
```
> fix large_image.bin
File 'large_image.bin' has been fixed successfully.
```

---

### `exec` — Execute a Script File

Reads a text file from the filesystem line-by-line and enqueues each line as a console command.

```
exec <filename>
```

**Example script file `setup.sh`:**
```
md configs
store configs/brightness.txt brightness=100
store configs/color.txt color=FF0000
cd ..
```

**Running the script:**
```
> exec setup.sh
```

---

## Typical Usage Workflows

### First-time setup (no filesystem on device)

The firmware calls `mkfs` automatically at boot if no filesystem is found. You can also do it manually:

```
> mkfs
> fsinfo --no-map
> md data
> md scripts
```

### Transferring a binary file from the host

Use the `FileExchange` helper tool (see `Helper-Tools/FileExchange/`) to split the binary into Base64 chunks and issue the `store --alloc / --append / --finish` sequence automatically. Manually the flow looks like:

```
> store firmware.bin --alloc 4096
Allocated content size of 4096 bytes.
> store --append -b64 AAEC...  (chunk 1)
> store --append -b64 BACD...  (chunk 2)
...
> store --finish
Content finalized successfully.
```

### Version rollback inspection

```
> cat -1 config.txt    # previous version
> cat -2 config.txt    # two versions back
> tag config.txt -r    # check version descriptions
```

### Navigating the directory tree

```
> dir                  # list root
> cd scripts
> dir                  # list scripts/
> cat setup.sh
> cd ..
```

---

## Low-level Flash Commands

These commands operate directly on the raw flash memory and are independent of the SPFS filesystem layer. Use them for diagnostics only.

### `read` — Read Raw Flash Bytes

```
read <offset> <length>
```

Reads `<length>` bytes from flash starting at byte offset `<offset>` and prints them as a hexadecimal dump.

```
> read 0x1E0000 64
Data read from flash: 0x101E0000
FA A3 6C A3 ...
```

### `write` — Write Raw Flash Bytes

```
write <offset> <byte0> [<byte1> ...]
```

Reads the 256-byte page at `<offset>`, patches it with the supplied bytes starting from byte 0 of that page, and writes it back.

> **Use with extreme caution.** Writing into SPFS metadata blocks without understanding the on-disk format will corrupt the filesystem.

---

## Error Handling Notes

- All filesystem commands print a descriptive message and return `-1` on failure — the console does not abort a running script automatically.
- A file in the `R` (reserved) state means a chunked write was started but not finished. Run `fix <file>` or `mkfs` (destructive) to recover.
- `cat -<N>` where `N >= current_version` is rejected with an error message.
- `store --append` and `store --finish` operate on a single implicit "current file" slot; you cannot have two chunked writes open simultaneously.
