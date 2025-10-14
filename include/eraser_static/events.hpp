// Records shared memory events

#include <string>
#include "sourceloc.hpp"

enum class EventKind {Lock, Unlock, Read, Write, Barrier};
struct Event {
    EventKind kind;
    std::string sym;
    SourceLoc loc;
    std::string extra;
    
};