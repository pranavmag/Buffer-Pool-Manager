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

    bool AddPage(int page_id);

public:
  BufferPoolManager(std::size_t num_frames, std::size_t num_disk_pages)
      : clock_replacer_(num_frames), disk_manager_(num_disk_pages) {
        frame_array_.reserve(num_frames);

        for (std::size_t i{}; i < num_frames; ++i) {
            frame_array_.emplace_back(i);
        }
    }

    bool UnpinPage(int page_id, bool is_dirty);

    Page* FetchPage(int page_id);

    bool FlushPage(int page_id);

    bool FlushAllPages();
};