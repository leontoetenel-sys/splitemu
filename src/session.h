#pragma once
#include <algorithm>
#include <array>
#include <vector>

namespace session {
inline bool isPermutation(std::array<int, 4> ids) {
    std::sort(ids.begin(), ids.end());
    return ids == std::array<int, 4>{0, 1, 2, 3};
}
// Swapping keeps each profile assigned to exactly one screen/cable position.
inline void cycleAssignment(std::array<int, 4>& ids, int slot, int direction) {
    int next = (ids[slot] + direction + 4) % 4;
    auto other = std::find(ids.begin(), ids.end(), next);
    std::iter_swap(ids.begin() + slot, other);
}
inline bool validLink(int count, const std::array<int, 4>& screens, int mode,
                      const std::array<int, 4>& cable) {
    if (count < 1 || count > 4 || !isPermutation(screens) || !isPermutation(cable)) return false;
    if (mode == 0 || mode == 2) return true;
    if (mode != 1 && mode != 3) return false;
    int needed = mode == 1 ? 2 : 4;
    for (int i = 0; i < needed; ++i)
        if (std::find(screens.begin(), screens.begin() + count, cable[i]) == screens.begin() + count)
            return false;
    return true;
}
inline std::vector<std::vector<int>> linkGroups(int count, const std::array<int, 4>& screens,
                                               int mode, const std::array<int, 4>& cable) {
    if (!validLink(count, screens, mode, cable) || mode == 0 || count < 2) return {};
    if (mode == 2) {
        std::vector<int> all;
        for (int i = 0; i < count; ++i) all.push_back(i);
        return {all};
    }
    std::vector<std::vector<int>> groups;
    for (int pair = 0; pair < (mode == 3 ? 2 : 1); ++pair) {
        std::vector<int> group;
        for (int i = pair * 2; i < pair * 2 + 2; ++i)
            group.push_back(int(std::find(screens.begin(), screens.end(), cable[i]) - screens.begin()));
        groups.push_back(group);
    }
    return groups;
}
}
