#include "util/json.h"
#include <sstream>

namespace util::json {

static std::string esc(const std::string& s) {
  std::ostringstream oss;
  for (char c : s) {
    switch (c) {
      case '\\': oss << "\\\\"; break;
      case '"':  oss << "\\\""; break;
      case '\n': oss << "\\n"; break;
      case '\r': oss << "\\r"; break;
      case '\t': oss << "\\t"; break;
      default:
        if ((unsigned char)c < 0x20) oss << "\\u" << "001F"; // cheap fallback
        else oss << c;
    }
  }
  return oss.str();
}

static void dump(std::ostringstream& oss, const Value& val);

static void dump_obj(std::ostringstream& oss, const Object& obj) {
  oss << "{";
  bool first = true;
  for (const auto& kv : obj) {
    if (!first) oss << ",";
    first = false;
    oss << "\"" << esc(kv.first) << "\":";
    dump(oss, kv.second);
  }
  oss << "}";
}

static void dump(std::ostringstream& oss, const Value& val) {
  if (std::holds_alternative<std::nullptr_t>(val.v)) {
    oss << "null";
  } else if (std::holds_alternative<bool>(val.v)) {
    oss << (std::get<bool>(val.v) ? "true" : "false");
  } else if (std::holds_alternative<int>(val.v)) {
    oss << std::get<int>(val.v);
  } else if (std::holds_alternative<std::string>(val.v)) {
    oss << "\"" << esc(std::get<std::string>(val.v)) << "\"";
  } else {
    dump_obj(oss, std::get<Object>(val.v));
  }
}

std::string stringify(const Object& obj) {
  std::ostringstream oss;
  dump_obj(oss, obj);
  return oss.str();
}

} // namespace util::json

