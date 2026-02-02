#pragma once
#include "basic_node.h"
#include <clang-c/Index.h>
#include "typedef.h"

class WriteNode : public BasicNode {
public:
  explicit WriteNode(std::string varName, CXSourceLocation loc);
  virtual ~WriteNode();

  std::string getPrintableName();
  std::string varName;
  LocationInfo loc;
};