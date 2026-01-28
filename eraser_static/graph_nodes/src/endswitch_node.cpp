#include "endswitch_node.h"
#include "node_types.h"

EndswitchNode::EndswitchNode() : BasicNode::BasicNode(NodeType::ENDSWITCH) {}
EndswitchNode::~EndswitchNode() = default;

std::string EndswitchNode::getPrintableName() { return "end switch"; }