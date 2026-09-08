# pragma once

#include "page.h"

#include <cstddef>
#include <cstdint>
#include <vector>


class DiskManager {
private:
    std::vector<uint8_t> bytes_;

public:
    explicit DiskManager(size_t num_pages): bytes_(num_pages * PAGE_SIZE) {}
    
    std::size_t ReadPage(int page_id, Page& page) const;

    std::size_t WritePage(int page_id, const Page& page);
};