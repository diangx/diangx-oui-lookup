cmake_minimum_required(VERSION 3.16)
project(oui_tool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(oui
  src/main.cpp
  src/cli/cli.cpp
  src/oui/mac.cpp
  src/oui/manuf_db.cpp
  src/update/updater.cpp
  src/web/http_server.cpp
  src/util/fs.cpp
  src/util/str.cpp
  src/util/json.cpp
)

target_include_directories(oui PRIVATE src)

