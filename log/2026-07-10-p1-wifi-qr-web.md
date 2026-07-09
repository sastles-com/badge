# 2026-07-10 P1: Wi-Fi + QR + Web 基盤

## スコープ(IMPLEMENTATION.md P1)

1. net_manager: SoftAP 起動 + DNS 全リダイレクト
2. ui_screens: 2 段階 QR 画面(内接正方形)、ダイヤルで 1↔2 切替
3. input: BtnA 長押し 2 秒 → QR、短押し → 復帰
4. web_server: `/` 仮ページ、接続性チェック URL → 案内ページ
5. `/api/status` 実装

## ソース構成(新規)

P0 の単一 main.cpp を機能別に分割した(CLAUDE.md / IMPLEMENTATION.md §2 準拠)。

| ファイル | 役割 |
| --- | --- |
| `app_state.h` | AppMode / WifiMode / QrPage の enum と共有状態 AppState |
| `net_manager.{h,cpp}` | オープン SoftAP + DNS 全リダイレクト、SSID/IP/QR ペイロード提供 |
| `web_server.{h,cpp}` | 仮トップページ、キャプティブ案内ページ、`/api/status` |
| `ui_screens.{h,cpp}` | スライドショー仮画面 / 2 段階 QR / ステータス画面の描画 |
| `input.{h,cpp}` | ダイヤル / タッチ / BtnA(短押し・長押し 2 秒)のイベント化 |
| `main.cpp` | setup/loop と状態遷移の統括のみ |

## 実装上の判断・メモ

- **オープン AP**: SPEC 3.1 どおりパスワードなし。`WiFi.softAP(ssid, nullptr, channel)` で開放。
  Wi-Fi QR は `WIFI:T:nopass;S:M5Dial-Badge;;`(パスワードありなら `T:WPA;...;P:...`)。
- **IPAddress は constexpr 不可**: Arduino-ESP32 の `IPAddress` コンストラクタは constexpr
  ではないため、AP 固定 IP(192.168.4.1)は定数化せず `begin()` 内で直接構築。
- **キャプティブポータル方針**: OS の接続性チェック URL(Android `/generate_204`,
  iOS `/hotspot-detect.html`, Windows `/ncsi.txt` 等)には 302 リダイレクトではなく
  **案内ページを 200 で返す**。ポータル内疑似ブラウザでの操作はサポートしない
  (SPEC 3.1、不安定なため)。`onNotFound` も案内ページに集約。
- **QR レイアウト**: 円形 240×240 の内接正方形に収めるため QR 一辺 160px、
  x=(240-160)/2=40, y=40 で中央配置。上下に日本語キャプション。
  QR バージョンは 4(33×33 モジュール)固定で Wi-Fi/URL 両ペイロードに十分。
- **日本語表示**: `fonts::lgfxJapanGothicP_16`(M5GFX 同梱)を使用。
- **状態遷移(P1)**:
  - SLIDESHOW: BtnA 長押し→QR / BtnA 短押し→STATUS(5 秒で自動復帰)
  - QR: BtnA 短押し→SLIDESHOW / ダイヤル・タッチ→Wi-Fi QR↔URL QR(AP 時のみ)
  - STATUS: BtnA→SLIDESHOW / 5 秒経過→SLIDESHOW
- **`/api/status`**: `{fw, mode, ssid, ip, totalBytes, freeBytes, contentCount}`。
  contentCount は P2 の content_store 実装まで 0 固定。

## ビルド結果

- ビルド成功。RAM 14.9%(48876 / 327680 B)、Flash 35.7%(1123765 / 3145728 B)。
- DNSServer / WebServer / WiFi は ESP32 Arduino フレームワーク同梱のため lib_deps 追加不要。

## 起動ログ(実機書き込み後)

```text
=== M5Dial-Badge P1 ===
[SYS] free heap: 326964 bytes
[FS] mounted: used=8192 total=5111808 bytes
[NET] SoftAP up: ssid=M5Dial-Badge ip=192.168.4.1 (open)
[NET] DNS captive redirect started
[WEB] HTTP server started on port 80
[SYS] ready
```

## 実機確認(完了 2026-07-10)

- [x] BtnA 長押し 2 秒で QR モード、ダイヤル/タッチで Wi-Fi QR↔URL QR、短押しで復帰
- [x] iPhone / Android で QR 2 回読み → ブラウザで仮ページが開く
- [x] キャプティブポータルの案内が出る
- [x] BtnA 短押しでステータス画面(モード/SSID/IP/空き容量)が 5 秒表示

→ 実機で全項目 OK。P1 完了。
