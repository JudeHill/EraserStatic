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


#include "if_node.h"
#include "node_types.h"

IfNode::IfNode() : GraphNode::GraphNode(NodeType::IF) {}
IfNode::~IfNode() = default;

GraphNode *IfNode::getNextNode() {
  if (visits == 0) {
    return ifNode;
  } else if (visits == 1) {
    return elseNode;
  }
  return nullptr;
}

GraphNode *IfNode::getDefaultNextNode() {
  return ifNode;
}

void IfNode::add(GraphNode *node) {
  if (hasElse) {
    elseNode = node;
  } else {
    ifNode = node;
  }
}

std::string IfNode::getPrintableName() { return "if"; }

std::vector<GraphNode *> IfNode::getNextNodes() {
  std::vector<GraphNode *> result = {};
  if (ifNode != nullptr) {
    result.push_back(ifNode);
  }
  if (elseNode != nullptr) {
    result.push_back(elseNode);
  }
  return result;
}