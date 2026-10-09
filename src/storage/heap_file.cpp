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
    int page_id = rid.page_id;
    std::uint16_t slot_id = rid.slot_id;

    Page* page = bpm_.FetchPage(page_id);

    if (page == nullptr) {
        return std::nullopt;
    }

    std::optional<OwnedRecord> result;

    {
        auto guard = page->ReadLatch();

        HeapPage heap_page(*page);

        auto record = heap_page.GetRecord(slot_id);

        if (record.has_value()) {
            result = OwnedRecord(
                record->begin(),
                record->end()
            );
        }
    }

    bpm_.UnpinPage(page_id, false);

    return result;
}

bool HeapFile::DeleteRecord(const RID &rid) {
    int page_id = rid.page_id;
    std::uint16_t slot_id = rid.slot_id;

    Page* page = nullptr;

    try {
        page = bpm_.FetchPage(page_id);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    bool is_deleted = false;

    {
        auto guard = page->WriteLatch();

        HeapPage heap_page(*page);

        is_deleted = heap_page.DeleteRecord(slot_id);
    }

    bpm_.UnpinPage(page_id, is_deleted);

    return is_deleted;
}

bool HeapFile::UpdateRecord(const RID &rid, const Record &rec) {
    int page_id = rid.page_id;
    std::uint16_t slot_id = rid.slot_id;

    Page* page = bpm_.FetchPage(page_id);

    if (page == nullptr) {
        return false;
    }

    bool is_updated = false;

    {
        auto guard = page->WriteLatch();

        HeapPage heap_page(*page);

        is_updated = heap_page.UpdateRecord(slot_id, rec);
    }

    bpm_.UnpinPage(page_id, is_updated);

    return is_updated;
}