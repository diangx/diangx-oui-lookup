#include "oui/mac.h"
#include <cctype>
#include <sstream>

namespace oui {

std::optional<MacParse> parse_mac_or_prefix(const std::string& input) {
  std::string hex;
  hex.reserve(input.size());
  for (char ch : input) {
    if (std::isxdigit(static_cast<unsigned char>(ch))) {
      hex.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
  }

  if (hex.size() % 2 != 0) return std::nullopt;
  if (hex.size() == 0) return std::nullopt;
  if (hex.size() > 12) return std::nullopt;

  int bitsHint = static_cast<int>(hex.size() / 2) * 8;

  auto hexval = [](char c)->int {
    if (c >= '0' && c <= '9') return c - '0';
    return 10 + (c - 'a');
  };

  uint64_t v = 0;
  for (size_t i = 0; i < hex.size(); i += 2) {
    int byte = (hexval(hex[i]) << 4) | hexval(hex[i + 1]);
    v = (v << 8) | static_cast<uint8_t>(byte);
  }

  // left-align to 48-bit
  int shift = (12 - static_cast<int>(hex.size())) * 4;
  v <<= shift;

  return MacParse{v, bitsHint};
}

uint64_t mask48(int bits) {
  if (bits <= 0) return 0;
  if (bits >= 48) return 0xFFFFFFFFFFFFULL;
  return (0xFFFFFFFFFFFFULL << (48 - bits)) & 0xFFFFFFFFFFFFULL;
}

std::string prefix_to_string(uint64_t prefix48, int maskBits) {
  uint64_t p = prefix48 & mask48(maskBits);

  int bytes = (maskBits + 7) / 8;
  if (bytes < 1) bytes = 1;
  if (bytes > 6) bytes = 6;

  std::ostringstream oss;
  for (int i = 0; i < bytes; i++) {
    uint8_t b = (p >> (8 * (5 - i))) & 0xFF;
    if (i) oss << ":";
    oss << std::hex << std::uppercase;
    oss.width(2);
    oss.fill('0');
    oss << (int)b;
  }
  return oss.str();
}

} // namespace oui

