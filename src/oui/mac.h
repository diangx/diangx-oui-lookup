#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace oui {

struct MacParse {
  uint64_t mac48 = 0;   // 48-bit aligned value
  int bitsHint = 0;     // inferred bits from input length (e.g., 24 for OUI)
};

std::optional<MacParse> parse_mac_or_prefix(const std::string& input);
uint64_t mask48(int bits);
std::string prefix_to_string(uint64_t prefix48, int maskBits);

} // namespace oui

