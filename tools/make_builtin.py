#!/usr/bin/env python3
"""assets/builtin.png -> src/builtin_image.h(240x240 JPEG の PROGMEM バイト列)。

空状態画面で表示する組み込みデフォルト画像を生成する。
PIL(Pillow)が必要。ビルド前に一度だけ手動実行する想定(生成物はコミットする)。

  python3 tools/make_builtin.py

生成物 src/builtin_image.h の中身が変わらない場合は書き込まない。
"""

import io
import os

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(HERE)
SRC_PNG = os.path.join(PROJECT_ROOT, "assets", "builtin.png")
DST_HEADER = os.path.join(PROJECT_ROOT, "src", "builtin_image.h")
SIZE = 240
JPEG_QUALITY = 85


def crop_square(im: "Image.Image", size: int) -> "Image.Image":
    im = im.convert("RGB")
    s = min(im.width, im.height)
    left = (im.width - s) // 2
    top = (im.height - s) // 2
    im = im.crop((left, top, left + s, top + s))
    return im.resize((size, size), Image.LANCZOS)


def main() -> None:
    if not os.path.exists(SRC_PNG):
        raise SystemExit(f"[make_builtin] not found: {SRC_PNG}")

    im = crop_square(Image.open(SRC_PNG), SIZE)
    buf = io.BytesIO()
    im.save(buf, format="JPEG", quality=JPEG_QUALITY)
    jpg = buf.getvalue()

    lines = [
        "// 自動生成物: tools/make_builtin.py が assets/builtin.png から生成。手で編集しない。",
        "#pragma once",
        "#include <Arduino.h>",
        "",
        f"// 240x240 JPEG / {len(jpg)} bytes",
        f"constexpr size_t kBuiltinImageLen = {len(jpg)};",
        "const uint8_t kBuiltinImage[] PROGMEM = {",
    ]
    for i in range(0, len(jpg), 16):
        chunk = jpg[i:i + 16]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.append("};")
    lines.append("")
    out = "\n".join(lines)

    old = None
    if os.path.exists(DST_HEADER):
        with open(DST_HEADER, "r", encoding="utf-8") as f:
            old = f.read()
    if old == out:
        print(f"[make_builtin] up-to-date: {len(jpg)}B JPEG")
        return

    with open(DST_HEADER, "w", encoding="utf-8") as f:
        f.write(out)
    print(f"[make_builtin] wrote src/builtin_image.h: {len(jpg)}B JPEG")


if __name__ == "__main__":
    main()
