#include <gtest/gtest.h>

#include "storage/heap_page.h"
#include "storage/page.h"

#include <algorithm>
#include <array>
#include <optional>

/*
initialization gives slot_count == 0 and expected free space ✓
insert one record and read it back ✓
insert multiple records and verify all contents
delete a middle record and verify the others still work after compaction
reuse a deleted slot
same-size update preserves slot ID and changes contents
shrinking update preserves slot ID and increases free space
growing update succeeds when space exists
growing update fails when insufficient space and leaves the old record intact
*/

TEST(HeapPageTest, InitializesEmptyPage) {
    Page page;
    HeapPage heap_page(page);

    heap_page.Initialize();

    EXPECT_EQ(heap_page.GetSlotCount(), 0);

    EXPECT_EQ(
        heap_page.GetFreeSpace(),
        PAGE_SIZE - sizeof(HeapPageHeader)
    );
}

TEST(HeapPageTest, InsertAndGetRecord) {
    Page page;
    HeapPage heap_page(page);

    heap_page.Initialize();

    std::array<std::byte, 3> data {
        std::byte{1},
        std::byte{2},
        std::byte{3}
    };

    Record rec{data.data(), data.size()};

    auto slot_id = heap_page.InsertRecord(rec);

    EXPECT_EQ(heap_page.GetSlotCount(), 1);
    ASSERT_TRUE(slot_id.has_value());

    auto record = heap_page.GetRecord(*slot_id);

    ASSERT_TRUE(record.has_value());
    ASSERT_EQ(record->size(), rec.size());

    EXPECT_TRUE(std::equal(
        rec.begin(),
        rec.end(),
        record->begin(),
        record->end()
    ));
}