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

void HeapPage::Compact(std::uint16_t deleted_offset, std::uint16_t deleted_length) {
    std::uint16_t slot_count = GetSlotCount();

    for (std::uint16_t i{}; i < slot_count; ++i) {
        SlotEntry slot = ReadSlot(i);

        if (!slot.IsValid()) {
            continue;
        }

        if (slot.offset < deleted_offset) {
            std::memmove(
                page_.GetData().data() + slot.offset + deleted_length,
                page_.GetData().data() + slot.offset,
                slot.length
            );

            slot.offset += deleted_length;

            WriteSlot(i, slot);
        }
    }

    HeapPageHeader header = ReadHeader();
    header.free_end += deleted_length;
    WriteHeader(header);
}

bool HeapPage::RelocateRecord(std::uint16_t slot_id, SlotEntry& slot, const Record& rec) {
    HeapPageHeader header = ReadHeader();

    std::size_t record_size = rec.size();

    std::uint16_t old_offset = slot.offset;
    std::uint16_t old_length = slot.length;

    Compact(old_offset, old_length);

    header = ReadHeader();

    std::size_t new_offset = header.free_end - record_size;

    std::memmove(page_.GetData().data() + new_offset, rec.data(), record_size);

    slot.offset = static_cast<std::uint16_t>(new_offset);
    slot.length = static_cast<std::uint16_t>(record_size);
    WriteSlot(slot_id, slot);

    header.free_end = static_cast<std::uint16_t>(new_offset);
    WriteHeader(header);

    return true;
}

std::optional<std::uint16_t> HeapPage::InsertRecord(const Record& rec) {
    HeapPageHeader header = ReadHeader();

    if (rec.empty()) {
        return std::nullopt;
    }   

    std::size_t record_size = rec.size();
    std::size_t free_space = GetFreeSpace();

    std::uint16_t slot_count = GetSlotCount();
    std::uint16_t slot_id = slot_count;
    std::size_t required_space = record_size + sizeof(SlotEntry);
    bool reusing_slot = false;

    for (std::uint16_t i{}; i < slot_count; ++i) {
        SlotEntry slot = ReadSlot(i);

        if (!slot.IsValid()) {
            slot_id = i;
            reusing_slot = true;
            required_space = record_size;
            break;
        }
    }

    if (free_space < required_space) {
        return std::nullopt;
    }

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

    if (!reusing_slot) {
        header.free_start += sizeof(SlotEntry);
        ++header.slot_count;
    }

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

    std::uint16_t deleted_offset = slot.offset;
    std::uint16_t deleted_length = slot.length;

    slot.length = 0;
    slot.offset = 0;

    WriteSlot(slot_id, slot);

    Compact(deleted_offset, deleted_length);

    return true;
}

bool HeapPage::GetRecord(std::uint16_t slot_id) {

}

bool HeapPage::UpdateRecord(std::uint16_t slot_id, const Record& rec) {
    if (rec.empty()) {
        return false;
    } 

    if (slot_id >= GetSlotCount()) {
        return false;
    }

    SlotEntry slot = ReadSlot(slot_id);

    if (!slot.IsValid()) {
        return false;
    }

    std::size_t record_size = rec.size();

    if (record_size == slot.length) {
        std::memmove(
            page_.GetData().data() + slot.offset,
            rec.data(),
            record_size
        );

        return true;
    }
    else if (record_size < slot.length) {
        return RelocateRecord(slot_id, slot, rec);
    }
    else {
        std::size_t free_space = GetFreeSpace() + slot.length;

        if (free_space < record_size) {
            return false;
        }

        return RelocateRecord(slot_id, slot, rec);
    }
}

std::uint16_t HeapPage::GetFreeSpace() const {
    HeapPageHeader header = ReadHeader();

    return header.free_end - header.free_start;
}

std::uint16_t HeapPage::GetSlotCount() const {
    HeapPageHeader header = ReadHeader();

    return header.slot_count;
}