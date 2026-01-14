#include "oui/manuf_db.h"
#include "oui/mac.h"
#include "util/str.h"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace oui {

static bool parse_line(const std::string& raw, Entry& out) {
  std::string line = util::str::trim(raw);
  if (line.empty() || line[0] == '#') return false;

  std::string comment;
  auto posHash = line.find('#');
  if (posHash != std::string::npos) {
    comment = util::str::trim(line.substr(posHash + 1));
    line = util::str::trim(line.substr(0, posHash));
  }
  if (line.empty()) return false;

  std::istringstream iss(line);
  std::string prefixToken;
  if (!(iss >> prefixToken)) return false;

  std::string vendor;
  std::getline(iss, vendor);
  vendor = util::str::trim(vendor);
  if (vendor.empty()) return false;

  int maskBits = -1;
  auto slash = prefixToken.find('/');
  if (slash != std::string::npos) {
    try {
      maskBits = std::stoi(prefixToken.substr(slash + 1));
    } catch (...) {
      return false;
    }
    prefixToken = prefixToken.substr(0, slash);
  }

  auto mp = parse_mac_or_prefix(prefixToken);
  if (!mp) return false;

  if (maskBits < 0) maskBits = mp->bitsHint;
  if (maskBits < 0 || maskBits > 48) return false;

  out.prefix = mp->mac48 & mask48(maskBits);
  out.maskBits = maskBits;
  out.vendor = vendor;
  out.comment = comment;
  return true;
}

LoadResult ManufDB::load(const std::string& path) {
  index_.clear();
  masks_desc_.clear();

  std::ifstream in(path);
  if (!in) {
    return {false, "Cannot open file: " + path, 0};
  }

  size_t count = 0;
  std::string line;
  while (std::getline(in, line)) {
    Entry e;
    if (!parse_line(line, e)) continue;
    index_[e.maskBits][e.prefix] = e; // 마지막이 덮어쓰지만 보통 문제 없음
    count++;
  }

  masks_desc_.reserve(index_.size());
  for (auto& kv : index_) masks_desc_.push_back(kv.first);
  std::sort(masks_desc_.begin(), masks_desc_.end(), std::greater<int>());

  return {true, "ok", count};
}

LookupResult ManufDB::lookup(const std::string& macOrPrefix) const {
  auto mp = parse_mac_or_prefix(macOrPrefix);
  if (!mp) return {false, {}, ""};

  const uint64_t mac = mp->mac48;

  // 최장 매칭: mask 큰 것부터 확인
  for (int bits : masks_desc_) {
    uint64_t key = mac & mask48(bits);
    auto itMask = index_.find(bits);
    if (itMask == index_.end()) continue;

    auto it = itMask->second.find(key);
    if (it != itMask->second.end()) {
      LookupResult r;
      r.found = true;
      r.entry = it->second;
      r.best_prefix = prefix_to_string(it->second.prefix, it->second.maskBits);
      return r;
    }
  }

  return {false, {}, ""};
}

} // namespace oui

