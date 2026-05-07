/*
 * This file was originally part of Eraser-CD
 * (https://github.com/ProgrammerByte/Eraser-CD)
 *
 * Copyright (C) 2025 Thomas Popay
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
#include "break_node.h"
#include "continue_node.h"
#include "continue_return_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "if_node.h"
#include "return_node.h"
#include "start_node.h"
#include "startwhile_node.h"
#include "while_node.h"
#include <memory>
#include <vector>

class ConstructionEnvironment {
public:
  GraphNode *currNode;
  explicit ConstructionEnvironment() = default;
  virtual ~ConstructionEnvironment() = default;

  StartNode *startNewTree(std::string funcName);
  void goBackToStartWhile();
  void onAdd(GraphNode *node);
  void onAdd(IfNode *node);
  void onElseAdd();
  void onAdd(EndifNode *node);
  void onAdd(StartwhileNode *node);
  void onAdd(WhileNode *node);
  void onAdd(EndwhileNode *node);
  void onAdd(BreakNode *node);
  void onAdd(ContinueNode *node);
  void onAdd(ContinueReturnNode *node);
  void onAdd(ReturnNode *node);

private:
  int currId;
  void callOnAdd(GraphNode *node);
  void setNodeId(GraphNode *node);

  std::vector<IfNode *> ifStack;
  std::vector<std::vector<BasicNode *>> endifListStack;
  std::vector<WhileNode *> whileStack;
  std::vector<StartwhileNode *> startwhileStack;
  std::vector<std::vector<BreakNode *>> breakListStack;
  std::vector<std::vector<ContinueNode *>> continueListStack;
};