#include "storage/page_directory.h"

#include <cstring>
#include <stdexcept>

void PageDirectory::Initialize() {
    DirectoryPageHeader header{
        -1, // no next directory page yet
        0   // no entries yet
    };

    WriteHeader(header);
}

DirectoryPageHeader PageDirectory::ReadHeader() const {
    DirectoryPageHeader header;

    std::memcpy(
        &header,
        page_.GetData().data(),
        sizeof(DirectoryPageHeader)
    );

    return header;
}

void PageDirectory::WriteHeader(const DirectoryPageHeader& header) {
    std::memcpy(
        page_.GetData().data(),
        &header,
        sizeof(DirectoryPageHeader)
    );
}

PageDirectoryEntry PageDirectory::ReadEntry(std::uint16_t index) const {
    if (index >= DIRECTORY_ENTRY_CAPACITY) {
        throw std::out_of_range("Page directory entry read out of bounds");
    }

    PageDirectoryEntry entry;

    std::size_t offset = sizeof(DirectoryPageHeader) + index * sizeof(PageDirectoryEntry);

    std::memcpy(
        &entry,
        page_.GetData().data() + offset,
        sizeof(PageDirectoryEntry)
    );

    return entry;
}

void PageDirectory::WriteEntry(std::uint16_t index, const PageDirectoryEntry& entry) {
    if (index >= DIRECTORY_ENTRY_CAPACITY) {
        throw std::out_of_range("Page directory entry write out of bounds");
    }

    std::size_t offset = sizeof(DirectoryPageHeader) + index * sizeof(PageDirectoryEntry);

    std::memcpy(
        page_.GetData().data() + offset,
        &entry,
        sizeof(PageDirectoryEntry)
    );
}

std::uint16_t PageDirectory::GetEntryCount() const {
    return ReadHeader().num_entries;
}