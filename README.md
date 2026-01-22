# diangx-oui-lookup (OpenWrt)

OpenWrt package notes and usage for the OUI lookup tool.

---

## Build (OpenWrt)

From your OpenWrt/SDK tree:
```bash
make -j1 V=sc package/feeds/quantum/diangx-oui-lookup/{clean,compile}
```

---

## Install / Remove

```bash
opkg install /tmp/diangx-oui-lookup_*.ipk
opkg remove diangx-oui-lookup
```

---

## Installed paths

```
/usr/bin/oui
/usr/share/oui/manuf.gz
```

---

## Usage

### Lookup
```bash
/usr/bin/oui lookup 00:11:22:33:44:55
/usr/bin/oui lookup 001122334455
/usr/bin/oui lookup 00-11-22-33-44-55
/usr/bin/oui lookup 00:11:22
```

JSON output:
```bash
/usr/bin/oui lookup --json 00:11:22:33:44:55
```

### Serve Web UI / API
```bash
/usr/bin/oui serve --port 8080
```

### Update DB (OpenWrt)
The updater prefers `uclient-fetch` when available and falls back to `curl`, `wget`, or `python3`.
```bash
/usr/bin/oui update --db /usr/share/oui/manuf.gz \
  --url https://www.wireshark.org/download/automated/data/manuf.gz
```

If the default DB path is not found, pass it explicitly on lookup:
```bash
/usr/bin/oui lookup --db /usr/share/oui/manuf.gz 00:11:22:33:44:55
```
