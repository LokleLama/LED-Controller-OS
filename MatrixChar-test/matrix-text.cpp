
#include "MatrixChar5x5.h"
#include <iostream>
#include <vector>

int _bit_vector_length;
int _total_columns;
int _current_offset;
std::vector<uint32_t> _ledData;

void setValue(const std::string& value){
  if (value.length() < 1) {
    std::cerr << "Invalid value length for dotMatrix5x5 display: " << value << std::endl;
    return;
  }

  _bit_vector_length = (value.length() + 2) * 6; // Each character is 5 columns wide + 1 spacer column
  _current_offset = 0; // Start at the beginning of the LED data

  _total_columns = ((_bit_vector_length + 29) / 30);

  _ledData.resize(_total_columns * 5, 0); // Initialize all LEDs to off

  const char* str = value.c_str();
  for (size_t i = 0; i < value.length(); i++) {
    int data_index = (i + 1) / 5;
    int bit_offset = ((i + 1) - (data_index * 5)) * 6;

    const uint8_t* charData = MatricChar5x5::getChar(str[i]);

    for (int col = 0; col < 5; col++) {
      uint32_t columnData = charData[col];
      columnData = (columnData & 0x1F) << bit_offset; // Shift to correct position in the LED data

      _ledData[data_index + col * _total_columns] |= columnData; // Combine with existing data for this column
    }
  }
}

int main() {
    for(int c = ' '; c <= '~'; c++) {
        const uint8_t* charData = MatricChar5x5::getChar(c);
        std::cout << "Character: '" << (char)c << "'\n";
        for(int i = 0; i < 5; i++) {
            for(int j = 0; j < 5; j++) {
                std::cout << ((charData[i] & (1 << j)) ? '#' : ' ');
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    setValue("Hello, World!");

    for(int row = 0; row < 5; row++) {
        for(int col = 0; col < _total_columns; col++) {
            uint32_t columnData = _ledData[col + row * _total_columns];
            for(int bit = 0; bit < 30; bit++) {
                std::cout << ((columnData & 1) ? '#' : ' ');
                columnData >>= 1;
            }
        }
        std::cout << "\n";
    }

    return 0;
}

