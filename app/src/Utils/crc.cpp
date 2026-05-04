#include "Utils/crc.h"

uint8_t CRC8::calculate(const uint8_t* data, int length) {
    uint8_t crc = INITIAL_VALUE; // Initial value
    for (int i = 0; i < length; ++i) {
        crc = crc8_table[crc ^ data[i]];
    }
    return crc;
}

uint8_t CRC8::calculate(const uint8_t* data, int length, uint8_t polynomial, uint8_t initial_value) {
    uint8_t crc = initial_value; // Initial value
    for (int i = 0; i < length; ++i) {
        crc ^= data[i]; // XOR byte into the CRC
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial; // Polynomial: x^8 + x^2 + x + 1 (0x07)
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}