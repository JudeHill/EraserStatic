#include <string>

struct SourceLoc{
    std::string file;
    unsigned line = 0;
    unsigned column = 0;
    bool in_macro = false;
    bool is_system = false;

    std::string expansion_file;
    unsigned expansion_line = 0;
    unsigned expansion_column = 0;

    std::string key() const {
        return file + ":" + std::to_string(line) + std::to_string(column);
    }
};