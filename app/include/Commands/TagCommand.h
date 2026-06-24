#pragma once

#include "../ICommand.h"
#include "../Utils/base64.h"
#include "../Utils/hexadecimal.h"
#include "Console.h"
#include <iostream>

class TagCommand : public ICommand {
public:
  // Constructor
  TagCommand(const Console &console) : _console(console) {}
  // Returns the name of the command
  const std::string getName() const override { return "tag"; }

  const std::string getHelp() const override {
    return "Usage: tag <file> [-r] [-c <version> <description>]\n"
           "\n"
           "       tag <file> -r\n"
           "           Read all tags of the specified file\n"
           "       tag <file> -c <version> <description>\n"
           "           Create a new tag for the specified file with the given version and description\n"
           "\n";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args[1] == "-h" || args[1] == "--help" || args.size() < 2) {
        std::cout << getHelp() << std::endl;
        return 0;
    }

    if(args.size() < 3) {
        std::cout << "Error: Insufficient arguments provided." << std::endl;
        return -1;
    }

    std::string filename = args[1];
    auto file = _console.currentDirectory->openFile(filename);
    if (file == nullptr) {
        std::cout << "Error: Unable to open file '" << filename << "'" << std::endl;
        return -1;
    }

    if (args[2] == "-r") {
        auto version = file->getVersion();
        for(size_t v = 1; v <= version; ++v) {
            auto version_file = file->openVersion(v);
            if(version_file != nullptr) {
                std::string tag_description = version_file->readTag();
                if(tag_description.empty()) {
                    std::cout << "Version " << v << ": [no tag]" << std::endl;
                } else {
                    std::cout << "Version " << v << ": " << tag_description << std::endl;
                }
            } else {
                std::cout << "Version " << v << ": Unable to open version" << std::endl;
            }
        }
        return 0;
    }

    if (args[2] == "-c") {
        if (args.size() < 5) {
            std::cout << "Error: Version and description must be specified for creating a tag." << std::endl;
            return -1;
        }
        size_t version = std::strtoul(args[3].c_str(), nullptr, 0);
        const std::string& tag_description = args[4];

        auto version_file = file->openVersion(version);
        if(version_file == nullptr) {
            std::cout << "Error: Unable to open version " << version << " of file '" << filename << "'" << std::endl;
            return -1;
        }
        if(!version_file->createTag(tag_description)) {
            std::cout << "Error: Unable to create tag for version " << version << " of file '" << filename << "'" << std::endl;
            return -1;
        }

        return 0;
    }

    return -1;
  }

private:
  const Console &_console;

  std::shared_ptr<SPFS::File> _currentFile = nullptr;
};