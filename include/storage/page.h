# pragma once

#include <array>
#include <cstddef>
#include <mutex>
#include <shared_mutex>

constexpr std::size_t PAGE_SIZE = 4096;

class Page {
private:
    int page_id_ = -1;
    std::array<std::byte, PAGE_SIZE> data_{};
    mutable std::shared_mutex rw_mutex_;
public:
    Page() = default;
    Page(int page_id): page_id_(page_id) {} 

    std::array<std::byte, PAGE_SIZE>& GetData() {
        return data_;
    }

    const std::array<std::byte, PAGE_SIZE>& GetData() const{
        return data_;
    }

    [[nodiscard]] int GetPageId() const {
        return page_id_;
    }

    void Reset(int page_id) {
        page_id_ = page_id;
    }

    std::shared_lock<std::shared_mutex> ReadLatch() const {
        return std::shared_lock{rw_mutex_};
    }

    std::unique_lock<std::shared_mutex> WriteLatch() {
        return std::unique_lock{rw_mutex_};
    }
};