#include <gtest/gtest.h>
#include "Flash/SPFS.h"
#include "Flash/flashHAL.h"
#include "mock_flash.h"

#include <cstdlib>
#include <cstring>

static constexpr size_t FLASH_SIZE = 256 * 1024; // 256 KB simulated flash

class SPFSTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = mock_flash_init(FLASH_SIZE);
        ASSERT_NE(buffer, nullptr);
        spfs = std::make_shared<SPFS>();
    }

    void TearDown() override {
        spfs.reset();
        mock_flash_cleanup();
        buffer = nullptr;
    }

    uint8_t *buffer = nullptr;
    std::shared_ptr<SPFS> spfs;
};

// ---------------------------------------------------------------------------
//  Filesystem creation
// ---------------------------------------------------------------------------

TEST_F(SPFSTest, CreateFileSystem) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getName(), "root");
}

TEST_F(SPFSTest, FileSystemName) {
    spfs->createNewFileSystem(0, FLASH_SIZE, "MyFS", "root");
    EXPECT_EQ(spfs->getFileSystemName(), "MyFS");
}

TEST_F(SPFSTest, FileSystemVersion) {
    spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    std::string ver = spfs->getFileSystemVersion();
    EXPECT_FALSE(ver.empty());
}

TEST_F(SPFSTest, FileSystemSize) {
    spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    EXPECT_EQ(spfs->getFileSystemSize(), (int)FLASH_SIZE);
}

// ---------------------------------------------------------------------------
//  Search for existing filesystem
// ---------------------------------------------------------------------------

TEST_F(SPFSTest, SearchFindsExistingFS) {
    spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");

    auto spfs2 = std::make_shared<SPFS>();
    auto root = spfs2->searchFileSystem(0, FLASH_SIZE);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(spfs2->getFileSystemName(), "TestFS");
}

TEST_F(SPFSTest, SearchReturnsNullOnEmpty) {
    auto root = spfs->searchFileSystem(0, FLASH_SIZE);
    EXPECT_EQ(root, nullptr);
}

// ---------------------------------------------------------------------------
//  Directory operations
// ---------------------------------------------------------------------------

TEST_F(SPFSTest, CreateSubdirectory) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    auto subdir = root->createDirectory("subdir");
    ASSERT_NE(subdir, nullptr);
    EXPECT_EQ(subdir->getName(), "subdir");
}

TEST_F(SPFSTest, ListSubdirectories) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    root->createDirectory("dir1");
    root->createDirectory("dir2");

    auto subdirs = root->getSubdirectories();
    EXPECT_EQ(subdirs.size(), 2u);
}

TEST_F(SPFSTest, OpenSubdirectory) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);
    root->createDirectory("mydir");

    auto opened = root->openSubdirectory("mydir");
    ASSERT_NE(opened, nullptr);
    EXPECT_EQ(opened->getName(), "mydir");
}

TEST_F(SPFSTest, NestedDirectories) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    auto sub = root->createDirectory("level1");
    ASSERT_NE(sub, nullptr);
    auto subsub = sub->createDirectory("level2");
    ASSERT_NE(subsub, nullptr);

    EXPECT_EQ(subsub->getName(), "level2");
    EXPECT_NE(subsub->getParent(), nullptr);
}

// ---------------------------------------------------------------------------
//  File operations
// ---------------------------------------------------------------------------

TEST_F(SPFSTest, CreateAndWriteFile) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    auto file = root->createFile("test.txt");
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(file->write("Hello, SPFS!"));
}

TEST_F(SPFSTest, ReadBackFileContent) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    auto file = root->createFile("readme.txt");
    ASSERT_NE(file, nullptr);
    std::string content = "Test content 12345";
    EXPECT_TRUE(file->write(content));

    auto opened = root->openFile("readme.txt");
    ASSERT_NE(opened, nullptr);
    // write() stores string + null terminator, so readAsString() includes trailing \0
    EXPECT_STREQ(opened->readAsString().c_str(), content.c_str());
}

TEST_F(SPFSTest, ReadFileAsVector) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    auto file = root->createFile("data.bin");
    ASSERT_NE(file, nullptr);
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_TRUE(file->write(data));

    auto opened = root->openFile("data.bin");
    ASSERT_NE(opened, nullptr);
    auto readback = opened->readAsVector();
    EXPECT_EQ(readback, data);
}

TEST_F(SPFSTest, FileCount) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    root->createFile("a.txt");
    root->createFile("b.txt");
    root->createFile("c.txt");

    EXPECT_EQ(root->getFileCount(), 3u);
}

TEST_F(SPFSTest, ListFiles) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    root->createFile("alpha.txt");
    root->createFile("beta.txt");

    auto files = root->getFiles();
    EXPECT_EQ(files.size(), 2u);
}

// ---------------------------------------------------------------------------
//  File versioning
// ---------------------------------------------------------------------------

TEST_F(SPFSTest, FileVersioning) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    auto file = root->createFile("versioned.txt");
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(file->write("version 1"));
    EXPECT_TRUE(file->write("version 2"));

    // Opening the file should give the latest version
    auto opened = root->openFile("versioned.txt");
    ASSERT_NE(opened, nullptr);
    EXPECT_STREQ(opened->readAsString().c_str(), "version 2");
}

// ---------------------------------------------------------------------------
//  Stream interface
// ---------------------------------------------------------------------------

TEST_F(SPFSTest, GetInputStream) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    auto file = root->createFile("stream.txt");
    ASSERT_NE(file, nullptr);
    std::string content = "line1\nline2\nline3";
    EXPECT_TRUE(file->write(content));

    auto opened = root->openFile("stream.txt");
    ASSERT_NE(opened, nullptr);
    auto stream = opened->getInputStream();
    ASSERT_NE(stream, nullptr);

    std::string line;
    std::getline(*stream, line);
    EXPECT_EQ(line, "line1");
    std::getline(*stream, line);
    EXPECT_EQ(line, "line2");
    std::getline(*stream, line);
    EXPECT_STREQ(line.c_str(), "line3");  // last line may include trailing null from SPFS storage
}

// ---------------------------------------------------------------------------
//  Block usage map
// ---------------------------------------------------------------------------

TEST_F(SPFSTest, BlockUsageMap) {
    auto root = spfs->createNewFileSystem(0, FLASH_SIZE, "TestFS", "root");
    ASSERT_NE(root, nullptr);

    auto blocks = spfs->getBlockUsageMap();
    EXPECT_FALSE(blocks.empty());

    // At least some blocks should be USED (header, root dir)
    bool has_used = false;
    for (auto b : blocks) {
        if (b != SPFS::BlockState::FREE) {
            has_used = true;
            break;
        }
    }
    EXPECT_TRUE(has_used);
}
