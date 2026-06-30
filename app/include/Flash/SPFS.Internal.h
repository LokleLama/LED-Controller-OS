// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SPFS.h"

struct SPFS::FileSystemHeader{
  uint32_t magic;                        //!< Magic number to identify the file system
  uint32_t version;                      //!< Version number of the file system
  uint32_t size;                         //!< Size of the file system
  uint32_t block_and_page_size;          //!< Block size and page size of the file system (upper 16 bits: block size, lower 16 bits: page size)
                                         //!< the configuration contains the following fields:
                                         //!< - uint16_t block_size;   //!< Block size of the file system (mask: 0x0000FFFF) (in bytes)
                                         //!< - uint16_t page_size;    //!< Page size of the file system (mask: 0xFFFF0000) (in bytes)
  uint32_t meta_offset;                  //!< Offset to the file system metadata (in bytes from start (must be a multiple of sizeof(uint32_t) = 4)))
  uint32_t checksum;                     //!< Checksum of the file system header
};

struct SPFS::FileSystemMetadata{
  uint16_t magic;                        //!< Magic number to identify the file system metadata
  uint16_t root_directory_block;         //!< Block number of the root directory
  uint16_t name_size;                    //!< Size of the file system name
                                         //!< the configuration contains the following fields:
                                         //!< - uint8_t size;        //!< Size of the file system name (mask: 0x00FF) (max: 180 bytes)
                                         //!< - uint8_t reserved;    //!< Reserved for future use (mask: 0xFF00) (must be 0xFF)
  uint16_t checksum;                     //!< Checksum of the file system metadata
                                         //! the name of the file system will follow after this structure
};

struct SPFS::SPFSBlockHeader{
  uint16_t magic;                        //!< Magic number to identify the file system
  uint16_t size;                         //!< Size of the following (in blocks)
};

struct SPFS::DirectoryHeader{
  SPFSBlockHeader block;                 //!< Block description
  uint16_t name_size_meta_offset;        //!< the configuration contains the following fields:
                                         //!< - uint8_t size;        //!< Size of the directory name (mask: 0x00FF) (max: 200 bytes)
                                         //!< - uint8_t offset;      //!< Offset within the content block of the directory (mask: 0xFF00) (max: 200 bytes)
};

struct SPFS::DirectoryContentHeader{
  uint16_t type;
  int16_t block_offset;
};

struct SPFS::DirectoryMetadataHeader{
  uint16_t checksum;                     //!< Checksum of the directory header
  uint16_t next;                         //!< offset of the extension block (in blocks)
                                         //!< The directory content will follow after the name. any file or directory in the current directory will be listed here as uint16_t block offsets.
  DirectoryContentHeader content[1];     //!< Content entries in the directory block
};

struct SPFS::DirectoryExtensionHeader{
  SPFSBlockHeader block;                 //!< Block description
  int16_t previous;                      //!< offset of the previous extension block (in blocks)
  uint16_t checksum;                     //!< Checksum of the directory extension header
  int16_t next;                          //!< offset of the next extension block (in blocks)
  DirectoryContentHeader content[1];     //!< Content entries in the extension block
};

struct SPFS::FileHeader{
  SPFSBlockHeader block;                 //!< Block description
  uint16_t name_size_meta_offset;        //!< the configuration contains the following fields:
                                         //!< - uint8_t size;        //!< Size of the file name (mask: 0x00FF) (max: 200 bytes)
                                         //!< - uint8_t offset;      //!< Offset within the current block of the file metadata block (mask: 0xFF00) (max: 200 bytes)
};

struct SPFS::FileMetadataHeader{
  uint16_t checksum;                     //!< Checksum of the file header
  uint16_t file_type_flags;              //!< the configuration contains the following fields:
                                         //!< - uint8_t file_type;   //!< Type of the file (mask: 0x00FF) (e.g., 0 = binary, 1 = text, etc.)
                                         //!< - uint8_t flags;       //!< Flags for the file (mask: 0xFF00) (e.g., read-only, hidden, executable, etc.)
                                         //!< - 0x01: Read-only
                                         //!< - 0x02: Hidden
                                         //!< - 0x04: Executable
  uint16_t content_block;                //!< offset of the file content (in blocks)
};

struct SPFS::FileContentLegacyHeader{
  SPFSBlockHeader block;                 //!< Block description
  uint16_t size;                         //!< Size of the file data (in bytes)
  uint16_t data_offset;                  //!< Offset within the current block of the file data (in bytes)
                                         //!< - uint8_t offset;      //!< Offset within the current block of the file data (mask: 0x00FF) (in bytes)
                                         //!< - uint8_t reserved;    //!< Reserved for future use (mask: 0xFF00) (must be 0xFF)
  uint16_t checksum;                     //!< Checksum of the file data
  uint16_t next_partition;               //!< offset of the next file content block (in blocks)
  uint16_t next_version;                 //!< offset of the next file version content block (in blocks)
};

struct SPFS::FileContentHeader{
  SPFSBlockHeader block;                 //!< Block description
  uint16_t size;                         //!< Size of the file data (in bytes)
  uint16_t data_offset;                  //!< Offset within the current block of the file data (in bytes)
                                         //!< - uint8_t offset;      //!< Offset within the current block of the file data (mask: 0x00FF) (in bytes)
                                         //!< - uint8_t reserved;    //!< Reserved for future use (mask: 0xFF00) (must be 0xFF)
  uint16_t checksum;                     //!< Checksum of the file data
  uint16_t next_partition;               //!< offset of the next file content block (in blocks)
  uint16_t next_version;                 //!< offset of the next file version content block (in blocks)
  uint16_t reserved_blocks;              //!< the size that has been reserved for the file content (in blocks)
                                         //!< if no blocks have been reserved yet, this field will be 0xFFFF
                                         //!< if the file content has been written to the reserved blocks, this field will be changed to 0
  uint16_t tag_metadata_block;           //!< the offset in blocks to the tag metadata when this content has been tagged (in blocks) (untagged versions have 0xFFFF in this field)
};

struct SPFS::FileContentTagHeader{
  SPFSBlockHeader block;                 //!< Block description
  uint16_t tag_size;                     //!< Size of the tag data (in bytes)
  uint16_t data_offset;                  //!< Offset within the current block of the tag data (in bytes)
                                         //!< - uint8_t offset;      //!< Offset within the current block of the tag data (mask: 0x00FF) (in bytes)
                                         //!< - uint8_t reserved;    //!< Reserved for future use (mask: 0xFF00) (must be 0xFF)
  uint16_t checksum;                     //!< Checksum of the tag data
};

inline SPFS::ReadOnlyFileStreamBuf::ReadOnlyFileStreamBuf(const uint8_t* data, size_t size)
    : _data(data), _size(size), _pos(0) {
  // Set up the get area to point to the file data
  char* base = const_cast<char*>(reinterpret_cast<const char*>(_data));
  setg(base, base, base + _size);
}

inline std::streampos SPFS::ReadOnlyFileStreamBuf::seekoff(std::streamoff off, std::ios_base::seekdir dir,
                                                           std::ios_base::openmode which) {
  if (which & std::ios_base::in) {
    std::streampos new_pos;

    if (dir == std::ios_base::beg) {
      new_pos = off;
    } else if (dir == std::ios_base::cur) {
      new_pos = (gptr() - eback()) + off;
    } else if (dir == std::ios_base::end) {
      new_pos = _size + off;
    } else {
      return -1;
    }

    if (new_pos < 0 || new_pos > static_cast<std::streampos>(_size)) {
      return -1;
    }

    char* base = const_cast<char*>(reinterpret_cast<const char*>(_data));
    setg(base, base + new_pos, base + _size);
    return new_pos;
  }
  return -1;
}

inline std::streampos SPFS::ReadOnlyFileStreamBuf::seekpos(std::streampos pos,
                                                           std::ios_base::openmode which) {
  return seekoff(pos, std::ios_base::beg, which);
}

inline SPFS::ReadOnlyFile::ReadOnlyFile(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent,
                                        const FileHeader* header, const FileContentHeader* content_header,
                                        size_t content_version)
    : _fs(fs), _parent(parent), _header(header), _content_header(content_header),
      _content_version(content_version) {}

inline const SPFS::FileHeader* SPFS::ReadOnlyFile::getHeader() const {
  return _header;
}

inline const SPFS::FileMetadataHeader* SPFS::ReadOnlyFile::getMetadataHeader(const SPFS::FileHeader* header) const {
  return reinterpret_cast<const FileMetadataHeader *>(
      reinterpret_cast<const uint8_t*>(header) + (header->name_size_meta_offset >> 8));
}

inline const SPFS::FileMetadataHeader* SPFS::ReadOnlyFile::getMetadataHeader() const {
  return getMetadataHeader(getHeader());
}

inline const SPFS::FileContentHeader* SPFS::ReadOnlyFile::getContentHeader() const {
  return _content_header;
}

inline SPFS::File::File(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const FileHeader* header)
    : ReadOnlyFile(fs, parent, header, nullptr, 0) {
  _content_header = FindNewestContentHeader(getHeader(), _content_version);
}

inline SPFS::Directory::Directory(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent,
                                  const DirectoryHeader* header)
    : _fs(fs), _parent(parent), _header(header) {}

inline const SPFS::DirectoryHeader* SPFS::Directory::getHeader() const {
  return _header;
}

inline const SPFS::DirectoryMetadataHeader* SPFS::Directory::getMetadataHeader(const SPFS::DirectoryHeader* header) const {
  return reinterpret_cast<const DirectoryMetadataHeader *>(
      reinterpret_cast<const uint8_t*>(header) + (header->name_size_meta_offset >> 8));
}

inline const SPFS::DirectoryMetadataHeader* SPFS::Directory::getMetadataHeader() const {
  return getMetadataHeader(getHeader());
}

inline const SPFS::DirectoryContentHeader* SPFS::Directory::getContentHeaders() const {
  return getMetadataHeader()->content;
}

inline int SPFS::Directory::getMaxContentCount() const {
  return static_cast<int>((FS_BLOCK_SIZE - (_header->name_size_meta_offset >> 8)) /
                          sizeof(DirectoryContentHeader)) + 1;
}

class SPFS::DirectoryInternal : public SPFS::Directory {
public:
  DirectoryInternal(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent,
                    const DirectoryHeader* header)
      : Directory(fs, parent, header) {}
};

class SPFS::FileInternal : public SPFS::File {
public:
  FileInternal(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const FileHeader* header)
      : File(fs, parent, header) {}
};

class SPFS::ReadOnlyFileInternal : public SPFS::ReadOnlyFile {
public:
  ReadOnlyFileInternal(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent,
                       const FileHeader* header, const FileContentHeader* content_header,
                       size_t content_version)
      : ReadOnlyFile(fs, parent, header, content_header, content_version) {}
};
