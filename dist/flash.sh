#!/usr/bin/env bash
# M5Dial-Badge 統合バイナリ書き込みスクリプト(macOS / Linux)
#
# 使い方: ./flash.sh <ポート>
#   例:    ./flash.sh /dev/cu.usbmodem101
# ポートは `ls /dev/cu.usbmodem*`(Mac)や `pio device list` で確認。
#
# 事前に esptool が必要: python3 -m pip install --user esptool

set -e
PORT="${1:?usage: ./flash.sh <port>  例: /dev/cu.usbmodem101}"
DIR="$(cd "$(dirname "$0")" && pwd)"

python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 \
  write_flash 0x0 "$DIR/m5dial-badge-merged.bin"

echo "書き込み完了。M5Dial が再起動してスライドショーが始まります。"
