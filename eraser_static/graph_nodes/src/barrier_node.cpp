#include "barrier_node.h"
#include "node_types.h"

BarrierNode::BarrierNode(std::string varName,
                         bool global)
    :varName(varName),
      global(global), BasicNode::BasicNode(NodeType::BARRIER) {
        std::cout << "Made a barrier node" << std::endl;
      }

BarrierNode::~BarrierNode() = default;

std::string BarrierNode::getPrintableName() {
  return "barrier " + (global ? std::string("global ") : std::string("")) +
         "var: " + varName;
}