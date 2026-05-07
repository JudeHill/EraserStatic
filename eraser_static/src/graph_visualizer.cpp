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

#include "graph_visualizer.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <format>
static uint16_t graph_number = 0;

template <typename T> std::string GraphVisualizer::pointerToString(T *ptr) {
  if (ptr == nullptr) {
    return "nullptr";
  }

  std::size_t hashValue = std::hash<T *>{}(ptr);

  const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
  const size_t alphabetSize = sizeof(alphabet) - 1;

  std::string result;
  while (hashValue > 0) {
    result += alphabet[hashValue % alphabetSize];
    hashValue /= alphabetSize;
  }

  return result;
}

void GraphVisualizer::visitNode(GraphNode *node) {
  if (node == nullptr) {
    return;
  }
  if (adjacencyMatrix.find(node) == adjacencyMatrix.end()) {
    nodes.push_back(node);
    adjacencyMatrix.insert({node, std::vector<GraphNode *>(0)});
    nodeNames.insert({node, node->getPrintableNameWithId()});
  }
  GraphNode *nextNode = node->getNextNode();
  node->visits += 1;
  if (nextNode != nullptr) {
    adjacencyMatrix[node].push_back(nextNode);
    visitNode(nextNode);
    visitNode(node);
  }
}

void GraphVisualizer::visualizeGraph(StartNode *node) {
  visitNode(node);
  graph_number++;
  std::string fileName = std::format("graph_{}.dot", graph_number);
  std::ofstream file(fileName);
  if (!file.is_open()) {
    std::cerr << "Error: Unable to create DOT file!" << std::endl;
    return;
  }

  // create a pointer to node map.

  file << "digraph G {\n";
  for (GraphNode *node : nodes) {
    file << "  " << pointerToString(node) << " [label=\"" << nodeNames[node]
         << "\"];\n";
  }
  for (GraphNode *start : nodes) {
    for (GraphNode *end : adjacencyMatrix[start]) {
      file << "  " << pointerToString(start) << " -> " << pointerToString(end)
           << ";\n";
    }
  }
  file << "}\n";

  file.close();
  if (system(std::format("dot -Tpng {} -o graph_{}.png", fileName, graph_number).c_str()) == -1) {
    std::cerr << "Error: Unable to create PNG file!" << std::endl;
  }
}