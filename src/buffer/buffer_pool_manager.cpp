#include "buffer/buffer_pool_manager.h"

#include <cassert>
#include <optional>

// Caller must hold bpm_mutex_.
bool BufferPoolManager::AddPage(int page_id) {
    if (page_table_.GetMapping(page_id)) {
        return true;
    }

    for (auto& frame_ptr : frame_array_) {
        Frame& frame = *frame_ptr;

        if (frame.IsEmpty()) {
            frame.AssignPage(page_id);

            Page& page = frame.GetPage();

            auto page_guard = page.WriteLatch();

            disk_manager_.ReadPage(page_id, page);

            page_table_.AddMapping(page_id, frame.GetFrameId());

            return true;
        }
    }

    std::optional<int> victim = clock_replacer_.FindVictim(frame_array_);
    if (victim) {
        Frame& frame = *frame_array_[*victim];
        Page& page = frame.GetPage();

        auto page_guard = page.WriteLatch();

        if (frame.IsDirty()) {
            disk_manager_.WritePage(page.GetPageId(), page);
            
            frame.ClearDirty();
        }

        page_table_.RemoveMapping(page.GetPageId());

        frame.AssignPage(page_id);

        disk_manager_.ReadPage(page_id, page);

        page_table_.AddMapping(page_id, frame.GetFrameId());

        return true;
    }

    return false;
}

Page* BufferPoolManager::NewPage(int& page_id) {
    page_id = disk_manager_.AllocatePage();
    
    return FetchPage(page_id);
}

Page* BufferPoolManager::FetchPage(int page_id) {
    std::lock_guard<std::mutex> guard(bpm_mutex_);

    auto frame_id = page_table_.GetMapping(page_id);

    if (!frame_id) {
        if (!AddPage(page_id)) {
            return nullptr;
        }

        frame_id = page_table_.GetMapping(page_id);

        assert(frame_id.has_value());
    }

    Frame &frame = *frame_array_[*frame_id];

    frame.IncrementPinCount();
    clock_replacer_.SetReferenceBit(*frame_id);

    return &frame.GetPage();
}

bool BufferPoolManager::UnpinPage(int page_id, bool is_dirty) {
    std::lock_guard<std::mutex> guard(bpm_mutex_);

    std::optional<int> frame_id = page_table_.GetMapping(page_id);

    if (frame_id) {
        Frame& frame = *frame_array_[*frame_id];

        if (is_dirty) {
            frame.MarkDirty();
        }

        return frame.DecrementPinCount();
    }

    return false;
}

bool BufferPoolManager::FlushPage(int page_id) {
    std::lock_guard<std::mutex> guard(bpm_mutex_);

    std::optional<int> frame_id = page_table_.GetMapping(page_id);

    if (frame_id) {
        Frame& frame = *frame_array_[*frame_id];

        if (!frame.IsDirty()) {
            return true;
        }

        const Page& page = frame.GetPage();

        auto page_guard = page.ReadLatch();

        disk_manager_.WritePage(page_id, page);

        frame.ClearDirty();

        return true;
    }

    return false;
}

bool BufferPoolManager::FlushAllPages() {
    std::lock_guard<std::mutex> guard(bpm_mutex_);

    for (auto& frame_ptr: frame_array_) {
        Frame& frame = *frame_ptr;

        if (frame.IsEmpty() || !frame.IsDirty()) {
            continue;
        }

        const Page& page = frame.GetPage();

        auto page_guard = page.ReadLatch();

        disk_manager_.WritePage(page.GetPageId(), page);

        frame.ClearDirty();
    }

    return true;
}
