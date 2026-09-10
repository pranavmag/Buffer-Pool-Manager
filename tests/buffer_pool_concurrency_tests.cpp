#include "buffer_pool_manager.h"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

void BPMConcurrentFetchTest() {
    BufferPoolManager bpm(2, 3);

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 1000;

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&bpm]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                Page* page = bpm.FetchPage(0);
                assert(page != nullptr);

                {
                    auto guard = page->ReadLatch();

                    assert(page->GetPageId() == 0);
                }

                assert(bpm.UnpinPage(0, false));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Concurrent fetch test passed!\n";
}

void BPMConcurrentWriteTest() {
    BufferPoolManager bpm(2, 3);

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 20;

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&bpm]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                Page* page = bpm.FetchPage(0);
                assert(page != nullptr);

                {
                    auto guard = page->WriteLatch();

                    auto& data = page->GetData();

                    unsigned int value = std::to_integer<unsigned int>(data[0]);

                    data[0] = static_cast<std::byte>(value + 1);
                }

                assert(bpm.UnpinPage(0, true));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    Page* page = bpm.FetchPage(0);
    assert(page != nullptr);

    {
        auto guard = page->ReadLatch();

        unsigned int final_value = std::to_integer<unsigned int>(page->GetData()[0]);

        assert(final_value == THREAD_COUNT * ITERATIONS);
    }

    assert(bpm.UnpinPage(0, false));

    std::cout << "Concurrent write test passed!\n";
}