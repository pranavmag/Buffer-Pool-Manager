#include "disk_manager.h"

#include <stdexcept>

size_t DiskManager::ReadPage(int page_id, Page &page) const {
  size_t offset = page_id * PAGE_SIZE;

  if (offset + page.GetData().size() > bytes.size()) {
    throw std::out_of_range("Read exceeds page bounds");
  }

  std::memcpy(page.GetData().data(), bytes.data() + offset,
              page.GetData().size());

  return page.GetData().size();
}

size_t DiskManager::WritePage(int page_id, Page &page) {
  size_t offset = page_id * PAGE_SIZE;

  if (offset + page.GetData().size() > bytes.size()) {
    throw std::out_of_range("Write exceeds page bounds");
  }

  std::memcpy(bytes.data() + offset, page.GetData().data(),
              page.GetData().size());

  return page.GetData().size();
}