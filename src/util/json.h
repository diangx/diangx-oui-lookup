#pragma once
#include <string>
#include <unordered_map>
#include <variant>

namespace util::json {

struct Value;
using Object = std::unordered_map<std::string, Value>;

struct Value {
  using Var = std::variant<std::nullptr_t, bool, int, std::string, Object>;
  Var v;

  Value() : v(nullptr) {}
  Value(std::nullptr_t) : v(nullptr) {}
  Value(bool b) : v(b) {}
  Value(int i) : v(i) {}
  Value(const std::string& s) : v(s) {}
  Value(const char* s) : v(std::string(s)) {}
  Value(const Object& o) : v(o) {}
};

std::string stringify(const Object& obj);

} // namespace util::json

