#include "read_node.h"
#include "node_types.h"

ReadNode::ReadNode(std::string varName, CXSourceLocation loc)
    : varName(varName), BasicNode::BasicNode(NodeType::READ) {
        CXFile file;
        unsigned line, column, offset;

        clang_getSpellingLocation(loc, &file, &line, &column, &offset);

        CXString cxName = clang_getFileName(file);
        const char *cstr = clang_getCString(cxName);

        std::string filename = cstr ? cstr : "";
        clang_disposeString(cxName);
        this->loc = LocationInfo{
            .file_name = filename,
            .line = line,
            .column = column,
        };
    }
ReadNode::~ReadNode() = default;

std::string ReadNode::getPrintableName() { return "Read " + varName; }