# 2026-07-10 P2: 画像アップロード・表示

## スコープ(IMPLEMENTATION.md P2)

1. web/index.html: ファイル選択 → canvas で 240×240 中央クロップ + JPEG 化 + 円形プレビュー
2. サムネイル(80×80)もブラウザ側で生成し同時アップロード
3. web_upload: multipart チャンク受信、`/content/<id>.jpg` 保存、容量チェック
4. content_store: id 採番、playlist.json 追記
5. player: 単一画像の全画面表示(drawJpgFile)
6. tools/build_webui.py 整備(extra_scripts でビルド前に自動生成)

## 追加・変更ファイル

| ファイル | 役割 |
| --- | --- |
| `web/index.html` | 単一ページ Web UI(容量バー / アップロード / 一覧 / 削除) |
| `tools/build_webui.py` | index.html → gzip → `src/web_ui.h`(PROGMEM)。PlatformIO pre スクリプト |
| `src/web_ui.h` | 自動生成物(7607B → gzip 3136B) |
| `content_store.{h,cpp}` | playlist.json の CRUD、id 採番、パス生成、容量取得 |
| `web_upload.{h,cpp}` | `/api/upload` の multipart チャンク受信(image + thumb 2 パート) |
| `player.{h,cpp}` | 現在コンテンツの全画面表示 / 空状態 / 新着通知 |
| `web_server.cpp` | Web UI 配信(gzip)、`/api/playlist`、`/api/thumb/<id>`、`/api/content/<id>` DELETE、upload 登録、status に件数追加 |
| `main.cpp` | content_store/player 初期化、SLIDESHOW で player::renderCurrent、新着で再描画 |
| `platformio.ini` | ArduinoJson 追加、extra_scripts=pre:tools/build_webui.py |

## 設計・実装メモ

- **アップロード形式**: 1 リクエストに `image`(240×240 JPEG)と `thumb`(80×80 JPEG)の
  2 パートを multipart で送信。サーバは最初のパート開始時に id を採番し、パート名で
  保存先(`.jpg` / `.thm`)を振り分ける。完了時に content_store へ登録して player に新着通知。
- **id 採番**: `esp_random()` の 32bit を 8 桁 hex 化。既存と衝突したら再試行。
- **Web UI 埋め込み**: gzip 圧縮して PROGMEM。`send_P(code, type, ptr, len)` +
  `Content-Encoding: gzip` ヘッダで配信。build_webui.py は生成物が変わらなければ
  書き込まず、不要な再コンパイルを避ける。`__file__` は SConscript で未定義のため
  PROJECT_DIR から解決する。
- **パスパラメータ**: `/api/thumb/{}` `/api/content/{}` は WebServer の `UriBraces` +
  `server.pathArg(0)` で取得。
- **画像表示**: `M5Dial.Display.drawJpgFile(LittleFS, path, 0, 0)`。240×240 なので等倍。

## ハマりどころ: StaticJsonDocument をスタックに置くとクラッシュ

- **症状**: 書き込み後にシリアル出力が一瞬(core dump 関連ログ)出た後に沈黙。
  ブートループに陥り、ネイティブ USB CDC が再列挙を繰り返して以降のログも取れない。
- **原因**: `StaticJsonDocument<8192>`(8KB)を content_store の関数ローカル
  (`load`/`save`/`toJson`)に確保していた。Arduino の loopTask スタックは既定 8KB のため、
  8KB のドキュメントで**スタックオーバーフロー**して起動時にクラッシュしていた。
- **対処**: JSON ドキュメントをファイル静的(BSS)に 1 つ確保して使い回す
  (単一スレッドのため安全)。web_server の playlist 用バッファ(~6KB)も `static` 化。
- **教訓**: PSRAM なし・小スタック環境では、KB 単位の一時バッファ/ドキュメントを
  スタックに置かない。大きい固定バッファは static / グローバルに確保する。

## ブートループからの復旧(ネイティブ USB CDC)

- ブートループ中は esptool の自動リセットで同期できず
  `Failed to connect ... No serial data received` となる。
- **手動ダウンロードモード**: G0(BOOT)を押しながら RST を一度押す(または G0 を
  押したまま USB を挿す)。`boot:0x0 (DOWNLOAD)` に入れば esptool で書き込める。
- 書き込み後は `esptool --after hard_reset run` でアプリ起動。
  ※ pyserial の DTR/RTS を手で叩くと逆にダウンロードモードへ入ることがあるため、
  通常起動は esptool の hard_reset に任せるのが確実。

## ビルド結果

- RAM 16.0%(52276 / 327680 B)、Flash 36.6%(1150273 / 3145728 B)。
- 起動ヒープ 309KB、初期化後 249KB 空き。

## 起動ログ(修正後)

```text
=== M5Dial-Badge P2 ===
[SYS] free heap: 309124 bytes
[FS] mounted: used=16384 total=5111808 bytes
[CS] no playlist.json (empty)
[NET] SoftAP up: ssid=M5Dial-Badge ip=192.168.4.1 (open)
[NET] DNS captive redirect started
[WEB] HTTP server started on port 80
[SYS] ready
```

※ 空デバイスでは playlist.json 未存在で VFS が `[E] open()` を 1 回出すが、
  arduino-esp32 の `exists()`/VFS 層由来の無害なログ。コンテンツが 1 件できれば消える。

## 実機確認(2026-07-10)

- [x] スマホで AP 接続 → Web UI が開く(アップロードできたので到達確認)
- [x] 写真を選択 → 円形プレビュー表示 → アップロード → LCD に即表示される
- [ ] 複数枚アップロードでき、一覧にサムネイルが出る(未個別確認)
- [ ] 一覧から削除するとファイルとプレイリストから消える(未個別確認)
- [ ] 再起動後もアップロード済み画像が表示される(playlist.json 永続化・未個別確認)

→ 核心(アップロード → LCD 即表示)は OK。一覧・削除・永続化は P3 の巡回実装時に
  あわせて確認する。P2 の主目的は達成のためコミットする。
