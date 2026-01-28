#pragma once
#include "basic_node.h"

class EndswitchNode : public BasicNode {
public:
  explicit EndswitchNode();
  virtual ~EndswitchNode();

  std::string getPrintableName();
};