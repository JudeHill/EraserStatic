/* 
 * This file is part of EraserStatic
 *
 * Original code from: Eraser-CD
 * (https://github.com/ProgrammerByte/Eraser-CD)
 *
 * Copyright (C) 2025 Thomas Pompay
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
#include "break_node.h"
#include "continue_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "if_node.h"
#include "return_node.h"
#include "start_node.h"
#include "while_node.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class GraphVisualizer {
public:
  explicit GraphVisualizer() = default;
  virtual ~GraphVisualizer() = default;

  void visualizeGraph(StartNode *node);

private:
  std::vector<GraphNode *> nodes;
  std::unordered_map<GraphNode *, std::vector<GraphNode *>> adjacencyMatrix =
      {};
  std::unordered_map<GraphNode *, std::string> nodeNames = {};

  template <typename T> std::string pointerToString(T *ptr);
  void visitNode(GraphNode *node);
};