#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace util::json {

struct Object;

struct Value {
  using ObjectPtr = std::shared_ptr<Object>;
  using Var = std::variant<std::nullptr_t, bool, int, std::string, ObjectPtr>;
  Var v;

  Value() : v(nullptr) {}
  Value(std::nullptr_t) : v(nullptr) {}
  Value(bool b) : v(b) {}
  Value(int i) : v(i) {}
  Value(const std::string& s) : v(s) {}
  Value(const char* s) : v(std::string(s)) {}
  Value(const Object& o) : v(std::make_shared<Object>(o)) {}
};

struct Object {
  std::unordered_map<std::string, Value> values;

  Value& operator[](const std::string& key) { return values[key]; }
  const Value& at(const std::string& key) const { return values.at(key); }
};

std::string stringify(const Object& obj);

} // namespace util::json
