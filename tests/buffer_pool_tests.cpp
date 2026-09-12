#include "buffer/buffer_pool_manager.h"

#include <cassert>
#include <iostream>

void BufferPoolTests() {
    // 2 frames in the buffer pool
    // 3 pages available on the simulated disk
    BufferPoolManager bpm(2, 3);

    // --------------------------------------------------
    // TEST 1: Fetch two pages
    // --------------------------------------------------

    Page* page0 = bpm.FetchPage(0);
    assert(page0 != nullptr);

    {
        auto guard = page0->ReadLatch();
        assert(page0->GetPageId() == 0);
    }

    Page* page1 = bpm.FetchPage(1);
    assert(page1 != nullptr);

    {
        auto guard = page1->ReadLatch();
        assert(page1->GetPageId() == 1);
    }

    std::cout << "TEST 1 PASSED\n";


    // --------------------------------------------------
    // TEST 2: Unpin pages
    // --------------------------------------------------

    assert(bpm.UnpinPage(0, false));
    assert(bpm.UnpinPage(1, false));

    std::cout << "TEST 2 PASSED\n";


    // --------------------------------------------------
    // TEST 3: Modify page 0 and mark it dirty
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    assert(page0 != nullptr);

    {
        auto guard = page0->WriteLatch();
        page0->GetData()[0] = std::byte{'A'};
    }

    assert(bpm.UnpinPage(0, true));

    std::cout << "TEST 3 PASSED\n";


    // --------------------------------------------------
    // TEST 4: Fetch page 2
    // This should force an eviction because we only
    // have 2 frames.
    // --------------------------------------------------

    Page* page2 = bpm.FetchPage(2);
    assert(page2 != nullptr);

    {
        auto guard = page2->ReadLatch();
        assert(page2->GetPageId() == 2);
    }

    assert(bpm.UnpinPage(2, false));

    std::cout << "TEST 4 PASSED\n";


    // --------------------------------------------------
    // TEST 5: Fetch page 0 again
    // Page 0 may have been evicted, so this tests whether
    // the dirty data was written to the simulated disk.
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    assert(page0 != nullptr);

    {
        auto guard = page0->ReadLatch();
        assert(page0->GetData()[0] == std::byte{'A'});
    }

    assert(bpm.UnpinPage(0, false));

    std::cout << "TEST 5 PASSED\n";


    // --------------------------------------------------
    // TEST 6: FlushAllPages
    // --------------------------------------------------

    assert(bpm.FlushAllPages());

    std::cout << "TEST 6 PASSED\n";


    // --------------------------------------------------
    // TEST 7: All Frames Pinned
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    assert(page0 != nullptr);

    page1 = bpm.FetchPage(1);
    assert(page1 != nullptr);

    page2 = bpm.FetchPage(2);

    assert(page2 == nullptr);

    std::cout << "TEST 7 PASSED\n";


    // --------------------------------------------------
    // TEST 8: Multiple Pins
    //
    // Page 0 is already pinned from TEST 7.
    // Fetching it again increments its pin count.
    // One unpin should still leave it pinned.
    // Page 1 is also pinned, so page 2 should still fail.
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    assert(page0 != nullptr);

    assert(bpm.UnpinPage(0, false));

    page2 = bpm.FetchPage(2);
    assert(page2 == nullptr);

    std::cout << "TEST 8 PASSED\n";


    // --------------------------------------------------
    // TEST 9: Dirty eviction and reload
    //
    // Release the remaining pin on page 0 and mark it dirty.
    // Then page 2 should be able to enter the buffer pool.
    // Reload page 0 afterward and verify its data survived.
    // --------------------------------------------------

    assert(bpm.UnpinPage(0, true));

    page2 = bpm.FetchPage(2);
    assert(page2 != nullptr);

    assert(bpm.UnpinPage(2, false));

    Page* page0_reloaded = bpm.FetchPage(0);
    assert(page0_reloaded != nullptr);

    {
        auto guard = page0_reloaded->ReadLatch();
        assert(page0_reloaded->GetData()[0] == std::byte{'A'});
    }

    assert(bpm.UnpinPage(0, false));

    std::cout << "TEST 9 PASSED\n";


    // Clean up the remaining pin on page 1 from TEST 7.
    assert(bpm.UnpinPage(1, false));

    std::cout << "\nAll tests passed!\n";
}