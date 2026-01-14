#pragma once
#include <string>

namespace oui { class ManufDB; }

namespace web {

class HttpServer {
public:
  HttpServer(std::string host, int port, std::string dbPath, oui::ManufDB* db);

  int serve_forever();

private:
  std::string host_;
  int port_;
  std::string dbPath_;
  oui::ManufDB* db_;

  std::string handle_request(const std::string& req, int& status, std::string& contentType);
};

} // namespace web

