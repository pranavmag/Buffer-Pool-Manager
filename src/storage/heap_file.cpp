#include "storage/heap_file.h"

std::optional<RID> HeapFile::InsertRecord(const Record &record) {
    if (record.empty()) {
        return std::nullopt;
    }

    for (int page_id : page_ids_) {
        Page* page = bpm_.FetchPage(page_id);

        if (page == nullptr) {
            continue;
        }

        std::optional<std::uint16_t> slot_id;

        {
            auto guard = page->WriteLatch();

            HeapPage heap_page(*page);

            slot_id = heap_page.InsertRecord(record);
        }

        if (slot_id.has_value()) {
            bpm_.UnpinPage(page_id, true);

            return RID{
                page_id,
                *slot_id
            };
        }

        bpm_.UnpinPage(page_id, false);
    }

    // No existing page had enough room
    int new_page_id = -1;

    Page* page = bpm_.NewPage(new_page_id);

    if (page == nullptr) {
        return std::nullopt;
    }

    std::optional<std::uint16_t> slot_id;

    {
        auto guard = page->WriteLatch();

        HeapPage heap_page(*page);

        heap_page.Initialize();

        slot_id = heap_page.InsertRecord(record);
    }

    if (!slot_id.has_value()) {
        bpm_.UnpinPage(new_page_id, false);
        return std::nullopt;
    }

    page_ids_.push_back(new_page_id);

    bpm_.UnpinPage(new_page_id, true);

    return RID {
        new_page_id,
        *slot_id
    };
}

std::optional<OwnedRecord> HeapFile::GetRecord(const RID &rid) {
    
}

bool HeapFile::DeleteRecord(const RID &rid) {

}

bool HeapFile::UpdateRecord(const RID &rid, const Record &rec) {

}