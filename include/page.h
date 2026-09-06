# pragma once

#include <array>
#include <cstddef>

constexpr int PAGE_SIZE = 4096;

class Page {
private:
    int page_id_ = -1;
    std::array<std::byte, PAGE_SIZE> data_{};
public:
    Page() = default;
    Page(int page_id): page_id_(page_id) {} 

    std::array<std::byte, PAGE_SIZE>& GetData() {
        return data_;
    }

    const std::array<std::byte, PAGE_SIZE>& GetData() const{
        return data_;
    }

    int GetPageId() const {
        return page_id_;
    }


};