#pragma once
#include "basic_node.h"
#include <iostream>

class BarrierNode : public BasicNode {
public:
  explicit BarrierNode(std::string varName,
                            bool global);
  virtual ~BarrierNode();
  std::string getPrintableName();
  std::string varName;
  bool global;
};