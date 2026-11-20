#!/usr/bin/env python3
"""Phase 12: reconcile R173 Game.exe addresses against D2VP function bodies.
Maps address -> D2VP name/file/line, prints bodies of the BNet-path core."""
import json, os, re

BASE = "/home/z/my-project/D2VP-QoL/Source"
ADDRS = {
    "0x51d1c0": "BNET_Connect entry", "0x51d0a0": "BattleConnectFastest",
    "0x51ca90": "sFastestConnect", "0x51c470": "reset connection state",
    "0x51c5c0": "BNCS_SendPacket", "0x51a540": "ack/server-token store",
    "0x520cb0": "BDClientId recv handler", "0x520dc0": "auth status dispatcher",
    "0x520e50": "vercheck result handler", "0x5215b0": "account logon chain",
    "0x5216b0": "account logon proof", "0x520b70": "auth step wait",
    "0x520b30": "auth state reset", "0x523ac0": "SHA1 init",
    "0x449e70": "D2CS recv dispatch", "0x44aae0": "D2CS send wrapper",
    "0x44a2e0": "D2CS send createchar", "0x449bf0": "join-result setter",
    "0x449c00": "join-result getter", "0x437ab0": "charlist cache add",
    "0x438ad0": "char record create", "0x43aab0": "charlist reply walk",
    "0x435e10": "charcreate handler", "0x44c000": "gamesrv enter",
    "0x44c060": "gamesrv leave", "0x462d00": "gamesrv transition",
    "0x468730": "mouse X?", "0x468740": "mouse Y?", "0x51bdc0": "BnSend buf A",
    "0x51c0e0": "bnserver ini parse", "0x5182b0": "realms.bin load",
    "0x51a480": "debug log", "0x518d40": "status log", "0x520520": "status set",
}
addr_re = re.compile(r'"address":\s*"(0*[0-9a-fA-F]{8})"')
index = {}  # addr -> (module, file, name, line)
for mod in os.listdir(BASE):
    d = os.path.join(BASE, mod)
    if not os.path.isdir(d): continue
    for fn in os.listdir(d):
        if not fn.endswith(".map"): continue
        try:
            data = json.load(open(os.path.join(d, fn), encoding="utf-8"))
        except Exception: continue
        for f in data.get("functions", []):
            a = f.get("address", "").lower().lstrip("0")
            if a: index.setdefault(a, []).append((mod, fn[:-4], f["name"], f["lines"][0]["line"] if f.get("lines") else 0))

found = {}
for raddr, label in ADDRS.items():
    key = raddr[2:].lower().lstrip("0")
    hits = index.get(key, [])
    found[raddr] = (label, hits)
    if hits:
        for mod, fil, name, line in hits:
            print(f"{raddr}  {label:28s} -> {mod}/{fil}.cpp:{line}  {name}")
    else:
        print(f"{raddr}  {label:28s} -> NOT PRESENT IN D2VP")
