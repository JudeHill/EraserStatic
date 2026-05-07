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

#pragma once
#include "node_types.h"
#include <string>
#include <vector>

class GraphNode {
public:
  NodeType type;
  int visits = 0;
  bool eraserIgnoreOn = true;

  explicit GraphNode(NodeType type);
  virtual ~GraphNode() = default;

  virtual void add(GraphNode *node) = 0;
  virtual GraphNode *getNextNode() = 0;
  virtual GraphNode* getDefaultNextNode() = 0;

  virtual std::string getPrintableName() = 0;

  std::string getPrintableNameWithId();

  int id = 0;
  virtual std::vector<GraphNode *> getNextNodes() = 0;
  std::string getNodeType();
};

struct CompareGraphNode {
  bool operator()(const GraphNode *a, const GraphNode *b) {
    return a->id > b->id;
  }
};