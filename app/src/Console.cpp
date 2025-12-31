#include "Console.h"
#include "Config.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>

bool Console::registerCommand(std::shared_ptr<ICommand> command) {
  if (!command) {
    return false;
  }
  commandList.push_back(command);
  return true;
}

std::shared_ptr<ICommand> Console::findCommand(const std::string &name) const {
  for (const auto &cmd : commandList) {
    if (cmd->getName() == name) {
      return cmd;
    }
  }
  return nullptr;
}

std::vector<std::string> Console::getCommandList() const {
  std::vector<std::string> commandNames;
  for (const auto &cmd : commandList) {
    commandNames.push_back(cmd->getName());
  }
  return commandNames;
}

bool Console::ExecuteLine(const std::string &line) {
  std::string processedLine = _variableStore.findAndReplaceVariables(line);

  // Process the command
  std::vector<std::string> tokens;
  std::string token;
  bool inQuotes = false;
  char quoteChar = '\0';
  
  for (size_t i = 0; i < processedLine.length(); i++) {
    char c = processedLine[i];
    
    if (!inQuotes && (c == '"' || c == '\'')) {
      // Start of quoted string
      inQuotes = true;
      quoteChar = c;
    } else if (inQuotes && c == quoteChar) {
      // End of quoted string
      inQuotes = false;
      quoteChar = '\0';
    } else if (!inQuotes && (c == ' ' || c == '\t')) {
      // Whitespace outside quotes - token separator
      if (!token.empty()) {
        tokens.push_back(token);
        token.clear();
      }
    } else {
      // Regular character or whitespace inside quotes
      token += c;
    }
  }

  if(inQuotes) {
    // Handle unclosed quotes if necessary
    std::cout << "Error: Unclosed quotation mark." << std::endl;
    return false;
  } else{
    // Add the last token if any
    if (!token.empty()) {
      tokens.push_back(token);
    }

    std::string command = tokens.empty() ? "" : tokens[0];
    auto cmd = findCommand(command);

    if (cmd != nullptr) {
      cmd->execute(tokens);
      return true;
    } else {
      std::cout << "Unknown command: " << command << std::endl;
      return false;
    }
  }
}

bool Console::ExecuteTask() {
  if (!isConnected && tud_cdc_n_connected(INTERFACE_NUMBER)) {
    isConnected = true;
    OnNewConnection();
  }

  if (tud_cdc_n_available(INTERFACE_NUMBER) == 0) {
    return true;
  }

  char buf[64];
  char echo[128];
  size_t echo_len = 0;
  uint32_t count = tud_cdc_n_read(INTERFACE_NUMBER, buf, sizeof(buf));

  for (uint32_t i = 0; i < count; i++) {
    if (buf[i] == '\r') {
      echo[echo_len++] = '\r';
      echo[echo_len++] = '\n';
      
      if( openingQuoteChar != '\0' ) {
        inputBuffer += buf[i];
        inputBuffer += '\n';
      }else{
        if (echo_len > 64) {
          tud_cdc_n_write(INTERFACE_NUMBER, echo, 64);
          tud_cdc_n_write(INTERFACE_NUMBER, &echo[64], echo_len - 64);
          echo_len = 0;
        }else{
          tud_cdc_n_write(INTERFACE_NUMBER, echo, echo_len);
          echo_len = 0;
        }
        if (!inputBuffer.empty()) {
          ExecuteLine(inputBuffer);
          inputBuffer.clear();
        }
        outputPrompt();
      }
    } else if(buf[i] == '\n'){
      // Ignore LF characters
    } else if (buf[i] == 0x7F) { // Backspace
      if (!inputBuffer.empty()) {
        inputBuffer.pop_back();
        echo[echo_len++] = buf[i];
      }
    } else if (buf[i] == '"' || buf[i] == '\'') {
      if (openingQuoteChar == '\0') {
        openingQuoteChar = buf[i];
      } else if (openingQuoteChar == buf[i]) {
        openingQuoteChar = '\0';
      }
      inputBuffer += buf[i];
      echo[echo_len++] = buf[i];
    } else if (isprint(static_cast<unsigned char>(buf[i]))) {
      inputBuffer += buf[i];
      echo[echo_len++] = buf[i];
    } else {
      echo[echo_len++] = '~';
      echo[echo_len++] = ((buf[i] >> 4) < 10) ? ('0' + (buf[i] >> 4)) : ('A' + (buf[i] >> 4) - 10);
      echo[echo_len++] = ((buf[i] & 0x0F) < 10) ? ('0' + (buf[i] & 0x0F)) : ('A' + (buf[i] & 0x0F) - 10);
    }
  }
  if (echo_len > 64) {
    tud_cdc_n_write(INTERFACE_NUMBER, echo, 64);
    tud_cdc_n_write(INTERFACE_NUMBER, &echo[64], echo_len - 64);
  }else{
    tud_cdc_n_write(INTERFACE_NUMBER, echo, echo_len);
  }
  tud_cdc_n_write_flush(INTERFACE_NUMBER);

  return true;
}

void Console::OnNewConnection() const {
  std::cout << "Clock Rate: " << clock_get_hz(clk_sys) << std::endl;
  std::cout << "Console connected. Type 'exit' to quit." << std::endl;
  outputPrompt();
}

void Console::outputPrompt() const {
  if(!currentDirectory) {
    std::cout << " > ";
  }else{
    std::cout << currentDirectory->getFullPath() << " > ";
  }
  std::cout.flush();
}