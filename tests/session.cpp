#include "../src/session.h"
#undef NDEBUG
#include <cassert>
#include <iostream>
int main() {
    std::array<int, 4> screens{0,1,2,3};
    int cases = 0;
    do {
        std::array<int,4> cable{0,1,2,3};
        do {
            auto pairs = session::linkGroups(4, screens, 3, cable);
            assert(pairs.size() == 2);
            for (int p=0;p<2;++p)
                for(int i=0;i<2;++i) assert(screens[pairs[p][i]] == cable[p*2+i]);
            auto pair = session::linkGroups(4, screens, 1, cable);
            assert(pair.size() == 1 && pair[0] == pairs[0]);
            assert(session::linkGroups(4, screens, 0, cable).empty());
            assert(session::linkGroups(4, screens, 2, cable)[0] == std::vector<int>({0,1,2,3}));
            assert(!session::validLink(3,screens,3,cable));
            for(int slot=0;slot<4;++slot) {
                auto swap = screens;
                session::cycleAssignment(swap,slot,1);
                assert(session::isPermutation(swap));
                session::cycleAssignment(swap,slot,-1);
                assert(swap == screens);
            }
            ++cases;
        } while (std::next_permutation(cable.begin(),cable.end()));
    } while (std::next_permutation(screens.begin(),screens.end()));
    assert(!session::isPermutation({0,0,2,3}));
    assert(!session::validLink(2,{0,1,2,3},1,{0,2,1,3}));
    auto cross = session::linkGroups(4,{0,1,2,3},3,{0,2,1,3});
    assert(cross == std::vector<std::vector<int>>({{0,2},{1,3}}));
    std::cout << cases << " screen/cable permutations passed\n";
}
