#include "buffer_pool_manager.h"
#include "clock_replacer.h"
#include "frame.h"
#include "page_table.h"

void BufferPoolManager::AddPage(int page_id) {
    if (page_table_.GetMapping(page_id)) {
        return;
    }

    for (auto& frame : frame_array_) {
        if (frame.IsEmpty()) {
            // add the page into the frame? do we have a way to do that right now?
            // we can't do push_back because that would add a new frame but how do we get that page
            // in the frame? hm

            page_table_.AddMapping(page_id, frame.GetFrameId());
        }
    }

    std::optional<int> victim = clock_replacer_.FindVictim(frame_array_);
    if (victim) {
      // somehow evict from frame
    } else {
      // no victim available?
    }
}