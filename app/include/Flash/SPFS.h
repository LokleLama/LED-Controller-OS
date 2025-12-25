
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
class SPFS : public std::enable_shared_from_this<SPFS> {
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

  struct SPFSBlockHeader{
    uint16_t magic;                        //!< Magic number to identify the file system
    uint16_t size;                         //!< Size of the following (in blocks)
  };

  struct DirectoryHeader{
    SPFSBlockHeader block;                 //!< Block description
    uint16_t name_size_meta_offset;        //!< the configuration contains the following fields:
                                           //!< - uint8_t size;        //!< Size of the directory name (mask: 0x00FF) (max: 200 bytes)
                                           //!< - uint8_t offset;      //!< Offset within the content block of the directory (mask: 0xFF00) (max: 200 bytes)
  };

  struct DirectoryContentHeader{
    uint16_t type;
    int16_t block_offset;
  };

  struct DirectoryMetadataHeader{
    uint16_t checksum;                     //!< Checksum of the directory header
    uint16_t next;                         //!< offset of the extension block (in blocks)
                                           //!< The directory content will follow after the name. any file or directory in the current directory will be listed here as uint16_t block offsets.
    DirectoryContentHeader content[1];     //!< Content entries in the directory block
  };

  struct DirectoryExtensionHeader{
    SPFSBlockHeader block;                 //!< Block description
    int16_t previous;                      //!< offset of the previous extension block (in blocks)
    uint16_t checksum;                     //!< Checksum of the directory extension header
    int16_t next;                          //!< offset of the next extension block (in blocks)
    DirectoryContentHeader content[1];     //!< Content entries in the extension block
  };

  struct FileHeader{
    SPFSBlockHeader block;                 //!< Block description
    uint16_t name_size_meta_offset;        //!< the configuration contains the following fields:
                                           //!< - uint8_t size;        //!< Size of the file name (mask: 0x00FF) (max: 200 bytes)
                                           //!< - uint8_t offset;      //!< Offset within the current block of the file metadata block (mask: 0xFF00) (max: 200 bytes)
  };
  struct FileMetadataHeader{
    uint16_t checksum;                     //!< Checksum of the file header
    uint16_t file_type_flags;              //!< the configuration contains the following fields:
                                           //!< - uint8_t file_type;   //!< Type of the file (mask: 0x00FF) (e.g., 0 = binary, 1 = text, etc.)
                                           //!< - uint8_t flags;       //!< Flags for the file (mask: 0xFF00) (e.g., read-only, hidden, executable, etc.)
                                           //!< - 0x01: Read-only
                                           //!< - 0x02: Hidden
                                           //!< - 0x04: Executable
    uint16_t content_block;                //!< offset of the file content (in blocks)
  };
  struct FileContentHeader{
    SPFSBlockHeader block;                 //!< Block description
    uint16_t size;                         //!< Size of the file data (in bytes)
    uint16_t data_offset;                  //!< Offset within the current block of the file data (in bytes)
                                           //!< - uint8_t offset;      //!< Offset within the current block of the file data (mask: 0x00FF) (in bytes)
                                           //!< - uint8_t reserved;    //!< Reserved for future use (mask: 0xFF00) (must be 0xFF)
    uint16_t checksum;                     //!< Checksum of the file data
    uint16_t next;                         //!< offset of the next file version content block (in blocks)
  };

public:
  class Directory;
  class ReadOnlyFile{
    friend class Directory;

    protected:
      ReadOnlyFile(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const FileHeader* header, const FileContentHeader* content_header, size_t content_version)
          : _fs(fs), _parent(parent), _header(header), _content_header(content_header), _content_version(content_version) {}

      const FileHeader* getHeader() const { return _header; }
      const FileMetadataHeader* getMetadataHeader() const {
        return reinterpret_cast<const FileMetadataHeader *>(reinterpret_cast<const uint8_t*>(_header) + (_header->name_size_meta_offset >> 8));
      }
      const FileContentHeader* getContentHeader() const {
        return _content_header;
      }
    public:
      size_t getSize() const;
      size_t getSizeOnDisk() const;

      std::shared_ptr<Directory> getParent() const { return _parent; }
      const std::string getName() const;
      
      size_t getVersion() const { return _content_version; }

      std::shared_ptr<ReadOnlyFile> openVersion(size_t version) const;

      std::string readAsString() const;
      std::vector<uint8_t> readAsVector() const;
      std::vector<uint8_t> readBytes(size_t offset, size_t size) const;

      const uint8_t* getMemoryMappedAddress() const;

    protected:
      std::shared_ptr<SPFS> _fs;                //!< Reference to the SPFS instance
      std::shared_ptr<Directory> _parent;       //!< Reference to the parent directory
      const FileHeader* _header;                //!< Header information for the file
      const FileContentHeader* _content_header; //!< Header information for the file content
      size_t _content_version;                  //!< Version of the file content
  };
  class File : public ReadOnlyFile{
    public:
      File(std::shared_ptr<Directory> parent, const std::string& name);

    protected:
      File(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const FileHeader* header)
          : ReadOnlyFile(fs, parent, header, nullptr, 0) {
            FindCurrentContentHeader();
      }
      void FindCurrentContentHeader();

    public:
      bool write(const std::string& data);
      bool write(const std::vector<uint8_t>& data);
      bool write(const uint8_t* data, size_t size);
  };
  class Directory : public std::enable_shared_from_this<Directory> {
    public:
      Directory(std::shared_ptr<Directory> parent, const std::string& name);

    protected:
      Directory(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const DirectoryHeader* header)
          : _fs(fs), _parent(parent), _header(header) { }

      const DirectoryHeader* getHeader() const { return _header; }
      const DirectoryMetadataHeader* getMetadataHeader() const {
        return reinterpret_cast<const DirectoryMetadataHeader *>(reinterpret_cast<const uint8_t*>(_header) + (_header->name_size_meta_offset >> 8));
      }
      const DirectoryContentHeader* getContentHeaders() const {
        return getMetadataHeader()->content;
      }
      int getMaxContentCount() const {
        return (int)((FS_BLOCK_SIZE - (_header->name_size_meta_offset >> 8)) / sizeof(DirectoryContentHeader)) + 1;
      }

      bool addContent(uint16_t type, uintptr_t content_address);
      bool removeContent(uintptr_t content_address);
      bool addContent(std::shared_ptr<Directory> dir);
      bool addContent(std::shared_ptr<File> file);

    public:
      std::shared_ptr<Directory> getParent() const { return _parent; }
      const std::string getName() const;
      const std::string getFullPath() const;

      size_t getSizeOnDisk() const;

      size_t getFileCount() const;
      size_t getDirectoryCount() const;

      std::vector<std::shared_ptr<Directory>> getSubdirectories();
      std::vector<std::shared_ptr<File>> getFiles();

      std::shared_ptr<Directory> createDirectory(const std::string& name);
      std::shared_ptr<File> createFile(const std::string& name);
      std::shared_ptr<Directory> openSubdirectory(const std::string& name);
      std::shared_ptr<File> openFile(const std::string& name);

      bool remove(std::shared_ptr<File> file);
      bool remove(std::shared_ptr<Directory> dir);

      std::shared_ptr<Directory> createHardlink(std::shared_ptr<Directory> subdir);
      std::shared_ptr<File> createHardlink(std::shared_ptr<File> file, const std::string& new_name = "");

    protected:
      std::shared_ptr<SPFS> _fs;          //!< Reference to the SPFS instance
      std::shared_ptr<Directory> _parent; //!< Reference to the parent directory
      const DirectoryHeader* _header;     //!< Header information for the directory
  };

private:
  class DirectoryInternal : public Directory {
  public:
    DirectoryInternal(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const DirectoryHeader* header)
        : Directory(fs, parent, header) { }

    // Additional methods specific to internal directory management can be added here
  };

  class FileInternal : public File {
  public:
    FileInternal(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const FileHeader* header)
        : File(fs, parent, header) { }

    // Additional methods specific to internal directory management can be added here
  };
  class ReadOnlyFileInternal : public ReadOnlyFile {
    public:
      ReadOnlyFileInternal(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const FileHeader* header, const FileContentHeader* content_header, size_t content_version)
          : ReadOnlyFile(fs, parent, header, content_header, content_version) { }
  };

public:
  //! \brief Initialize the Flash File System
  /*!
   * This method initializes the Flash File System by checking the flash memory
   * and preparing it for use.
   * \return 0 on success, negative error code on failure.
   */
  std::shared_ptr<Directory> searchFileSystem(int start_offset, int end_offset = -1);
  std::shared_ptr<Directory> createNewFileSystem(int offset, size_t size, const std::string& fs_name = "SPFS", const std::string& root_dir_name = "root");
  
  std::shared_ptr<Directory> getRootDirectory();
  
  std::string getFileSystemName() const{
    if(_fs_header == nullptr){
      return "";
    }
    const FileSystemMetadata* fsm = reinterpret_cast<const FileSystemMetadata *>(reinterpret_cast<const uint8_t*>(_fs_header) + _fs_header->meta_offset);
    const char* name_ptr = reinterpret_cast<const char*>(fsm) + sizeof(FileSystemMetadata);
    return std::string(name_ptr, fsm->name_size);
  }

  int getFileSystemSize() const{
    if(_fs_header == nullptr){
      return -1;
    }
    return _fs_header->size;
  }

  enum class BlockState {
    FREE,
    USED,
    USED_FILE,
    USED_DIR,
    BAD
  };

  std::vector<BlockState> getBlockUsageMap() const;

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
  static constexpr uint16_t MAGIC_SUBDIRMARKER = 0xA498;            //!< Magic number for Directory entries (sdi)
  static constexpr uint16_t MAGIC_FILEMARKER = 0xB313;              //!< Magic number for File entries (fil)
  static constexpr uint16_t MAGIC_ENDMARKER = 0xFFFF;               //!< Magic number for End entries

  static constexpr uint16_t MAGIC_FILE_NUMBER = 0xB313;             //!< Magic number for SPFS File (fil)
  static constexpr uint16_t MAGIC_FILE_CONTENT_NUMBER = 0x70CD;     //!< Magic number for SPFS File Extension (con)

  static constexpr int FS_ALIGNMENT = 4096;                         //!< Alignment for SPFS operations
  static constexpr int FS_BLOCK_SIZE = 256;                         //!< Block size for SPFS operations

  std::shared_ptr<Directory> createNewFileSystem(const void *address, size_t size, const std::string& fs_name, const std::string& root_dir_name);
  std::shared_ptr<Directory> findFileSystemStart(int start_offset, int end_offset);
  std::shared_ptr<Directory> initializeFileSystem(const void *address);
  bool formatDisk(const void *address, size_t size);

  std::shared_ptr<DirectoryInternal> createDirectory(std::shared_ptr<SPFS::Directory> parent, const std::string& dir_name);
  std::shared_ptr<DirectoryInternal> createDirectory(const void* address, std::shared_ptr<SPFS::Directory> parent, const std::string& dir_name);

  std::shared_ptr<FileInternal> createFile(const std::shared_ptr<SPFS::Directory> parent, const std::string& file_name, const FileContentHeader* initial_content = nullptr);
  std::shared_ptr<FileInternal> createFile(const void* address, const std::shared_ptr<SPFS::Directory> parent, const std::string& file_name, const FileContentHeader* initial_content = nullptr);

  std::shared_ptr<DirectoryInternal> openDirectory(const void* address, std::shared_ptr<SPFS::Directory> parent);
  std::shared_ptr<FileInternal> openFile(const void* address, std::shared_ptr<SPFS::Directory> parent);

  const DirectoryHeader* findFreeSpaceForDirectory(size_t name_size);
  const FileHeader* findFreeSpaceForFile(size_t name_size);
  const FileContentHeader* findFreeSpaceForFileContent(size_t content_size);
  const void* findFreeSpace(const uint8_t* start_search, size_t size);
  const void* findFreeSpace(size_t size);

  uint16_t calculateContentBlockOffset(const void* reference_address, const FileContentHeader* content_header) const;
  const FileContentHeader* calculateContentHeaderAddress(const void* reference_address, uint16_t content_block_offset) const;

  uint32_t calculateCRC32(const void *address, size_t size);
  uint16_t calculateCRC16(const void *address, size_t size);
};
