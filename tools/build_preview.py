#!/usr/bin/env python3
"""Embed the artwork exported from the firmware renderer; no runtime downloads."""
import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
art = json.loads((root / "web/art.json").read_text())
template = (root / "web/preview-fragment.html").read_text()
fragment = template.replace("__ART_JSON__", json.dumps(art, separators=(",", ":")))
(root / "web/preview.inline.html").write_text(fragment)
wrapper = '<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Pocket Rally</title><style>:root{color-scheme:light dark}body{margin:0;padding:24px;max-width:800px;margin:auto;background:light-dark(#faf8fc,#171524)}</style>'
(root / "web/preview.html").write_text(wrapper + fragment + "</html>")
print("Generated web/preview.html and web/preview.inline.html")
