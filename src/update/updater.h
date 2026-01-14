#pragma once
#include <string>
#include <cstddef>

namespace update {

struct UpdateResult {
  bool ok = false;
  std::string message;
  size_t bytes = 0;
};

UpdateResult download_manuf(const std::string& url, const std::string& outPath);

} // namespace update

