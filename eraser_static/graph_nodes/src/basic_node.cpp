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

#include "basic_node.h"
#include <vector>

BasicNode::BasicNode(NodeType type) : GraphNode::GraphNode(type) {}

BasicNode::~BasicNode() = default;

GraphNode *BasicNode::getNextNode() {
  if (visits == 0) {
    return next;
  }
  return nullptr;
}

GraphNode *BasicNode::getDefaultNextNode(){
  return next;
}

void BasicNode::add(GraphNode *node) { next = node; }

std::vector<GraphNode *> BasicNode::getNextNodes() {
  if (next == nullptr) {
    return {};
  }
  return {next};
}