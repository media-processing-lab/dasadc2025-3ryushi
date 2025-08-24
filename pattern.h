#include <array>
#include <utility>
#include <algorithm>

using Pattern = std::array<bool, 8>;

constexpr int comb(int n, int k) {
    int ans = 1;
    for (int i = 0; i < k; ++i) {
        ans *= n - i;
        ans /= i + 1;
    }
    return ans;
}

constexpr std::array<Pattern, 256> gen_pattern_list() {
    int tail[9] = {0};
    for (int i = 0; i < 8; ++i) {
        tail[i+1] = tail[i] + comb(8, i);
    }
    std::array<Pattern, 256> patterns = {};
    for (int i = 0; i < 256; ++i) {
        int count = __builtin_popcount(i);
        for (int j = 0; j < 8; ++j) {
            patterns[tail[count]][j] = (i >> j) & 1;
        }
        tail[count]++;
    }
    return patterns;
}

constexpr std::array<std::pair<int, int>, 9> gen_pattern_range_for_count() {
    std::array<std::pair<int, int>, 9> ranges = {};
    int bounds[10] = {0};
    for (int i = 0; i <= 8; ++i) {
        bounds[i+1] = bounds[i] + comb(8, i);
    }
    for (int i = 0; i <= 8; ++i) {
        ranges[i].first = bounds[i];
        ranges[i].second = bounds[i+1];
    }
    return ranges;
}

// { FFFFFFFF, FFFFFFFT, FFFFFFTF, ... , TTTTTTTT }
static constexpr std::array<Pattern, 256> pattern_list = gen_pattern_list();

// { {0, 1}, {1, 9}, ... , {255, 256} }
static constexpr std::array<std::pair<int, int>, 9> pattern_range_for_count = gen_pattern_range_for_count();