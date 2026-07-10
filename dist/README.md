# 配布用バイナリ(書き込みだけで使う)

ソースをビルドせず、**でき上がったファームウェアを M5Dial に書き込むだけ**で使えます。

- `m5dial-badge-merged.bin` … 統合バイナリ(bootloader + パーティション + アプリを 1 つにまとめたもの)
- `flash.sh` … macOS / Linux 用の書き込みスクリプト
- `flash.bat` … Windows 用の書き込みスクリプト

## 手順

### 1. 書き込みツール(esptool)を入れる

```bash
# macOS / Linux
python3 -m pip install --user esptool
# Windows
python -m pip install --user esptool
```

### 2. M5Dial を USB で接続してポートを確認

- macOS: `ls /dev/cu.usbmodem*` → 例 `/dev/cu.usbmodem101`
- Windows: 「デバイスマネージャー」→ ポート(COM…) → 例 `COM3`

### 3. 書き込む

```bash
# macOS / Linux
cd dist
./flash.sh /dev/cu.usbmodem101

# Windows
cd dist
flash.bat COM3
```

書き込みツールを直接使う場合:

```bash
python3 -m esptool --chip esp32s3 --port <ポート> --baud 460800 write_flash 0x0 m5dial-badge-merged.bin
```

## うまくいかないとき

- **「No serial data received」/ つながらない** … 起動失敗中かもしれません。
  M5Dial の G0(BOOT)ボタンを押しながら RST を一度押して、ダウンロードモードにしてから再実行してください。
- **ポートが見つからない** … USB ケーブルがデータ通信対応か確認。`pio device list` でも確認できます。

## このバイナリの作り直し(開発者向け)

ソースを変更したら、ビルド後に以下で統合バイナリを作り直します(オフセットは
[../partitions.csv](../partitions.csv) と PlatformIO のフラッシュ配置に対応):

```bash
pio run -e m5dial   # まずビルド

python3 -m esptool --chip esp32s3 merge_bin -o dist/m5dial-badge-merged.bin \
  --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0     .pio/build/m5dial/bootloader.bin \
  0x8000  .pio/build/m5dial/partitions.bin \
  0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/m5dial/firmware.bin
```
