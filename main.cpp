#include "buffer_pool_manager.h"
#include <cassert>
#include <iostream>

int main() {
    // 2 frames in the buffer pool
    // 3 pages available on the simulated disk
    BufferPoolManager bpm(2, 3);

    // --------------------------------------------------
    // TEST 1: Fetch two pages
    // --------------------------------------------------

    Page* page0 = bpm.FetchPage(0);
    assert(page0 != nullptr);
    assert(page0->GetPageId() == 0);

    Page* page1 = bpm.FetchPage(1);
    assert(page1 != nullptr);
    assert(page1->GetPageId() == 1);

    std::cout << "TEST 1 PASSED\n";


    // --------------------------------------------------
    // TEST 2: Unpin pages
    // --------------------------------------------------

    bpm.UnpinPage(0, false);
    bpm.UnpinPage(1, false);

    std::cout << "TEST 2 PASSED\n";


    // --------------------------------------------------
    // TEST 3: Modify page 0 and mark it dirty
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    assert(page0 != nullptr);

    page0->GetData()[0] = std::byte{'A'};

    bpm.UnpinPage(0, true);

    std::cout << "TEST 3 PASSED\n";


    // --------------------------------------------------
    // TEST 4: Fetch page 2
    // This should force an eviction because we only
    // have 2 frames.
    // --------------------------------------------------

    Page* page2 = bpm.FetchPage(2);
    assert(page2 != nullptr);
    assert(page2->GetPageId() == 2);

    bpm.UnpinPage(2, false);

    std::cout << "TEST 4 PASSED\n";


    // --------------------------------------------------
    // TEST 5: Fetch page 0 again
    // Page 0 may have been evicted, so this tests whether
    // the dirty data was written to the simulated disk.
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    assert(page0 != nullptr);

    assert(page0->GetData()[0] == std::byte{'A'});

    bpm.UnpinPage(0, false);

    std::cout << "TEST 5 PASSED\n";


    // --------------------------------------------------
    // TEST 6: FlushAllPages
    // --------------------------------------------------

    bpm.FlushAllPages();

    std::cout << "TEST 6 PASSED\n";


    // --------------------------------------------------
    // TEST 7: All Frames Pinned
    // --------------------------------------------------

    page0 = bpm.FetchPage(0);
    page1 = bpm.FetchPage(1);
    page2 = bpm.FetchPage(2);

    assert(page2 == nullptr);

    std::cout << "TEST 7 PASSED\n";


    // --------------------------------------------------
    // TEST 8: Multiple Pins
    // Page 1 should have 1 pin still and
    // Page 0 should have 1 pin after these operations
    // It should not be evicting Page 0 because of one unpin
    // Page 2 should still be nullptr
    // --------------------------------------------------

    
    page0 = bpm.FetchPage(0);
    bpm.UnpinPage(0, false);

    assert(page2 == nullptr);

    std::cout << "TEST 8 PASSED\n";


    // --------------------------------------------------
    // TEST 9: Flushing Dirty Page
    // --------------------------------------------------

    bpm.UnpinPage(0, true);

    page2 = bpm.FetchPage(2);
    assert(page2 != nullptr);

    bpm.UnpinPage(2, false);

    Page* page0_reloaded = bpm.FetchPage(0);
    assert(page0_reloaded != nullptr);
    assert(page0_reloaded->GetData()[0] == std::byte{'A'});

    std::cout << "TEST 9 PASSED\n";
    

    std::cout << "\nAll tests passed!\n";
}