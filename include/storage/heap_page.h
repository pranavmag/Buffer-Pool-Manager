# pragma once

#include "storage/page.h"

#include <cstdint>
#include <optional>
#include <span>

using Record = std::span<const std::byte>;   

struct HeapPageHeader {
    std::uint16_t slot_count;
    std::uint16_t free_start; // next free slot offset (grows upwards)
    std::uint16_t free_end; // first occupied record offset (grows downwards, i.e. decrements on insert)
};

struct SlotEntry {
    std::uint16_t offset;
    std::uint16_t length;

    bool IsValid() const {
        return offset != 0 && length != 0;
    }
};

class HeapPage {
private:
    Page& page_;

    HeapPageHeader ReadHeader() const;

    void WriteHeader(const HeapPageHeader& header);

    SlotEntry ReadSlot(std::uint16_t slot_id) const;

    void WriteSlot(std::uint16_t slot_id, const SlotEntry& slot);

public:
    explicit HeapPage(Page& page): page_(page) {}

    void Initialize();

    void Compact(std::uint16_t slot_id, std::uint16_t shift);

    bool RelocateRecord(std::uint16_t slot_id, SlotEntry& slot, const Record& rec);

    std::optional<std::uint16_t> InsertRecord(const Record& rec);
    bool DeleteRecord(std::uint16_t slot_id);
    bool GetRecord(std::uint16_t slot_id);
    bool UpdateRecord(std::uint16_t slot_id, const Record& rec);

    std::uint16_t GetFreeSpace() const;
    std::uint16_t GetSlotCount() const;
};