#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include "Flash/flashHAL.h"
#include "Flash/SPFS.h"

#include "Console.h"

struct config_t {
  size_t flash_size;
  size_t fs_offset;
  size_t fs_size;
  size_t sector_size;
  size_t page_size;
  char* flash_file;

  uint8_t* flash_data;
} config = {
    .flash_size = 2 * 1024 * 1024,
    .fs_offset = 4 * 4096,
    .fs_size = 8 * 1024,
    .sector_size = 4096,
    .page_size = 256,
    .flash_file = "flash",
    .flash_data = NULL
};

#define MAGIC_NUMBER  0xA36CA3FA
#define SPFS_VERSION  0x01000000

static void OpenOrCreateFlashFile() {
  char flash_file_name[256];
  snprintf(flash_file_name, sizeof(flash_file_name), "%s-in.bin", config.flash_file);
  FILE* f = fopen(flash_file_name, "r+b");
  if (f != NULL) {
    //find filesize
    printf("Opened file \"%s\"\n", flash_file_name);
    fseek(f, 0, SEEK_END);
    config.flash_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // read it
    config.flash_data = (uint8_t*)malloc(config.flash_size);
    if (config.flash_data == NULL) {
      perror("Failed to allocate flash data");
      fclose(f);
      return;
    }

    if (fread(config.flash_data, 1, config.flash_size, f) != config.flash_size) {
      perror("Failed to read flash data");
      free(config.flash_data);
      fclose(f);
      return;
    }

    fclose(f);
    return;
  }

  // allocate flash data
  config.flash_data = (uint8_t*)malloc(config.flash_size);
  if (config.flash_data == NULL) {
    perror("Failed to allocate flash data");
    return;
  }

  memset(config.flash_data, 0xFF, config.flash_size);

  // write magic number at fs_offset
  uint32_t* fs_header = (uint32_t*)(config.flash_data + config.fs_offset);
  fs_header[0] = MAGIC_NUMBER;
  fs_header[1] = SPFS_VERSION; // version 1.0
  fs_header[2] = config.fs_size; // size of the filesystem

  // file doesn't exist, create it
  f = fopen(flash_file_name, "w+b");
  if (f == NULL) {
    perror("Failed to create flash file");
    return;
  }

  if (fwrite(config.flash_data, 1, config.flash_size, f) != config.flash_size) {
    perror("Failed to write initial flash data");
    fclose(f);
    return;
  }

  fflush(f);
  fclose(f);
  printf("Created file \"%s\"\n", flash_file_name);
}

static void SaveFlashStateInFlashFile() {
  char flash_file_name[256];
  snprintf(flash_file_name, sizeof(flash_file_name), "%s-out.bin", config.flash_file);
  FILE* f = fopen(flash_file_name, "w+b");
  if (f == NULL) {
    perror("Failed to open flash file for writing");
    return;
  }

  if (fwrite(config.flash_data, 1, config.flash_size, f) != config.flash_size) {
    perror("Failed to write flash data");
    fclose(f);
    return;
  }

  fflush(f);
  fclose(f);
}

static size_t RoundSizeToSector(size_t size) {
  return (size + config.sector_size - 1) & ~(config.sector_size - 1);
}

static std::shared_ptr<SPFS::Directory> CreateSubDirectory(std::shared_ptr<SPFS::Directory> parent, std::string name) {
  auto subdir = parent->createDirectory(name);
  if (subdir == nullptr) {
    printf("Failed to create subdirectory (%s)\n", name.c_str());
  } else {
    printf("Created subdirectory  : %s\n", subdir->getName().c_str());
  }
  return subdir;
}

int main(int argc, char **argv) {
  printf("Usage: fs-test [filesystem-size]\n");
  if (argc > 1) {
    config.fs_size = strtoul(argv[1], NULL, 0);
    printf("round size argument %zu", config.fs_size);
    config.fs_size = RoundSizeToSector(config.fs_size);
    printf(" to %zu\n", config.fs_size);
  }

  OpenOrCreateFlashFile();
  if (config.flash_data == NULL) {
    return -1;
  }

  FlashHAL::setFlashMemoryOffset(config.flash_data);

  printf("******************************\n");
  printf("using flash file %s-in.bin\n", config.flash_file);
  printf("saving result in %s-out.bin\n", config.flash_file);
  printf("using flash offset of %zu\n", config.fs_offset);
  printf("using flash size of %zu\n", config.fs_size);
  printf("using flash sector size of %zu\n", config.sector_size);
  printf("using flash page size of %zu\n", config.page_size);
  printf("using flash Size of %zu\n", config.flash_size);

  std::shared_ptr<SPFS> spfs = std::make_shared<SPFS>();
  auto root = spfs->searchFileSystem(0);

  printf("******************************\n");
  printf("File System Size      : %i\n", spfs->getFileSystemSize());
  printf("Directory Name        : %s\n", root->getName().c_str());
  printf("Directory Disk Space  : %zu bytes\n", root->getSizeOnDisk());

  CreateSubDirectory(root, "config");
  auto data = CreateSubDirectory(root, "data");

  CreateSubDirectory(data, "files");

  printf("******************************\n");
  printf("reopening the same filesystem\n");
  std::shared_ptr<SPFS> reopened_spfs = std::make_shared<SPFS>();
  auto reopened_root = reopened_spfs->searchFileSystem(0);
  printf("File System Size      : %i\n", spfs->getFileSystemSize());
  printf("Root Directory Name   : %s\n", reopened_root->getName().c_str());
  printf("Directory Disk Space  : %zu bytes\n", reopened_root->getSizeOnDisk());

  auto subdirs = reopened_root->getSubdirectories();
  std::shared_ptr<SPFS::Directory> data_dir = nullptr;

  printf("Found Subdirectories  : ");
  for (const auto& dir : subdirs) {
    printf("%s   ", dir->getName().c_str());
    if(dir->getName() == "data") {
      data_dir = dir;
    }
  }
  printf("\n");

  printf("******************************\n");
  printf("creating new files in the \"%s\" directory\n", data_dir->getName().c_str());
  auto file1 = data_dir->createFile("testfile.txt");
  data_dir->createFile("testfile1.txt");

  auto files = data_dir->getFiles();
  printf("Found Files           : ");
  for (const auto& file : files) {
    printf("%s   ", file->getName().c_str());
  }
  printf("\n");

  printf("******************************\n");
  printf("getting stats of file \"%s\"\n", file1->getName().c_str());
  printf("File Version          : %zu\n", file1->getVersion());
  printf("File Size             : %zu bytes\n", file1->getSize());
  printf("File Size on Disk     : %zu bytes\n", file1->getSizeOnDisk());

  printf("******************************\n");
  printf("add content to file \"%s\"\n", file1->getName().c_str());

  std::string file_content = "This is a test content for the file.\nIt has multiple lines.\nThis is line 3.\nEnd of file.";
  if (file1->write(file_content)) {
    printf("Wrote %zu bytes\n", file_content.length());
  } else {
    printf("Failed to write to file \"%s\"\n", file1->getName().c_str());
  }

  printf("******************************\n");
  printf("getting stats of file \"%s\"\n", file1->getName().c_str());
  printf("File Version          : %zu\n", file1->getVersion());
  printf("File Size             : %zu bytes\n", file1->getSize());
  printf("File Size on Disk     : %zu bytes\n", file1->getSizeOnDisk());

  printf("******************************\n");
  printf("add content to file \"%s\"\n", file1->getName().c_str());

  std::string file_content_300 = "This is a test content for the file with at least 300 bytes of data.\n"
    "It has multiple lines to ensure the content is long enough.\n"
    "This is line 3 of the test content.\n"
    "Line 4: The file system needs to handle larger file contents correctly.\n"
    "Line 5: Additional padding to ensure we reach the minimum 300 byte threshold.\n"
    "Line 6: Testing file I/O with a reasonable amount of test data.\n"
    "End of file content - this string is now over 300 bytes long.";
  if (file1->write(file_content_300)) {
    printf("Wrote %zu bytes\n", file_content_300.length());
  } else {
    printf("Failed to write to file \"%s\"\n", file1->getName().c_str());
  }

  printf("******************************\n");
  printf("getting stats of file \"%s\"\n", file1->getName().c_str());
  printf("File Version          : %zu\n", file1->getVersion());
  printf("File Size             : %zu bytes\n", file1->getSize());
  printf("File Size on Disk     : %zu bytes\n", file1->getSizeOnDisk());
  printf("******************************\n");
  printf("reading current file content of \"%s\"\n", file1->getName().c_str());
  std::string read_content = file1->readAsString();
  printf("File Content          : \"%s\"\n", read_content.c_str());

  printf("******************************\n");
  printf("opening old version of \"%s\"\n", file1->getName().c_str());
  auto old_version_file = file1->openVersion(1);
  if (old_version_file != nullptr) {
    printf("File Size             : %zu bytes\n", old_version_file->getSize());
    printf("File Size on Disk     : %zu bytes\n", old_version_file->getSizeOnDisk());
    std::string old_content = old_version_file->readAsString();
    printf("Version %zu Content     : \"%s\"\n", old_version_file->getVersion(), old_content.c_str());
  } else {
    printf("Failed to open old version of file \"%s\"\n", file1->getName().c_str());
  }

  printf("******************************\n");
  printf("creating a hardlink with different name to file \"%s\"\n", file1->getName().c_str());
  auto hardlink_file = data_dir->createHardlink(file1, "hardlink_to_testfile.txt");
  if (hardlink_file != nullptr) {
    printf("Created hardlink file          : %s\n", hardlink_file->getName().c_str());
    printf("Hardlink File Size             : %zu bytes\n", hardlink_file->getSize());
    printf("Hardlink File Size on Disk     : %zu bytes\n", hardlink_file->getSizeOnDisk());
    printf("File Version                   : %zu\n", hardlink_file->getVersion());
    std::string hardlink_content = hardlink_file->readAsString();
    printf("Hardlink File Content          : \"%s\"\n", hardlink_content.c_str());
  } else {
    printf("Failed to create hardlink to file \"%s\"\n", file1->getName().c_str());
  }

  printf("******************************\n");
  std::shared_ptr<SPFS> new_fs = std::make_shared<SPFS>();
  if(!new_fs->createNewFileSystem(2*1024*1024 - 256*1024, 256*1024, "NewFS", "new_root")) {
    printf("Failed to create new filesystem\n");
    return -1;
  }

  Console console(new_fs);
  console.ExecuteTask();

  SaveFlashStateInFlashFile();
  free(config.flash_data);

  return 0;
}