#include <gtest/gtest.h>

#include "storage/heap_file.h"
#include "buffer/buffer_pool_manager.h"
#include "storage/page.h"


/*
1. InsertAndGetSingleRecord
   - create HeapFile
   - insert one record
   - verify returned RID
   - call GetRecord
   - verify bytes match
2. InsertMultipleRecordsSamePage
   - insert several small records
   - verify they all land on the same page_id
   - verify different slot_ids
   - read all records back
3. InsertSpillsAcrossMultiplePages
   - insert records until one page fills
   - keep inserting
   - verify a later record gets a different page_id
   - verify records from both pages are readable
4. GetRecordSurvivesEvictionAndReload
   - insert records
   - use a very small buffer pool
   - force their pages out by touching other pages
   - call GetRecord again
   - verify data reloads correctly from disk
5. DeleteRecord
   - insert a record
   - delete via RID
   - verify DeleteRecord returns true
   - GetRecord(rid) returns nullopt
6. DeleteInvalidRID
   - invalid page_id
   - valid page but invalid slot_id
   - already-deleted RID
   - all should return false
7. UpdateRecordSameSize
   - insert record
   - update with same-size data
   - RID stays the same
   - GetRecord returns new bytes
8. UpdateRecordSmaller
   - insert larger record
   - update with smaller one
   - verify success and new data
9. UpdateRecordLargerWhenSpaceExists
   - insert record
   - update to larger record while page still has room
   - verify success and same RID
10. UpdateRecordFailsWhenPageLacksSpace
    - fill the page enough that growing one record cannot fit
    - update should return false
    - original record must remain unchanged
11. UpdateInvalidRID
    - nonexistent page
    - nonexistent slot
    - deleted slot
    - update should return false
12. OversizedRecordFails
    - construct a record larger than any HeapPage can store
    - InsertRecord returns nullopt
    - later, once we fix allocation semantics, also verify no unreachable disk page was allocated
13. ReturnedRecordOwnsItsData
    - insert record
    - call HeapFile::GetRecord
    - force eviction / overwrite frames
    - verify the returned OwnedRecord still contains the original data
    - this specifically proves the span-to-vector copy boundary is correct
14. DeletedSlotCanBeReused
    - insert A, B, C
    - delete B
    - insert D
    - verify insertion still works and ideally that slot reuse happens as expected through HeapPage
15. RecordsRemainIndependentAcrossPages
    - deliberately create at least two heap pages
    - modify/delete/update records on one page
    - verify records on the other page are untouched
*/

class HeapFileTest : public ::testing::Test {
protected:
    fs::path db_path = "heap_file_test.db";

    std::unique_ptr<BufferPoolManager> bpm;

    void SetUp() override {
        if (fs::exists(db_path)) {
            fs::remove(db_path);
        }

        bpm = std::make_unique<BufferPoolManager>(
            2,
            db_path
        );
    }

    void TearDown() override {
        bpm.reset();

        if (fs::exists(db_path)) {
            fs::remove(db_path);
        }
    }
};

TEST_F(HeapFileTest, InsertAndGetSingleRecord) {
    HeapFile heap_file(*bpm);

    std::array<std::byte, 4> data {
        std::byte{'A'},
        std::byte{'B'},
        std::byte{'C'},
        std::byte{'D'}
    };

    Record record {
        data.data(),
        data.size()
    };

    auto rid = heap_file.InsertRecord(record);

    ASSERT_TRUE(rid.has_value());

    auto stored = heap_file.GetRecord(*rid);

    ASSERT_TRUE(stored.has_value());
    ASSERT_EQ(stored->size(), data.size());

    EXPECT_TRUE(
        std::equal(
            stored->begin(),
            stored->end(),
            data.begin()
        )
    );
}

TEST_F(HeapFileTest, InsertMultipleRecordsSamePage) {
    HeapFile heap_file(*bpm);

    std::array<std::array<std::byte, 1>, 3> records{{
        {std::byte{'A'}},
        {std::byte{'B'}},
        {std::byte{'C'}}
    }};

    std::array<RID, 3> rids{};

    for (std::size_t i = 0; i < records.size(); ++i) {
        auto rid = heap_file.InsertRecord(records[i]);

        ASSERT_TRUE(rid.has_value());

        rids[i] = *rid;
    }

    for (std::size_t i = 1; i < rids.size(); ++i) {
        EXPECT_EQ(rids[i].page_id, rids[0].page_id);
        EXPECT_NE(rids[i].slot_id, rids[0].slot_id);
    }

    for (std::size_t i = 0; i < rids.size(); ++i) {
        auto stored = heap_file.GetRecord(rids[i]);

        ASSERT_TRUE(stored.has_value());
        ASSERT_EQ(stored->size(), records[i].size());

        EXPECT_TRUE(
            std::equal(
                stored->begin(),
                stored->end(),
                records[i].begin()
            )
        );
    }
}

TEST_F(HeapFileTest, InsertSpillsAcrossMultiplePages) {
    HeapFile heap_file(*bpm);

    std::array<std::byte, 1500> data1{};
    std::array<std::byte, 1500> data2{};
    std::array<std::byte, 1500> data3{};

    data1.fill(std::byte{'A'});
    data2.fill(std::byte{'B'});
    data3.fill(std::byte{'C'});

    auto rid1 = heap_file.InsertRecord(data1);
    auto rid2 = heap_file.InsertRecord(data2);
    auto rid3 = heap_file.InsertRecord(data3);

    ASSERT_TRUE(rid1.has_value());
    ASSERT_TRUE(rid2.has_value());
    ASSERT_TRUE(rid3.has_value());

    EXPECT_EQ(rid1->page_id, rid2->page_id);
    EXPECT_NE(rid3->page_id, rid1->page_id);

    auto rec1 = heap_file.GetRecord(*rid1);
    auto rec3 = heap_file.GetRecord(*rid3);

    ASSERT_TRUE(rec1.has_value());
    ASSERT_TRUE(rec3.has_value());

    EXPECT_TRUE(
        std::equal(
            rec1->begin(),
            rec1->end(),
            data1.begin()
        )
    );

    EXPECT_TRUE(
        std::equal(
            rec3->begin(),
            rec3->end(),
            data3.begin()
        )
    );
}

TEST_F(HeapFileTest, GetRecordSurvivesEvictionAndReload) {
    bpm.reset();

    bpm = std::make_unique<BufferPoolManager>(
        1,
        db_path
    );

    HeapFile heap_file(*bpm);

    std::array<std::byte, 2500> data1{};
    std::array<std::byte, 2500> data2{};

    data1.fill(std::byte{'A'});
    data2.fill(std::byte{'B'});

    auto rid1 = heap_file.InsertRecord(data1);
    auto rid2 = heap_file.InsertRecord(data2);

    ASSERT_TRUE(rid1.has_value());
    ASSERT_TRUE(rid2.has_value());

    ASSERT_NE(rid1->page_id, rid2->page_id);

    // With one frame, accessing page 2 should force page 1 out.
    auto rec2 = heap_file.GetRecord(*rid2);
    ASSERT_TRUE(rec2.has_value());

    auto rec1 = heap_file.GetRecord(*rid1);
    ASSERT_TRUE(rec1.has_value());

    EXPECT_TRUE(
        std::equal(
            rec1->begin(),
            rec1->end(),
            data1.begin()
        )
    );
}

TEST_F(HeapFileTest, DeleteRecord) {
    HeapFile heap_file(*bpm);

    std::array<std::byte, 4> data{
        std::byte{'A'},
        std::byte{'B'},
        std::byte{'C'},
        std::byte{'D'}
    };

    auto rid = heap_file.InsertRecord(data);

    ASSERT_TRUE(rid.has_value());

    auto before_delete = heap_file.GetRecord(*rid);
    ASSERT_TRUE(before_delete.has_value());

    EXPECT_TRUE(
        heap_file.DeleteRecord(*rid)
    );

    auto after_delete = heap_file.GetRecord(*rid);

    EXPECT_FALSE(after_delete.has_value());
}

TEST_F(HeapFileTest, DeleteInvalidRID) {
    HeapFile heap_file(*bpm);

    std::array<std::byte, 4> data{
        std::byte{'A'},
        std::byte{'B'},
        std::byte{'C'},
        std::byte{'D'}
    };

    auto rid = heap_file.InsertRecord(data);

    ASSERT_TRUE(rid.has_value());

    // Invalid page_id
    RID invalid_page{
        rid->page_id + 100,
        0
    };

    EXPECT_FALSE(
        heap_file.DeleteRecord(invalid_page)
    );

    // Valid page, invalid slot_id
    RID invalid_slot{
        rid->page_id,
        static_cast<std::uint16_t>(rid->slot_id + 100)
    };

    EXPECT_FALSE(
        heap_file.DeleteRecord(invalid_slot)
    );

    // Delete once successfully
    EXPECT_TRUE(
        heap_file.DeleteRecord(*rid)
    );

    // Deleting the same RID again should fail
    EXPECT_FALSE(
        heap_file.DeleteRecord(*rid)
    );
}

TEST_F(HeapFileTest, UpdateRecordSameSize) {
    HeapFile heap_file(*bpm);

    std::array<std::byte, 4> original{
        std::byte{'A'},
        std::byte{'B'},
        std::byte{'C'},
        std::byte{'D'}
    };

    std::array<std::byte, 4> updated{
        std::byte{'W'},
        std::byte{'X'},
        std::byte{'Y'},
        std::byte{'Z'}
    };

    auto rid = heap_file.InsertRecord(original);

    ASSERT_TRUE(rid.has_value());

    RID original_rid = *rid;

    EXPECT_TRUE(
        heap_file.UpdateRecord(
            *rid,
            updated
        )
    );

    // RID should still refer to the same logical record.
    EXPECT_EQ(rid->page_id, original_rid.page_id);
    EXPECT_EQ(rid->slot_id, original_rid.slot_id);

    auto stored = heap_file.GetRecord(*rid);

    ASSERT_TRUE(stored.has_value());
    ASSERT_EQ(stored->size(), updated.size());

    EXPECT_TRUE(
        std::equal(
            stored->begin(),
            stored->end(),
            updated.begin()
        )
    );
}