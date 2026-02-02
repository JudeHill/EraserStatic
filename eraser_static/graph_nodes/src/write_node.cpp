#include "write_node.h"
#include "node_types.h"

WriteNode::WriteNode(std::string varName, CXSourceLocation loc)
    : varName(varName), BasicNode::BasicNode(NodeType::WRITE) {
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
WriteNode::~WriteNode() = default;

std::string WriteNode::getPrintableName() { return "Write " + varName; }