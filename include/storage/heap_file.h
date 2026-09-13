#include "heap_page.h"

struct PageDirectoryEntry {
    int page_id;
    std::uint16_t free_space;
};