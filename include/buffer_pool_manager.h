# pragma once

#include "clock_replacer.h"
#include "disk_manager.h"
#include "frame.h"
#include "page_table.h"

#include <vector>

class BufferPoolManager {
private:
    std::vector<Frame> frame_array_;
    PageTable page_table_;
    ClockReplacer clock_replacer_;
    DiskManager disk_manager_;

public:
  BufferPoolManager(size_t num_frames, size_t num_disk_pages)
      : clock_replacer_(num_frames), disk_manager_(num_disk_pages) {
        frame_array_.reserve(num_frames);

        for (size_t i{}; i < num_frames; ++i) {
            frame_array_.emplace_back(i);
        }
    }

    std::optional<int> AddPage(int page_id);

    void UnpinPage(int page_id, bool is_dirty);

    Page* FetchPage(int page_id);

    void FlushPage(int page_id);

    void FlushAllPages();
};