#ifndef PATTERNS_H
#define PATTERNS_H

#include <string>
#include <vector>

struct Pattern {
    std::string name;                           // name of the pattern
    std::vector<std::pair<int, int>> cells;     // cell coordinates relative to center
};

extern const std::vector<Pattern> PATTERNS;     // collection of predefined patterns

#endif
