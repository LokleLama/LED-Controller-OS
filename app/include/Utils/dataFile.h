#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "Flash/SPFS.h"

/* This class is to manage and access binary data file container.
 * it has been designed to be ligth weigth and to be able to support
 * binary data for many different applications.
 * Tis class only manages the file container not the actual data.
 * 
 * Currently this class only supports memory mapped files but can be 
 * adapted to handle file streams in the future.
 * It therefore currently leaves the file as is on the disk and does
 * not copy filedata into RAM but leaves ti on the disk and outputs
 * pointers to the memory mapped file.
 */

using dataFileFieldSignature_t = uint16_t;

class dataFileReader {
private:
    dataFileReader(const uint8_t* data, size_t size, std::shared_ptr<SPFS::ReadOnlyFile> file);
public:
    dataFileReader(std::shared_ptr<SPFS> spfs, std::string fileName) : dataFileReader(spfs->getRootDirectory(), fileName) {}
    dataFileReader(std::shared_ptr<SPFS::Directory> directory, std::string fileName) : dataFileReader(directory->openFile(fileName)) {}
    dataFileReader(std::shared_ptr<SPFS::ReadOnlyFile> file) : dataFileReader(file->getMemoryMappedAddress(), file->getSize(), file) {}
    dataFileReader(const uint8_t* data, size_t size) : dataFileReader(data, size, nullptr) {}

    dataFileReader(const dataFileReader& other) = delete; // prevent copying
    dataFileReader& operator=(const dataFileReader& other) = delete; // prevent assignment


    bool isExpectedFile(const std::string& expected_magic_number);
    bool isExpectedFile(uint32_t expected_magic_number);
    bool isFileHeaderValid();

    const void* getFieldData(dataFileFieldSignature_t signature, size_t* out_size = nullptr) const;

    // Calling this method will fill an internal map for fast data access.
    // This is not required to access the data but will make it faster.
    std::vector<dataFileFieldSignature_t> createFieldIndex();

    static dataFileFieldSignature_t makeSignature(const char* signature, size_t length = 3);
    static dataFileFieldSignature_t makeSignature(const std::string& signature) { return makeSignature(signature.c_str(), signature.size()); }

    static const std::string signatureToString(dataFileFieldSignature_t signature);

    static bool checkSignature(dataFileFieldSignature_t signature);

protected:
    struct dataFileHeader {
        uint32_t magic;                            // Magic number to identify the dat file
        uint32_t size;                             // the size of the file excluding the header
        uint32_t version_properties_checksum;      // the version, properties and checksum
                                                   //     8bit version  (MSB)
                                                   //     16bit properties
                                                   //     8bit CRC checksum of the header  (LSB)
    };
    
    struct dataFileField {
        uint32_t signature_size;                   // signature to identify the field type
                                                   //     16bit signature (MSB) (must be an even number of 1 bits, so the MSB is a parity bit to make the field even)
                                                   //     16bit size of the field data (LSB)
        uint32_t flags_checksum;                   // flags and checksum of the field
                                                   //     24bit flags (MSB) (the structure is not yet specified, so it remains 0 for future use)
                                                   //     8bit CRC checksum of the field header (LSB)
    };

    bool isHeaderValid() const {
        return _is_valid;
    }

    size_t getDataSize() const {
        return _size;
    }

    void changeDataSize(size_t new_size) {
        _size = new_size;
    }

    const dataFileField* getFieldHeader(const void* pointer) const {
        if(pointer == nullptr || reinterpret_cast<intptr_t>(pointer) % sizeof(uint32_t) != 0) {
            return nullptr; // invalid data pointer
        }
        return reinterpret_cast<const dataFileField*>(pointer);
    }
    bool isValidFieldHeader(const dataFileField* field) const;

    dataFileFieldSignature_t getFieldSignature(const dataFileField* field) const {
        return static_cast<dataFileFieldSignature_t>(field->signature_size >> 16);
    }
    uint16_t getFieldSize(const dataFileField* field) const {
        return static_cast<uint16_t>(field->signature_size & 0xFFFF);
    }

private:
    std::shared_ptr<SPFS::ReadOnlyFile> _file;
    const uint8_t* _data;
    size_t _size;
    bool _is_valid = false;

    std::map<dataFileFieldSignature_t, const dataFileField*> _field_indizes;

    const void* findFieldDataInIndex(dataFileFieldSignature_t signature, size_t* out_size = nullptr) const;

    static constexpr uint8_t _signature_lookup[40] = {
        '.', 'e', '0', 'a', '1', 's', 'i', 'u', 
        '2', 'd', 'r', 'j', 'n', 'f', 'c', 'k', 
        't', 'z', 'l', 'w', 'h', 'y', 'p', 'q',
        'o', 'b', 'g', '3', 'm', 'x', 'v', '4',

        ' ', '?', '!', '5', '6', '7', '8', '9' };

    static constexpr uint8_t _bit_count_lookup[32] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5 };
};



class dataFileMemoryWriter : public dataFileReader {
public:
    dataFileMemoryWriter(uint8_t* buffer, size_t capacity);

    bool setHeader(const std::string& magic_number, uint32_t fileSize = -1);
    bool setHeader(uint32_t magic_number, uint32_t fileSize = -1);

    bool setAppendMode();

    bool addField(dataFileFieldSignature_t signature, const std::string& data) {
        return addField(signature, data.c_str(), static_cast<uint16_t>(data.size()));
    }
    bool addField(dataFileFieldSignature_t signature, const std::vector<uint8_t>& data) {
        return addField(signature, data.data(), static_cast<uint16_t>(data.size()));
    }
    bool addField(dataFileFieldSignature_t signature, const void* data, uint16_t size);

private:
    uint8_t* _buffer;
    size_t _capacity;

    void recalculateHeaderChecksum(uint32_t fileSize = -1);
};