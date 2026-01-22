# OUI Lookup Tool (Wireshark `manuf`-based)

A small, product-style OUI/MAC vendor lookup utility.
It resolves a MAC address (or prefix) to the best matching vendor entry using **Longest Prefix Match** against Wireshark’s `manuf` database, and provides both **CLI** and **local Web UI/JSON API**.

---

## Key Features

### 1) Correctness: Longest Prefix Match
Wireshark `manuf` entries may contain:
- plain prefixes (e.g., `00:11:22`), inferred mask by length
- explicit masks (e.g., `00:11:22:33/32`)
- comments (e.g., `# some note`)

This tool:
- normalizes input MAC formats
- builds a mask-indexed map
- searches masks in descending order
- returns the **first match** → the **most specific (best) match**

### 2) CLI + Web UI + JSON API
- `oui update` downloads the latest `manuf` DB (atomic replace, gzip supported)
- `oui lookup` returns vendor info (text or JSON)
- `oui serve` provides:
  - browser UI at `/`
  - JSON API at `/api/lookup?mac=...`

### 3) Embedded-friendly deployment
- Default DB can be installed to a read-only location like `/usr/share/oui/manuf`
- `update` can be disabled or replaced depending on the target environment
- Works on Linux-based targets (Raspberry Pi, OpenWrt boards) with minimal changes

---

## Project Layout

```bash
diangx-oui-lookup/
├── CMakeLists.txt
├── README.md
├── data/ # local DB (default output of update)
└── src/
  ├── main.cpp
  ├── cli/ # command parsing + subcommands
  │ ├── cli.h
  │ └── cli.cpp
  ├── oui/ # MAC parsing + manuf DB loader + lookup
  │ ├── mac.h
  │ ├── mac.cpp
  │ ├── manuf_db.h
  │ └── manuf_db.cpp
  ├── update/ # DB downloader + atomic replace
  │ ├── updater.h
  │ └── updater.cpp
  ├── web/ # minimal HTTP server + UI + API endpoint
  │ ├── http_server.h
  │ └── http_server.cpp
  ├── util/ # small helpers
  │ ├── fs.h / fs.cpp
  │ ├── str.h / str.cpp
  │ └── json.h / json.cpp
```

---

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

Binary output: 
build/oui

---

## Usage

### 1) Update DB (download Wireshark manuf)
```bash
./build/oui update
```

Custom output path:
```bash
./build/oui update --db data/manuf
./build/oui update --db data/manuf.gz
```

Custom URL:
```bash
./build/oui update --url https://www.wireshark.org/download/automated/data/manuf
./build/oui update --url https://www.wireshark.org/download/automated/data/manuf.gz
```

Tip: the loader auto-detects gzip files (magic header), so storing `manuf.gz` keeps disk usage smaller without extra steps.

### 2) Lookup (text output)
```bash
./build/oui lookup 00:11:22:33:44:55
./build/oui lookup 001122334455
./build/oui lookup 00-11-22-33-44-55
./build/oui lookup 00:11:22
```

Example output:
```bash
Vendor: Cisco Systems, Inc.
Prefix: 00:11:22/24
Comment: ...
```

### 3) Lookup (JSON output)
```bash
./build/oui lookup --json 00:11:22:33:44:55
```

Example:
```bash
{
  "found": true,
  "vendor": "Cisco Systems, Inc.",
  "prefix": "00:11:22",
  "mask_bits": 24,
  "comment": "",
  "db": "data/manuf"
}
```

---

## Local Web UI + API
Start server:
```bash
./build/oui serve --port 8080
```
* UI: http://127.0.0.1:8080/
* API: http://127.0.0.1:8080/api/lookup?mac=00:11:22:33:44:55

Expose to LAN:
```bash
./build/oui serve --host 0.0.0.0 --port 8080
```

---

## How to Works

### Input normalization
Accepted MAC formats:
```bash
00:11:22:33:44:55
00-11-22-33-44-55
001122334455
short prefix such as 00:11:22 or 001122
```
All inputs are normalized by extracting hex digits and left-aligning to 48 bits.

### Parsing `manuf`
Each non-comment line is treated as:
```bash
<prefix>[/mask] <vendor> [# comment]
```

If /mask is omitted, the mask is inferred from prefix length:
```bash
3 bytes → /24
6 bytes → /48
etc.
```

### Indexing strategy
The DB is stored as:
```bash
map(maskBits -> map(prefix -> entry))
masks_desc sorted in descending order
```

Lookup does:
```bash
1. normalize target to 48-bit value
2. for each mask in masks_desc:
    - key = mac & mask(maskBits)
    - check map for match
3. return first match (best match)
```

This is simple, fast, and deterministic.

---

## Portability Notes (Raspberry Pi / OpenWrt)

For OpenWrt 
```bash
https://github.com/diangx/diangx-oui-lookup/tree/openwrt_19.07
```

### Default DB location
For embedded deployments, a typical layout is:
```bash
binary: /usr/bin/oui
DB: /usr/share/oui/manuf
```

You can build with a default DB path via CMake:
```bash
target_compile_definitions(oui PRIVATE OUI_DEFAULT_DB=\"/usr/share/oui/manuf\")
```

### Updating DB on embedded targets
Current update uses an external downloader via system() (curl).
On OpenWrt, you may prefer uclient-fetch or wget.

Recommended approach:
```bash
try uclient-fetch → fallback to wget → fallback to curl
or disable update in restricted environments and ship a fixed DB
```

### Security & Reliability Considerations
```bash
update downloads to a temporary file, validates non-zero size, then atomically replaces the DB.
The web server is intentionally minimal (GET-only). Do not expose it to untrusted networks without hardening.
Shell usage in update is kept minimal, and paths are shell-escaped.
```
