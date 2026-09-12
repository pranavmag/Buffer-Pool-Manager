# pragma once

#include "buffer/clock_replacer.h"
#include "storage/disk_manager.h"
#include "buffer/frame.h"
#include "buffer/page_table.h"

#include <memory>
#include <mutex>
#include <vector>

class BufferPoolManager {
private:
    std::vector<std::unique_ptr<Frame>> frame_array_;
    PageTable page_table_;
    ClockReplacer clock_replacer_;
    DiskManager disk_manager_;

    mutable std::mutex bpm_mutex_;

    bool AddPage(int page_id);

public:
  BufferPoolManager(std::size_t num_frames, std::size_t num_disk_pages)
      : clock_replacer_(num_frames), disk_manager_(num_disk_pages) {
        frame_array_.reserve(num_frames);

        for (std::size_t i{}; i < num_frames; ++i) {
            frame_array_.emplace_back(std::make_unique<Frame>(i));
        }
    }

    Page* FetchPage(int page_id);

    bool UnpinPage(int page_id, bool is_dirty);

    bool FlushPage(int page_id);

    bool FlushAllPages();
};