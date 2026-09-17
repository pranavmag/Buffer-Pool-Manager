#include <gtest/gtest.h>

#include "buffer/buffer_pool_manager.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <thread>
#include <vector>

// --------------------------------------------------
// Concurrent Fetch
// --------------------------------------------------

TEST(BufferPoolConcurrencyTest, ConcurrentFetch) {
    BufferPoolManager bpm(2, 3);

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 1000;

    std::atomic<bool> failed{false};

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                Page* page = bpm.FetchPage(0);

                if (page == nullptr) {
                    failed = true;
                    return;
                }

                {
                    auto guard = page->ReadLatch();

                    if (page->GetPageId() != 0) {
                        failed = true;
                        return;
                    }
                }

                if (!bpm.UnpinPage(0, false)) {
                    failed = true;
                    return;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_FALSE(failed.load());
}


// --------------------------------------------------
// Concurrent Write
// --------------------------------------------------

TEST(BufferPoolConcurrencyTest, ConcurrentWrite) {
    BufferPoolManager bpm(2, 3);

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 20;

    std::atomic<bool> failed{false};

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                Page* page = bpm.FetchPage(0);

                if (page == nullptr) {
                    failed = true;
                    return;
                }

                {
                    auto guard = page->WriteLatch();

                    auto& data = page->GetData();

                    unsigned int value =
                        std::to_integer<unsigned int>(data[0]);

                    data[0] =
                        static_cast<std::byte>(value + 1);
                }

                if (!bpm.UnpinPage(0, true)) {
                    failed = true;
                    return;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    ASSERT_FALSE(failed.load());

    Page* page = bpm.FetchPage(0);
    ASSERT_NE(page, nullptr);

    {
        auto guard = page->ReadLatch();

        unsigned int final_value =
            std::to_integer<unsigned int>(
                page->GetData()[0]
            );

        EXPECT_EQ(
            final_value,
            THREAD_COUNT * ITERATIONS
        );
    }

    EXPECT_TRUE(bpm.UnpinPage(0, false));
}


// --------------------------------------------------
// Concurrent Eviction
// --------------------------------------------------

TEST(BufferPoolConcurrencyTest, ConcurrentEviction) {
    BufferPoolManager bpm(2, 3);

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 1000;

    std::atomic<bool> failed{false};

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                int page_id = j % 3;

                Page* page = bpm.FetchPage(page_id);

                // Legitimate under contention if all frames are pinned.
                if (page == nullptr) {
                    continue;
                }

                {
                    auto guard = page->ReadLatch();

                    if (page->GetPageId() != page_id) {
                        failed = true;
                        return;
                    }
                }

                if (!bpm.UnpinPage(page_id, false)) {
                    failed = true;
                    return;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_FALSE(failed.load());
}


// --------------------------------------------------
// Concurrent Dirty Eviction
// --------------------------------------------------

TEST(BufferPoolConcurrencyTest, ConcurrentDirtyEviction) {
    BufferPoolManager bpm(2, 3);

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 20;

    std::atomic<bool> failed{false};

    std::array<std::atomic<int>, 3> successful_writes{};

    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                int page_id = j % 3;

                Page* page = bpm.FetchPage(page_id);

                if (page == nullptr) {
                    continue;
                }

                {
                    auto guard = page->WriteLatch();

                    auto& data = page->GetData();

                    unsigned int value =
                        std::to_integer<unsigned int>(
                            data[0]
                        );

                    data[0] =
                        static_cast<std::byte>(value + 1);
                }

                if (!bpm.UnpinPage(page_id, true)) {
                    failed = true;
                    return;
                }

                successful_writes[page_id].fetch_add(1);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    ASSERT_FALSE(failed.load());

    for (int page_id = 0; page_id < 3; ++page_id) {
        Page* page = bpm.FetchPage(page_id);

        ASSERT_NE(page, nullptr);

        {
            auto guard = page->ReadLatch();

            unsigned int stored =
                std::to_integer<unsigned int>(
                    page->GetData()[0]
                );

            unsigned int expected =
                successful_writes[page_id].load();

            EXPECT_EQ(stored, expected);
        }

        EXPECT_TRUE(
            bpm.UnpinPage(page_id, false)
        );
    }
}


// --------------------------------------------------
// Randomized Stress
// --------------------------------------------------

TEST(BufferPoolConcurrencyTest, RandomizedStress) {
    constexpr int NUM_FRAMES = 3;
    constexpr int NUM_PAGES = 8;

    constexpr int THREAD_COUNT = 8;
    constexpr int ITERATIONS = 2000;

    BufferPoolManager bpm(
        NUM_FRAMES,
        NUM_PAGES
    );

    std::array<std::atomic<int>, NUM_PAGES> expected{};

    std::atomic<bool> failed{false};

    std::vector<std::thread> threads;

    for (int thread_id = 0;
         thread_id < THREAD_COUNT;
         ++thread_id) {

        threads.emplace_back(
            [&, thread_id]() {
                std::mt19937 rng(
                    static_cast<unsigned int>(
                        std::chrono::steady_clock::now()
                            .time_since_epoch()
                            .count()
                    ) + thread_id
                );

                std::uniform_int_distribution<int>
                    page_dist(0, NUM_PAGES - 1);

                std::uniform_int_distribution<int>
                    operation_dist(0, 1);

                for (int i = 0;
                     i < ITERATIONS;
                     ++i) {

                    int page_id = page_dist(rng);
                    bool do_write = operation_dist(rng);

                    Page* page =
                        bpm.FetchPage(page_id);

                    if (page == nullptr) {
                        continue;
                    }

                    if (do_write) {
                        {
                            auto guard =
                                page->WriteLatch();

                            auto& data =
                                page->GetData();

                            std::uint32_t value{};

                            std::memcpy(
                                &value,
                                data.data(),
                                sizeof(value)
                            );

                            ++value;

                            std::memcpy(
                                data.data(),
                                &value,
                                sizeof(value)
                            );
                        }

                        if (!bpm.UnpinPage(
                                page_id,
                                true)) {
                            failed = true;
                            return;
                        }

                        expected[page_id]
                            .fetch_add(1);
                    }
                    else {
                        {
                            auto guard =
                                page->ReadLatch();

                            if (page->GetPageId()
                                != page_id) {
                                failed = true;
                                return;
                            }
                        }

                        if (!bpm.UnpinPage(
                                page_id,
                                false)) {
                            failed = true;
                            return;
                        }
                    }
                }
            }
        );
    }

    for (auto& thread : threads) {
        thread.join();
    }

    ASSERT_FALSE(failed.load());

    ASSERT_TRUE(bpm.FlushAllPages());

    for (int page_id = 0;
         page_id < NUM_PAGES;
         ++page_id) {

        Page* page = bpm.FetchPage(page_id);

        ASSERT_NE(page, nullptr);

        {
            auto guard = page->ReadLatch();

            std::uint32_t stored{};

            std::memcpy(
                &stored,
                page->GetData().data(),
                sizeof(stored)
            );

            std::uint32_t expected_value =
                expected[page_id].load();

            EXPECT_EQ(
                stored,
                expected_value
            );
        }

        EXPECT_TRUE(
            bpm.UnpinPage(page_id, false)
        );
    }
}