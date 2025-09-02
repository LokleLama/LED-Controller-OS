#pragma once

#include "../ICommand.h"
#include "../Utils/base64.h"
#include <iostream>

class EchoCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "echo"; }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    if (args[1] == "-h"){
      std::cout << "Usage: " << args[0] << " [-b64] [+b64] <message>" << std::endl;
      std::cout << "  Echos the message to the console" << std::endl;
      std::cout << "  -b64: Encode the message in Base64" << std::endl;
      std::cout << "  +b64: Decodes the message in Base64" << std::endl;
      return 0;
    }
    bool encodeBase64 = false;
    bool decodeBase64 = false;
    for (size_t i = 1; i < args.size(); ++i) {
      if (args[i] == "-b64") {
        encodeBase64 = true;
        continue;
      }
      if (args[i] == "+b64") {
        decodeBase64 = true;
        continue;
      }
      if (encodeBase64) {
        // If Base64 encoding is enabled, encode the argument
        std::string encoded = Base64::encode(
            std::vector<uint8_t>(args[i].begin(), args[i].end()));
        std::cout << encoded << " ";
        encodeBase64 = false;
        continue;
      }
      if (decodeBase64) {
        // If Base64 decoding is enabled, decode the argument
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
};