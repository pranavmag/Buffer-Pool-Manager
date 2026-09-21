#include "storage/disk_manager.h"
#include "storage/page.h"

#include <cstring>
#include <stdexcept>

int DiskManager::AllocatePage() {
    int page_id =
        static_cast<int>(bytes_.size() / PAGE_SIZE);

    bytes_.resize(
        bytes_.size() + PAGE_SIZE,
        std::uint8_t{0}
    );

    return page_id;
}

std::size_t DiskManager::ReadPage(int page_id, Page &page) const {
  if (page_id < 0) {
    throw std::out_of_range("Invalid page id");
  
  }

  size_t offset = static_cast<std::size_t>(page_id) * PAGE_SIZE;

  if (offset + page.GetData().size() > bytes_.size()) {
    throw std::out_of_range("Read exceeds page bounds");
  }

  std::memcpy(page.GetData().data(), bytes_.data() + offset,
              PAGE_SIZE);

  return page.GetData().size();
}

std::size_t DiskManager::WritePage(int page_id, const Page &page) {
  if (page_id < 0) {
    throw std::out_of_range("Invalid page id");
  }

  size_t offset = static_cast<std::size_t>(page_id) * PAGE_SIZE;

  if (offset + page.GetData().size() > bytes_.size()) {
    throw std::out_of_range("Write exceeds page bounds");
  }

  std::memcpy(bytes_.data() + offset, page.GetData().data(),
              PAGE_SIZE);

  return page.GetData().size();
}