#include "graph_node.h"
#include "node_types.h"

GraphNode::GraphNode(NodeType type) : type(type) {}

std::string GraphNode::getPrintableNameWithId() {
  return std::to_string(id) + " " + getPrintableName();
}

std::string GraphNode::getNodeType() {
  switch (type) {
  case NodeType::START:
    return "START";
  case NodeType::LOCK:
    return "LOCK";
  case NodeType::UNLOCK:
    return "UNLOCK";
  case NodeType::READ:
    return "READ";
  case NodeType::WRITE:
    return "WRITE";
  case NodeType::FUNCTION_CALL:
    return "FUNCTION_CALL";
  case NodeType::WHILE:
    return "WHILE";
  case NodeType::ENDWHILE:
    return "ENDWHILE";
  case NodeType::BREAK:
    return "BREAK";
  case NodeType::CONTINUE:
    return "CONTINUE";
  case NodeType::IF:
    return "IF";
  case NodeType::ENDIF:
    return "ENDIF";
  case NodeType::RETURN:
    return "RETURN";
  default:
    break;
  return "UNKNOWN";
}
}