/*
 * This file is part of EraserStatic
 *
 * Original code from: Eraser-CD
 * (https://github.com/ProgrammerByte/Eraser-CD)
 *
 * Copyright (C) 2025 Thomas Popay
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

#include "graph_node.h"
#include "node_types.h"

GraphNode::GraphNode(NodeType type) : type(type) {}

std::string GraphNode::getPrintableNameWithId() {
  return std::to_string(id) + " " + getPrintableName();
}

std::string GraphNode::getNodeType() {
  switch (type) {
  case NodeType::START:
    return "START";
  case NodeType::LOCK:
    return "LOCK";
  case NodeType::UNLOCK:
    return "UNLOCK";
  case NodeType::READ:
    return "READ";
  case NodeType::WRITE:
    return "WRITE";
  case NodeType::FUNCTION_CALL:
    return "FUNCTION_CALL";
  case NodeType::WHILE:
    return "WHILE";
  case NodeType::ENDWHILE:
    return "ENDWHILE";
  case NodeType::BREAK:
    return "BREAK";
  case NodeType::CONTINUE:
    return "CONTINUE";
  case NodeType::IF:
    return "IF";
  case NodeType::ENDIF:
    return "ENDIF";
  case NodeType::RETURN:
    return "RETURN";
  default:
    break;
    return "UNKNOWN";
  }
}