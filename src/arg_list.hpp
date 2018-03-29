#ifndef BUILDCACHE_ARG_LIST_HPP_
#define BUILDCACHE_ARG_LIST_HPP_

#include <string>
#include <vector>

namespace bcache {
using arg_list_t = std::vector<std::string>;

arg_list_t make_arg_list(const int argc, const char** argv) {
  arg_list_t result;
  for (int i = 0; i < argc; ++i) {
    result.emplace_back(std::string(argv[i]));
  }
  return result;
}
}  // namespace bcache

#endif  // BUILDCACHE_ARG_LIST_HPP_
