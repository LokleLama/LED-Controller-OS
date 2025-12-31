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
           "       Stores content in the specified file\n"
           "           -b64: when this flag has been set the content is encoded in Base64";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args[1] == "-h" || args.size() < 3 || args.size() > 4){
      std::cout << getHelp() << std::endl;
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
    
    bool decodeBase64 = false;
    for (size_t i = 2; i < args.size(); ++i) {
      if (args[i] == "-b64") {
        decodeBase64 = true;
        continue;
      }
      if (decodeBase64) {
        auto decoded = Base64::decode(args[i]);
        std::string decodedStr(decoded.begin(), decoded.end());
        file->write(decodedStr);
      } else {
        file->write(args[i]);
      }
    }
    std::cout << std::endl;
    return 0; // Return 0 to indicate success
  }

private:
  const Console &_console;
};