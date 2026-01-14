#include "util/fs.h"
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>

namespace util::fs {

static std::string parent_dir(const std::string& path) {
  auto pos = path.find_last_of('/');
  if (pos == std::string::npos) return "";
  return path.substr(0, pos);
}

static bool mkdir_p(const std::string& dir) {
  if (dir.empty()) return true;
  // naive mkdir -p
  std::string cur;
  for (size_t i = 0; i < dir.size(); i++) {
    cur.push_back(dir[i]);
    if (dir[i] == '/' || i == dir.size() - 1) {
      if (cur.size() == 1 && cur[0] == '/') continue;
      ::mkdir(cur.c_str(), 0755);
    }
  }
  return true;
}

bool ensure_parent_dir(const std::string& path) {
  return mkdir_p(parent_dir(path));
}

bool atomic_replace(const std::string& from, const std::string& to) {
  // rename is atomic on same filesystem
  if (::rename(from.c_str(), to.c_str()) == 0) return true;
  return false;
}

bool remove_file(const std::string& path) {
  ::unlink(path.c_str());
  return true;
}

size_t file_size(const std::string& path) {
  struct stat st{};
  if (::stat(path.c_str(), &st) != 0) return 0;
  return (size_t)st.st_size;
}

std::string shell_escape(const std::string& s) {
  // wrap with single quotes and escape existing single quotes
  std::string out = "'";
  for (char c : s) {
    if (c == '\'') out += "'\\''";
    else out.push_back(c);
  }
  out += "'";
  return out;
}

} // namespace util::fs

