// Console renderer for 8x8 dot matrix LED frames
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

class DotMatrixRenderer {
public:
    // Map logical (row, col) to the zigzag-corrected frame index.
    // Odd columns are wired bottom-to-top on WS2812 8x8 modules.
    static size_t frameIndex(int row, int col) {
        int led_row = (col & 1) ? (7 - row) : row;
        return led_row + col * 8;
    }

    // Render an 8xN LED frame (zigzag layout) to a multi-line string.
    // cols defaults to 8 (single module). For daisy-chained modules, pass cols = num_modules * 8.
    static std::string render(const std::vector<uint32_t>& frame, int cols = 8) {
        std::string result;
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < cols; col++) {
                size_t idx = frameIndex(row, col);
                uint32_t pixel = (idx < frame.size()) ? frame[idx] : 0;
                result += pixel ? "\u2588\u2588" : "  ";
            }
            result += '\n';
        }
        return result;
    }

    // Print the frame to stdout with an optional label.
    static void print(const std::vector<uint32_t>& frame, const std::string& label = "", int cols = 8) {
        if (!label.empty()) {
            std::cout << "=== " << label << " ===" << std::endl;
        }
        std::cout << render(frame, cols);
    }

    // Extract an 8x8 boolean grid (row-major) from the LED frame.
    // grid[row][col] = true if LED is lit. Accounts for zigzag wiring.
    static std::vector<std::vector<bool>> toGrid(const std::vector<uint32_t>& frame) {
        std::vector<std::vector<bool>> grid(8, std::vector<bool>(8, false));
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                grid[row][col] = frame[frameIndex(row, col)] != 0;
            }
        }
        return grid;
    }

    // Get the lit pattern for a given character from the MatrixChar8x8 font.
    // Returns an 8x8 boolean grid (row-major) matching the font definition.
    static std::vector<std::vector<bool>> getExpectedChar(const uint8_t* charData) {
        std::vector<std::vector<bool>> grid(8, std::vector<bool>(8, false));
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                grid[row][col] = (charData[row] >> col) & 1;
            }
        }
        return grid;
    }
};
