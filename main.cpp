#include <iostream>         // for console input/output
#include <vector>           // for 2D grid representation
#include <string>           // for string handling
#include <cstdlib>          // for random number generation
#include <ctime>            // for random number generation
#include <chrono>           // for delays
#include <thread>           // for delays
#include <sys/ioctl.h>      // for terminal size
#include <unistd.h>         // for terminal size
#include <unordered_map>    // for generation history
#include "patterns.h"       // contains predefined patterns



/*
 * GameOfLife - Main class that implements cellular automaton.
 *
 * The game follows 4 rules:
 * 1. A live cell with fewer than two live neighbors dies due to underpopulation.
 * 2. A live cell with more than three live neighbors dies due to overpopulation.
 * 3. A live cell with two or three live neighbors stays alive.
 * 4. A dead cell with exactly three neighbors comes to life.
 */
class GameOfLife {
    static constexpr std::string ALIVE_CHAR {"■"};      // for displaying ALIVE cells
    static constexpr std::string DEAD_CHAR  {' '};      // for displaying DEAD cells
    static constexpr bool ALIVE            {true};      // state of ALIVE cells
    static constexpr bool DEAD            {false};      // state of DEAD cells
    static constexpr int  DELAY_MS          {100};      // delay in milliseconds between generations
    static constexpr int  MAX_GENERATIONS {10000};      // maximum limit of generations

    std::vector<std::vector<bool>> grid;        // current state of the grid
    std::vector<std::vector<bool>> lastDead;    // cells that died in the last generation
    int rows                   {};              // number of rows in the grid
    int cols                   {};              // number of columns in the grid
    int generation             {};              // current generation count
    int currentAliveCells      {};              // number of currently alive cells
    int totalBirths            {};              // total number of cells that were born
    int totalDeaths            {};              // total number of cells that died
    int loopLength             {};              // length of detected loop (-1=extinction)
    float aliveProbability {0.2f};              // probability of a cell to be alive
    Pattern pattern;                            // selected pattern
    std::unordered_map<std::string, int> generationHistory;

    static std::pair<int, int> getTerminalSize() {
        struct winsize size{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1) {
            return {24, 80}; // default size
        }
        return {size.ws_row - 5, size.ws_col / 2};
    }

    // counts the number of alive neighbors for a given cell
    int countAliveNeighbors(const int row, const int col) const {
        int count = 0;

        // check all 8 possible neighbors
        for (int dRow = -1; dRow <= 1; dRow++) {
            for (int dCol = -1; dCol <= 1; dCol++) {
                if (dRow == 0 && dCol == 0) continue; // skip the cell itself

                // handle edges in toroidal manner
                const int neighborRow = (row + dRow + rows) % rows;
                const int neighborCol = (col + dCol + cols) % cols;

                if (grid[neighborRow][neighborCol] == ALIVE) {
                    count++;
                }
            }
        }

        return count;
    }

    // converts the grid to string for loop detection
    std::string serializeGrid() const {
        std::string result;
        for (const auto& row : grid) {
            for (bool cell : row) {
                result.push_back(cell ? '1' : '0');
            }
        }
        return result;
    }

    // displays the loop or extinction message on the screen
    void displayState() const {
        std::string message;

        if (loopLength > 0) {
            message = "LOOP DETECTED (IN GENERATION: " +
                std::to_string(generationHistory.size()) + ")";
        } else if (loopLength == -1) {
            message = "ALL CELLS HAVE DIED (IN GENERATION: " +
                std::to_string(generationHistory.size()) + ")";
        } else {
            return;
        }

        int messageLength = static_cast<int>(message.length());
        int centerRow = rows / 6;
        int startCol = (cols - messageLength / 2);

        std::cout << "\033[s";  // save cursor position
        std::cout << "\033[" << centerRow << ";" << startCol << "H";
        std::cout << "\033[7m " << message << " \033[0m"; // invert the colors
        std::cout << "\033[u";  // restore cursor position
        std::cout.flush();
    }

public:
    // constructor
    GameOfLife() {
        auto [termRows, termCols] = getTerminalSize();
        rows = termRows;
        cols = termCols;
        grid     = std::vector<std::vector<bool>>(rows, std::vector<bool>(cols, DEAD));
        lastDead = std::vector<std::vector<bool>>(rows, std::vector<bool>(cols, DEAD));
    }

    static void clearScreen() {
        std::cout << "\033[2J\033[3J\033[1;1H"; // clear terminal screen
    }

    static void moveCursor() {
        std::cout << "\033[1;1H";   // move cursor to the top-left corner
    }

    static void hideCursor() {
        std::cout << "\033[?25l";   // hide cursor during simulation to reduce blinking
    }

    static void showCursor() {
        std::cout << "\033[?25h";   // show cursor after simulation
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

            // input validation
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
            // center pattern on the grid
            const auto centerRow = rows / 2;
            const auto centerCol = cols / 2;

            // add each cell from the pattern
            for (const auto& [rowOffset, colOffset] : pattern.cells) {
                int r = (centerRow + rowOffset + rows) % rows;
                int c = (centerCol + colOffset + cols) % cols;
                grid[r][c] = ALIVE;
                currentAliveCells++;
            }
        } else {
            // random initialization
            for (auto& row : grid) {
                for (auto cell : row) {
                    cell = (static_cast<float>(rand()) / RAND_MAX < aliveProbability);
                    if (cell == ALIVE) {
                        currentAliveCells++;
                    }
                }
            }
        }
    }

    // renders the grid and statistics to the console
    void displayGrid() const {
        moveCursor();

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (grid[i][j] == ALIVE) {
                    std::cout << ALIVE_CHAR << ' ';
                } else if (loopLength == -1 && lastDead[i][j]) {
                    std::cout << "\033[31m" << ALIVE_CHAR << "\033[0m "; // mark in red
                } else {
                    std::cout << DEAD_CHAR << ' ';
                }
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
        std::vector<std::vector<bool>> newGrid = grid;
        lastDead = std::vector<std::vector<bool>>(rows, std::vector<bool>(cols, DEAD));

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                const int neighbors = countAliveNeighbors(i, j);

                if (grid[i][j] == ALIVE) {
                    // ALIVE cell
                    if (neighbors < 2 || neighbors > 3) {
                        // dies due to underpopulation or overpopulation
                        newGrid[i][j]  =  DEAD;
                        lastDead[i][j] = ALIVE;
                        totalDeaths++;
                        currentAliveCells--;
                    }
                    // if neighbors is 2 or 3, cell stays alive (already ALIVE)
                } else {
                    // DEAD cell
                    if (neighbors == 3) {
                        // becomes ALIVE due to reproduction
                        newGrid[i][j] = ALIVE;
                        totalBirths++;
                        currentAliveCells++;
                    }
                    // otherwise, cell stays DEAD (already DEAD)
                }
            }
        }

        grid = newGrid;
        generation++;
    }

    // check if all cells in the grid are dead
    bool areAllDead() const {
        for (const auto& row : grid) {
            for (bool cell : row) {
                if (cell == ALIVE) {
                    return false;  // found a live cell
                }
            }
        }
        return true;  // no live cells found
    }

    // checks if all cells are dead or loop is detected
    void detectLoop() {
        if (areAllDead()) {
            loopLength = -1;
            return;
        }

        std::string currentState = serializeGrid();

        // checks if we've seen the current grid state before
        if (generationHistory.contains(currentState)) {
            loopLength = generation - generationHistory[currentState];
        }

        // save the current state
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

        // main simulation loop
        for (int gen = 0; gen < MAX_GENERATIONS; gen++) {
            displayGrid();
            if (loopLength == -1) {
                break; // exit if all cells died
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
            computeNextGeneration();
            detectLoop();
        }

        showCursor();
        std::cout << "\nGame ended.\n";
    }
};




int main() {
    srand(time(nullptr));
    GameOfLife game;
    game.run();
    return 0;
}