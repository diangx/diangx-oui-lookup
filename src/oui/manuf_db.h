#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace oui {

struct Entry {
  uint64_t prefix = 0; // masked prefix (48-bit aligned)
  int maskBits = 0;    // 0..48
  std::string vendor;
  std::string comment;
};

struct LoadResult {
  bool ok = false;
  std::string message;
  size_t entries = 0;
};

struct LookupResult {
  bool found = false;
  Entry entry;
  std::string best_prefix; // human-readable prefix string
};

class ManufDB {
public:
  LoadResult load(const std::string& path);
  LookupResult lookup(const std::string& macOrPrefix) const;

private:
  // maskBits -> (prefix -> entry)
  std::unordered_map<int, std::unordered_map<uint64_t, Entry>> index_;
  std::vector<int> masks_desc_; // existing masks, sorted desc
};

} // namespace oui

