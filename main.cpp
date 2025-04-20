#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>

constexpr char ALIVE    {'#'};
constexpr char DEAD     {'.'};
constexpr int  ROWS      {40};
constexpr int  COLS      {40};
constexpr int  DELAY_MS {100};

struct Pattern {
    std::string name;
    std::vector<std::pair<int, int>> cells; // relative to center
};

static const std::vector<Pattern> PATTERNS = {
    {"Random", {}},
    {"Glider", {{0,1}, {1,2}, {2,0}, {2,1}, {2,2}}},
    {"Blinker", {{0,0}, {1,0}, {2,0}}},
    {"Toad", {{0,1}, {0,2}, {0,3}, {1,0}, {1,1}, {1,2}}},
    {"Beacon", {{0,0}, {0,1}, {1,0}, {2,3}, {3,2}, {3,3}}},
    {"Glider Gun", {
        {0,4}, {0,5}, {1,4}, {1,5}, {10,4}, {10,5}, {10,6}, {11,3}, {11,7},
        {12,2}, {12,8}, {13,2}, {13,8}, {14,5}, {15,3}, {15,7}, {16,4},
        {16,5}, {16,6}, {17,5}, {20,2}, {20,3}, {20,4}, {21,2}, {21,3},
        {21,4}, {22,1}, {22,5}, {24,0}, {24,1}, {24,5}, {24,6}
    }}
};

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

void setInitialPattern(std::vector<std::vector<char>>& grid, const Pattern& pattern) {
    const auto rows = static_cast<int>(grid.size());
    const auto cols = static_cast<int>(grid[0].size());
    const auto centerRow = rows / 2;
    const auto centerCol = cols / 2;

    for (const auto& [rowOffset, colOffset] : pattern.cells) {
        int r = (centerRow + rowOffset + rows) % rows;
        int c = (centerCol + colOffset + cols) % cols;

        grid[r][c] = ALIVE;
    }
}

void setRandomPattern(auto& grid, float aliveProbability = 0.2f) {
    srand(time(nullptr));

    for (auto& row : grid) {
        for (auto& cell : row) {
            cell = (static_cast<float>(rand()) / RAND_MAX < aliveProbability) ? ALIVE : DEAD;
        }
    }
}

Pattern selectPattern() {
    while (true) {
        std::cout << "Select an initial pattern. Available patterns:\n";
        std::cout << "0. " << PATTERNS[0].name << "\n";

        for (size_t i = 1; i < PATTERNS.size(); ++i) {
            std::cout << i << ". " << PATTERNS[i].name << "\n";
        }

        std::cout << "Enter your choice (0-" << PATTERNS.size()-1 << "): ";
        int choice;
        std::cin >> choice;

        if (std::cin.fail() || choice < 0 || choice >= static_cast<int>(PATTERNS.size())) {
            std::cout << "\033[2J\033[1;1H";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            std::cout << "Invalid input. Please try again.\n";
        } else {
            return PATTERNS[choice];
        }
    }
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
    Pattern selectedPattern = selectPattern();

    if (selectedPattern.name == "Random") {
        setRandomPattern(grid);
    } else {
        setInitialPattern(grid, selectedPattern);
    }

    std::cout << "\033[2J\033[1;1H";
    displayGrid(grid);
    std::cout << "\nInitial pattern: " << selectedPattern.name
              << ". Press Enter to start simulation...";

    std::cin.ignore(1000, '\n');
    std::cin.get();

    int generation = 0;
    while (true) {
        std::cout << "\033[2J\033[1;1H";
        displayGrid(grid);

        std::cout << "\nPattern: " << selectedPattern.name << " | "
                  << "Generation: " << generation << " | "
                  << "Press Ctrl+C to exit\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

        grid = nextGeneration(grid);
        generation++;
    }

    std::cout << "Game ended." << '\n';
    return 0;
}