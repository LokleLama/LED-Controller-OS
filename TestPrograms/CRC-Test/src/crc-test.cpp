#include <cstdio>
#include <cstring>

#include "Utils/crc.h"

static void CRC_CalculateCRC8Table(uint8_t* CRC8Table) {
    uint16_t i = 0u, j = 0u;

    for (i = 0u; i < 256u; ++i) {
        uint8_t curr = i;

        for (j = 0u; j < 8u; ++j) {
            if ((curr & 0x80u) != 0u) {
                curr = (curr << 1u) ^ CRC8::POLYNOMIAL;
            } else {
                curr <<= 1u;
            }
        }

        CRC8Table[i] = curr;
    }
}

static void showCRC8Table() {
    uint8_t CRC8Table[256];
    CRC_CalculateCRC8Table(CRC8Table);

    printf("CRC-8 Table:\n");
    printf("uint8_t CRC8Table[256] = { ");
    for (int i = 0; i < 256; ++i) {
        printf("0x%02X, ", CRC8Table[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n                           ");
        }
    }
    printf("};\n");
}

int main(int argc, char **argv) {
    printf("Usage: crc-test [data]\n");
    const char* data = "Hello, World!";
    if (argc > 1) {
        data = argv[1];
    }
    int data_length = strlen(data);
    
    uint8_t crc8 = CRC8::calculate(reinterpret_cast<const uint8_t*>(data), data_length, CRC8::POLYNOMIAL);
    printf("CRC-8 of \"%s\" is: 0x%02X\n", data, crc8);

    uint8_t crc8_data[data_length + 1];
    memcpy(crc8_data, data, data_length);
    crc8_data[data_length] = crc8; // Append CRC-8 to the data
    
    uint8_t crc8_check = CRC8::calculate(crc8_data, data_length + 1);
    if (crc8_check == 0) {
        printf("CRC-8 check passed, data integrity verified.\n");
    } else {
        printf("CRC-8 check failed, data integrity compromised. (0x%02X)\n", crc8_check);
        return -1;
    }
    
    uint8_t crc8_table = CRC8::calculate(reinterpret_cast<const uint8_t*>(data), data_length);
    printf("CRC-8 of \"%s\" is: 0x%02X\n", data, crc8_table);

    memcpy(crc8_data, data, data_length);
    crc8_data[data_length] = crc8_table; // Append CRC-8 to the data
    
    uint8_t crc8_check_table = CRC8::calculate(crc8_data, data_length + 1);
    if (crc8_check_table == 0) {
        printf("CRC-8 check passed, data integrity verified.\n");
    } else {
        printf("CRC-8 check failed, data integrity compromised. (0x%02X)\n", crc8_check_table);
        return -1;
    }

    showCRC8Table();
    
    return 0;
}