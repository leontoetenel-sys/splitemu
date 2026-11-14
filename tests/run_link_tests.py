#!/usr/bin/env python3
"""Generate uniquely tagged test ROMs; verify physical senders, local IDs and isolation."""
import importlib.util
import pathlib
import struct
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("rom", root / "tools/make_link_test_rom.py")
rom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rom)
with tempfile.TemporaryDirectory(prefix="splitemu-link-roms-") as tmp:
    paths = []
    for screen in range(4):
        image = bytearray(rom.ROM_SIZE)
        image[:rom.HEADER_SIZE] = rom.build_header()
        code = rom.build_code().replace(struct.pack("<I",0xE3877CAB),struct.pack("<I",0xE3877CA0+screen))
        image[rom.HEADER_SIZE:rom.HEADER_SIZE+len(code)] = code
        path = pathlib.Path(tmp) / f"sender{screen}.gba"
        path.write_bytes(image)
        paths.append(str(path))
    for mode, order in [(2,"0123"),(1,"0213"),(1,"3102"),(3,"0213"),(3,"2301")]:
        subprocess.run([sys.argv[1],str(mode),order,*paths],check=True,timeout=20)
