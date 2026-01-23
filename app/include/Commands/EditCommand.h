#pragma once

#include "../ICommand.h"
#include "../Console.h"
#include <iostream>
#include <string>
#include <vector>

class EditCommand : public ICommand {
public:
  // Constructor
  EditCommand(Console &console) : _console(console), _isEditing(false) {}

  // Returns the name of the command
  const std::string getName() const override { return "edit"; }

  const std::string getHelp() const override {
    return "Usage: edit <filename>\n"
           "\n"
           "Opens a simple line-based text editor for creating/editing files.\n"
           "\n"
           "Editor Commands (while editing):\n"
           "  .save          - Save the file and exit editor\n"
           "  .quit          - Discard changes and exit editor\n"
           "  .list          - Show current buffer contents with line numbers\n"
           "  .clear         - Clear the buffer\n"
           "  .del <line>    - Delete line number (1-based)\n"
           "  .help          - Show this help\n"
           "\n"
           "All other lines are added to the file buffer.\n"
           "Note: Use Unix line endings (LF). Files are saved to current directory.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (_isEditing) {
      // We're in edit mode, handle edit commands
      return handleEditMode(args);
    }
    
    // Not editing, this is the initial 'edit <filename>' command
    if (args.size() < 2) {
      std::cout << "Usage: " << args[0] << " <filename>" << std::endl;
      return -1;
    }

    _filename = args[1];
    _buffer.clear();
    _isEditing = true;
    
    // Notify console that we're entering edit mode
    _console.setEditCommand(this);
    
    // Try to load existing file
    auto dir = _console.currentDirectory;
    if (dir) {
      auto file = dir->openFile(_filename);
      if (file) {
        std::cout << "Loading existing file '" << _filename << "'..." << std::endl;
        auto stream = file->getInputStream();
        std::string line;
        while (std::getline(*stream, line)) {
          _buffer.push_back(line);
        }
        std::cout << "Loaded " << _buffer.size() << " lines." << std::endl;
      } else {
        std::cout << "Creating new file '" << _filename << "'." << std::endl;
      }
    }
    
    std::cout << "\n=== Edit Mode (type .help for commands, .save to save) ===" << std::endl;
    return 0;
  }

  // Check if currently in edit mode
  bool isEditing() const { return _isEditing; }
  
  // Process a line while in edit mode (called by Console)
  int processEditLine(const std::string &line) {
    // Parse line into args for command handling
    std::vector<std::string> args;
    args.push_back("edit");
    
    if (!line.empty() && line[0] == '.') {
      // It's a command, parse it
      size_t pos = 0;
      size_t start = 0;
      while (pos <= line.length()) {
        if (pos == line.length() || line[pos] == ' ') {
          if (pos > start) {
            args.push_back(line.substr(start, pos - start));
          }
          start = pos + 1;
        }
        pos++;
      }
    } else {
      // Regular line, add to args as-is
      args.push_back(line);
    }
    
    return handleEditMode(args);
  }

private:
  Console &_console;
  bool _isEditing;
  std::string _filename;
  std::vector<std::string> _buffer;

  int handleEditMode(const std::vector<std::string> &args) {
    if (args.size() < 2) {
      // Empty line, just add it
      _buffer.push_back("");
      return 0;
    }

    const std::string &cmd = args[1];

    if (cmd == ".save") {
      return saveAndExit();
    } else if (cmd == ".quit") {
      std::cout << "Discarding changes." << std::endl;
      _isEditing = false;
      _buffer.clear();
      return 0;
    } else if (cmd == ".list") {
      return listBuffer();
    } else if (cmd == ".clear") {
      _buffer.clear();
      std::cout << "Buffer cleared." << std::endl;
      return 0;
    } else if (cmd == ".del") {
      if (args.size() < 3) {
        std::cout << "Usage: .del <line_number>" << std::endl;
        return -1;
      }
      return deleteLine(std::stoi(args[2]));
    } else if (cmd == ".help") {
      std::cout << getHelp() << std::endl;
      return 0;
    } else if (cmd[0] == '.') {
      std::cout << "Unknown command: " << cmd << " (type .help for help)" << std::endl;
      return -1;
    } else {
      // Not a command, add the entire line to buffer
      // Reconstruct the original line (everything after "edit ")
      std::string fullLine = cmd;
      for (size_t i = 2; i < args.size(); i++) {
        fullLine += " " + args[i];
      }
      _buffer.push_back(fullLine);
      return 0;
    }
  }

  int listBuffer() {
    std::cout << "=== Buffer Contents (" << _buffer.size() << " lines) ===" << std::endl;
    for (size_t i = 0; i < _buffer.size(); i++) {
      std::cout << (i + 1) << ": " << _buffer[i] << std::endl;
    }
    return 0;
  }

  int deleteLine(int lineNum) {
    if (lineNum < 1 || lineNum > static_cast<int>(_buffer.size())) {
      std::cout << "Line number out of range (1-" << _buffer.size() << ")" << std::endl;
      return -1;
    }
    _buffer.erase(_buffer.begin() + lineNum - 1);
    std::cout << "Line " << lineNum << " deleted." << std::endl;
    return 0;
  }

  int saveAndExit() {
    auto dir = _console.currentDirectory;
    if (!dir) {
      std::cout << "Error: No filesystem available." << std::endl;
      return -1;
    }

    // Create the file content
    std::string content;
    for (size_t i = 0; i < _buffer.size(); i++) {
      content += _buffer[i];
      if (i < _buffer.size() - 1) {
        content += "\n";
      }
    }
    
    // Check if file exists and delete it
    auto existingFile = dir->openFile(_filename);
    if (existingFile) {
      std::cout << "Overwriting existing file..." << std::endl;
      dir->remove(existingFile);
    }

    // Create and write the file
    auto newFile = dir->createFile(_filename);
    if (!newFile) {
      std::cout << "Error: Failed to create file." << std::endl;
      return -1;
    }

    if (!newFile->write(reinterpret_cast<const uint8_t*>(content.c_str()), content.size())) {
      std::cout << "Error: Failed to write file." << std::endl;
      return -1;
    }

    std::cout << "File '" << _filename << "' saved (" << _buffer.size() 
              << " lines, " << content.size() << " bytes)." << std::endl;
    
    _isEditing = false;
    _buffer.clear();
    return 0;
  }
};
