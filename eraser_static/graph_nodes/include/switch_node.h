#pragma once
#include "graph_node.h"
#include "endif_node.h"
#include <vector>

class SwitchNode : public GraphNode {
public:
  std::vector<GraphNode*> caseNodes;
  explicit SwitchNode();
  virtual ~SwitchNode();
  GraphNode *getNextNode();
  GraphNode *getDefaultNextNode();
  void add(GraphNode *node);
  std::string getPrintableName();

  std::vector<GraphNode *> getNextNodes();
};