# pragma once

#include "buffer/buffer_pool_manager.h"
#include "storage/heap_page.h"

#include <cstdint>
#include <optional>
#include <vector>

using Rec = std::vector<std::byte>;

struct PageDirectoryEntry {
    int page_id;
    std::uint16_t free_space;
};

struct RID {
    int page_id;
    std::uint16_t slot_id;
};

class HeapFile {
private:
    BufferPoolManager& bpm_;
    std::vector<int> page_ids_;

public:
    explicit HeapFile(BufferPoolManager& bpm): bpm_(bpm) {}

    std::optional<RID> InsertRecord(const Record& record);

    std::optional<Rec> GetRecord(const RID& rid);

    bool DeleteRecord(const RID& rid);

    bool UpdateRecord(const RID& rid, const Record& rec);
};