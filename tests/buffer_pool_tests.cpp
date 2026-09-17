#include <gtest/gtest.h>

#include "buffer/buffer_pool_manager.h"

TEST(BufferPoolManagerTest, BasicFetchEvictionAndDirtyReload) {
    BufferPoolManager bpm(2, 3);

    // --------------------------------------------------
    // Fetch two pages
    // --------------------------------------------------

    Page* page0 = bpm.FetchPage(0);
    ASSERT_NE(page0, nullptr);

    {
        auto guard = page0->ReadLatch();
        EXPECT_EQ(page0->GetPageId(), 0);
    }

    Page* page1 = bpm.FetchPage(1);
    ASSERT_NE(page1, nullptr);

    {
        auto guard = page1->ReadLatch();
        EXPECT_EQ(page1->GetPageId(), 1);
    }

    // --------------------------------------------------
    // Unpin both pages
    // --------------------------------------------------

    EXPECT_TRUE(bpm.UnpinPage(0, false));
    EXPECT_TRUE(bpm.UnpinPage(1, false));

    // --------------------------------------------------
    // Modify page 0 and mark it dirty
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    ASSERT_NE(page0, nullptr);

    {
        auto guard = page0->WriteLatch();
        page0->GetData()[0] = std::byte{'A'};
    }

    EXPECT_TRUE(bpm.UnpinPage(0, true));

    // --------------------------------------------------
    // Fetch page 2, forcing an eviction
    // --------------------------------------------------

    Page* page2 = bpm.FetchPage(2);
    ASSERT_NE(page2, nullptr);

    {
        auto guard = page2->ReadLatch();
        EXPECT_EQ(page2->GetPageId(), 2);
    }

    EXPECT_TRUE(bpm.UnpinPage(2, false));

    // --------------------------------------------------
    // Reload page 0 and verify dirty data survived
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    ASSERT_NE(page0, nullptr);

    {
        auto guard = page0->ReadLatch();
        EXPECT_EQ(page0->GetData()[0], std::byte{'A'});
    }

    EXPECT_TRUE(bpm.UnpinPage(0, false));

    // --------------------------------------------------
    // Flush all pages
    // --------------------------------------------------

    EXPECT_TRUE(bpm.FlushAllPages());

    // --------------------------------------------------
    // All frames pinned
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    ASSERT_NE(page0, nullptr);

    page1 = bpm.FetchPage(1);
    ASSERT_NE(page1, nullptr);

    page2 = bpm.FetchPage(2);

    EXPECT_EQ(page2, nullptr);

    // --------------------------------------------------
    // Multiple pins
    //
    // page0 already has one pin. Fetch again -> pin count +1.
    // One unpin still leaves page0 pinned.
    // page1 is pinned too, so page2 should still fail.
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    ASSERT_NE(page0, nullptr);

    EXPECT_TRUE(bpm.UnpinPage(0, false));

    page2 = bpm.FetchPage(2);

    EXPECT_EQ(page2, nullptr);

    // --------------------------------------------------
    // Dirty eviction and reload
    // --------------------------------------------------

    EXPECT_TRUE(bpm.UnpinPage(0, true));

    page2 = bpm.FetchPage(2);
    ASSERT_NE(page2, nullptr);

    EXPECT_TRUE(bpm.UnpinPage(2, false));

    Page* page0_reloaded = bpm.FetchPage(0);
    ASSERT_NE(page0_reloaded, nullptr);

    {
        auto guard = page0_reloaded->ReadLatch();
        EXPECT_EQ(
            page0_reloaded->GetData()[0],
            std::byte{'A'}
        );
    }

    EXPECT_TRUE(bpm.UnpinPage(0, false));

    // Clean up page1's remaining pin.
    EXPECT_TRUE(bpm.UnpinPage(1, false));
}