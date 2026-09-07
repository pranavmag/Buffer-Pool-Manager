#include "buffer_pool_manager.h"
#include "clock_replacer.h"
#include "disk_manager.h"
#include "frame.h"
#include "page_table.h"

std::optional<int> BufferPoolManager::AddPage(int page_id) {
    if (page_table_.GetMapping(page_id)) {
        return page_id;
    }

    Page page(page_id);


    for (auto& frame : frame_array_) {
        if (frame.IsEmpty()) {
            disk_manager_.ReadPage(page_id,page);

            frame.SetPage(std::move(page));

            page_table_.AddMapping(page_id, frame.GetFrameId());

            return page_id;
        }
    }

    std::optional<int> victim = clock_replacer_.FindVictim(frame_array_);
    if (victim) {
        Frame& frame = frame_array_[*victim];
        if (frame.IsDirty()) {
            disk_manager_.WritePage(frame.GetPage().GetPageId(), frame.GetPage());
            
            frame_array_[*victim].ClearDirty();
        }

        page_table_.RemoveMapping(frame.GetPage().GetPageId());

        disk_manager_.ReadPage(page_id,page);

        frame.SetPage(std::move(page));

        page_table_.AddMapping(page_id, frame.GetFrameId());

        return page_id;
    } else {
      return std::nullopt;
    }
}

void BufferPoolManager::UnpinPage(int page_id, bool is_dirty) {
    std::optional<int> frame_id = page_table_.GetMapping(page_id);

    if (frame_id) {
        Frame& frame = frame_array_[*frame_id];

        if (is_dirty) {
            frame.MarkDirty();
        }

        frame.DecrementPinCount();
    }
}

Page* BufferPoolManager::FetchPage(int page_id) {
    std::optional<int> frame_id = page_table_.GetMapping(page_id);

    if (frame_id) {
        Frame& frame = frame_array_[*frame_id];

        frame.IncrementPinCount();

        return &frame.GetPage();
    }
    else {
        std::optional<int> result = AddPage(page_id);

        if (result) {
            std::optional<int> frame_id = page_table_.GetMapping(page_id);

            Frame& frame = frame_array_[*frame_id];

            frame.IncrementPinCount();

            return &frame.GetPage();
        }
        else {
            return nullptr;
        }
    }
}

void BufferPoolManager::FlushPage(int page_id) {
    std::optional<int> frame_id = page_table_.GetMapping(page_id);

    if (frame_id) {
        Frame& frame = frame_array_[*frame_id];

        if (!frame.IsDirty()) {
            return;
        }

        Page& page = frame.GetPage();

        disk_manager_.WritePage(page_id, page);

        frame.ClearDirty();
    }
}

void BufferPoolManager::FlushAllPages() {
    for (auto& frame: frame_array_) {
        if (frame.IsEmpty() || !frame.IsDirty()) {
            continue;
        }

        Page& page = frame.GetPage();

        disk_manager_.WritePage(page.GetPageId(), page);

        frame.ClearDirty();
    }
}
