#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <sys/ioctl.h>      // for terminal size
#include <unistd.h>         // for terminal size
#include <unordered_map>    // for generation history


struct Pattern {
    std::string name;
    std::vector<std::pair<int, int>> cells; // relative to center
};

class GameOfLife {
    static constexpr char ALIVE             {'#'};
    static constexpr char DEAD              {'.'};
    static constexpr int  DELAY_MS          {50};
    static constexpr int  MAX_GENERATIONS {10000};

    std::vector<std::vector<char>> grid;
    int rows                   {};
    int cols                   {};
    int generation             {};
    int currentAliveCells      {};
    int totalBirths            {};
    int totalDeaths            {};
    int loopLength             {};
    float aliveProbability {0.2f};
    Pattern pattern;
    std::unordered_map<std::string, int> generationHistory;

    static const std::vector<Pattern> PATTERNS;

    static std::pair<int, int> getTerminalSize() {
        struct winsize size{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1) {
            return {24, 80};
        }
        return {size.ws_row - 5, size.ws_col / 2};
    }

    int countAliveNeighbors(const int row, const int col) const {
        int count = 0;

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

    std::string serializeGrid() const {
        std::string result;
        for (const auto& row : grid) {
            for (char cell : row) {
                result.push_back(cell);
            }
        }
        return result;
    }

    void displayState() const {
        std::string message;

        if (loopLength > 0) {
            message = "LOOP DETECTED (IN GENERATION: " +
                std::to_string(generationHistory.size()) + ")";
        } else if (loopLength == -1) {
            message = "ALL CELLS HAVE DIED";
        } else {
            return;
        }

        int messageLength = message.length();
        int centerRow = rows / 2;
        int startCol = (cols - messageLength / 2);

        std::cout << "\033[s";  // save cursor position
        std::cout << "\033[" << centerRow << ";" << startCol << "H";
        std::cout << "\033[7m " << message << " \033[0m";
        std::cout << "\033[u";  // restore cursor position
        std::cout.flush();
    }

public:
    GameOfLife() {
        auto [termRows, termCols] = getTerminalSize();
        rows = termRows;
        cols = termCols;
        grid = std::vector<std::vector<char>>(rows, std::vector<char>(cols, DEAD));
    }

    static void clearScreen() {
        std::cout << "\033[2J\033[3J\033[1;1H";
    }

    static void moveCursor() {
        std::cout << "\033[1;1H";
    }

    static void hideCursor() {
        std::cout << "\033[?25l";
    }

    static void showCursor() {
        std::cout << "\033[?25h";
    }

    void selectPattern() {
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
                clearScreen();
                std::cin.clear();
                std::cin.ignore(1000, '\n');
                std::cout << "Invalid input. Please try again.\n";
            } else {
                pattern = PATTERNS[choice];
                break;
            }
        }
    }

    void setPattern() {
        if (pattern.name != "Random") {
            const auto centerRow = rows / 2;
            const auto centerCol = cols / 2;

            for (const auto& [rowOffset, colOffset] : pattern.cells) {
                int r = (centerRow + rowOffset + rows) % rows;
                int c = (centerCol + colOffset + cols) % cols;
                grid[r][c] = ALIVE;
                currentAliveCells++;
            }
        } else {
            for (auto& row : grid) {
                for (auto& cell : row) {
                    cell = (static_cast<float>(rand()) / RAND_MAX < aliveProbability) ? ALIVE : DEAD;
                    if (cell == ALIVE) {
                        currentAliveCells++;
                    }
                }
            }
        }
    }

    void displayGrid() const {
        moveCursor();

        for (const auto& row : grid) {
            for (const char cell : row) {
                std::cout << cell << ' ';
            }
            std::cout << '\n';
        }
        std::cout << "\nPattern: "          << pattern.name
                  << " | Generation: "      << generation
                  << " | Alive cells: "     << currentAliveCells
                  << " | Total births: "    << totalBirths
                  << " | Total deaths: "    << totalDeaths;

        if (loopLength > 0) {
            std::cout << " | State: Loop (period: " << loopLength << ")";
        } else if (loopLength == -1) {
            std::cout << " | State: Extinction";
        } else {
            std::cout << " | State: Evolving";
        }

        std::cout << "\nPress Ctrl+C to exit\n";
        std::cout.flush();

        if (loopLength != 0) {
            displayState();
        }
    }

    void computeNextGeneration() {
        std::vector<std::vector<char>> newGrid = grid;

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                const int neighbors = countAliveNeighbors(i, j);

                if (grid[i][j] == ALIVE) {
                    // Alive cell
                    if (neighbors < 2 || neighbors > 3) {
                        // Dies due to underpopulation or overpopulation
                        newGrid[i][j] = DEAD;
                        totalDeaths++;
                        currentAliveCells--;
                    }
                    // If neighbors is 2 or 3, cell stays alive (already ALIVE)
                } else {
                    // Dead cell
                    if (neighbors == 3) {
                        // Becomes alive due to reproduction
                        newGrid[i][j] = ALIVE;
                        totalBirths++;
                        currentAliveCells++;
                    }
                    // Otherwise, cell stays dead (already DEAD)
                }
            }
        }

        grid = newGrid;
        generation++;
    }

    bool areAllDead() const {
        for (const auto& row : grid) {
            for (char cell : row) {
                if (cell == ALIVE) {
                    return false;  // Found a live cell
                }
            }
        }
        return true;  // No live cells found
    }

    void detectLoop() {
        if (areAllDead()) {
            loopLength = -1;
            return;
        }

        std::string currentState = serializeGrid();

        if (generationHistory.contains(currentState)) {
            loopLength = generation - generationHistory[currentState];
        }

        generationHistory[currentState] = generation;
    }

    void run() {
        selectPattern();
        setPattern();
        hideCursor();
        clearScreen();
        displayGrid();

        std::cout << "Press Enter to start simulation...";
        std::cout.flush();
        std::cin.ignore(1000, '\n');
        std::cin.get();
        clearScreen();

        for (int gen = 0; gen < MAX_GENERATIONS; gen++) {
            displayGrid();
            std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
            computeNextGeneration();
            detectLoop();
        }

        showCursor();
        std::cout << "\nGame ended.\n";
    }
};

const std::vector<Pattern> GameOfLife::PATTERNS = {
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


int main() {
    srand(time(nullptr));

    GameOfLife game;
    game.run();

    return 0;
}