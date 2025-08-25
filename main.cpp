#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
#include <unordered_set>
#include <array>
#include <stack>
#include <set>
#include <cassert>
#include <cstdint>
#include <queue>
#include <limits>

#include "pattern.h"

constexpr std::pair<int, int> neighbors[] = {
    {-1, -1}, {-1, 0}, {-1, 1},
    {0, -1},          {0, 1},
    {1, -1}, {1, 0}, {1, 1}
};
constexpr std::pair<int, int> neighbors_two[] = {
    {-2, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-2, 2},
    {-1, -2}, {-1, -1}, {-1, 0}, {-1, 1}, {-1, 2},
    {0, -2}, {0, -1},         {0, 1}, {0, 2},
    {1, -2}, {1, -1}, {1, 0}, {1, 1}, {1, 2},
    {2, -2}, {2, -1}, {2, 0}, {2, 1}, {2, 2}
};

enum class SearchState {
    Undetermined = 0, // 未確定
    Safe = 1, // 安全なセル
    Mine = 2 // 地雷
};

struct PatternReference {
    uint8_t index;
    PatternReference (const Pattern &pattern) {
        index = std::addressof(pattern) - std::addressof(pattern_list[0]);
    }
    operator const Pattern &() const {
        return get();
    }
    const Pattern &get() const {
        return pattern_list[index];
    }
    bool operator==(const PatternReference &other) const {
        return index == other.index;
    }
    bool operator<(const PatternReference &other) const {
        return index < other.index;
    }
    bool operator[](size_t i) const {
        return get()[i];
    }
};

struct Solver {
    int w, h;
    std::vector<std::vector<int>> board;

    std::vector<std::vector<bool>> opened;
    std::vector<std::vector<SearchState>> state;

    std::set<std::pair<int, int>> cells_to_evaluate;

    std::vector<std::vector<std::set<PatternReference>>> remaining_pattern;

    Solver(std::vector<std::vector<int>> &board, int w, int h):
        board(board), w(w), h(h),
        opened(h, std::vector<bool>(w, false)),
        state(h, std::vector<SearchState>(w, SearchState::Undetermined)),
        remaining_pattern(h, std::vector<std::set<PatternReference>>(w, std::set<PatternReference>())) {
    }

    /**
     * @brief パターンを盤面に適用
     */
    void apply_pattern(int r, int c, const Pattern &pattern) {
        for (int i = 0; i < 8; ++i) {
            int nr = r + neighbors[i].first;
            int nc = c + neighbors[i].second;
            if (nr < 0 || nr >= h || nc < 0 || nc >= w) continue; // 範囲外はスキップ
            if (pattern[i]) {
                state[nr][nc] = SearchState::Mine;
            } else {
                state[nr][nc] = SearchState::Safe;
            }
        }
    }

    /**
     * @brief パターンが隣接セルの状態と一致するかチェック
     */
    bool pattern_check(int r, int c, const Pattern &pattern) {
        for (int i = 0; i < 8; ++i) {
            int nr = r + neighbors[i].first;
            int nc = c + neighbors[i].second;
            if (nr < 0 || nr >= h || nc < 0 || nc >= w) {
                if (pattern[i]) {
                    return false; // 範囲外に地雷があるパターンは無効
                }
                continue;
            }
            if (state[nr][nc] == SearchState::Mine && !pattern[i]) {
                return false;
            }
            if (state[nr][nc] == SearchState::Safe && pattern[i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 盤面の数字が隣接セルの状態と一致するかチェック
     */
    bool board_number_check(int r, int c) {
        int mine = 0, undetermined = 0;
        for (auto [dr, dc] : neighbors) {
            int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= h || nc < 0 || nc >= w) continue; // 範囲外はスキップ
            if (state[nr][nc] == SearchState::Mine) {
                mine++;
            } else if (state[nr][nc] == SearchState::Undetermined) {
                undetermined++;
            }
        }
        assert(opened[r][c]); // 開けてないセルは参照しないように
        return mine <= board[r][c] && undetermined >= board[r][c] - mine;
    }

    /**
     * @brief 隣接セルの状態を取得
     */
    std::array<SearchState, 8> get_neighbors_state(int r, int c) {
        std::array<SearchState, 8> neighbor_states;
        for (int i = 0; i < 8; ++i) {
            int nr = r + neighbors[i].first;
            int nc = c + neighbors[i].second;
            if (nr < 0 || nr >= h || nc < 0 || nc >= w) {
                neighbor_states[i] = SearchState::Undetermined; // 範囲外は未確定
            } else {
                neighbor_states[i] = state[nr][nc];
            }
        }
        return neighbor_states;
    }

    /**
     * @brief 隣接セルの状態を設定
     */
    void set_neighbors_state(int r, int c, const std::array<SearchState, 8> &neighbor_states) {
        for (int i = 0; i < 8; ++i) {
            int nr = r + neighbors[i].first;
            int nc = c + neighbors[i].second;
            if (nr < 0 || nr >= h || nc < 0 || nc >= w) continue; // 範囲外はスキップ
            state[nr][nc] = neighbor_states[i];
        }
    }

    /**
     * @brief 安全セルとして確定
     */
    void confirm_to_be_safe(int r, int c) {
        std::cout << c << " " << r << "\n";
        if (board[r][c] == 9) {
            // 地雷を開けてしまった場合は、開けなかったことにして地雷セルとして確定
            // 出力はしているのでペナルティは入る
            confirm_to_be_mine(r, c);
            return;
        }
        opened[r][c] = true;
        state[r][c] = SearchState::Safe;
        cells_to_evaluate.emplace(r, c);

        // パターンを初期化
        auto [first, last] = pattern_range_for_count[board[r][c]];
        for (int i = first; i < last; ++i) {
            const auto &p = pattern_list[i];
            remaining_pattern[r][c].emplace(p);
        }

        // 隣接セルを再評価
        for (const auto &[dr, dc] : neighbors) {
            int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= h || nc < 0 || nc >= w) continue; // 範囲外はスキップ
            if (state[nr][nc] == SearchState::Safe) {
                cells_to_evaluate.emplace(nr, nc);
            }
        }
    }

    /**
     * @brief 地雷セルとして確定
     */
    void confirm_to_be_mine(int r, int c) {
        state[r][c] = SearchState::Mine;

        // 隣接セルを再評価
        for (const auto &[dr, dc] : neighbors) {
            int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= h || nc < 0 || nc >= w) continue; // 範囲外はスキップ
            if (state[nr][nc] == SearchState::Safe) {
                cells_to_evaluate.emplace(nr, nc);
            }
        }
    }

    /**
     * @brief 残っているパターンに共通の状態を確定する
     * 
     * 盤面のセル(r, c)に残っているパターンについて、あるセルの状態がすべてのパターンで共通している場合、
     * そのセルを確定する。
     */
    void confirm_common_elements(int r, int c) {
        if (remaining_pattern[r][c].empty()) return; // パターンがない場合はスキップ

        std::array<bool, 8> differ = {}; // 共通ならfalse, 異なるならtrue
        const Pattern &reference_pattern = *remaining_pattern[r][c].begin();
        for (const auto &pattern : remaining_pattern[r][c]) {
            for (int i = 0; i < 8; ++i) {
                differ[i] |= reference_pattern[i] != pattern[i];
            }
        }

        for (int i = 0; i < 8; ++i) {
            if (!differ[i]) {
                int nr = r + neighbors[i].first;
                int nc = c + neighbors[i].second;
                if (nr < 0 || nr >= h || nc < 0 || nc >= w) continue; // 範囲外はスキップ
                if (state[nr][nc] == SearchState::Undetermined) {
                    if (reference_pattern[i]) {
                        confirm_to_be_mine(nr, nc);
                    } else {
                        confirm_to_be_safe(nr, nc);
                    }
                }
            }
        }
    }

    /**
     * @brief 簡易パターン決定
     */
    void simple_algorithm() {
        while (!cells_to_evaluate.empty()) {
            auto [r, c] = *cells_to_evaluate.begin();
            cells_to_evaluate.erase(cells_to_evaluate.begin());

            for (auto it = remaining_pattern[r][c].begin(); it != remaining_pattern[r][c].end(); ) {
                auto &pattern = *it;

                // パターンと周囲の状態を比較
                if (!pattern_check(r, c, pattern)) {
                    it = remaining_pattern[r][c].erase(it); // パターンを削除
                    continue;
                }

                // 周囲の状態を保存
                auto saved_state = get_neighbors_state(r, c);

                // パターンを適用
                apply_pattern(r, c, pattern);

                // 盤面の数字と一致するかチェック
                bool valid = true;
                for (const auto &[dr, dc] : neighbors_two) {
                    int nr = r + dr, nc = c + dc;
                    if (nr < 0 || nr >= h || nc < 0 || nc >= w) continue;
                    if (!opened[nr][nc]) continue;

                    if (!board_number_check(nr, nc)) {
                        valid = false;
                        break;
                    }
                }

                // 周囲の状態をリストア
                set_neighbors_state(r, c, saved_state);

                if (valid) {
                    ++it; // 次のパターンへ
                } else {
                    it = remaining_pattern[r][c].erase(it); // パターンを削除
                }
            }

            assert(!remaining_pattern[r][c].empty());

            // パターンすべてで共通する状態のセルは確定
            confirm_common_elements(r, c);
        }
    }

    /**
     * @brief パターンの組み合わせについて再帰的に探索する
     * 
     * search_listにあるセルのパターンの組み合わせを再帰的に探索します。
     * 矛盾しないパターンの組み合わせが見つかった場合、trueを返します。
     *
     * @param search_list 探索するセルのリスト
     * @param search_index 現在の探索インデックス
     * @return true パターンの組み合わせが見つかった場合
     * @return false すべてのパターンの組み合わせが失敗した場合
     */
    bool recursive_search(const std::vector<std::pair<int, int>> &search_list, int search_index = 0) {
        if (search_index == search_list.size()) {
            // すべてのセルを探索した場合、成功
            return true;
        }
        auto [r, c] = search_list[search_index];

        for (const auto &pattern : remaining_pattern[r][c]) {
            if (!pattern_check(r, c, pattern)) continue;
            auto saved_state = get_neighbors_state(r, c); // 周囲の状態を保存
            apply_pattern(r, c, pattern); // パターンを適用

            bool found = recursive_search(search_list, search_index + 1);

            set_neighbors_state(r, c, saved_state); // 周囲の状態をリストア
            if (found) {
                return true;
            }
        }
        return false; // すべてのパターンが失敗した場合はfalse
    }

    /**
     * @brief 再帰パターン決定
     * 
     * @return true 少なくとも1つのパターンが削除された
     * @return false パターンが削除されなかった
     */
    bool recursive_algorithm() {
        static const std::pair<int, int> search_order[] = {
            // distance=1
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},          {0, 1},
            {1, -1}, {1, 0}, {1, 1},
            // distance=2
            {-2, -2}, {-2, -1}, {-2, 0}, {-2, 1}, {-2, 2},
            {-1, -2},                             {-1, 2},
            {0, -2},                              {0, 2},
            {1, -2},                              {1, 2},
            {2, -2}, {2, -1}, {2, 0}, {2, 1}, {2, 2},
        };

        bool pattern_removed = false;
        for (int r = 0; r < h; ++r) {
            for (int c = 0; c < w; ++c) {
                if (!opened[r][c]) continue; // 開けてないセルはスキップ
                if (remaining_pattern[r][c].size() <= 1) continue; // パターンが1つ以下ならスキップ

                // 探索するセルのリストを作成
                std::vector<std::pair<int, int>> search_list;
                for (auto [dr, dc] : search_order) {
                    int nr = r + dr, nc = c + dc;
                    if (nr < 0 || nr >= h || nc < 0 || nc >= w || !opened[nr][nc]) {
                        continue; // 範囲外または開けてないセルは除外
                    }
                    search_list.emplace_back(nr, nc);
                }

                int before_size = remaining_pattern[r][c].size();

                // 各パターンについて削除できるかチェック
                for (auto it = remaining_pattern[r][c].begin(); it != remaining_pattern[r][c].end();) {
                    const auto &pattern = *it;

                    if (!pattern_check(r, c, pattern)) {
                        it = remaining_pattern[r][c].erase(it); // パターンを削除
                        continue;
                    }

                    auto saved_state = get_neighbors_state(r, c); // 周囲の状態を退避
                    apply_pattern(r, c, pattern); // パターンを仮置き

                    // 周囲5x5のパターンの組み合わせから、互いに矛盾しないものが存在するか
                    bool found = recursive_search(search_list);

                    set_neighbors_state(r, c, saved_state); // 周囲の状態をリストア

                    if (found) {
                        // 矛盾しない組み合わせが見つかったので、パターンを削除できない
                        ++it;
                    } else {
                        // どのような組み合わせでも矛盾するので、パターンを削除
                        it = remaining_pattern[r][c].erase(it);
                    }
                }

                int after_size = remaining_pattern[r][c].size();

                if (before_size != after_size) {
                    pattern_removed = true; // 何かパターンが削除された
                    confirm_common_elements(r, c); // パターンが削除された場合、共通要素を確定
                }
            }
        }
        return pattern_removed; // 何かパターンが削除された場合はtrue
    }

    int count_undetermined_cells() const {
        int count = 0;
        for (int r = 0; r < h; ++r) {
            for (int c = 0; c < w; ++c) {
                if (state[r][c] == SearchState::Undetermined) {
                    count++;
                }
            }
        }
        return count;
    }

    bool destiny_flip() {
        // 地雷確率が最小のセルを探す
        double min_probability = std::numeric_limits<double>::max();
        std::pair<int, int> best_cell = {-1, -1};

        for (int r = 0; r < h; ++r) {
            for (int c = 0; c < w; ++c) {
                if (opened[r][c]) {
                    if (remaining_pattern[r][c].size() <= 1) continue; // パターンが1つ以下ならスキップ

                    int mine_count[8] = {};
                    for (auto &pattern: remaining_pattern[r][c]) {
                        for (int i = 0; i < 8; ++i) {
                            mine_count[i] += pattern[i];
                        }
                    }

                    for (int i = 0; i < 8; ++i) {
                        if (mine_count[i] > 0) {
                            // 隣接セルを開けたときの地雷確率を計算
                            double probability = static_cast<double>(mine_count[i]) / remaining_pattern[r][c].size();
                            if (probability < min_probability) {
                                min_probability = probability;
                                best_cell = {r + neighbors[i].first, c + neighbors[i].second};
                            }
                        }
                    }
                }
            }
        }

        if (min_probability<1 && (count_undetermined_cells()*4 -20 * min_probability > 0)) {
            // std::cerr << "Destiny flip: " << best_cell.second << " " << best_cell.first << "\n";
            auto [r, c] = best_cell;
            confirm_to_be_safe(r, c);
            return true;
        } 

        return false;
    }

    void solve() {
        std::vector<std::pair<int, int>> first_to_open;

// モードの定義
#define CORNER 1
#define CENTER 2
#define BOTH 3

#ifndef MODE
#define MODE BOTH
#endif

#if MODE == CORNER || MODE == BOTH
        // 四隅をあける
        first_to_open.emplace_back(0, 0);
        first_to_open.emplace_back(0, w-1);
        first_to_open.emplace_back(h-1, 0);
        first_to_open.emplace_back(h-1, w-1);
#endif
#if MODE == CENTER || MODE == BOTH
        // 中央をあける
        first_to_open.emplace_back(h / 2, w / 2);
        if (h % 2 == 0) first_to_open.emplace_back(h / 2 - 1, w / 2);
        if (w % 2 == 0) first_to_open.emplace_back(h / 2, w / 2 - 1);
        if (h % 2 == 0 && w % 2 == 0) first_to_open.emplace_back(h / 2 - 1, w / 2 - 1);
#endif

        for (const auto &[r, c] : first_to_open) {
            confirm_to_be_safe(r, c);
        }

        while (!cells_to_evaluate.empty()) {
            simple_algorithm();

            bool pattern_removed;
            do {
                pattern_removed = recursive_algorithm();
            } while (pattern_removed && cells_to_evaluate.empty());

            if (!pattern_removed && cells_to_evaluate.empty()) {
                destiny_flip();
            }
        }
    }
};

int main(int argc, char* argv[]) {
    std::string filename;
    if (argc >= 2) {
        filename = argv[1];
    } else {
        filename = "input.txt"; // デフォルトのファイル名
    }

    std::ifstream infile(filename);
    std::string line;

    while (std::getline(infile, line)) {
        if (line.empty()) continue; // 空行はスキップ

        // 1行目の読み込み
        std::istringstream iss(line);
        int h, w;
        std::string board_name;
        iss >> w >> h >> board_name;

        // 盤面データを読み込む
        std::vector<std::vector<int>> board(h, std::vector<int>(w));
        for (int i = 0; i < h; ++i) {
            for (int j = 0; j < w; ++j) {
                char c;
                infile >> c;
                board[i][j] = c - '0'; // 文字を整数に変換
            }
        }

        // 出力の最初の行
        std::cout << w << " " << h << " " << board_name << "\n";

        // ソルバーの結果を出力
        Solver solver(board, w, h);
        solver.solve();

        // 各盤面の出力の間に空行を入れる
        std::cout << "\n"; 
    }
}