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

#include "while_node.h"
#include "node_types.h"

WhileNode::WhileNode() : GraphNode::GraphNode(NodeType::WHILE) {}
WhileNode::~WhileNode() = default;

GraphNode *WhileNode::getNextNode() {
  if (visits == 0) {
    return whileNode;
  } else if (visits == 1) {
    return endWhile;
  } else {
    return nullptr;
  }
}

GraphNode *WhileNode::getDefaultNextNode() { return endWhile; }

void WhileNode::add(GraphNode *node) { whileNode = node; }

void WhileNode::add(EndwhileNode *node) { endWhile = node; }

std::string WhileNode::getPrintableName() { return "while"; }

std::vector<GraphNode *> WhileNode::getNextNodes() {
  std::vector<GraphNode *> result = {};
  if (whileNode != nullptr) {
    result.push_back(whileNode);
  }
  if (endWhile != nullptr) {
    result.push_back(endWhile);
  }
  return result;
}