# pragma once

#include "clock_replacer.h"
#include "frame.h"
#include "page_table.h"

#include <vector>

class BufferPoolManager {
private:
    std::vector<Frame> frame_array_;
    PageTable page_table_;
    ClockReplacer clock_replacer_;

public:
    BufferPoolManager(size_t num_frames): clock_replacer_(num_frames) {
        frame_array_.reserve(num_frames);

        for (size_t i{}; i < num_frames; ++i) {
            frame_array_.emplace_back(i);
        }
    }

    void AddPage(int page_id);

    void EvictPage();
};