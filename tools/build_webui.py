#!/usr/bin/env python3
"""web/index.html -> src/web_ui.h 変換(gzip + PROGMEM バイト列)。

PlatformIO の pre スクリプトとしても、単体でも実行できる。
  - PlatformIO: platformio.ini の extra_scripts = pre:tools/build_webui.py
  - 単体:       python3 tools/build_webui.py

生成物 src/web_ui.h の中身が変わらない場合は書き込まない(不要な再コンパイル回避)。
"""

import gzip
import os


def generate(project_root: str) -> None:
    SRC_HTML = os.path.join(project_root, "web", "index.html")
    DST_HEADER = os.path.join(project_root, "src", "web_ui.h")

    if not os.path.exists(SRC_HTML):
        print(f"[build_webui] SKIP: {SRC_HTML} not found")
        return

    with open(SRC_HTML, "rb") as f:
        raw = f.read()

    # mtime を固定してビルドごとの差分を無くす(再現性確保)。
    gz = gzip.compress(raw, compresslevel=9, mtime=0)

    lines = [
        "// 自動生成物: tools/build_webui.py が web/index.html から生成。手で編集しない。",
        "#pragma once",
        "#include <Arduino.h>",
        "",
        f"// 元 HTML: {len(raw)} bytes / gzip 後: {len(gz)} bytes",
        f"constexpr size_t kWebUiHtmlLen = {len(gz)};",
        "const uint8_t kWebUiHtml[] PROGMEM = {",
    ]
    for i in range(0, len(gz), 16):
        chunk = gz[i:i + 16]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.append("};")
    lines.append("")
    out = "\n".join(lines)

    old = None
    if os.path.exists(DST_HEADER):
        with open(DST_HEADER, "r", encoding="utf-8") as f:
            old = f.read()

    if old == out:
        print(f"[build_webui] up-to-date: {len(raw)}B -> gzip {len(gz)}B")
        return

    os.makedirs(os.path.dirname(DST_HEADER), exist_ok=True)
    with open(DST_HEADER, "w", encoding="utf-8") as f:
        f.write(out)
    print(f"[build_webui] wrote src/web_ui.h: {len(raw)}B -> gzip {len(gz)}B")


# PlatformIO の pre スクリプトとして呼ばれた場合は Import('env') が使える。
try:
    Import("env")  # type: ignore  # noqa: F821
    generate(env.subst("$PROJECT_DIR"))  # type: ignore  # noqa: F821
except NameError:
    if __name__ == "__main__":
        root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        generate(root)
