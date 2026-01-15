#include "cli/cli.h"

#include "oui/manuf_db.h"
#include "update/updater.h"
#include "web/http_server.h"
#include "util/fs.h"
#include "util/str.h"
#include "util/json.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

void print_usage() {
  std::cout <<
R"(OUI Lookup Tool (manuf based)

Usage:
  oui update [--db <path>] [--url <manuf_url>]
  oui lookup [--db <path>] [--json] <mac-or-prefix>
  oui serve  [--db <path>] [--host <ip>] [--port <n>]

Examples:
  oui update
  oui lookup 00:11:22:33:44:55
  oui lookup --json 001122
  oui serve --port 8080
)";
}

struct Opts {
  std::string cmd;
  std::string db = "data/manuf";
  std::string url = "https://www.wireshark.org/download/automated/data/manuf";
  bool json = false;
  std::string target;
  std::string host = "127.0.0.1";
  int port = 8080;
};

bool take_arg(std::vector<std::string>& args, size_t& i, std::string& out) {
  if (i + 1 >= args.size()) return false;
  out = args[++i];
  return true;
}

bool take_int(std::vector<std::string>& args, size_t& i, int& out) {
  if (i + 1 >= args.size()) return false;
  out = std::stoi(args[++i]);
  return true;
}

Opts parse(int argc, char** argv) {
  Opts o;
  if (argc < 2) {
    return o;
  }
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) args.emplace_back(argv[i]);

  o.cmd = args[0];

  for (size_t i = 1; i < args.size(); i++) {
    const auto& a = args[i];
    if (a == "--db") {
      if (!take_arg(args, i, o.db)) throw std::runtime_error("Missing value for --db");
    } else if (a == "--url") {
      if (!take_arg(args, i, o.url)) throw std::runtime_error("Missing value for --url");
    } else if (a == "--json") {
      o.json = true;
    } else if (a == "--host") {
      if (!take_arg(args, i, o.host)) throw std::runtime_error("Missing value for --host");
    } else if (a == "--port") {
      if (!take_int(args, i, o.port)) throw std::runtime_error("Missing value for --port");
    } else if (!a.empty() && a[0] == '-') {
      throw std::runtime_error("Unknown option: " + a);
    } else {
      // positional
      o.target = a;
    }
  }
  return o;
}

int cmd_update(const Opts& o) {
  util::fs::ensure_parent_dir(o.db);
  update::DownloadOptions opt;
  update::UpdateResult r = update::download_manuf(o.url, o.db, opt);
  if (!r.ok) {
    std::cerr << "Update failed: " << r.message << "\n";
    return 1;
  }
  std::cout << "Updated DB: " << o.db << "\n";
  std::cout << "Bytes: " << r.bytes << "\n";
  return 0;
}

int cmd_lookup(const Opts& o) {
  if (o.target.empty()) {
    std::cerr << "lookup: missing <mac-or-prefix>\n";
    return 2;
  }
  oui::ManufDB db;
  auto lr = db.load(o.db);
  if (!lr.ok) {
    std::cerr << "DB load failed: " << lr.message << "\n";
    return 1;
  }

  auto res = db.lookup(o.target);
  if (!res.found) {
    if (o.json) {
      std::cout << "{\"found\":false}\n";
    } else {
      std::cout << "No match\n";
    }
    return 0;
  }

  if (o.json) {
    util::json::Object obj;
    obj["found"] = util::json::Value(true);
    obj["vendor"] = util::json::Value(res.entry.vendor);
    obj["prefix"] = util::json::Value(res.best_prefix);
    obj["mask_bits"] = util::json::Value(res.entry.maskBits);
    obj["comment"] = util::json::Value(res.entry.comment);
    std::cout << util::json::stringify(obj) << "\n";
  } else {
    std::cout << "Vendor: " << res.entry.vendor << "\n";
    std::cout << "Prefix: " << res.best_prefix << "/" << res.entry.maskBits << "\n";
    if (!res.entry.comment.empty()) std::cout << "Comment: " << res.entry.comment << "\n";
  }
  return 0;
}

int cmd_serve(const Opts& o) {
  oui::ManufDB db;
  auto lr = db.load(o.db);
  if (!lr.ok) {
    std::cerr << "DB load failed: " << lr.message << "\n";
    std::cerr << "Tip: run `oui update` first.\n";
    return 1;
  }

  web::HttpServer server(o.host, o.port, o.db, &db);
  std::cout << "Serving on http://" << o.host << ":" << o.port << "\n";
  std::cout << "DB: " << o.db << "\n";
  return server.serve_forever();
}

} // namespace

namespace cli {

int run(int argc, char** argv) {
  Opts o = parse(argc, argv);

  if (o.cmd.empty() || o.cmd == "-h" || o.cmd == "--help" || o.cmd == "help") {
    print_usage();
    return 0;
  }

  if (o.cmd == "update") return cmd_update(o);
  if (o.cmd == "lookup") return cmd_lookup(o);
  if (o.cmd == "serve")  return cmd_serve(o);

  std::cerr << "Unknown command: " << o.cmd << "\n";
  print_usage();
  return 2;
}

} // namespace cli

