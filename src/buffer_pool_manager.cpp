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

        disk_manager_.ReadPage(page_id,page);
        
        frame_array_[*victim].SetPage(std::move(page));

        page_table_.AddMapping(page_id, frame.GetFrameId());

        return page_id;
    } else {
      // no victim available what should we do in this case, what if it cant find a victim, does it loop
      // clock again or what?
      return std::nullopt;
    }
}