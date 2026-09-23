/*
1. CreatesDatabaseFile
give DiskManager a path that does not exist
construct it
verify the file now exists
verify initial size is 0

2. AllocatePageCreatesZeroFilledPage
create a fresh DB file
call AllocatePage()
expect returned ID 0
expect file size PAGE_SIZE
ReadPage(0, page)
verify the page bytes are all zero

3. AllocateMultiplePagesReturnsSequentialIds
allocate three pages
expect IDs 0, 1, 2
expect file size:
3 * PAGE_SIZE

4. WriteAndReadPage
allocate a page
modify some bytes in a Page
WritePage(0, page)
read page 0 into a different Page
verify the bytes match

5. PagesRemainIndependent
allocate page 0 and page 1
write different data to each
read both back
verify page 0 contains only page-0 data and page 1 contains only page-1 data

6. DataPersistsAfterReopen
this is probably the most important one
create DiskManager
allocate page
write recognizable data
let that DiskManager go out of scope
construct a new DiskManager with the same path
read the page
verify data survived

7. ReadInvalidPageThrows
fresh file has zero pages
try:
disk.ReadPage(0, page);
expect:
EXPECT_THROW(..., std::out_of_range);
also test -1

8. WriteInvalidPageThrows
same idea for WritePage()
page must be allocated before it can be written

9. RejectsMisalignedDatabaseFile
manually create a file whose size is not divisible by 4096, perhaps 10 bytes
constructing DiskManager should throw std::runtime_error
*/

#include <gtest/gtest.h>

#include "storage/disk_manager.h"
#include "storage/page.h"

class DiskManagerTest : public ::testing::Test {
protected:
    fs::path CreateDatabaseFile() {
        fs::path db_path = "test.db";

        if (fs::exists(db_path)) {
          fs::remove(db_path);
        }

        return db_path;
    }

};

TEST_F(DiskManagerTest, CreateDatabaseFile) {
    fs::path db_path = CreateDatabaseFile();

    DiskManager disk_manager(db_path);

    EXPECT_TRUE(fs::exists(db_path));
    EXPECT_EQ(fs::file_size(db_path), 0);

    fs::remove(db_path);
}

TEST_F(DiskManagerTest, AllocatePageCreatesZeroFilledPage) {
    fs::path db_path = CreateDatabaseFile();

    DiskManager disk_manager(db_path);

    EXPECT_TRUE(fs::exists(db_path));
    EXPECT_EQ(fs::file_size(db_path), 0);

    int page_id = disk_manager.AllocatePage();

    ASSERT_EQ(page_id, 0);
    ASSERT_EQ(fs::file_size(db_path), PAGE_SIZE);

    Page page(page_id);

    std::size_t bytes_read = disk_manager.ReadPage(page_id, page);

    ASSERT_EQ(bytes_read, PAGE_SIZE);

    for (const auto& byte : page.GetData()) {
        EXPECT_EQ(byte, std::byte{0});
    }

    fs::remove(db_path);
}

TEST_F(DiskManagerTest, AllocateMultiplePagesReturnsSequentialIds) {
    fs::path db_path = CreateDatabaseFile();

    DiskManager disk_manager(db_path);

    EXPECT_TRUE(fs::exists(db_path));
    EXPECT_EQ(fs::file_size(db_path), 0);

    for (int i{}; i < 3; ++i) {
        int page_id = disk_manager.AllocatePage();

        ASSERT_EQ(page_id, i);
    }

    ASSERT_EQ(fs::file_size(db_path), 3 * PAGE_SIZE);
}

TEST_F(DiskManagerTest, WriteAndReadPage) {
    fs::path db_path = CreateDatabaseFile();

    DiskManager disk_manager(db_path);

    EXPECT_TRUE(fs::exists(db_path));
    EXPECT_EQ(fs::file_size(db_path), 0);

    int page_id = disk_manager.AllocatePage();

    Page page0(page_id);
    page0.GetData().fill(std::byte{'1'});

    std::size_t bytes_written = disk_manager.WritePage(page_id, page0);

    Page page1(page_id);
    
    std::size_t bytes_read = disk_manager.ReadPage(page_id, page1);

    ASSERT_EQ(bytes_written, PAGE_SIZE);
    ASSERT_EQ(bytes_read, PAGE_SIZE);

    EXPECT_EQ(page0.GetData(), page1.GetData());
}