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
- `oui update` downloads the latest `manuf` DB (atomic replace)
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
└── util/ # small helpers
├── fs.h / fs.cpp
├── str.h / str.cpp
└── json.h / json.cpp
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
```

Custom URL:
```bash
./build/oui update --url https://www.wireshark.org/download/automated/data/manuf
```

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

## How Lookup Works
- TODO
