#include "update/updater.h"
#include "util/fs.h"
#include <cstdlib>
#include <sstream>

namespace update {

UpdateResult download_manuf(const std::string& url, const std::string& outPath) {
  // 제품형 느낌: 임시 파일로 받고 atomic replace
  std::string tmpPath = outPath + ".tmp";

  // curl 사용 (macOS 기본 탑재)
  // -L: redirect follow, -f: fail on HTTP errors, -sS: silent but show errors
  std::ostringstream cmd;
  cmd << "curl -L -f -sS "
      << "-o " << util::fs::shell_escape(tmpPath) << " "
      << util::fs::shell_escape(url);

  int rc = std::system(cmd.str().c_str());
  if (rc != 0) {
    util::fs::remove_file(tmpPath);
    return {false, "curl failed (is curl installed? url reachable?)", 0};
  }

  size_t sz = util::fs::file_size(tmpPath);
  if (sz == 0) {
    util::fs::remove_file(tmpPath);
    return {false, "downloaded file is empty", 0};
  }

  if (!util::fs::atomic_replace(tmpPath, outPath)) {
    util::fs::remove_file(tmpPath);
    return {false, "failed to replace db file", 0};
  }

  return {true, "ok", sz};
}

} // namespace update

