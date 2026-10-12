#!/usr/bin/env python3
"""Run after pio run -e jc2432w328c -e diagnostic."""
import os
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parent.parent
core = Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio"))
esptool = core / "packages/tool-esptoolpy/esptool.py"
boot_app = core / "packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
dist = root / "dist"
dist.mkdir(exist_ok=True)
for env, name in [("jc2432w328c", "pocket-rally-0.5.bin"), ("diagnostic", "pocket-rally-diagnostic-0.5.bin")]:
    build = root / ".pio/build" / env
    inputs = [("0x1000", build / "bootloader.bin"), ("0x8000", build / "partitions.bin"), ("0xe000", boot_app), ("0x10000", build / "firmware.bin")]
    for _, file in inputs:
        if not file.is_file():
            raise SystemExit(f"Missing {file}. Build the {env} environment first.")
    command = [sys.executable, str(esptool), "--chip", "esp32", "merge_bin", "-o", str(dist / name), "--flash_mode", "dio", "--flash_size", "4MB"]
    for offset, file in inputs:
        command += [offset, str(file)]
    subprocess.run(command, check=True)
