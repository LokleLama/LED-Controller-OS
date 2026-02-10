#pragma once

#include "../ICommand.h"
#include "../Utils/base64.h"
#include <iostream>

class StoreCommand : public ICommand {
public:
  // Constructor
  StoreCommand(const Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "store"; }

  const std::string getHelp() const override {
    return "Usage: store <file> [-b64] <content>\n"
           "\n"
           "       store <file> --alloc <size>\n"
           "       store --append [-n] [-b64 | -hex] <content>\n"
           "       store --finish\n"
           "\n"
           "       Stores content in the specified file\n"
           "           -b64: when this flag has been set the content is encoded in Base64\n"
           "           -hex: when this flag has been set the content is encoded in hexadecimal\n"
           "\n"
           "           --alloc <size>: allocates a new content block of the specified size (in bytes) for appending data\n"
           "                           after allocation, use --append to add data and --finish to finalize the content block\n"
           "           --append <content>: appends the specified content to the current content block\n"
           "           -n: do not add a newline after the appended content\n"
           "           --finish: finalizes the current content block\n";
           "                     Note: If the content is smaller then the allocated size, the remaining space will be not be used and remains free.\n";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args[1] == "-h" || args[1] == "--help" || args.size() < 2) {
      std::cout << getHelp() << std::endl;
      return 0;
    }

    if (args[1] == "--append") {
      if (_currentFile == nullptr) {
        std::cout << "Error: No file is currently opened for appending. Use 'store --alloc <size>' to allocate a new content block." << std::endl;
        return -1;
      }
      bool newLine = true;
      bool decodeBase64 = false;
      bool decodeHex = false;
      for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "-b64") {
          decodeBase64 = true;
          continue;
        }
        if (args[i] == "-hex") {
          decodeHex = true;
          continue;
        }
        if (args[i] == "-n") {
          newLine = false;
          continue;
        }
        if (decodeBase64) {
          auto decoded = Base64::decode(args[i]);
          std::string decodedStr(decoded.begin(), decoded.end());
          if (!_currentFile->append(decodedStr)) {
            std::cout << "Error: Unable to append decoded content." << std::endl;
            return -1;
          }
          decodeBase64 = false;
        } else if (decodeHex) {
          std::vector<uint8_t> decoded = Hexadecimal::decode(args[i]);
          std::string decodedStr(decoded.begin(), decoded.end());
          if (!_currentFile->append(decodedStr)) {
            std::cout << "Error: Unable to append decoded content." << std::endl;
            return -1;
          }
          decodeHex = false;
        } else {
          bool success = false;
          if(newLine) {
            success = _currentFile->append(args[i] + "\n");
          }else{
            success = _currentFile->append(args[i]);
          }
          if (!success) {
            std::cout << "Error: Unable to append content." << std::endl;
            return -1;
          }
        }
      }
      std::cout << "Content appended successfully." << std::endl;
      return 0;
    }

    if (args[1] == "--finish") {
      if (_currentFile == nullptr) {
        std::cout << "Error: No file is currently opened for closing." << std::endl;
        return -1;
      }
      if (!_currentFile->finishContent()) {
        std::cout << "Error: Unable to finalize content." << std::endl;
        return -1;
      }
      std::cout << "Content finalized successfully." << std::endl;
      _currentFile = nullptr;
      return 0;
    }

    std::string filename = args[1];
    auto file = _console.currentDirectory->openFile(filename);
    if (file == nullptr) {
      file = _console.currentDirectory->createFile(filename);
      if (file == nullptr) {
        std::cout << "Error: Unable to create or open file '" << filename << "'" << std::endl;
        return -1;
      }
    }
    if (args[2] == "--alloc") {
      if (_currentFile != nullptr) {
        std::cout << "Error: File already opened for appending. Use 'store --finish' to close the file first." << std::endl;
        return -1;
      }
      if (args.size() < 4) {
        std::cout << "Error: Size not specified for allocation." << std::endl;
        return -1;
      }
      _currentFile = file;
      size_t size = std::stoul(args[3]);
      if (!_currentFile->allocateContenSize(size)) {
        std::cout << "Error: Unable to allocate content size of " << size << " bytes." << std::endl;
        return -1;
      }
      std::cout << "Allocated content size of " << size << " bytes." << std::endl;
      return 0;
    }
    
    bool decodeBase64 = false;
    bool decodeHex = false;
    for (size_t i = 2; i < args.size(); ++i) {
      if (args[i] == "-b64") {
        decodeBase64 = true;
        continue;
      }
      if (args[i] == "-hex") {
        decodeHex = true;
        continue;
      }
      if (decodeBase64) {
        auto decoded = Base64::decode(args[i]);
        std::string decodedStr(decoded.begin(), decoded.end());
        file->write(decodedStr);
        decodeBase64 = false;
      } else if (decodeHex) {
        std::vector<uint8_t> decoded = Hexadecimal::decode(args[i]);
        std::string decodedStr(decoded.begin(), decoded.end());
        file->write(decodedStr);
        decodeHex = false;
      }
      else {
        file->write(args[i]);
      }
    }
    std::cout << std::endl;
    return 0; // Return 0 to indicate success
  }

private:
  const Console &_console;

  std::shared_ptr<SPFS::File> _currentFile = nullptr;
};