# Text Editor Usage Guide

## Overview
The `edit` command provides a simple line-based text editor for creating and editing files directly on the device.

## Basic Usage

### Create a New File
```bash
edit myfile.txt
```

### Edit an Existing File
```bash
edit myfile.txt
```
If the file exists, it will be loaded into the editor buffer.

## Editor Commands

While in edit mode, these commands are available:

### `.save`
Save the file and exit the editor.
```
.save
```

### `.quit`
Exit the editor without saving changes.
```
.quit
```

### `.list`
Show the current buffer contents with line numbers.
```
.list
```

### `.clear`
Clear all lines from the buffer.
```
.clear
```

### `.del <line>`
Delete a specific line (1-based line numbering).
```
.del 5
```

### `.help`
Show editor help.
```
.help
```

## Example Session

```bash
# Create a animation script
root > edit animation.txt
Creating new file 'animation.txt'.

=== Edit Mode (type .help for commands, .save to save) ===
led1 use 0
led1 use 1
led1 use 2
led1 use 3
.list
=== Buffer Contents (4 lines) ===
1: led1 use 0
2: led1 use 1
3: led1 use 2
4: led1 use 3
.save
File 'animation.txt' saved (4 lines, 47 bytes).
root > 

# Execute the script
root > exec animation.txt

# View the file
root > cat animation.txt
```

## Tips

1. **Line endings**: The editor uses Unix line endings (LF)
2. **Current directory**: Files are saved to the current directory (`pwd` shows this in the prompt)
3. **Overwriting**: Editing an existing file will overwrite it when you save
4. **No confirmation**: `.quit` discards changes immediately without confirmation
5. **Empty lines**: Pressing Enter without text adds an empty line to the buffer

