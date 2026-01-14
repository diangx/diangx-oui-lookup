#pragma once
#include <string>
#include <cstddef>

namespace util::fs {
bool ensure_parent_dir(const std::string& path);
bool atomic_replace(const std::string& from, const std::string& to);
bool remove_file(const std::string& path);
size_t file_size(const std::string& path);

// for system() safety-ish
std::string shell_escape(const std::string& s);
}

