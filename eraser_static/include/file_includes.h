#pragma once
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "usings.h"

class FileIncludes {
public:
  explicit FileIncludes();
  virtual ~FileIncludes() = default;
  void clearIncludes(std::string fileName);
  void addInclude(std::string fileName, std::string includedFile);
  std::unordered_set<std::string> getChildren(std::string fileName);
private:
  std::unordered_map<Filename, std::unordered_set<Filename>> includesMap;
};