# pragma once

#include "frame.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>


class ClockReplacer {
private:
    std::vector<std::uint8_t> reference_bit_{};
    std::size_t clock_hand_{};

public:
    explicit ClockReplacer(std::size_t num_frames): reference_bit_(num_frames, 0) {}

    [[nodiscard]] std::optional<std::size_t> FindVictim(std::vector<std::unique_ptr<Frame>>& frames);

    void SetReferenceBit(int frame_id);
};