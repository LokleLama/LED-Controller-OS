#pragma once

#include "../ICommand.h"
#include "../Utils/base64.h"
#include <iostream>

class EvalCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "eval"; }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    for (size_t i = 1; i < args.size(); ++i) {
      if (args[i] == "-b64") {
        _useBase64 = true;
        continue;
      }
      if (_useBase64) {
        // If Base64 encoding is enabled, encode the argument
        auto decoded = Base64::decode(args[i]);
        std::string decodedStr(decoded.begin(), decoded.end());
        std::cout << decodedStr << " ";
        continue;
      }
      std::cout << args[i] << " ";
    }
    std::cout << std::endl;
    return 0; // Return 0 to indicate success
  }

private:
  bool _useBase64 = false; // Flag to indicate if Base64 encoding is used
};