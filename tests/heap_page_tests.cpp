#include <cstddef>
#include <gtest/gtest.h>

#include "storage/heap_page.h"
#include "storage/page.h"

#include <algorithm>
#include <array>

/*
initialization gives slot_count == 0 and expected free space ✓
insert one record and read it back ✓
insert multiple records and verify all contents ✓
delete a middle record and verify the others still work after compaction ✓
reuse a deleted slot ✓
same-size update preserves slot ID and changes contents ✓
shrinking update preserves slot ID and increases free space ✓
growing update succeeds when space exists ✓
growing update fails when insufficient space and leaves the old record intact ✓
*/

class HeapPageTest : public ::testing::Test {
protected:
    Page page;
    HeapPage heap_page{page};

    void SetUp() override {
        heap_page.Initialize();
    }

    // Getting the Record and Asserting
    void ExpectRecordEquals(std::uint16_t slot_id, const Record &expected) {
      auto actual = heap_page.GetRecord(slot_id);

      ASSERT_TRUE(actual.has_value());
      ASSERT_EQ(actual->size(), expected.size());

      EXPECT_TRUE(std::equal(
        actual->begin(),
        actual->end(),
        expected.begin(),
        expected.end()
       ));
    }

    std::array<std::uint16_t, 4>
    InsertFourRecords(const std::array<std::array<std::byte, 3>, 4> &data) {
      std::array<std::uint16_t, 4> slots{};

      for (std::size_t i{}; i < data.size(); ++i) {
        Record rec{data[i].data(), data[i].size()};

        auto slot = heap_page.InsertRecord(rec);

        EXPECT_TRUE(slot.has_value());

        if (slot.has_value()) {
          slots[i] = *slot;
        }
      }

      return slots;
    }
};

TEST_F(HeapPageTest, InitializesEmptyPage) {
    EXPECT_EQ(heap_page.GetSlotCount(), 0);

    EXPECT_EQ(
        heap_page.GetFreeSpace(),
        PAGE_SIZE - sizeof(HeapPageHeader)
    );
}

TEST_F(HeapPageTest, InsertAndGetRecord) {
    std::array<std::byte, 3> data {
        std::byte{1},
        std::byte{2},
        std::byte{3}
    };

    Record rec{data.data(), data.size()};

    auto slot_id = heap_page.InsertRecord(rec);

    EXPECT_EQ(heap_page.GetSlotCount(), 1);
    ASSERT_TRUE(slot_id.has_value());

    ExpectRecordEquals(*slot_id, rec);
}

TEST_F(HeapPageTest, InsertMultipleRecords) {
    std::array<std::byte, 3> data {
        std::byte{1},
        std::byte{2},
        std::byte{3}
    };

    Record rec{data.data(), data.size()};

    for (std::uint16_t x{}; x < 4; ++x) {
        auto slot_id = heap_page.InsertRecord(rec);

        EXPECT_EQ(heap_page.GetSlotCount(), x + 1);
        ASSERT_TRUE(slot_id.has_value());

        ExpectRecordEquals(*slot_id, rec);
    }
}

TEST_F(HeapPageTest, DeleteRecordAndCompaction) {
    std::array<std::array<std::byte, 3>, 4> data {{
        {std::byte{1}, std::byte{1}, std::byte{1}},
        {std::byte{2}, std::byte{2}, std::byte{2}},
        {std::byte{3}, std::byte{3}, std::byte{3}},
        {std::byte{4}, std::byte{4}, std::byte{4}}
    }};

    std::array<std::uint16_t, 4> slots = InsertFourRecords(data);

    auto free_space_before = heap_page.GetFreeSpace();

    ASSERT_TRUE(heap_page.DeleteRecord(slots[1]));

    EXPECT_EQ(heap_page.GetSlotCount(), 4);
    EXPECT_FALSE(heap_page.GetRecord(slots[1]).has_value());

    EXPECT_GT(heap_page.GetFreeSpace(), free_space_before);

    for (std::size_t i{}; i < data.size(); ++i) {
        if (i == 1) {
            continue;
        }

        Record expected{data[i].data(), data[i].size()};

        ExpectRecordEquals(slots[i], expected);
    }
}

TEST_F(HeapPageTest, ReuseDeletedSlot) {
    std::array<std::array<std::byte, 3>, 4> data {{
        {std::byte{1}, std::byte{1}, std::byte{1}},
        {std::byte{2}, std::byte{2}, std::byte{2}},
        {std::byte{3}, std::byte{3}, std::byte{3}},
        {std::byte{4}, std::byte{4}, std::byte{4}}
    }};

    std::array<std::uint16_t, 4> slots = InsertFourRecords(data);

    ASSERT_TRUE(heap_page.DeleteRecord(slots[1]));

    Record rec{data[1].data(), data[1].size()};

    auto reuse_slot = heap_page.InsertRecord(rec);

    ASSERT_TRUE(reuse_slot.has_value());
    ExpectRecordEquals(*reuse_slot, rec);

    EXPECT_EQ(*reuse_slot, slots[1]);
}

TEST_F(HeapPageTest, SameSizeUpdatePreservesSlotIdAndChangesContents) {
    std::array<std::byte, 3> original_data {
        std::byte{1},
        std::byte{2},
        std::byte{3}
    };

    std::array<std::byte, 3> updated_data {
        std::byte{7},
        std::byte{8},
        std::byte{9}
    };

    Record original{original_data.data(), original_data.size()};

    Record updated{updated_data.data(), updated_data.size()};

    auto slot_id = heap_page.InsertRecord(original);

    ASSERT_TRUE(slot_id.has_value());

    auto slot_count_before = heap_page.GetSlotCount();
    auto free_space_before = heap_page.GetFreeSpace();

    ASSERT_TRUE(
        heap_page.UpdateRecord(*slot_id, updated)
    );

    EXPECT_EQ(
        heap_page.GetSlotCount(),
        slot_count_before
    );

    EXPECT_EQ(
        heap_page.GetFreeSpace(),
        free_space_before
    );

    ExpectRecordEquals(*slot_id, updated);
}

TEST_F(HeapPageTest, ShrinkingUpdateIncreasesFreeSpace) {
    std::array<std::byte, 3> original_data {
        std::byte{1},
        std::byte{2},
        std::byte{3}
    };

    std::array<std::byte, 2> updated_data {
        std::byte{7},
        std::byte{8},
    };

    Record original{original_data.data(), original_data.size()};

    Record updated{updated_data.data(), updated_data.size()};

    auto slot_id = heap_page.InsertRecord(original);

    ASSERT_TRUE(slot_id.has_value());

    auto slot_count_before = heap_page.GetSlotCount();
    auto free_space_before = heap_page.GetFreeSpace();

    ASSERT_TRUE(
        heap_page.UpdateRecord(*slot_id, updated)
    );

    EXPECT_EQ(
        heap_page.GetSlotCount(),
        slot_count_before
    );

    EXPECT_GT(
        heap_page.GetFreeSpace(),
        free_space_before
    );

    ExpectRecordEquals(*slot_id, updated);
}

TEST_F(HeapPageTest, GrowingUpdateSucceedsWhenSpaceAvailable) {
    std::array<std::byte, 3> original_data {
        std::byte{1},
        std::byte{2},
        std::byte{3}
    };

    std::array<std::byte, 4> updated_data {
        std::byte{7},
        std::byte{8},
        std::byte{9},
        std::byte{10}
    };

    Record original{original_data.data(), original_data.size()};

    Record updated{updated_data.data(), updated_data.size()};

    auto slot_id = heap_page.InsertRecord(original);

    ASSERT_TRUE(slot_id.has_value());

    auto slot_count_before = heap_page.GetSlotCount();
    auto free_space_before = heap_page.GetFreeSpace();

    ASSERT_TRUE(
        heap_page.UpdateRecord(*slot_id, updated)
    );

    EXPECT_EQ(
        heap_page.GetSlotCount(),
        slot_count_before
    );

    EXPECT_LT(
        heap_page.GetFreeSpace(),
        free_space_before
    );

    ExpectRecordEquals(*slot_id, updated);
}

TEST_F(HeapPageTest, GrowingUpdateFailsWhenInsufficientSpace) {
    std::array<std::byte, 3> original_data {
        std::byte{1},
        std::byte{2},
        std::byte{3}
    };

    Record original{
        original_data.data(),
        original_data.size()
    };

    auto slot_id = heap_page.InsertRecord(original);

    ASSERT_TRUE(slot_id.has_value());

    // Leave only a tiny amount of free space.
    auto free_space = heap_page.GetFreeSpace();

    std::vector<std::byte> filler_data(
        free_space - sizeof(SlotEntry) - 2,
        std::byte{5}
    );

    Record filler{
        filler_data.data(),
        filler_data.size()
    };

    auto filler_slot = heap_page.InsertRecord(filler);

    ASSERT_TRUE(filler_slot.has_value());

    auto free_space_before_update = heap_page.GetFreeSpace();

    // Original record is 3 bytes. Growing it to 10 bytes requires
    // more space than we intentionally left available.
    std::array<std::byte, 10> updated_data {
        std::byte{7},
        std::byte{7},
        std::byte{7},
        std::byte{7},
        std::byte{7},
        std::byte{7},
        std::byte{7},
        std::byte{7},
        std::byte{7},
        std::byte{7}
    };

    Record updated{
        updated_data.data(),
        updated_data.size()
    };

    EXPECT_FALSE(
        heap_page.UpdateRecord(*slot_id, updated)
    );

    // Failure should not modify page bookkeeping.
    EXPECT_EQ(
        heap_page.GetFreeSpace(),
        free_space_before_update
    );

    EXPECT_EQ(
        heap_page.GetSlotCount(),
        2
    );

    ExpectRecordEquals(*slot_id, original);
    ExpectRecordEquals(*filler_slot, filler);
}