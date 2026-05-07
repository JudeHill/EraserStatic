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
#include "endwhile_node.h"
#include "graph_node.h"
#include <vector>

class WhileNode : public GraphNode {
public:
  GraphNode *whileNode = nullptr;
  EndwhileNode *endWhile = nullptr;
  bool isDoWhile = false;
  explicit WhileNode();
  virtual ~WhileNode();
  GraphNode *getNextNode();
  GraphNode *getDefaultNextNode();
  void add(GraphNode *node);
  void add(EndwhileNode *node);
  std::string getPrintableName();

  std::vector<GraphNode *> getNextNodes();
};