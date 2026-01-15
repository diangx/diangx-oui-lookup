#include "update/updater.h"
#include "util/fs.h"
#include <cstdlib>
#include <sstream>
#include <vector>

namespace update {

struct Attempt {
  std::string name;
  std::string cmd;
};

static void add_attempt(std::vector<Attempt>& attempts,
                        Downloader d,
                        const std::string& url,
                        const std::string& tmpPath) {
  if (d == Downloader::Curl) {
    std::ostringstream cmd;
    cmd << "curl -L -f -sS "
        << "-o " << util::fs::shell_escape(tmpPath) << " "
        << util::fs::shell_escape(url);
    attempts.push_back({"curl", cmd.str()});
    return;
  }

  if (d == Downloader::Wget) {
    std::ostringstream cmd;
    cmd << "wget -q "
        << "-O " << util::fs::shell_escape(tmpPath) << " "
        << util::fs::shell_escape(url);
    attempts.push_back({"wget", cmd.str()});
    return;
  }

  if (d == Downloader::Python3) {
    std::ostringstream cmd;
    cmd << "python3 -c "
        << util::fs::shell_escape(
             "import sys, urllib.request; "
             "urllib.request.urlretrieve(sys.argv[1], sys.argv[2])"
           )
        << " " << util::fs::shell_escape(url)
        << " " << util::fs::shell_escape(tmpPath);
    attempts.push_back({"python3", cmd.str()});
    return;
  }
}

static bool run_download_attempts(const std::vector<Attempt>& attempts,
                                  const std::string& tmpPath,
                                  const std::string& outPath,
                                  size_t& outBytes,
                                  std::string& outUsedTool) {
  for (const auto& a : attempts) {
    util::fs::remove_file(tmpPath);

    int rc = std::system(a.cmd.c_str());
    if (rc != 0) continue;

    size_t sz = util::fs::file_size(tmpPath);
    if (sz == 0) {
      util::fs::remove_file(tmpPath);
      continue;
    }

    if (!util::fs::atomic_replace(tmpPath, outPath)) {
      util::fs::remove_file(tmpPath);
      return false;
    }

    outBytes = sz;
    outUsedTool = a.name;
    return true;
  }

  util::fs::remove_file(tmpPath);
  return false;
}

UpdateResult download_manuf(const std::string& url,
                           const std::string& outPath,
                           const DownloadOptions& opt) {
  std::string tmpPath = outPath + ".tmp";

  std::vector<Attempt> attempts;
  attempts.reserve(opt.order.size());

  for (auto d : opt.order) {
    add_attempt(attempts, d, url, tmpPath);
  }

  size_t bytes = 0;
  std::string used;

  bool ok = run_download_attempts(attempts, tmpPath, outPath, bytes, used);
  if (!ok) {
    return {false, "all download methods failed", 0};
  }

  return {true, "ok (" + used + ")", bytes};
}

} // namespace update

