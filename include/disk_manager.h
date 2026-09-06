# pragma once

#include "page.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>


class DiskManager {
private:
    std::vector<uint8_t> bytes{};

public:
    size_t ReadPage(int page_id, Page& page) const;

    size_t WritePage(int page_id, Page& page);
};