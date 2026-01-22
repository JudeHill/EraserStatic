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