#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

constexpr char ALIVE    {'#'};
constexpr char DEAD     {'.'};
constexpr int  ROWS      {40};
constexpr int  COLS      {40};
constexpr int  DELAY_MS {100};

void displayGrid(const std::vector<std::vector<char>>& grid) {
    for (const auto& row : grid) {
        for (const char cell : row) {
            std::cout << cell << ' ';
        }
        std::cout << '\n';
    }
}

std::vector<std::vector<char>> createGrid(const int rows, const int cols) {
    return std::vector<std::vector<char>>(rows, std::vector<char>(cols, DEAD));
}

void setInitialPattern(std::vector<std::vector<char>>& grid) {
    const auto centerRow = grid.size() / 2;
    const auto centerCol = grid[0].size() / 2;

    grid[centerRow][centerCol+1]   = ALIVE;
    grid[centerRow+1][centerCol+2] = ALIVE;
    grid[centerRow+2][centerCol]   = ALIVE;
    grid[centerRow+2][centerCol+1] = ALIVE;
    grid[centerRow+2][centerCol+2] = ALIVE;
}

int countAliveNeighbors(const std::vector<std::vector<char>>& grid, const int row, const int col) {
    int count = 0;
    const auto rows = static_cast<int>(grid.size());
    const auto cols = static_cast<int>(grid[0].size());

    for (int dRow = -1; dRow <= 1; dRow++) {
        for (int dCol = -1; dCol <= 1; dCol++) {
            if (dRow == 0 && dCol == 0) continue;

            const int neighborRow = (row + dRow + rows) % rows;
            const int neighborCol = (col + dCol + cols) % cols;

            if (grid[neighborRow][neighborCol] == ALIVE) {
                count++;
            }
        }
    }

    return count;
}

std::vector<std::vector<char>> nextGeneration(const std::vector<std::vector<char>>& grid) {
    const auto rows = static_cast<int>(grid.size());
    const auto cols = static_cast<int>(grid[0].size());
    std::vector<std::vector<char>> newGrid = grid;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            const int neighbors = countAliveNeighbors(grid, i, j);

            if (grid[i][j] == ALIVE) {
                // Alive cell
                if (neighbors < 2 || neighbors > 3) {
                    // Dies due to underpopulation or overpopulation
                    newGrid[i][j] = DEAD;
                }
                // If neighbors is 2 or 3, cell stays alive (already ALIVE)
            } else {
                // Dead cell
                if (neighbors == 3) {
                    // Becomes alive due to reproduction
                    newGrid[i][j] = ALIVE;
                }
                // Otherwise, cell stays dead (already DEAD)
            }
        }
    }

    return newGrid;
}

int main() {
    auto grid = createGrid(ROWS, COLS);
    setInitialPattern(grid);

    int generation = 0;

    while (true) {
        std::cout << "\033[2J\033[1;1H";
        displayGrid(grid);

        std::cout << "Grid (" << ROWS << "x" << COLS << "). " <<
                     "Generation " << generation << ". " <<
                     "Press Ctrl+C to exit.\n";


        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

        grid = nextGeneration(grid);
        generation++;
    }

    std::cout << "Game ended." << '\n';
    return 0;
}