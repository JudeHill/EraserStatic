/* 
 * This file was originally part of Eraser-CD
 * (https://github.com/ProgrammerByte/Eraser-CD)
 *
 * Copyright (C) 2025 Thomas Pompay
 * Copyright (C) 2026 Jude Hill <jude-stephen-hill@outlook.com>
 *
 * This file was modified by Jude Hill in 2026 for use in EraserStatic.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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