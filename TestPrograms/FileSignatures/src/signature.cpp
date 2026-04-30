#include <cstdio>
#include <cstring>

#include "Utils/dataFile.h"

int main(int argc, char **argv) {
    printf("Usage: crc-test <string | signature>\n");

    if (argc < 2) {
        return -1;
    }
    
    if(argv[1][0] == '0' && argv[1][1] == 'x') {
        // parse as hex
        dataFileFieldSignature_t signature = std::strtoul(argv[1], nullptr, 16);
        printf("Signature: 0x%04X, String: %s, Valid: %s\n", signature, dataFileReader::signatureToString(signature).c_str(), dataFileReader::checkSignature(signature) ? "Yes" : "No");
    } else {
        // parse as string
        dataFileFieldSignature_t signature = dataFileReader::makeSignature(argv[1], strlen(argv[1]));
        printf("String: %s, Signature: 0x%04X, Valid: %s\n", argv[1], signature, dataFileReader::checkSignature(signature) ? "Yes" : "No");
    }
    
    return 0;
}