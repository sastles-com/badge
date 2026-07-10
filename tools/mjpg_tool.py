#!/usr/bin/env python3
"""独自 MJPG コンテナの生成 / 分解(デバッグ・ベンチ用)。

コンテナ(リトルエンディアン、SPEC 3.3):
  magic   : "MJPG"(4 バイト)
  version : u8 = 1
  fps     : u8
  frames  : u16
  reserved: u32
  以降: [u32 size][JPEG(240x240)] の繰り返し

使い方:
  # 合成パターンのベンチ用 MJPG を生成
  python3 tools/mjpg_tool.py gen --out data/bench.mjpg --frames 30 --fps 10 --quality 0.7
  # 動画/GIF から生成(要 ffmpeg 無し、PIL のみ。GIF/連番は未対応、合成のみ)
  # 既存 MJPG の情報表示・フレーム分解
  python3 tools/mjpg_tool.py info bench.mjpg
  python3 tools/mjpg_tool.py split bench.mjpg --outdir /tmp/frames
"""

import argparse
import io
import os
import struct

from PIL import Image, ImageDraw

MAGIC = b"MJPG"
VERSION = 1
SIZE = 240


def make_synthetic_frame(i: int, total: int, quality: float) -> bytes:
    """移動する円 + グラデーション背景の合成フレーム(JPEG バイト列)。"""
    im = Image.new("RGB", (SIZE, SIZE))
    px = im.load()
    # 斜めグラデーション(フレームで色相を回す)
    base = int(255 * i / max(1, total))
    for y in range(SIZE):
        for x in range(0, SIZE, 4):  # 4px 刻みで軽量化
            r = (x + base) % 256
            g = (y + base * 2) % 256
            b = (x + y) % 256
            for dx in range(4):
                if x + dx < SIZE:
                    px[x + dx, y] = (r, g, b)
    d = ImageDraw.Draw(im)
    t = i / max(1, total)
    cx = int(40 + (SIZE - 80) * t)
    cy = int(SIZE / 2 + 60 * __import__("math").sin(t * 6.28))
    d.ellipse((cx - 28, cy - 28, cx + 28, cy + 28), fill=(255, 255, 255))
    d.text((10, 10), f"{i+1}/{total}", fill=(0, 0, 0))

    buf = io.BytesIO()
    im.save(buf, format="JPEG", quality=int(quality * 100))
    return buf.getvalue()


def make_photo_frame(src: "Image.Image", i: int, total: int, quality: float) -> bytes:
    """実写真をパン(少しずつずらし)してフレーム化。実データに近いサイズになる。"""
    s = min(src.width, src.height)
    # フレームごとに切り出し位置を少し動かす
    max_shift = max(0, (max(src.width, src.height) - s))
    off = int(max_shift * (i / max(1, total)))
    if src.width >= src.height:
        box = (off, 0, off + s, s)
    else:
        box = (0, off, s, off + s)
    frame = src.crop(box).resize((SIZE, SIZE), Image.LANCZOS)
    buf = io.BytesIO()
    frame.save(buf, format="JPEG", quality=int(quality * 100))
    return buf.getvalue()


def cmd_gen(args) -> None:
    frames = []
    src = None
    if args.src:
        src = Image.open(args.src).convert("RGB")
    for i in range(args.frames):
        if src is not None:
            frames.append(make_photo_frame(src, i, args.frames, args.quality))
        else:
            frames.append(make_synthetic_frame(i, args.frames, args.quality))

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<BBHI", VERSION, args.fps, len(frames), 0))
        for fr in frames:
            f.write(struct.pack("<I", len(fr)))
            f.write(fr)

    total = os.path.getsize(args.out)
    avg = sum(len(x) for x in frames) // max(1, len(frames))
    mx = max(len(x) for x in frames)
    print(f"[mjpg] wrote {args.out}: {len(frames)} frames @ {args.fps}fps, "
          f"{total} bytes (avg {avg}B, max {mx}B)")


def _read_header(f):
    magic = f.read(4)
    if magic != MAGIC:
        raise SystemExit(f"bad magic: {magic!r}")
    version, fps, nframes, _reserved = struct.unpack("<BBHI", f.read(8))
    return version, fps, nframes


def cmd_info(args) -> None:
    with open(args.file, "rb") as f:
        version, fps, nframes = _read_header(f)
        sizes = []
        for _ in range(nframes):
            (sz,) = struct.unpack("<I", f.read(4))
            f.seek(sz, 1)
            sizes.append(sz)
    print(f"version={version} fps={fps} frames={nframes} "
          f"avg={sum(sizes)//max(1,len(sizes))}B max={max(sizes)}B")


def cmd_split(args) -> None:
    os.makedirs(args.outdir, exist_ok=True)
    with open(args.file, "rb") as f:
        _, _, nframes = _read_header(f)
        for i in range(nframes):
            (sz,) = struct.unpack("<I", f.read(4))
            data = f.read(sz)
            with open(os.path.join(args.outdir, f"frame_{i:03d}.jpg"), "wb") as o:
                o.write(data)
    print(f"[mjpg] split {nframes} frames -> {args.outdir}")


def main() -> None:
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)

    g = sub.add_parser("gen")
    g.add_argument("--out", required=True)
    g.add_argument("--frames", type=int, default=30)
    g.add_argument("--fps", type=int, default=10)
    g.add_argument("--quality", type=float, default=0.7)
    g.add_argument("--src", default=None,
                   help="実画像から生成(未指定なら合成パターン)")
    g.set_defaults(func=cmd_gen)

    i = sub.add_parser("info")
    i.add_argument("file")
    i.set_defaults(func=cmd_info)

    s = sub.add_parser("split")
    s.add_argument("file")
    s.add_argument("--outdir", default="/tmp/mjpg_frames")
    s.set_defaults(func=cmd_split)

    args = p.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
