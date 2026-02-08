#pragma once

#include "ICommand.h"
#include <iostream>
#include <iomanip>

// External symbols from linker script
extern char __StackLimit;
extern char __StackTop;
extern char __HeapLimit;
extern char __end__;

class MemInfoCommand : public ICommand {
public:
  // Returns the name of the command
  const std::string getName() const override { return "meminfo"; }

  const std::string getHelp() const override {
    return "Usage: meminfo\n"
           "       Displays current stack and heap memory usage.";
  }

  // Executes the command
  int execute(const std::vector<std::string> &args) override {
    // Get stack information
    char stack_var;
    char *stack_ptr = &stack_var;
    size_t stack_top = reinterpret_cast<size_t>(&__StackTop);
    size_t stack_limit = reinterpret_cast<size_t>(&__StackLimit);
    size_t current_stack = reinterpret_cast<size_t>(stack_ptr);
    size_t stack_size = stack_top - stack_limit;
    size_t stack_used = stack_top - current_stack;
    
    // Get heap information
    // Heap starts at __end__ (end of BSS) and grows upward to __HeapLimit
    size_t heap_base = reinterpret_cast<size_t>(&__end__);
    size_t heap_limit = reinterpret_cast<size_t>(&__HeapLimit);
    size_t heap_size = heap_limit - heap_base;
    
    // Calculate total RAM size (RP2040 has 264KB)
    size_t total_ram = reinterpret_cast<size_t>(&__StackTop) - 0x20000000;

    std::cout << "=== Memory Information ===" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Stack:" << std::endl;
    std::cout << "  Total Size:  " << std::setw(8) << stack_size << " bytes" << std::endl;
    std::cout << "  Used:        " << std::setw(8) << stack_used << " bytes" << std::endl;
    std::cout << "  Free:        " << std::setw(8) << (stack_size - stack_used) << " bytes" << std::endl;
    std::cout << "  Usage:       " << std::setw(7) << std::fixed << std::setprecision(1) 
              << (100.0 * stack_used / stack_size) << "%" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Heap:" << std::endl;
    std::cout << "  Available:    " << std::setw(8) << heap_size << " bytes" << std::endl;
    std::cout << "  (Dynamic allocation tracking not available)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Total RAM:      " << std::setw(8) << total_ram << " bytes" << std::endl;
    std::cout << "Stack used:     " << std::setw(8) << stack_used << " bytes" << std::endl;
    
    return 0; // Return 0 to indicate success
  }
};
