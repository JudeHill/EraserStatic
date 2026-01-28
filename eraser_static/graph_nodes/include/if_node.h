#pragma once
#include "graph_node.h"
#include "endif_node.h"
#include <vector>

class IfNode : public GraphNode {
public:
  GraphNode *ifNode = nullptr;
  GraphNode *elseNode = nullptr;
  EndifNode *endIf = nullptr;
  bool hasElse = false;
  explicit IfNode();
  virtual ~IfNode();
  GraphNode *getNextNode();
  GraphNode *getDefaultNextNode();
  void add(GraphNode *node);
  std::string getPrintableName();

  std::vector<GraphNode *> getNextNodes();
};