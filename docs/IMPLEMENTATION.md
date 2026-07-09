# M5Dial-Badge 実装計画書

作成日: 2026-07-09
前提: [PLAN.md](PLAN.md)・[SPEC.md](SPEC.md) 承認済みであること

## 1. 開発環境

- PlatformIO + Arduino フレームワーク
- pio コマンド: `/Users/katano/Library/Python/3.9/bin/pio`(splatoon と同じ)
- 参考実装: `~/Documents/PlatformIO/Projects/splatoon`(特に `src/main.cpp` の Web サーバー / キャプティブポータル / アップロード / QR 表示部)

### platformio.ini(案)

```ini
[platformio]
default_envs = m5dial

[env:m5dial]
platform = espressif32
framework = arduino
board = m5stack-stamps3
board_build.partitions = partitions.csv
monitor_speed = 115200
upload_speed = 460800
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
lib_deps =
    m5stack/M5Dial @ ^1.0.3
```

- `M5Dial` ライブラリは M5Unified / M5GFX を内包し、ロータリーエンコーダとタッチを `M5Dial.Encoder` / `M5Dial.Touch` で扱える
- PSRAM なしのため `-DBOARD_HAS_PSRAM` は付けない

### partitions.csv(案・OTA なし)

```csv
# Name,   Type, SubType, Offset,   Size
nvs,      data, nvs,     0x9000,   0x5000
phy_init, data, phy,     0xe000,   0x1000
app0,     app,  ota_0,   0x10000,  0x300000
littlefs, data, spiffs,  0x310000, 0x4E0000
```

→ アプリ 3MB、LittleFS 約 4.9MB。

## 2. ソース構成

単一巨大 main.cpp(splatoon 方式)は避け、機能別に分割する。

```text
src/
  main.cpp            setup/loop、状態機械の統括
  app_state.h         全体状態(enum: SLIDESHOW / QR / STATUS / UPLOADING / ERROR)と共有構造体
  net_manager.{h,cpp} Wi-Fi 起動シーケンス(STA 試行→AP フォールバック)、mDNS、資格情報の保存/削除
  web_server.{h,cpp}  WebServer ルーティング、キャプティブポータル、REST API
  web_upload.{h,cpp}  チャンクアップロード処理、容量チェック、id 採番
  content_store.{h,cpp} LittleFS 上のコンテンツ/プレイリスト/設定の CRUD(playlist.json, config.json)
  player.{h,cpp}      スライドショー状態機械、静止画表示、MJPG 再生
  mjpg_reader.{h,cpp} MJPG コンテナのパース・逐次フレーム読み出し
  ui_screens.{h,cpp}  QR 画面・ステータス画面・空状態・進捗・エラーの描画
  input.{h,cpp}       ダイヤル/タッチ/BtnA のイベント化(短押し・長押し 2 秒判定は splatoon から移植)
  web_ui.h            gzip 済み HTML の PROGMEM バイト列(自動生成物)
  builtin_image.{h,c} 組み込みデフォルト画像(自動生成物)
web/
  index.html          Web UI ソース(単一ファイル、ビルドで gzip → web_ui.h)
tools/
  build_webui.py      web/index.html → src/web_ui.h 変換(gzip + バイト列化)
  make_builtin.py     PNG → builtin_image.c 変換(splatoon の png2c.py を改修)
  mjpg_tool.py        MJPG コンテナの生成/分解(デバッグ用。ブラウザ実装の検証に使う)
```

### 設計上の注意(PSRAM なし)

- 静的確保を基本にし、最大フレームバッファ(48KB)は起動時に一度だけ `malloc`。失敗時は起動エラー表示
- `String` の乱用禁止(ヒープ断片化)。JSON は ArduinoJson の `StaticJsonDocument` 相当か手書きパーサで固定長運用
- Web UI HTML はストリーム送信(`server.sendContent` 分割か `send_P` gzip 一括)
- アップロード中はスライドショー描画を止める(SRAM とフラッシュ帯域をアップロードに集中)

## 3. フェーズ別タスク

各フェーズの最後に「実機確認」を必ず行い、確認内容を `log/` に記録する。

### P0: 開発基盤(0.5 日)

1. PlatformIO プロジェクト作成、platformio.ini / partitions.csv 配置
2. M5Dial 初期化、"Hello Badge" 表示、エンコーダ回転値・タッチ座標・BtnA をシリアルに出力
3. LittleFS マウント確認(フォーマット → 空き容量表示)

**確認**: ビルド・書き込み・LCD 表示・全入力デバイスの反応

### P1: Wi-Fi + QR + Web 基盤(1 日)

1. `net_manager`: SoftAP 起動(固定 SSID/パスワード)、DNS 全リダイレクト
2. `ui_screens`: 2 段階 QR 画面(円形画面の内接正方形に収める)、ダイヤルで 1↔2 切替
3. `input`: BtnA 長押し 2 秒 → QR モード、短押し → 復帰(splatoon から移植)
4. `web_server`: `/` に仮ページ、接続性チェック URL → 案内ページ
5. `/api/status` 実装

**確認**: iPhone / Android 実機で QR 2 回読み → ブラウザで仮ページが開く。キャプティブポータルの案内が出る

### P2: 画像アップロード・表示(1〜1.5 日)

1. `web/index.html`: ファイル選択 → canvas で 240×240 中央クロップ + JPEG 化 + 円形プレビュー
2. サムネイル(80×80)もブラウザ側で生成し同時アップロード
3. `web_upload`: multipart チャンク受信(splatoon の `handle_web_upload_chunk` を流用)、`/content/<id>.jpg` 保存、容量チェック
4. `content_store`: id 採番、playlist.json 追記
5. `player`: 単一画像の全画面表示(`M5.Display.drawJpg`)
6. `tools/build_webui.py` 整備(HTML 変更 → ヘッダ再生成をビルド前に自動実行: `extra_scripts`)

**確認**: スマホから写真をアップロード → LCD に即表示

### P3: プレイリスト・スライドショー(1 日)

1. `player`: 巡回状態機械(自動巡回 / 手動モード 10 秒復帰 / 一時停止)
2. `input`: ダイヤル送り/戻し、タッチで pause/resume
3. Web UI: サムネイル一覧、並べ替え、削除、duration 設定、「今すぐ表示」、容量バー
4. API: `/api/playlist` GET/POST、`/api/content/<id>` DELETE、`/api/thumb/<id>`、`/api/show/<id>`
5. 空状態画面(組み込み画像 + 案内)

**確認**: 3 件以上のコンテンツで巡回・ダイヤル操作・削除・並べ替え

### P4: 動画対応(1.5〜2 日)※技術リスク最大

1. **最初にベンチマーク**: 静的 MJPG(`tools/mjpg_tool.py` で PC 生成)を LittleFS に置き、読み出し + デコード + 描画の実測 fps を確認。10fps 未達なら品質/デコード方式を調整してから先へ進む
2. `mjpg_reader`: ヘッダ検証、フレーム逐次読み出し(シーク不要の順次読み)
3. `player`: 動画再生(fps 制御、ループ回数、次コンテンツへの遷移)
4. Web UI: `<video>` + canvas でフレームキャプチャ → MJPG コンテナを ArrayBuffer で組み立て → アップロード。fps / 品質 / 開始位置 UI、サイズ見積もり表示
5. HEIC/HEVC など「ブラウザが再生できない形式」のエラーハンドリング(その旨を表示)

**確認**: iPhone で撮った MOV をアップロード → 10fps でループ再生。24 時間連続再生でメモリ監視(`ESP.getFreeHeap()` を `/api/status` に含め、定期観測)

### P5: STA ハイブリッド(1 日)

1. `net_manager`: 起動時 STA 試行(15 秒)→ 失敗で AP フォールバック、wifi.json の保存/削除
2. mDNS(`badge.local`)、STA 時の URL QR 動的生成
3. Web UI 設定画面: `/api/wifi/scan` の SSID 一覧から選択、保存 → 再起動
4. ステータス画面にモード / IP 表示

**確認**: 自宅 Wi-Fi に参加 → スマホのネットを維持したまま Web UI が使える。ルーター停止 → AP フォールバックする

### P6: 仕上げ(1 日)

1. 設定(明るさ / デフォルト秒数 / ループ回数 / ブザー)の Web UI + `/api/config`
2. エラー画面、アップロード中プログレス表示、同時アップロード拒否(409)
3. ブザーのクリック音(設定で無効化可)
4. README.md(利用者向け手順 + 開発者向けビルド手順)
5. dist/: merged bin 生成 + flash スクリプト(splatoon の dist/ 構成を流用)

**確認**: 初見者テスト(QR から画像アップロードまで説明なしでできるか)

## 4. 実装順の依存関係

```text
P0 ─ P1 ─ P2 ─ P3 ─ P4 ─ P6
              └──── P5 ──┘   (P5 は P2 完了後なら並行可)
```

P4 のベンチマーク(タスク 1)だけは P2 完了時点で前倒し実行してよい(技術リスクの早期検証)。

## 5. splatoon からの移植対応表

| splatoon(main.cpp) | 本プロジェクトでの行き先 |
| --- | --- |
| `WiFi.softAP` / DNS リダイレクト周り | `net_manager.cpp` |
| 接続性チェック URL ハンドラ群 | `web_server.cpp`(案内ページ方式に変更) |
| `handle_web_upload_chunk/finish` | `web_upload.cpp`(multipart 化・容量チェック追加) |
| `build_wifi_ap_qr_payload` / `draw_web_qr_screen` | `ui_screens.cpp`(2 段階 QR に拡張) |
| BtnA 短押し/長押し判定 | `input.cpp` |
| LittleFS 優先読み込みロジック | `content_store.cpp`(プレイリスト方式に一般化) |
| png2c.py | `tools/make_builtin.py` |
| dist/ 一式 | `dist/`(ボード名等を書き換え) |

## 6. テスト計画

- **ユニット相当**: `tools/mjpg_tool.py` でブラウザ生成 MJPG を PC 上で分解・検証(コンテナ互換性の担保)
- **実機シナリオ**(各フェーズ確認に加え最終一括):
  1. 初回起動(空)→ QR → 画像 3 枚 + 動画 1 本アップロード → 巡回確認
  2. 容量ギリギリまでアップロード → 容量不足エラーの表示と復帰
  3. アップロード中の電源断 → 再起動後に書きかけファイルが残らない(起動時クリーンアップ)
  4. STA 設定 → 再起動 → badge.local アクセス → ルーター断 → AP フォールバック
  5. 24 時間連続再生でヒープ残量が単調減少しない
- **ブラウザ互換**: iOS Safari / Android Chrome の 2 系で全 UI 操作を確認(特に `<input type="file">` の動画選択と canvas キャプチャ)

## 7. 見積もり

合計 6〜8 日(実機確認込み)。最大の不確実性は P4 の再生性能。P2 完了時にベンチマークを前倒しし、未達なら fps 既定値を 8 に落とす判断を早期に行う。
