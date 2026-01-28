#include <clang-c/Index.h>
#include <unordered_map>
#include <vector>
#include <optional>


static std::unordered_map<unsigned, unsigned> parentChildCount;
static CXChildVisitResult countChildrenVisitor(CXCursor c, CXCursor parent, CXClientData data);
static unsigned getCachedChildCount(CXCursor parent);