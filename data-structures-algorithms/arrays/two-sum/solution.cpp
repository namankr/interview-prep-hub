#include <cassert>
#include <unordered_map>
#include <vector>
using namespace std;

vector<int> twoSum(const vector<int>& nums, int target) {
    unordered_map<int, int> seen;  // value -> index
    for (int i = 0; i < (int)nums.size(); ++i) {
        auto it = seen.find(target - nums[i]);
        if (it != seen.end()) return {it->second, i};
        seen[nums[i]] = i;
    }
    return {};
}

int main() {
    assert((twoSum({2, 7, 11, 15}, 9) == vector<int>{0, 1}));
    assert((twoSum({3, 3}, 6) == vector<int>{0, 1}));
    return 0;
}
