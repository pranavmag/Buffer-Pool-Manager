#include "buffer_pool_manager.h"

#include <cassert>
#include <optional>
#include <utility>

bool BufferPoolManager::AddPage(int page_id) {
    if (page_table_.GetMapping(page_id)) {
        return true;
    }

    Page page(page_id);

    for (auto& frame : frame_array_) {
        if (frame.IsEmpty()) {
            disk_manager_.ReadPage(page_id,page);

            frame.SetPage(std::move(page));

            page_table_.AddMapping(page_id, frame.GetFrameId());

            return true;
        }
    }

    std::optional<int> victim = clock_replacer_.FindVictim(frame_array_);
    if (victim) {
        Frame& frame = frame_array_[*victim];
        if (frame.IsDirty()) {
            disk_manager_.WritePage(frame.GetPage().GetPageId(), frame.GetPage());
            
            frame.ClearDirty();
        }

        page_table_.RemoveMapping(frame.GetPage().GetPageId());

        disk_manager_.ReadPage(page_id,page);

        frame.SetPage(std::move(page));

        page_table_.AddMapping(page_id, frame.GetFrameId());

        return true;
    }

    return false;
}

bool BufferPoolManager::UnpinPage(int page_id, bool is_dirty) {
    std::optional<int> frame_id = page_table_.GetMapping(page_id);

    if (frame_id) {
        Frame& frame = frame_array_[*frame_id];

        if (is_dirty) {
            frame.MarkDirty();
        }

        return frame.DecrementPinCount();
    }

    return false;
}

Page* BufferPoolManager::FetchPage(int page_id) {
    auto frame_id = page_table_.GetMapping(page_id);

    if (!frame_id) {
        if (!AddPage(page_id)) {
            return nullptr;
        }

        frame_id = page_table_.GetMapping(page_id);

        assert(frame_id.has_value());
    }

    Frame &frame = frame_array_[*frame_id];

    frame.IncrementPinCount();
    clock_replacer_.SetReferenceBit(*frame_id);

    return &frame.GetPage();
}

bool BufferPoolManager::FlushPage(int page_id) {
    std::optional<int> frame_id = page_table_.GetMapping(page_id);

    if (frame_id) {
        Frame& frame = frame_array_[*frame_id];

        if (!frame.IsDirty()) {
            return true;
        }

        const Page& page = frame.GetPage();

        disk_manager_.WritePage(page_id, page);

        frame.ClearDirty();

        return true;
    }

    return false;
}

bool BufferPoolManager::FlushAllPages() {
    for (auto& frame: frame_array_) {
        if (frame.IsEmpty() || !frame.IsDirty()) {
            continue;
        }

        const Page& page = frame.GetPage();

        disk_manager_.WritePage(page.GetPageId(), page);

        frame.ClearDirty();
    }

    return true;
}
