#include "page_table.h"

#include <optional>

void PageTable::AddMapping(int page_id, int frame_id) {
    pt_.insert({page_id, frame_id});
}

std::optional<int> PageTable::GetMapping(int page_id) const {
    auto it = pt_.find(page_id);
    if (it != pt_.end()) {
        return it->second;
    }

    return std::nullopt;
}

void PageTable::RemoveMapping(int page_id) {
    pt_.erase(page_id);
}
