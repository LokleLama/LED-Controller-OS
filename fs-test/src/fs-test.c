#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
    .flash_file = "flash.bin",
    .flash_data = NULL
};

#define MAGIC_NUMBER  0xA36CA3FA

static void OpenOrCreateFlashFile() {
  FILE* f = fopen(config.flash_file, "r+b");
  if (f != NULL) {
    //find filesize
    fseek(f, 0, SEEK_END);
    config.flash_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // read it
    config.flash_data = malloc(config.flash_size);
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

  // file doesn't exist, create it
  f = fopen(config.flash_file, "w+b");
  if (f == NULL) {
    perror("Failed to create flash file");
    return;
  }

  // allocate flash data
  config.flash_data = malloc(config.flash_size);
  if (config.flash_data == NULL) {
    perror("Failed to allocate flash data");
    fclose(f);
    return;
  }

  memset(config.flash_data, 0xFF, config.flash_size);

  // write magic number at fs_offset
  uint32_t* magic = (uint32_t*)(config.flash_data + config.fs_offset);
  *magic = MAGIC_NUMBER;

  if (fwrite(config.flash_data, 1, config.flash_size, f) != config.flash_size) {
    perror("Failed to write initial flash data");
    free(config.flash_data);
    fclose(f);
    return;
  }

  fflush(f);
  fclose(f);
}

static void SaveFlashStateInFlashFile() {
  FILE* f = fopen(config.flash_file, "r+b");
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

int main(int argc, char **argv) {
  printf("Usage: fs-test [flash-size]\n");
  if (argc > 1) {
    config.flash_size = strtoul(argv[1], NULL, 0);
  }
  config.flash_size = RoundSizeToSector(config.flash_size);

  OpenOrCreateFlashFile();
  if (config.flash_data == NULL) {
    return -1;
  }

  printf("using flash file %s\n", config.flash_file);
  printf("using flash offset of %zu\n", config.fs_offset);
  printf("using flash size of %zu\n", config.fs_size);
  printf("using flash sector size of %zu\n", config.sector_size);
  printf("using flash page size of %zu\n", config.page_size);
  printf("using flash Size of %zu\n", config.flash_size);

  //TODO: add tests here

  SaveFlashStateInFlashFile();
  free(config.flash_data);

  return 0;
}