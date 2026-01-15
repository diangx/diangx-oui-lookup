#pragma once
#include <string>
#include <cstddef>
#include <vector>

namespace update {

struct UpdateResult {
  bool ok = false;
  std::string message;
  size_t bytes = 0;
};

enum class Downloader {
  Curl,
  Wget,
  Python3,
};

struct DownloadOptions {
  std::vector<Downloader> order = {
    Downloader::Curl,
    Downloader::Wget,
    Downloader::Python3
  };
};

UpdateResult download_manuf(const std::string& url,
                           const std::string& outPath,
                           const DownloadOptions& opt);

} // namespace update
