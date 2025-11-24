#pragma once
#include <string>
#include <set>

class FileIncludes {
public:
  explicit FileIncludes();
  virtual ~FileIncludes() = default;
  void clearIncludes(std::string fileName);
  void addInclude(std::string fileName, std::string includedFile);
  std::set<std::string> getChildren(std::string fileName);
};