# pragma once

#include "frame.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>


class ClockReplacer {
private:
    std::vector<uint8_t> reference_bit_{};
    size_t clock_hand_{};

public:
    ClockReplacer(size_t num_frames): reference_bit_(num_frames, 0) {}

    std::optional<int> FindVictim(std::vector<Frame>& frames);

    void SetReferenceBit(int page_id);
};