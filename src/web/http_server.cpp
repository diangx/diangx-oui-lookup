#include "web/http_server.h"
#include "oui/manuf_db.h"
#include "util/str.h"
#include "util/json.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

namespace web {

static const char* kIndexHtml = R"HTML(<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <title>OUI Lookup</title>
  <style>
    body { font-family: -apple-system, system-ui, sans-serif; margin: 24px; }
    input { width: 360px; padding: 10px; font-size: 16px; }
    button { padding: 10px 14px; font-size: 16px; }
    .card { margin-top: 16px; padding: 14px; border: 1px solid #ddd; border-radius: 10px; width: 520px;}
    .muted { color: #777; font-size: 13px; }
    pre { background: #f6f6f6; padding: 10px; border-radius: 8px; }
  </style>
</head>
<body>
  <h2>OUI Lookup</h2>
  <div class="muted">Enter MAC address or prefix (e.g. 00:11:22:33:44:55 or 00:11:22)</div>
  <div style="margin-top: 10px;">
    <input id="mac" placeholder="00:11:22:33:44:55" />
    <button onclick="go()">Lookup</button>
  </div>

  <div class="card" id="out" style="display:none;">
    <div id="vendor" style="font-size:18px; font-weight:600;"></div>
    <div class="muted" id="prefix"></div>
    <div class="muted" id="comment"></div>
    <pre id="json"></pre>
  </div>

<script>
async function go() {
  const v = document.getElementById('mac').value.trim();
  if (!v) return;
  const r = await fetch('/api/lookup?mac=' + encodeURIComponent(v));
  const j = await r.json();
  const out = document.getElementById('out');
  out.style.display = 'block';
  document.getElementById('json').textContent = JSON.stringify(j, null, 2);

  if (!j.found) {
    document.getElementById('vendor').textContent = 'No match';
    document.getElementById('prefix').textContent = '';
    document.getElementById('comment').textContent = '';
    return;
  }
  document.getElementById('vendor').textContent = j.vendor;
  document.getElementById('prefix').textContent = 'Best match: ' + j.prefix + '/' + j.mask_bits;
  document.getElementById('comment').textContent = j.comment ? ('Comment: ' + j.comment) : '';
}
</script>
</body>
</html>)HTML";

static std::string http_response(int status, const std::string& contentType, const std::string& body) {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << status << (status==200 ? " OK" : status==404 ? " Not Found" : " Error") << "\r\n";
  oss << "Content-Type: " << contentType << "\r\n";
  oss << "Content-Length: " << body.size() << "\r\n";
  oss << "Connection: close\r\n";
  oss << "\r\n";
  oss << body;
  return oss.str();
}

static std::string get_query_param(const std::string& url, const std::string& key) {
  auto qpos = url.find('?');
  if (qpos == std::string::npos) return "";
  std::string q = url.substr(qpos + 1);

  // super simple parse: key=value&...
  std::istringstream iss(q);
  std::string part;
  while (std::getline(iss, part, '&')) {
    auto eq = part.find('=');
    if (eq == std::string::npos) continue;
    auto k = part.substr(0, eq);
    auto v = part.substr(eq + 1);
    if (k == key) return util::str::url_decode(v);
  }
  return "";
}

HttpServer::HttpServer(std::string host, int port, std::string dbPath, oui::ManufDB* db)
  : host_(std::move(host)), port_(port), dbPath_(std::move(dbPath)), db_(db) {}

std::string HttpServer::handle_request(const std::string& req, int& status, std::string& contentType) {
  // parse first line: METHOD URL HTTP/1.1
  std::istringstream iss(req);
  std::string method, url, ver;
  iss >> method >> url >> ver;

  if (method != "GET") {
    status = 405;
    contentType = "text/plain";
    return "Method Not Allowed";
  }

  if (url == "/" || url.rfind("/index.html", 0) == 0) {
    status = 200;
    contentType = "text/html; charset=utf-8";
    return kIndexHtml;
  }

  if (url.rfind("/api/lookup", 0) == 0) {
    std::string mac = get_query_param(url, "mac");
    if (mac.empty()) {
      status = 400;
      contentType = "application/json";
      return R"({"found":false,"error":"missing mac param"})";
    }

    auto r = db_->lookup(mac);
    util::json::Object obj;
    obj["found"] = util::json::Value(r.found);
    if (r.found) {
      obj["vendor"] = util::json::Value(r.entry.vendor);
      obj["prefix"] = util::json::Value(r.best_prefix);
      obj["mask_bits"] = util::json::Value(r.entry.maskBits);
      obj["comment"] = util::json::Value(r.entry.comment);
      obj["db"] = util::json::Value(dbPath_);
    }
    status = 200;
    contentType = "application/json";
    return util::json::stringify(obj);
  }

  status = 404;
  contentType = "text/plain";
  return "Not Found";
}

int HttpServer::serve_forever() {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    std::cerr << "socket() failed\n";
    return 1;
  }

  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(port_));
  addr.sin_addr.s_addr = inet_addr(host_.c_str());

  if (bind(fd, (sockaddr*)&addr, sizeof(addr)) != 0) {
    std::cerr << "bind() failed (host/port available?)\n";
    ::close(fd);
    return 1;
  }

  if (listen(fd, 64) != 0) {
    std::cerr << "listen() failed\n";
    ::close(fd);
    return 1;
  }

  while (true) {
    sockaddr_in caddr{};
    socklen_t clen = sizeof(caddr);
    int cfd = accept(fd, (sockaddr*)&caddr, &clen);
    if (cfd < 0) continue;

    // read request (simple)
    std::string req;
    req.resize(8192);
    ssize_t n = read(cfd, req.data(), req.size());
    if (n <= 0) {
      ::close(cfd);
      continue;
    }
    req.resize((size_t)n);

    int status = 200;
    std::string ct = "text/plain";
    std::string body = handle_request(req, status, ct);

    std::string resp = http_response(status, ct, body);
    (void)write(cfd, resp.data(), resp.size());
    ::close(cfd);
  }
  // unreachable
  ::close(fd);
  return 0;
}

} // namespace web

