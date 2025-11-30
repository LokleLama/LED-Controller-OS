
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "flash.h"
#include <memory>
#include <string>
#include <vector>

//! \brief Simple Pico File System (SPFS) class
/*!
 * This class provides a simple interface for managing a flash file system.
 * It allows reading, writing, and erasing data in flash memory.
 * The implementation is based on the FlashHAL class for low-level flash
 * operations. It is designed to be used with the Raspberry Pi Pico SDK.
 * \ingroup hardware_flash
 */
class SPFS {
private:
  struct FileSystemHeader{
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

  struct FileSystemMetadata{
    uint16_t magic;                        //!< Magic number to identify the file system metadata
    uint16_t root_directory_block;         //!< Block number of the root directory
    uint16_t name_size;                    //!< Size of the file system name
                                           //!< the configuration contains the following fields:
                                           //!< - uint8_t size;        //!< Size of the file system name (mask: 0x00FF) (max: 180 bytes)
                                           //!< - uint8_t reserved;    //!< Reserved for future use (mask: 0xFF00) (must be 0xFF)
    uint16_t checksum;                     //!< Checksum of the file system metadata
                                           //! the name of the file system will follow after this structure
  };

  struct DirectoryHeader{
    uint16_t magic;                        //!< Magic number to identify the directory
    uint16_t name_size_content_offset;     //!< the configuration contains the following fields:
                                           //!< - uint8_t size;        //!< Size of the directory name (mask: 0x00FF) (max: 200 bytes)
                                           //!< - uint8_t offset;      //!< Offset within the content block of the directory (mask: 0xFF00) (max: 200 bytes)
    uint16_t checksum;                     //!< Checksum of the directory header
    uint16_t next;                         //!< offset of the extension block (in blocks)
                                           //!< The directory content will follow after the name. any file or directory in the current directory will be listed here as uint16_t block offsets.
  };
  struct DirectoryExtensionHeader{
    uint16_t magic;                        //!< Magic number to identify the directory extension
    int16_t previous;                      //!< offset of the previous extension block (in blocks)
    uint16_t checksum;                     //!< Checksum of the directory extension header
    int16_t next;                          //!< offset of the next extension block (in blocks)
  };

  struct DirectoryContentHeader{
    uint16_t type;
    int16_t block_offset;
  };

  struct FileHeader{
    uint16_t magic;                        //!< Magic number to identify the file
    uint16_t content_offset;               //!< offset of the file content (in blocks)
    uint16_t name_offset_size;             //!< the configuration contains the following fields:
                                           //!< - uint8_t offset;      //!< Offset within the current block of the file name (mask: 0x00FF) (max: 200 bytes)
                                           //!< - uint8_t size;        //!< Size of the file name (mask: 0xFF00) (max: 200 bytes)
    uint16_t file_type_flags;              //!< the configuration contains the following fields:
                                           //!< - uint8_t file_type;   //!< Type of the file (mask: 0x00FF) (e.g., 0 = binary, 1 = text, etc.)
                                           //!< - uint8_t flags;       //!< Flags for the file (mask: 0xFF00) (e.g., read-only, hidden, executable, etc.)
    uint16_t checksum;                     //!< Checksum of the file header
  };
  struct FileContentHeader{
    uint16_t magic;                        //!< Magic number to identify the file content
    uint16_t size;                         //!< Size of the file data (in bytes)
    uint16_t checksum;                     //!< Checksum of the file data
    uint16_t next;                         //!< offset of the next file version content block (in blocks)
  };

public:
  class File {

  };
  class Directory : public std::enable_shared_from_this<Directory> {
    public:
      Directory(std::shared_ptr<Directory> parent) : Directory(nullptr, parent, nullptr) { };

    protected:
      Directory(SPFS* fs, std::shared_ptr<Directory> parent, const DirectoryHeader* header)
          : _fs(fs), _parent(parent), _header(header) { }

      const DirectoryHeader* getHeader() const { return _header; }

    public:
      std::shared_ptr<Directory> getParent() const { return _parent; }
      const std::string getName() const;

      std::vector<std::shared_ptr<Directory>> getSubdirectories();
      std::vector<std::shared_ptr<File>> getFiles();

      std::shared_ptr<Directory> createDirectory(const std::string& name);
      std::shared_ptr<File> createFile(const std::string& name);

    protected:
      SPFS* _fs;                          //!< Reference to the SPFS instance
      std::shared_ptr<Directory> _parent; //!< Reference to the parent directory
      const DirectoryHeader* _header;           //!< Header information for the directory
  };

private:
  class DirectoryInternal : public Directory {
  public:
    DirectoryInternal(SPFS* fs, std::shared_ptr<Directory> parent, const DirectoryHeader* header)
        : Directory(fs, parent, header) { }

    // Additional methods specific to internal directory management can be added here
  };

public:
  //! \brief Initialize the Flash File System
  /*!
   * This method initializes the Flash File System by checking the flash memory
   * and preparing it for use.
   * \return 0 on success, negative error code on failure.
   */
  std::shared_ptr<Directory> getRootDirectory(int start_offset = 0, int end_offset = -1);

  int getFileSystemSize() const{
    if(_fs_header == nullptr){
      return -1;
    }
    return _fs_header->size;
  }

private:
  const SPFS::FileSystemHeader *_fs_header = nullptr; //!< Start address of the flash memory for the file system
  const uint8_t* _start_search_address = nullptr;

  static constexpr uint32_t MAGIC_NUMBER = 0xA36CA3FA;              //!< Magic number for SPFS (SPFSv1)
  static constexpr uint32_t SPFS_VERSION = 0x01000000;              //!< Version number for SPFS (SPFSv1)
  static constexpr uint32_t VERSION_MAJOR_MASK = 0xFF000000;        //!< Major version mask
  static constexpr uint32_t VERSION_MINOR_MASK = 0x00FF0000;        //!< Minor version mask
  static constexpr uint32_t VERSION_PATCH_MASK = 0x0000FF00;        //!< Patch version mask
  static constexpr uint32_t VERSION_BUILD_MASK = 0x000000FF;        //!< Build version mask

  static constexpr uint16_t MAGIC_FS_METADATA_NUMBER = 0xB50E;      //!< Magic number for SPFS File System Metadata (fsm)

  static constexpr uint16_t MAGIC_DIR_NUMBER = 0x9314;              //!< Magic number for SPFS Directory (dir)
  static constexpr uint16_t MAGIC_DIR_EXTENSION_NUMBER = 0x85E5;    //!< Magic number for SPFS Directory Extension (exd)
  static constexpr uint16_t MAGIC_SUBDIRMARKER = 0xD1FF;            //!< Magic number for Directory entries (sdi)
  static constexpr uint16_t MAGIC_FILEMARKER = 0xB313;              //!< Magic number for File entries (fil)
  static constexpr uint16_t MAGIC_ENDMARKER = 0xFFFF;                //!< Magic number for End entries

  static constexpr uint16_t MAGIC_FILE_NUMBER = 0xB313;             //!< Magic number for SPFS File (fil)
  static constexpr uint16_t MAGIC_FILE_EXTENSION_NUMBER = 0x70CD;   //!< Magic number for SPFS File Extension (con)

  static constexpr int FS_ALIGNMENT = 4096;                         //!< Alignment for SPFS operations
  static constexpr int FS_BLOCK_SIZE = 256;                         //!< Block size for SPFS operations

  std::shared_ptr<Directory> findFileSystemStart(int start_offset, int end_offset);
  std::shared_ptr<Directory> initializeFileSystem(void *address);
  bool formatDisk(const void *address, size_t size);

  std::shared_ptr<Directory> createNewFileSystem(const void *address, size_t size, const std::string& fs_name, const std::string& root_dir_name);
  std::shared_ptr<DirectoryInternal> createDirectory(std::shared_ptr<SPFS::Directory> parent, const std::string& dir_name);
  std::shared_ptr<DirectoryInternal> createDirectory(const void* address, std::shared_ptr<SPFS::Directory> parent, const std::string& dir_name);

  std::shared_ptr<DirectoryInternal> openDirectory(const void* address, std::shared_ptr<SPFS::Directory> parent) ;

  const DirectoryHeader* findFreeSpaceForDirectory();
  const FileHeader* findFreeSpaceForFile(size_t name_size);
  const FileContentHeader* findFreeSpaceForFileContent(size_t content_size);
  const void* findFreeSpace(const uint8_t* start_search, size_t size);
  const void* findFreeSpace(size_t size);

  uint32_t calculateCRC32(const void *address, size_t size);
  uint16_t calculateCRC16(const void *address, size_t size);
};
