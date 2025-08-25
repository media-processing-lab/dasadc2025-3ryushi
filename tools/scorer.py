#!/usr/bin/env python3
import argparse
from pathlib import Path
import sys
import csv
import matplotlib.pyplot as plt
from dataclasses import dataclass
from enum import Enum


### 色付きのテーブルを使用するためのクラス定義
class CellColor(Enum):
    BLACK = "\033[30m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    MAGENTA = "\033[35m"
    CYAN = "\033[36m"
    WHITE = "\033[37m"
    DEFAULT = "\033[39m"

@dataclass
class Cell:
    content: str
    color: CellColor = CellColor.DEFAULT
    inverted: bool = False

class Table:
    def __init__(self, cells=None):
        self.cells = cells if cells is not None else []

    def add_row(self, row):
        self.cells.append(row)

    def draw(self):
        for i, row in enumerate(self.cells):
            if i == 0:
                print("┌" + "───┬" * (len(row) - 1) + "───┐")
            else:
                print("├" + "───┼" * (len(row) - 1) + "───┤")

            for cell in row:
                invert_code = "\033[7m"
                color_code = cell.color.value if cell.color else CellColor.DEFAULT.value
                reset_code = "\033[0m"

                print("│", end="")
                if cell.inverted:
                    print(f"{invert_code}{color_code}▌{cell.content}▐{reset_code}", end="")
                else:
                    print(f"{color_code} {cell.content} {reset_code}", end="")

            print("│")
        
        # ボトムボーダー
        print("└" + "───┴" * (len(self.cells[0]) - 1) + "───┘")

class Scorer:
    def __init__(self, board_file, solver_output_file):
        self.board_file = board_file
        self.solver_output_file = solver_output_file
        self.boards = {}
        self.solutions = {}
        self.total_score = 0
        self.total_maximum_score = 0
    def load_boards(self):
        """盤面データを読み込む"""
        with open(self.board_file, 'r') as f:
            lines = f.readlines()
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            if not line:
                i += 1
                continue
                
            # ボードヘッダー解析
            parts = line.split()
            if len(parts) >= 3:
                width = int(parts[0])
                height = int(parts[1])
                board_name = parts[2]
                
                # 盤面データ読み込み
                board_data = []
                for j in range(height):
                    i += 1
                    if i < len(lines):
                        board_data.append(lines[i].strip())
                
                self.boards[board_name] = {
                    'width': width,
                    'height': height,
                    'data': board_data
                }
            i += 1
            
    def load_solutions(self):
        """ソルバーの出力を読み込む"""
        with open(self.solver_output_file, 'r') as f:
            lines = f.readlines()
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            if not line:
                i += 1
                continue
                
            # ソリューションヘッダー解析
            parts = line.split()
            if len(parts) >= 3:
                width = int(parts[0])
                height = int(parts[1])
                board_name = parts[2]
                
                # セル選択リスト読み込み
                selected_cells = []
                i += 1
                while i < len(lines):
                    line = lines[i].strip()
                    if not line:
                        break
                    
                    parts = line.split()
                    if len(parts) == 2:
                        try:
                            x = int(parts[0])
                            y = int(parts[1])
                            selected_cells.append((x, y))
                            i += 1
                        except ValueError:
                            break
                    elif len(parts) >= 3:
                        # 次のボードヘッダーの場合
                        i -= 1
                        break
                    else:
                        i += 1
                        break
                
                self.solutions[board_name] = selected_cells
            i += 1
    
    def simulate_game(self, board_name, selected_sequence):
        """ゲームをシミュレートして開かれたセルを計算"""

        if board_name not in self.boards or board_name not in self.solutions:
            return 0
            
        board = self.boards[board_name]
        
        # ゲーム状態の初期化
        revealed = set()
        
        # 各選択を順番にシミュレート
        for x, y in selected_sequence:
            if 0 <= x < board['width'] and 0 <= y < board['height']:
                if (x, y) in revealed:
                    continue
                cell_value = board['data'][y][x]
                if cell_value == '9':
                    revealed.add((x, y))
                else:
                    newly_revealed = self.reveal_cell(board, x, y, revealed)
                    for rx, ry in newly_revealed:
                        revealed.add((rx, ry))
        return revealed

    def calculate_score(self, board_name):
        """指定された盤面のスコアを計算"""
        if board_name not in self.boards or board_name not in self.solutions:
            return 0
            
        board = self.boards[board_name]
        selected_sequence = self.solutions[board_name]
        
        # 開かれたセルをシミュレート
        revealed = self.simulate_game(board_name, selected_sequence)
        #print(f"Revealed cells for {board_name}: {(revealed)}")
        mine_penalty = len([pos for pos in revealed if board['data'][pos[1]][pos[0]] == '9']) * 20
        
        # 地雷を除く全てのセルの数値の合計
        total_non_mine_cells = 0
        for y in range(board['height']):
            for x in range(board['width']):
                if board['data'][y][x] != '9':
                    total_non_mine_cells += int(board['data'][y][x])
        
        revealed_non_mine_cells = sum(
            int(board['data'][pos[1]][pos[0]]) for pos in revealed
            if board['data'][pos[1]][pos[0]] != '9')
        unselected_penalty = total_non_mine_cells - revealed_non_mine_cells

        final_score = revealed_non_mine_cells - mine_penalty - unselected_penalty
        
        return max(0, final_score)  # 負の場合は0
    
    def calculate_maximum_score(self, board_name):
        """指定された盤面の最大スコアを計算"""
        if board_name not in self.boards:
            return 0
            
        board = self.boards[board_name]
        
        # 地雷を除く全てのセルの数値の合計
        total_non_mine_cells = 0
        for y in range(board['height']):
            for x in range(board['width']):
                if board['data'][y][x] != '9':
                    total_non_mine_cells += int(board['data'][y][x])

        return total_non_mine_cells

    def reveal_cell(self, board, x, y, revealed):
        """セルを開き、0の場合は再帰的に周囲も開く"""
        if (x, y) in revealed:
            return set()
        
        if not (0 <= x < board['width'] and 0 <= y < board['height']):
            return set()
            
        cell_value = board['data'][y][x]
        if cell_value == '9':  # 地雷は開かない
            return set()
        
        newly_revealed = {(x, y)}
        revealed.add((x, y))
        
        # 0の場合は周囲を再帰的に開く
        if cell_value == '0':
            for dx in [-1, 0, 1]:
                for dy in [-1, 0, 1]:
                    if dx == 0 and dy == 0:
                        continue
                    nx, ny = x + dx, y + dy
                    if (nx, ny) not in revealed:
                        newly_revealed.update(self.reveal_cell(board, nx, ny, revealed))
        
        return newly_revealed
        
    def score_all_boards(self):
        """全ての盤面のスコアを計算して表示"""
        print("Board Scoring Results:")
        print("-" * 50)
        
        board_scores = {}
        
        total_score = 0
        total_maximum_score = 0
        total_maximum_score_solved_board = 0
        for board_name in self.boards:
            max_score = self.calculate_maximum_score(board_name)
            total_maximum_score += max_score
            if board_name in self.solutions:
                total_maximum_score_solved_board += max_score
                score = self.calculate_score(board_name)
                board_scores[board_name] = score
                total_score += score
                percentage = (score / max_score) * 100 if max_score > 0 else 0
                print(f"{board_name}: {score} ({percentage:.2f}% of maximum {max_score})")
            else:
                print(f"{board_name}: No solution found")
        
        print("-" * 50)
        print(f"Total Score: {total_score}")
        print(f"Total Maximum Score: {total_maximum_score}")
        print(f"Total Maximum Score for Solved Boards: {total_maximum_score_solved_board}")
        
        return board_scores
    
    def score_all_boards_csv(self, output_file, chart_file):
        """全ての盤面のスコアを計算してCSVファイルに出力し、達成率を図示して保存"""
        board_scores = {}
        trial = 1
        running_total = 0
        percentages = []

        # CSVファイルに書き込み
        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['trial', 'boardname', 'score', 'maximum_score', 'percentage', 'total'])

            for board_name in self.boards:
                if board_name in self.solutions:
                    score = self.calculate_score(board_name)
                    max_score = self.calculate_maximum_score(board_name)
                    percentage = (score / max_score) * 100 if max_score > 0 else 0
                    percentages.append(percentage)
                    board_scores[board_name] = score
                    running_total += score
                    writer.writerow([trial, board_name, score, max_score, f"{percentage:.2f}%", running_total])
                    trial += 1
                else:
                    writer.writerow([trial, board_name, 'No solution found', 'N/A', '0%', running_total])
                    trial += 1

        self.total_score = running_total
        print(f"CSV results written to {output_file}")
        print(f"Total Score: {self.total_score}")

        # 達成率を図示して保存
        plt.hist(percentages, bins=10, range=(0, 100), edgecolor='black')
        plt.title('Distribution of Board Completion Percentages')
        plt.xlabel('Completion Percentage (%)')
        plt.ylabel('Number of Boards')
        plt.grid(axis='y')
        plt.savefig(chart_file)
        print(f"Chart saved to {chart_file}")

        return board_scores
    
    def show_board_state(self, board_name):
        """指定された盤面の状態を表示"""
        if board_name not in self.boards:
            print(f"Board {board_name} not found")
            return

        board = self.boards[board_name]
        revealed = self.simulate_game(board_name, self.solutions.get(board_name, []))
        table = Table()
        for y in range(board['height']):
            row = []
            for x in range(board['width']):
                cell = board['data'][y][x]
                if (x, y) in revealed:
                    if cell == '9':
                        row.append(Cell(cell, CellColor.RED))
                    else:
                        row.append(Cell(cell))
                else:
                    if cell == '9':
                        row.append(Cell(cell, inverted=True))
                    else:
                        row.append(Cell(cell, CellColor.MAGENTA, inverted=True))
            table.add_row(row)
        table.draw()
        
    def get_board_details(self, board_name):
        """指定された盤面の詳細情報を表示"""
        if board_name not in self.boards:
            print(f"Board {board_name} not found")
            return
            
        board = self.boards[board_name]
        selected_cells = self.solutions.get(board_name, [])
        
        print(f"\nBoard Details: {board_name}")
        print(f"Size: {board['width']}x{board['height']}")
        self.show_board_state(board_name)
        
        print(f"Selected cells: {len(selected_cells)}")

        # 地雷の数を数える
        mine_count = sum(row.count('9') for row in board['data'])
        print(f"Total mines: {mine_count}")

        revealed_cells = self.simulate_game(board_name, selected_cells)
        print(f"Revealed cells: {len(revealed_cells)}")

        selected_mines = len([pos for pos in revealed_cells if board['data'][pos[1]][pos[0]] == '9'])
        print(f"Selected mines: {selected_mines}")

        # 未選択の非地雷セル数
        total_non_mine_cells = len([cell for row in board['data'] for cell in row if cell != '9'])
        revealed_non_mine_cells = len([pos for pos in revealed_cells if board['data'][pos[1]][pos[0]] != '9'])
        unrevealed_non_mine_cells = total_non_mine_cells - revealed_non_mine_cells
        
        print(f"Total non-mine cells: {total_non_mine_cells}")
        print(f"Revealed non-mine cells: {revealed_non_mine_cells}")
        print(f"Unrevealed non-mine cells: {unrevealed_non_mine_cells}")
        
        # 計算過程を表示
        base_score = sum(int(board['data'][pos[1]][pos[0]]) for pos in revealed_cells if board['data'][pos[1]][pos[0]] != '9')
        highest_possible_score = sum(int(cell) for row in board['data'] for cell in row if cell != '9')
        mine_penalty = selected_mines * 20
        unselected_penalty = highest_possible_score - base_score
        
        print(f"\nScore calculation:")
        print(f"Base score (sum of revealed numbers): {base_score}")
        print(f"Mine penalty ({selected_mines} mines × 20): -{mine_penalty}")
        print(f"Unselected penalty: -{unselected_penalty}")
        print(f"Raw score: {base_score - mine_penalty - unselected_penalty}")
        print(f"Final score: {self.calculate_score(board_name)}")

    def get_incorrect_boards_details(self):
        """不正解の盤面を表示"""
        incorrect_boards = []
        for board_name in self.boards:
            if board_name in self.solutions:
                score = self.calculate_score(board_name)
                max_score = self.calculate_maximum_score(board_name)
                if score < max_score:
                    print(board_name)
                    print(f"Score: {score} / {max_score}")
                    self.show_board_state(board_name)
    
def main():
    parser = argparse.ArgumentParser(description='Mine Sweeper Scoring System')
    parser.add_argument("banmen", type=Path, help="Board data file path")
    parser.add_argument("solver_output", type=Path, help="Solver output file path")
    parser.add_argument("--detailed", "-d", action="store_true", help="Show detailed scoring for each board")
    parser.add_argument("--board", "-b", type=str, help="Show detailed info for specific board")
    parser.add_argument("--csv", "-c", type=Path, help="Output results to CSV file")
    parser.add_argument("--incorrect_boards_details", "-i", action="store_true", help="Show details of incorrect boards")
    args = parser.parse_args()
    
    if not args.banmen.exists():
        print(f"Error: Board file {args.banmen} not found")
        sys.exit(1)
        
    if not args.solver_output.exists():
        print(f"Error: Solver output file {args.solver_output} not found")
        sys.exit(1)
    
    scorer = Scorer(args.banmen, args.solver_output)
    
    try:
        scorer.load_boards()
        scorer.load_solutions()
        
        if args.board:
            scorer.get_board_details(args.board)
        elif args.csv:
            board_scores = scorer.score_all_boards_csv(args.csv, "chart_output.png")
        elif args.incorrect_boards_details:
            scorer.get_incorrect_boards_details()
        else:
            board_scores = scorer.score_all_boards()
            
            if args.detailed:
                print("\nDetailed Board Information:")
                print("=" * 60)
                for board_name in board_scores:
                    scorer.get_board_details(board_name)
                    
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()