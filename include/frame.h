# pragma once

#include "page.h"

#include <cstdint>
#include <utility>

class Frame {
private:
    int frame_id_{};
    Page page_;
    std::uint32_t pin_count_{};
    bool is_dirty_{};
public:
    Frame(int frame_id): frame_id_(frame_id) {}
    
    const Page& GetPage() const {
        return page_;
    }

    Page& GetPage() {
        return page_;
    }

    void SetPage(Page page) {
        page_ = std::move(page);
    }

    [[nodiscard]] int GetFrameId() const {
        return frame_id_;
    }

    [[nodiscard]] std::uint32_t GetPinCount() const {
        return pin_count_;
    }

    void IncrementPinCount() {
        ++pin_count_;
    }

    bool DecrementPinCount() {
        if (pin_count_ == 0) {
            return false;
        }

        --pin_count_;
        return true;
    }

    [[nodiscard]] bool IsDirty() const {
        return is_dirty_;
    }

    void MarkDirty() {
        is_dirty_ = true;
    }

    void ClearDirty() {
        is_dirty_ = false;
    }

    [[nodiscard]] bool IsEmpty() const {
        return page_.GetPageId() == -1;
    }   
};