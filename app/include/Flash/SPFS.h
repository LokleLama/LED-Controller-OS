
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "flash.h"
#include <memory>
#include <string>
#include <vector>

//! \brief Simple Flash File System (SPFS) class
/*!
 * This class provides a simple interface for managing a flash file system.
 * It allows reading, writing, and erasing data in flash memory.
 * The implementation is based on the FlashHAL class for low-level flash
 * operations. It is designed to be used with the Raspberry Pi Pico SDK.
 * \ingroup hardware_flash
 */
class SPFS {
public:
  class Directory {
  protected:
    Directory(const SPFS &fs, const Directory *parent, const std::string &name)
        : _fs(fs), _parent(parent), _name(name) {}

  public:
    //! \brief Get the path of the directory
    /*!
     * \return The path of the directory.
     */
    virtual std::string getPath() const {
      return _parent ? _parent->getPath() + "/" + _name : _name;
    }
    std::string getName() const { return _name; }

    std::vector<std::string> listFiles();
    std::vector<std::string> listDirectories();

    std::shared_ptr<Directory> createDirectory(const std::string &name);
    std::shared_ptr<Directory> openDirectory(const std::string &name) const;
    int deleteDirectory(const std::string &name);
    int deleteFile(const std::string &name);
    int readFile(const std::string &name, std::vector<uint8_t> &buffer) const;
    int writeFile(const std::string &name,
                  const std::vector<uint8_t> &buffer) const;

  private:
    const SPFS &_fs;          //!< Reference to the Flash File System
    const Directory *_parent; //!< Reference to the parent directory
    const std::string _name;  //!< Name of the directory
  };

  SPFS() = default;  //!< Default constructor
  ~SPFS() = default; //!< Default destructor

  //! \brief Initialize the Flash File System
  /*!
   * This method initializes the Flash File System by checking the flash memory
   * and preparing it for use.
   * \return 0 on success, negative error code on failure.
   */
  std::shared_ptr<Directory>
  getRootDirectory(int start_address = 0, int end_address = -1, int size = -1);

private:
  void *_address = 0; //!< Start address of the flash memory for the file system
  int _size = -1; //!< Size of the flash memory for the file system, -1 means
                  //!< use full flash size

  static constexpr uint32_t MAGIC_NUMBER =
      0xA36CA3FA;                           //!< Magic number for SPFS
  static constexpr int FS_ALIGNMENT = 4096; //!< Alignment for SPFS operations

  struct FileSystemHeader {
    uint32_t magic; //!< Magic number to identify the file system
    uint32_t size;  //!< Size of the file system
  };

  bool findFileSystemStart(int start_address, int end_address);
  bool initializeFileSystem(int size);
};
