#include "clock_replacer.h"

std::optional<int> ClockReplacer::FindVictim(std::vector<Frame>& frames) {
    size_t starting_position = clock_hand_;

    do {
        if (frames[clock_hand_].GetPinCount() > 0) {
            clock_hand_ = (clock_hand_ + 1) % frames.size();
            continue;
        }

        if (reference_bit_[clock_hand_] == 1) {
            reference_bit_[clock_hand_] = 0;
        }
        else if (reference_bit_[clock_hand_] == 0) {
            return clock_hand_;
        }

        clock_hand_ = (clock_hand_ + 1) % frames.size();
    } while (clock_hand_ != starting_position);

    starting_position = clock_hand_;

    do {
        if (frames[clock_hand_].GetPinCount() == 0 && reference_bit_[clock_hand_] == 0) {
            return clock_hand_;
        }

        clock_hand_ = (clock_hand_ + 1) % frames.size();

    } while (clock_hand_ != starting_position);

    return std::nullopt;
}

void ClockReplacer::SetReferenceBit(int frame_id) {
    reference_bit_[frame_id] = 1; 
}