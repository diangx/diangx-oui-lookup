#include "oui/manuf_db.h"
#include "oui/mac.h"
#include "util/str.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <sys/stat.h>

#include <zlib.h>

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

static bool has_gzip_magic(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  unsigned char magic[2] = {0, 0};
  in.read(reinterpret_cast<char*>(magic), sizeof(magic));
  return in.gcount() == 2 && magic[0] == 0x1f && magic[1] == 0x8b;
}

static bool file_exists(const std::string& path) {
  struct stat st {};
  return ::stat(path.c_str(), &st) == 0;
}

static bool ends_with(const std::string& value, const std::string& suffix) {
  if (value.size() < suffix.size()) return false;
  return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static bool is_relative_path(const std::string& path) {
  return path.empty() || path[0] != '/';
}

static bool starts_with(const std::string& value, const std::string& prefix) {
  return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

static LoadResult load_from_gzip(const std::string& path,
                                 std::unordered_map<int, std::unordered_map<uint64_t, Entry>>& index,
                                 size_t& outCount) {
  gzFile gz = gzopen(path.c_str(), "rb");
  if (!gz) {
    return {false, "Cannot open gzip file: " + path, 0};
  }

  std::string line;
  std::vector<char> buffer(4096);

  while (true) {
    char* res = gzgets(gz, buffer.data(), static_cast<int>(buffer.size()));
    if (!res) break;

    line.append(res);
    if (!line.empty() && line.back() == '\n') {
      Entry e;
      if (parse_line(line, e)) {
        index[e.maskBits][e.prefix] = e;
        outCount++;
      }
      line.clear();
    }
  }

  if (!line.empty()) {
    Entry e;
    if (parse_line(line, e)) {
      index[e.maskBits][e.prefix] = e;
      outCount++;
    }
  }

  int errnum = Z_OK;
  const char* err = gzerror(gz, &errnum);
  gzclose(gz);
  if (errnum != Z_OK && errnum != Z_STREAM_END) {
    return {false, std::string("gzip read error: ") + (err ? err : "unknown"), 0};
  }

  return {true, "ok", outCount};
}

LoadResult ManufDB::load(const std::string& path) {
  index_.clear();
  masks_desc_.clear();

  std::vector<std::string> candidates;
  candidates.reserve(4);
  auto add_candidates = [&](const std::string& base) {
    if (base.empty()) return;
    candidates.push_back(base);
    if (ends_with(base, ".gz")) {
      candidates.push_back(base.substr(0, base.size() - 3));
    } else {
      candidates.push_back(base + ".gz");
    }
  };

  add_candidates(path);
  if (is_relative_path(path) && starts_with(path, "data/")) {
    add_candidates("../" + path);
  }

  std::string resolved;
  for (const auto& candidate : candidates) {
    if (file_exists(candidate)) {
      resolved = candidate;
      break;
    }
  }
  if (resolved.empty()) {
    return {false, "Cannot open file: " + path, 0};
  }

  size_t count = 0;
  if (has_gzip_magic(resolved)) {
    auto result = load_from_gzip(resolved, index_, count);
    if (!result.ok) return result;
  } else {
    std::ifstream in(resolved);
    if (!in) {
      return {false, "Cannot open file: " + resolved, 0};
    }

    std::string line;
    while (std::getline(in, line)) {
      Entry e;
      if (!parse_line(line, e)) continue;
      index_[e.maskBits][e.prefix] = e; // 마지막이 덮어쓰지만 보통 문제 없음
      count++;
    }
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
