#include <cstdio>
#include <cstring>

#include "Utils/dataFile.h"
#include "Utils/hexadecimal.h"
#include "Utils/base64.h"

void printHelp(const char* programName) {
    printf("Usage: %s list <file>\n", programName);
    printf("       %s read <signature> [-b64 | -hex] <file>\n", programName);
    printf("       %s create <magic number> <file>\n", programName);
    printf("       %s append <signature> [-b64 | -hex] <data> <file>\n\n", programName);
    printf(" command     description\n");
    printf("  list         List all field signatures in the file\n");
    printf("  read         Read the field data for the given signature (can be a string or a hex value)\n");
    printf("  create       Create a new data file with the given magic number\n");
    printf("  append       Append a new field with the given signature to the file\n\n");
    printf(" arguments\n");
    printf("  <signature>       The field signature to read or append (can be a string of up to 3 characters or a hex value starting with 0x)\n");
    printf("  <magic number>    The magic number to use when creating a new data file (can be a string of up to 4 characters or a hex value starting with 0x)\n");
    printf("  <data>            The data to append to the file (can be a string or base64 or hex encoded)\n");
    printf("  <file>            The data file to read or create\n");
}

std::vector<uint8_t> openFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if(file == nullptr) {
        return {};
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::vector<uint8_t> data(size);
    fread(data.data(), 1, size, file);
    fclose(file);
    return data;
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        printHelp(argv[0]);
        return -1;
    }
    
    if(strcmp(argv[1], "list") == 0) {
        if (argc != 3) {
            printHelp(argv[0]);
            return -1;
        }

        auto fdata = openFile(argv[2]);
        if(fdata.empty()) {
            printf("Failed to open file %s\n", argv[2]);
            return -1;
        }

        dataFileReader reader(fdata.data(), fdata.size());
        if (!reader.isFileHeaderValid()) {
            printf("Invalid file header in file %s\n", argv[2]);
            return -1;
        }

        auto current = reader.start();
        printf("Found fields:\n");
        while(current != nullptr && current != reader.end()) {
            printf("    %s (0x%04X): %u bytes @ %lu bytes\n", 
                dataFileReader::signatureToString(reader.getFieldSignature(current)).c_str(), 
                reader.getFieldSignature(current), 
                reader.getDataSize(current), 
                reinterpret_cast<const uint8_t*>(current) - fdata.data());
            current = reader.next(current);
        }
    } else if(strcmp(argv[1], "read") == 0) {
        if (argc < 4) {
            printHelp(argv[0]);
            return -1;
        }
        bool is_base64 = false;
        bool is_hex = false;
        const char* filename = argv[3];
        if (strcmp(argv[3], "-b64") == 0) {
            is_base64 = true;
            filename = argv[4];
        }else if (strcmp(argv[3], "-hex") == 0) {
            is_hex = true;
            filename = argv[4];
        }

        auto fdata = openFile(filename);
        if(fdata.empty()) {
            printf("Failed to open file %s\n", filename);
            return -1;
        }

        dataFileReader reader(fdata.data(), fdata.size());
        if (!reader.isFileHeaderValid()) {
            printf("Invalid file header in file %s\n", filename);
            return -1;
        }
        dataFileFieldSignature_t signature;
        if(strncmp(argv[2], "0x", 2) == 0) {
            signature = static_cast<dataFileFieldSignature_t>(strtoul(argv[2], nullptr, 16));
        } else {
            signature = dataFileReader::makeSignature(argv[2]);
        }
        size_t data_size;
        const void* data = reader.getFieldData(signature, &data_size);
        if(data != nullptr) {
            printf("Data for signature %s (0x%04X):\n", dataFileReader::signatureToString(signature).c_str(), signature);
            if(is_base64) {
                auto data_vec = std::vector<uint8_t>(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + data_size);
                std::string encoded = Base64::encode(data_vec);
                printf("%s\n", encoded.c_str());
            } else if(is_hex) {
                auto data_vec = std::vector<uint8_t>(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + data_size);
                std::string encoded = Hexadecimal::encode(data_vec);
                printf("%s\n", encoded.c_str());
            } else {
                fwrite(data, 1, data_size, stdout);
            }
            printf("\n");
        } else {
            printf("Field with signature %s (0x%04X) not found in file %s\n", dataFileReader::signatureToString(signature).c_str(), signature, filename);
        }
    } else if(strcmp(argv[1], "create") == 0) {
        if (argc != 4) {
            printHelp(argv[0]);
            return -1;
        }
        uint32_t magic;
        if(strncmp(argv[2], "0x", 2) == 0) {
            magic = static_cast<uint32_t>(strtoul(argv[2], nullptr, 16));
        } else {
            magic = 0;
            for(size_t i = 0; i < strlen(argv[2]) && i < 4; i++) {
                magic >>= 8;
                magic |= argv[2][i] << 24;
            }
        }
        
        uint8_t buffer[256] = {0};
        dataFileMemoryWriter writer(buffer, sizeof(buffer));
        if(writer.setHeader(magic)) {
            FILE* file = fopen(argv[3], "wb");
            if(file != nullptr) {
                fwrite(buffer, 1, writer.getFileSize(), file);
                fclose(file);
            }else{
                printf("Failed to create file %s\n", argv[3]);
                return -1;
            }
        }else{
            printf("Failed to create file header with magic number %s (0x%08X)\n", argv[2], magic);
            return -1;
        }
    } else if(strcmp(argv[1], "append") == 0) {
        if (argc < 5) {
            printHelp(argv[0]);
            return -1;
        }
        dataFileFieldSignature_t signature;
        if(strncmp(argv[2], "0x", 2) == 0) {
            signature = static_cast<dataFileFieldSignature_t>(strtoul(argv[2], nullptr, 16));
        } else {
            signature = dataFileReader::makeSignature(argv[2]);
        }

        bool is_base64 = false;
        bool is_hex = false;
        const char* data_arg = argv[3];
        const char* filename = argv[4];
        if (strcmp(argv[3], "-b64") == 0) {
            is_base64 = true;
            data_arg = argv[4];
            filename = argv[5];
        }else if (strcmp(argv[3], "-hex") == 0) {
            is_hex = true;
            data_arg = argv[4];
            filename = argv[5];
        }

        std::vector<uint8_t> data;
        if(is_base64) {
            data = Base64::decode(data_arg);
        } else if(is_hex) {
            data = Hexadecimal::decode(data_arg);
        } else {
            data = std::vector<uint8_t>(data_arg, data_arg + strlen(data_arg));
        }
        
        FILE* file = fopen(filename, "r+b");
        if(file == nullptr) {
            printf("Failed to open file %s for appending\n", filename);
            return -1;
        }
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        size_t new_size = file_size + 32 + data.size();
        uint8_t file_data[new_size];
        fread(file_data, 1, file_size, file);
        fclose(file);

        dataFileMemoryWriter writer(file_data, new_size);

        if(!writer.setAppendMode()) {
            printf("Failed to set append mode for file %s, not enough space to append new field\n", filename);
            return -1;
        }
        if(!writer.addField(signature, data.data(), static_cast<uint16_t>(data.size()))) {
            printf("Failed to append field with signature %s (0x%04X) to file %s, not enough space to add the field\n", dataFileReader::signatureToString(signature).c_str(), signature, filename);
            return -1;
        }

        file = fopen(filename, "wb");
        if(file != nullptr) {
            fwrite(file_data, 1, writer.getFileSize(), file);
            fclose(file);
        } else {
            printf("Failed to open file %s for writing\n", filename);
            return -1;
        }
    } else {
        printHelp(argv[0]);
        return -1;
    }
    
    return 0;
}