#include "storage/page.h"

#include <cstdint>

struct HeapPageHeader {
    std::uint16_t slot_count;
    std::uint16_t free_start;
    std::uint16_t free_end;

    int next_page_id;
    int prev_page_id;
};

struct SlotEntry {
    std::uint16_t offset;
    std::uint16_t length;
};

class HeapPage {
private:
    Page& page_;

public:
    
};