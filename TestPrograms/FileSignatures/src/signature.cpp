#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>

#include "Utils/dataFile.h"


static constexpr uint8_t _signature_lookup[40] = {
    '.', 'e', '0', 'a', '1', 's', 'i', 'u', 
    '2', 'd', 'r', 'j', 'n', 'f', 'c', 'k', 
    't', 'z', 'l', 'w', 'h', 'y', 'p', 'q',
    'o', 'b', 'g', '3', 'm', 'x', 'v', '4',

    ' ', '?', '!', '5', '6', '7', '8', '9' };

static constexpr uint8_t _bit_count_lookup[32] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5 };


bool checkSignature(uint32_t signature) {
    uint8_t bitcount = 0;
    for(size_t i = 0; i < sizeof(uint32_t) * 2; i++) {
        bitcount += _bit_count_lookup[signature & 0x0F];
        signature >>= 4;
    }
    return (bitcount % 2) == 0; // check if the number of 1 bits is even
}


const std::string signatureToString(uint32_t signature) {
    std::string result = "      ";
    int i = 0;
    for(; i < 3; i++) {
        uint8_t value = signature & 0x1F;
        result[5 - i] = _signature_lookup[value];
        signature >>= 5;
    }
    signature >>= 1;
    for(; i < 6; i++) {
        uint8_t value = signature & 0x1F;
        result[5 - i] = _signature_lookup[value];
        signature >>= 5;
    }
    return result;
}

uint32_t makeSignature(const char* signature) {
    uint32_t result = 0;
    uint8_t bitcount = 0;
    size_t i = 0;
    for(; i < 3; i++) {
        for(size_t j = 0; j < sizeof(_signature_lookup); j++) {
            if(_signature_lookup[j] == tolower(signature[i])) { // use only lower case characters
                auto value = j & 0x1F;
                result <<= 5;
                result |= value;
                bitcount += _bit_count_lookup[value];
                break;
            }
        }
    }
    if(bitcount % 2 == 1) {
        result |= (1 << 15); // set the parity bit if the number of 1 bits is odd to make the total number of 1 bits even
    }
    result <<= 1; // Shift the first 3 characters to the upper 24 bits
    bitcount = 0;
    for(; i < 6; i++) {
        for(size_t j = 0; j < sizeof(_signature_lookup); j++) {
            if(_signature_lookup[j] == tolower(signature[i])) {
                auto value = j & 0x1F;
                result <<= 5;
                result |= value;
                bitcount += _bit_count_lookup[value];
                break;
            }
        }
    }
    if(bitcount % 2 == 1) {
        result |= (1 << 15); // set the parity bit if the number of 1 bits is odd to make the total number of 1 bits even
    }
    return result;
}

int main(int argc, const char **argv) {
    printf("Usage: %s <string | signature>\n", argv[0]);

    if (argc < 2) {
        return -1;
    }
    
    if(argv[1][0] == '0' && argv[1][1] == 'x') {
        if(strlen(argv[1]) > 6) {
            uint32_t signature = std::strtoul(argv[1], nullptr, 16);
            printf("Signature: 0x%08X, String: %s, Valid: %s\n", signature, signatureToString(signature).c_str(), checkSignature(signature) ? "Yes" : "No");
        }else{
            // parse as hex
            dataFileFieldSignature_t signature = std::strtoul(argv[1], nullptr, 16);
            printf("Signature: 0x%04X, String: %s, Valid: %s\n", signature, dataFileReader::signatureToString(signature).c_str(), dataFileReader::checkSignature(signature) ? "Yes" : "No");
        }
    } else {
        if(strlen(argv[1]) > 6) {
            printf("String too long for a signature. Maximum length is 6 characters.\n");
            return -1;
        }
        if(strlen(argv[1]) > 3) {
            // parse as string
            uint32_t signature = makeSignature(argv[1]);
            printf("String: %s, Signature: 0x%08X, Valid: %s\n", argv[1], signature, checkSignature(signature) ? "Yes" : "No");
        }else{
            // parse as string
            dataFileFieldSignature_t signature = dataFileReader::makeSignature(argv[1], strlen(argv[1]));
            printf("String: %s, Signature: 0x%04X, Valid: %s\n", argv[1], signature, dataFileReader::checkSignature(signature) ? "Yes" : "No");
        }
    }
    
    return 0;
}