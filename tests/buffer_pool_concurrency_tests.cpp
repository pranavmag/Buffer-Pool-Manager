#include "buffer_pool_manager.h"

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
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

void BPMConcurrentEvictionTest() {
    BufferPoolManager bpm(2, 3);

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 1000;

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&bpm]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                int page_id = j % 3;

                Page* page = bpm.FetchPage(page_id);

                if (page == nullptr) {
                    continue;
                }

                {
                    auto guard = page->ReadLatch();

                    assert(page->GetPageId() == page_id);
                }

                assert(bpm.UnpinPage(page_id, false));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Concurrent eviction test passed!\n";
}

void BPMConcurrentDirtyEvictionTest() {
    BufferPoolManager bpm(2, 3);

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 20;

    std::vector<std::thread> threads;

    std::array<std::atomic<int>, 3> successful_writes{};

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&bpm, &successful_writes]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                int page_id = j % 3;

                Page* page = bpm.FetchPage(page_id);

                if (page == nullptr) {
                    continue;
                }

                {
                    auto guard = page->WriteLatch();

                    auto& data = page->GetData();

                    unsigned int value = std::to_integer<unsigned int>(data[0]);

                    data[0] = static_cast<std::byte>(value + 1);
                }

                assert(bpm.UnpinPage(page_id, true));

                successful_writes[page_id].fetch_add(1);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (int page_id = 0; page_id < 3; ++page_id) {
        Page* page = bpm.FetchPage(page_id);
        assert(page != nullptr);

        {
            auto guard = page->ReadLatch();

            unsigned int value =
            std::to_integer<unsigned int>(page->GetData()[0]);

            int expected = successful_writes[page_id].load();

            std::cout << "Page " << page_id
                << " stored value: " << value 
                << " expected=" << expected << '\n';
            
            assert(value == expected);
        }

        assert(bpm.UnpinPage(page_id, false));
    }

    std::cout << "Concurrent dirty eviction test passed!\n";
}

void BPMRandomizedStressTest() {
    constexpr int NUM_FRAMES = 3;
    constexpr int NUM_PAGES = 8;

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 2000;

    BufferPoolManager bpm(NUM_FRAMES, NUM_PAGES);

    std::array<std::atomic<int>, NUM_PAGES> expected{};

    std::vector<std::thread> threads;

    for (int thread_id = 0; thread_id < THREAD_COUNT; ++thread_id) {
        threads.emplace_back([&bpm, &expected, thread_id]() {
            std::mt19937 rng(
                static_cast<unsigned int>(
                    std::chrono::steady_clock::now()
                        .time_since_epoch()
                        .count()
                ) + thread_id
            );

            std::uniform_int_distribution<int> page_dist(
                0, NUM_PAGES - 1
            );

            std::uniform_int_distribution<int> operation_dist(
                0, 1
            );

            for (int i = 0; i < ITERATIONS; ++i) {
                int page_id = page_dist(rng);
                bool do_write = operation_dist(rng);

                Page* page = bpm.FetchPage(page_id);

                if (page == nullptr) {
                    continue;
                }

                if (do_write) {
                    {
                        auto guard = page->WriteLatch();

                        auto& data = page->GetData();

                        std::uint32_t value{};
                        std::memcpy(&value, data.data(), sizeof(value));

                        ++value;

                        std::memcpy(data.data(), &value, sizeof(value));
                    }

                    assert(bpm.UnpinPage(page_id, true));

                    expected[page_id].fetch_add(1);
                }
                else {
                    {
                        auto guard = page->ReadLatch();

                        assert(page->GetPageId() == page_id);
                    }

                    assert(bpm.UnpinPage(page_id, false));
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    bpm.FlushAllPages();

    for (int page_id = 0; page_id < NUM_PAGES; ++page_id) {
        Page* page = bpm.FetchPage(page_id);
        assert(page != nullptr);

        {
            auto guard = page->ReadLatch();

            std::uint32_t stored{};

            std::memcpy(&stored, page->GetData().data(), sizeof(stored));

            std::uint32_t expected_value = expected[page_id].load();

            std::cout << "Page " << page_id << " stored=" << stored
                      << " expected=" << expected_value << '\n';

            assert(stored == expected_value);
        }
        
        assert(bpm.UnpinPage(page_id, false));
    }

    std::cout << "Randomized stress test passed!\n";
}