#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include "Flash/flashHAL.h"
#include "Flash/SPFS.h"

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
    .fs_size = 1 * 1024 * 1024,
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
  auto root = spfs->getRootDirectory();

  printf("******************************\n");
  printf("File System Size      : %i\n", spfs->getFileSystemSize());
  printf("Directory Name        : %s\n", root->getName().c_str());

  CreateSubDirectory(root, "config");
  auto data = CreateSubDirectory(root, "data");

  CreateSubDirectory(data, "files");

  printf("******************************\n");
  printf("reopening the same filesystem\n");
  std::shared_ptr<SPFS> reopened_spfs = std::make_shared<SPFS>();
  auto reopened_root = reopened_spfs->getRootDirectory();
  printf("File System Size      : %i\n", spfs->getFileSystemSize());
  printf("Root Directory Name   : %s\n", reopened_root->getName().c_str());

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
  printf("File Version          : %zu\n", file1->getVersionCount());
  printf("File Size             : %zu bytes\n", file1->getFileSize());
  printf("File Size on Disk     : %zu bytes\n", file1->getFileSizeOnDisk());

  printf("******************************\n");
  printf("add content to file \"%s\"\n", file1->getName().c_str());

  std::string file_content = "This is a test content for the file.\nIt has multiple lines.\nThis is line 3.\nEnd of file.";
  if (file1->write(file_content)) {
    printf("Wrote %zu bytes\n", file_content.length());
  } else {
    printf("Failed to write to file \"%s\"\n", file1->getName().c_str());
  }
  
  SaveFlashStateInFlashFile();

  printf("******************************\n");
  printf("getting stats of file \"%s\"\n", file1->getName().c_str());
  printf("File Version          : %zu\n", file1->getVersionCount());
  printf("File Size             : %zu bytes\n", file1->getFileSize());
  printf("File Size on Disk     : %zu bytes\n", file1->getFileSizeOnDisk());

  SaveFlashStateInFlashFile();
  free(config.flash_data);

  return 0;
}