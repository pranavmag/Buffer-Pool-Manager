# pragma once

#include <optional>
#include <unordered_map>

class PageTable {
private:
    std::unordered_map<int, int> pt_{};

public:
    void AddMapping(int page_id, int frame_id);

    std::optional<int> GetMapping(int page_id) const;
    
    void RemoveMapping(int page_id);
};