#include "Utils/dataFile.h"
#include "Utils/crc.h"

#include <cstring>

dataFileMemoryWriter::dataFileMemoryWriter(uint8_t* buffer, size_t capacity) : dataFileReader(buffer, 0), _buffer(buffer), _capacity(capacity) { 
    if(_buffer == nullptr || _capacity < sizeof(dataFileHeader) || reinterpret_cast<intptr_t>(_buffer) % sizeof(uint32_t) != 0) { 
        _buffer = nullptr; 
        _capacity = 0; 
    }
    changeDataSize(sizeof(dataFileHeader));
}

bool dataFileMemoryWriter::setHeader(const std::string& magic_number, uint32_t fileSize){
    size_t length = magic_number.size();
    if(length > sizeof(uint32_t)) {
        length = sizeof(uint32_t);
    }
    uint32_t magic = 0;
    for(size_t i = 0; i < length; i++) {
        magic <<= 8;
        magic |= static_cast<uint8_t>(magic_number[i]);
    }
    return setHeader(magic, fileSize);
}
bool dataFileMemoryWriter::setHeader(uint32_t magic_number, uint32_t fileSize){
    if(_buffer == nullptr || _capacity < sizeof(dataFileHeader)) {
        return false; // not enough capacity to write the header
    }
    dataFileHeader* header = reinterpret_cast<dataFileHeader*>(_buffer);
    header->magic = magic_number;
    recalculateHeaderChecksum(fileSize);
    return isFileHeaderValid();
}

void dataFileMemoryWriter::recalculateHeaderChecksum(uint32_t fileSize) {
    if(_buffer != nullptr) {
        dataFileHeader* header = reinterpret_cast<dataFileHeader*>(_buffer);
        if(fileSize == static_cast<uint32_t>(-1)) {
            if (!isHeaderValid()){
                header->size = sizeof(dataFileHeader);
            }else{
                header->size = getDataSize();
            }
        }else{
            header->size = fileSize;
        }
        header->version_properties_checksum = (header->version_properties_checksum & 0x00FFFFFF) | (CRC8::calculate(_buffer, sizeof(dataFileHeader) - 1) << 24);
    }
}

bool dataFileMemoryWriter::setAppendMode() {
    if(!isFileHeaderValid()) {
        return false; // header is not valid, cannot switch to append mode
    }

    const uint8_t* ptr = _buffer + sizeof(dataFileHeader);
    const uint8_t* end = _buffer + _capacity;

    while(ptr + sizeof(dataFileField) <= end) {
        const dataFileField* field = getFieldHeader(ptr);
        if(field == nullptr) {
            changeDataSize(sizeof(dataFileHeader)); // skip the invalid byte and try to find the next field header
            return false; // alignment issue, cannot switch to append mode
        }
        if(!isValidFieldHeader(field)) {
            return true; // reached the end of the valid fields, can switch to append mode
        }
        changeDataSize(getDataSize() + sizeof(dataFileField) + getFieldSize(field));
        ptr += sizeof(dataFileField) + getFieldSize(field);
    }

    return false; // no more space to append new fields, cannot switch to append mode
}

bool dataFileMemoryWriter::addField(dataFileFieldSignature_t signature, const void* data, uint16_t size) {
    if(_buffer == nullptr || data == nullptr || size == 0 || 
       _capacity < getDataSize() + sizeof(dataFileField) + size) {
        return false; // not enough capacity to add the field
    }
    dataFileField* field = reinterpret_cast<dataFileField*>(_buffer + getDataSize());
    field->signature_size = (static_cast<uint32_t>(signature) << 16) | size;
    field->flags_checksum = 0;
    field->flags_checksum = (field->flags_checksum & 0x00FFFFFF) | (CRC8::calculate(reinterpret_cast<uint8_t*>(field), sizeof(dataFileField) - 1) << 24);
    if(data != nullptr && size > 0) {
        memcpy(field + 1, data, size);
    }
    changeDataSize(getDataSize() + sizeof(dataFileField) + size);
    recalculateHeaderChecksum();
    return true;
}

dataFileReader::dataFileReader(const uint8_t* data, size_t size, std::shared_ptr<SPFS::ReadOnlyFile> file) : 
                               _file(file), _data(data), _size(size) { 
    if(_data == nullptr) { _file = nullptr; _size = 0; } 
}

bool dataFileReader::isExpectedFile(const std::string& expected_magic_number) {
    size_t length = expected_magic_number.size();
    if(length > sizeof(uint32_t)) {
        length = sizeof(uint32_t);
    }
    uint32_t expected_magic = 0;
    for(size_t i = 0; i < length; i++) {
        expected_magic <<= 8;
        expected_magic |= static_cast<uint8_t>(expected_magic_number[i]);
    }
    return isExpectedFile(expected_magic);
}
bool dataFileReader::isExpectedFile(uint32_t expected_magic_number) {
    if (_is_valid || isFileHeaderValid()) {
        const dataFileHeader* header = reinterpret_cast<const dataFileHeader*>(_data);
        if(header->magic != expected_magic_number) {
            return false;
        }
        if(header->size < _size) {
            _size = header->size;
        }
        return true;
    }
    return false;
}

bool dataFileReader::isFileHeaderValid() {
    if(_size < sizeof(dataFileHeader)) {
        return false; // file is too small to contain a valid header
    }
    if(_data == nullptr || reinterpret_cast<intptr_t>(_data) % sizeof(uint32_t) != 0) {
        return false; // invalid data pointer
    }
    if(CRC8::calculate(_data, sizeof(dataFileHeader)) != 0x00) {
        return false; // invalid header checksum
    }
    _is_valid = true;
    return true;
}

const void* dataFileReader::getFieldData(dataFileFieldSignature_t signature, size_t* out_size) const{
    if(!_is_valid) {
        return nullptr;
    }
    const void* result = findFieldDataInIndex(signature, out_size);
    if(result != nullptr) {
        return result;
    }
    
    const uint8_t* ptr = _data + sizeof(dataFileHeader);
    const uint8_t* end = _data + _size;

    while(ptr + sizeof(dataFileField) <= end) {
        const dataFileField* field = getFieldHeader(ptr);
        if(field == nullptr) {
            ptr += 1;
            continue;
        }
        if(!isValidFieldHeader(field)) {
            ptr += sizeof(uint32_t);
            continue;
        }
        if(getFieldSignature(field) == signature) {
            if(out_size) {
                *out_size = getFieldSize(field);
            }
            return ptr + sizeof(dataFileField); // the field data starts immediately after the field header
        }
        ptr += sizeof(dataFileField) + getFieldSize(field);
    }
    return nullptr; // field not found
}

const void* dataFileReader::findFieldDataInIndex(dataFileFieldSignature_t signature, size_t* out_size) const {
    if(_field_indizes.empty()) {
        return nullptr; // field index is not created yet
    }
    auto it = _field_indizes.find(signature);
    if(it != _field_indizes.end()) {
        const dataFileField* field_header = it->second;
        if(out_size) {
            *out_size = getFieldSize(field_header);
        }
        return reinterpret_cast<const uint8_t*>(field_header) + sizeof(dataFileField); // the field data starts immediately after the field header
    }
    return nullptr; // field not found
}

// Calling this method will fill an internal map for fast data access.
// This is not required to access the data but will make it faster.
std::vector<dataFileFieldSignature_t> dataFileReader::createFieldIndex(){
    if(!_is_valid) {
        return {};
    }

    std::vector<dataFileFieldSignature_t> signatures;

    const uint8_t* ptr = _data + sizeof(dataFileHeader);
    const uint8_t* end = _data + _size;

    while(ptr + sizeof(dataFileField) <= end) {
        const dataFileField* field = getFieldHeader(ptr);
        if(field == nullptr) {
            ptr += 1;
            continue;
        }
        if(!isValidFieldHeader(field)) {
            ptr += sizeof(uint32_t);
            continue;
        }
        dataFileFieldSignature_t signature = getFieldSignature(field);
        _field_indizes[signature] = field;
        signatures.push_back(signature);
        ptr += sizeof(dataFileField) + getFieldSize(field);
    }

    return signatures;
}

bool dataFileReader::isValidFieldHeader(const dataFileField* field) const {
    if(field == nullptr) {
        return false;
    }
    if(CRC8::calculate(reinterpret_cast<const uint8_t*>(field), sizeof(dataFileField)) != 0x00) {
        return false; // invalid field header checksum
    }
    return true;
}


/********************
 * Static Methods
 ********************/

dataFileFieldSignature_t dataFileReader::makeSignature(const char* signature, size_t length) {
    if(length > 3) length = 3; // limit to 3 characters

    dataFileFieldSignature_t result = 0;
    uint8_t bitcount = 0;
    for(size_t i = 0; i < length; i++) {
        for(size_t j = 0; j < sizeof(_signature_lookup); j++) {
            if(_signature_lookup[j] == signature[i]) {
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

const std::string dataFileReader::signatureToString(dataFileFieldSignature_t signature) {
    std::string result = "   ";
    for(int i = 0; i < 3; i++) {
        uint8_t value = signature & 0x1F;
        result[2 - i] = _signature_lookup[value];
        signature >>= 5;
    }
    return result;
}

bool dataFileReader::checkSignature(dataFileFieldSignature_t signature) {
    uint8_t bitcount = 0;
    for(size_t i = 0; i < sizeof(dataFileFieldSignature_t) * 2; i++) {
        bitcount += _bit_count_lookup[signature & 0x0F];
        signature >>= 4;
    }
    return (bitcount % 2) == 0; // check if the number of 1 bits is even
}