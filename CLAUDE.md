# M5Dial-Badge

M5Dial を電子バッヂ化するファームウェア。本体が SoftAP + Web アプリを提供し、LCD の QR コードを読んだスマホから画像・動画をアップロードすると、円形 LCD でスライドショー再生する。

## 必読ドキュメント(実装前に読むこと)

1. [docs/PLAN.md](docs/PLAN.md) — 目的・スコープ・技術判断の理由
2. [docs/SPEC.md](docs/SPEC.md) — 機能仕様(API・画面・コンテナ形式・操作)。**仕様の一次情報はここ**
3. [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md) — ソース構成・フェーズ別タスク・移植対応表
4. `log/` — 仕様検討の経緯(なぜその決定になったか)

仕様と実装が食い違う判断が必要になったら、勝手に仕様を変えず SPEC.md の「要確認事項」に追記してユーザーに確認する。

## ハードウェア制約(最重要)

- M5Dial = M5StampS3(ESP32-S3FN8)。**PSRAM なし・SD カードなし・Flash 8MB**
- LCD: 円形 240×240(GC9A01)。四隅は見えないため、重要な情報は内接円内に描く
- H.264/MP4 デコード不可。動画はブラウザ側で JPEG 連番(独自 MJPG コンテナ)に変換して受け取る
- BLE は使わない(RAM 節約のため。Improv BLE 等を提案しない)
- `String` 乱用・大きなヒープ確保を避ける。最大フレームバッファ 48KB は起動時に一度だけ確保

## ビルド・書き込み

```bash
# ビルド
/Users/katano/Library/Python/3.9/bin/pio run -e m5dial

# 書き込み(ポートは ls /dev/cu.usbmodem* で確認)
/Users/katano/Library/Python/3.9/bin/pio run -e m5dial -t upload --upload-port /dev/cu.usbmodem1101

# シリアルモニタ(115200)
/Users/katano/Library/Python/3.9/bin/pio device monitor -b 115200
```

- board: `m5stack-stamps3`、パーティションは `partitions.csv`(OTA なし、LittleFS 約 4.9MB)
- Web UI を変更したら `tools/build_webui.py` で `src/web_ui.h` を再生成してからビルド(extra_scripts で自動化されていればビルドだけでよい)

## 参考実装

`~/Documents/PlatformIO/Projects/splatoon` — 同作者の AtomS3 プロジェクト。以下をここから移植・流用する(対応表は IMPLEMENTATION.md §5):

- SoftAP + DNS 全リダイレクト + 接続性チェック URL ハンドラ
- チャンクアップロード(`handle_web_upload_chunk`)
- `M5.Display.qrcode` による QR 表示、BtnA 短押し/長押し判定

## 開発の進め方

- IMPLEMENTATION.md のフェーズ順(P0→P6)に進める。**各フェーズ末尾の実機確認はユーザーに依頼する**(自動化できない)
- P4(動画)は最初に必ず再生 fps のベンチマークを行い、結果を報告してから本実装に進む
- 実機確認の結果・設計変更・ハマりどころは `log/YYYY-MM-DD-*.md` に記録する
- コミットはフェーズ単位以上の粒度で、動作確認済みの状態のみ

## コード規約

- ソースは機能別分割(IMPLEMENTATION.md §2 の構成に従う)。単一巨大 main.cpp にしない
- 定数は `constexpr`/`static const`、マジックナンバー禁止(SSID・タイムアウト・バッファサイズは先頭で定義)
- コメント・LCD 表示・Web UI は日本語可。シリアルログは英語
- JSON は ArduinoJson を使う場合は固定サイズドキュメントのみ
