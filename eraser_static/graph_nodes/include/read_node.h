#pragma once
#include "basic_node.h"
#include "typedef.h"
#include <clang-c/Index.h>

class ReadNode : public BasicNode {
public:
  explicit ReadNode(std::string varName, CXSourceLocation loc);
  virtual ~ReadNode();

  std::string getPrintableName();
  std::string varName;
  LocationInfo loc;
};