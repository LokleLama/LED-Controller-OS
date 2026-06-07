
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "flash.h"
#include <cstdint>
#include <ios>
#include <istream>
#include <memory>
#include <string>
#include <streambuf>
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
  struct FileSystemHeader;
  struct FileSystemMetadata;
  struct SPFSBlockHeader;
  struct DirectoryHeader;
  struct DirectoryContentHeader;
  struct DirectoryMetadataHeader;
  struct DirectoryExtensionHeader;
  struct FileHeader;
  struct FileMetadataHeader;
  struct FileContentHeader;

public:
  static constexpr size_t MAX_ENTRY_NAME_LENGTH = 200;

  class Directory;
  
  //! \brief Custom stream buffer for reading from SPFS files
  /*!
   * This class provides a stream buffer interface for reading from SPFS files.
   * It allows the ReadOnlyFile class to be used with standard C++ stream operators.
   */
  class ReadOnlyFileStreamBuf : public std::streambuf {
  public:
    ReadOnlyFileStreamBuf(const uint8_t* data, size_t size);

  protected:
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir dir,
                           std::ios_base::openmode which = std::ios_base::in) override;

    std::streampos seekpos(std::streampos pos,
                           std::ios_base::openmode which = std::ios_base::in) override;

  private:
    const uint8_t* _data;
    size_t _size;
    size_t _pos;
  };

  class ReadOnlyFile : public std::enable_shared_from_this<ReadOnlyFile>{
    friend class Directory;

    protected:
      ReadOnlyFile(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const FileHeader* header,
                   const FileContentHeader* content_header, size_t content_version);

      const FileHeader* getHeader() const;
      const FileMetadataHeader* getMetadataHeader() const;
      const FileContentHeader* getContentHeader() const;
    public:
      size_t getSize() const;
      size_t getSizeOnDisk() const;

      std::shared_ptr<Directory> getParent() const { return _parent; }
      const std::string getName() const;
      
      size_t getVersion() const { return _content_version; }

      std::shared_ptr<const ReadOnlyFile> openVersion(size_t version) const;

      std::string readAsString() const;
      std::vector<uint8_t> readAsVector() const;
      std::vector<uint8_t> readBytes(size_t offset = 0, size_t size = -1) const;

      const uint8_t* getMemoryMappedAddress() const;

      //! \brief Get an input stream for reading from the file
      /*!
       * This method returns a unique_ptr to an std::istream that can be used
       * to read from the file using standard C++ stream operators.
       * \return A unique_ptr to an input stream for the file
       */
      std::unique_ptr<std::istream> getInputStream() const;

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
      File(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const FileHeader* header);
      void FindCurrentContentHeader();

    public:
      bool write(const std::string& data);
      bool write(const std::vector<uint8_t>& data);
      bool write(const uint8_t* data, size_t size);

      //! \brief Write Content to the file in chunks
      /*!
       * This method allows writing data to the file in multiple chunks.
       * It is useful for writing large files that may not fit into memory all at once.
       * \param data Pointer to the data to write
       * \param size Size of the data to write (in bytes)
       * \return true on success, false on failure
       */
      bool allocateContentSize(size_t size);
      bool allocateContenSize(size_t size) { return allocateContentSize(size); }
       bool append(const std::string& data);
       bool append(const std::vector<uint8_t>& data);
       bool append(const uint8_t* data, size_t size);
       bool finishContent();

    private:
      size_t _allocated_content_size = 0; //!< Allocated size for content
      size_t _append_position = 0; //!< Current position for appending data
      const FileContentHeader* _current_content_header = nullptr; //!< Current content header for appending data
  };
  class Directory : public std::enable_shared_from_this<Directory> {
    public:
      Directory(std::shared_ptr<Directory> parent, const std::string& name);

    protected:
      Directory(std::shared_ptr<SPFS> fs, std::shared_ptr<Directory> parent, const DirectoryHeader* header);

      const DirectoryHeader* getHeader() const;
      const DirectoryMetadataHeader* getMetadataHeader() const;
      const DirectoryContentHeader* getContentHeaders() const;
      int getMaxContentCount() const;

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
  class DirectoryInternal;
  class FileInternal;
  class ReadOnlyFileInternal;

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

  std::string getFileSystemName() const;
  std::string getFileSystemVersion() const;
  int getFileSystemSize() const;
  int getBlockSize() const;

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

  static constexpr uint32_t MAGIC_NUMBER = 0xA36CA3FA;              //!< Magic number for SPFS (SPFSv1.1)
  static constexpr uint32_t SPFS_VERSION = 0x01010000;              //!< Version number for SPFS (SPFSv1.1)
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
