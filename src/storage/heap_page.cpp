#include "storage/heap_page.h"
#include "storage/page.h"

#include <cstring>

void HeapPage::Initialize() {
    HeapPageHeader header{
        0,
        sizeof(HeapPageHeader),
        PAGE_SIZE
    };

    WriteHeader(header);
}

HeapPageHeader HeapPage::ReadHeader() const {
    HeapPageHeader header{};

    std::memcpy(
        &header,
        page_.GetData().data(),
        sizeof(HeapPageHeader)
    );

    return header;
}

void HeapPage::WriteHeader(const HeapPageHeader& header) {
    std::memcpy(
        page_.GetData().data(),
        &header,
        sizeof(HeapPageHeader)
    );
}

SlotEntry HeapPage::ReadSlot(std::uint16_t slot_id) const {
    SlotEntry slot{};

    std::size_t slot_offset = sizeof(HeapPageHeader) + (slot_id * sizeof(SlotEntry));

    std::memcpy(
        &slot,
        page_.GetData().data() + slot_offset,
        sizeof(SlotEntry)
    );

    return slot;
}

void HeapPage::WriteSlot(std::uint16_t slot_id, const SlotEntry& slot) {
    std::size_t slot_offset = sizeof(HeapPageHeader) + (slot_id * sizeof(SlotEntry));

    std::memcpy(
        page_.GetData().data() + slot_offset,
        &slot,
        sizeof(SlotEntry)
    );
}

void HeapPage::Compact(std::uint16_t shift) {
    std::uint16_t slot_count = GetSlotCount();

    for (int i{}; i < slot_count; ++i) {
        SlotEntry slot = ReadSlot(i);

        std::memmove(
            page_.GetData().data() - shift,
            page_.GetData().data() - slot.offset,
            slot.length
        );

        slot.offset -= shift;
    }
}

std::optional<std::uint16_t> HeapPage::InsertRecord(const Record& rec) {
    HeapPageHeader header = ReadHeader();

    std::size_t record_size = rec.size();
    std::size_t required_space = record_size + sizeof(SlotEntry);
    std::size_t free_space = GetFreeSpace();

    if (free_space < required_space) {
        return std::nullopt;
    }

    std::uint16_t slot_id = header.slot_count;

    std::size_t new_record_offset = header.free_end - record_size;  

    std::memcpy(
        page_.GetData().data() + new_record_offset,
        rec.data(),
        record_size
    );

    SlotEntry slot {
        static_cast<std::uint16_t>(new_record_offset),
        static_cast<std::uint16_t>(record_size)
    };

    WriteSlot(slot_id, slot);

    header.free_end = new_record_offset;
    header.free_start += sizeof(SlotEntry);
    ++header.slot_count;

    WriteHeader(header);

    return slot_id;
}

bool HeapPage::DeleteRecord(std::uint16_t slot_id) {
    if (slot_id >= GetSlotCount()) {
        return false;
    }

    SlotEntry slot = ReadSlot(slot_id);

    if (!slot.IsValid()) {
        return false;
    }

    slot.length = 0;
    slot.offset = 0;

    WriteSlot(slot_id, slot);

    return true;
}

bool HeapPage::GetRecord(std::uint16_t slot_id) {

}

bool HeapPage::UpdateRecord(std::uint16_t slot_id, const Record& rec) {
    if (slot_id >= GetSlotCount()) {
        return false;
    }

    SlotEntry slot = ReadSlot(slot_id);

    if (!slot.IsValid()) {
        return false;
    }

    std::size_t record_size = rec.size();

    std::memcpy(
        page_.GetData().data() + slot.offset,
        rec.data(),
        record_size
    );

    return true;
}

std::uint16_t HeapPage::GetFreeSpace() const {
    HeapPageHeader header = ReadHeader();

    return header.free_end - header.free_start;
}

std::uint16_t HeapPage::GetSlotCount() const {
    HeapPageHeader header = ReadHeader();

    return header.slot_count;
}