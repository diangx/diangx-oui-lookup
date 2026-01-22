cmake_minimum_required(VERSION 3.15)
project(diangx_oui_lookup CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OUI_DEFAULT_DB "data/manuf.gz" CACHE STRING "Default OUI database path")

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
find_package(ZLIB REQUIRED)
target_link_libraries(oui PRIVATE ZLIB::ZLIB)
target_compile_definitions(oui PRIVATE OUI_DEFAULT_DB="${OUI_DEFAULT_DB}")

install(TARGETS oui RUNTIME DESTINATION bin)
install(FILES data/manuf.gz DESTINATION share/oui)
