# 2026-07-10 P3: プレイリスト・スライドショー

## スコープ(IMPLEMENTATION.md P3)

1. player: 巡回状態機械(自動巡回 / 手動モード 10 秒復帰 / 一時停止)
2. input: ダイヤル送り/戻し、タッチで pause/resume
3. Web UI: サムネイル一覧、並べ替え、削除、duration 設定、「今すぐ表示」、容量バー
4. API: `/api/playlist` GET/POST、`/api/content/<id>` DELETE、`/api/thumb/<id>`、`/api/show/<id>`
5. 空状態画面(組み込み画像 + 案内)

## 追加・変更

| ファイル | 変更 |
| --- | --- |
| `player.{h,cpp}` | 巡回状態機械に刷新。AUTO/MANUAL + pause、navigate/togglePause/showById/requestRedraw |
| `content_store.{h,cpp}` | `applyPlaylistJson()` 追加(並べ替え・duration_s/loops 更新) |
| `ui_screens.{h,cpp}` | `drawEmptyState()` 追加(組み込み画像 + 案内) |
| `web_server.cpp` | `/api/playlist` POST、`/api/show/<id>` POST、削除・並べ替え後の再描画 |
| `web/index.html` | 一覧に ▲▼ 並べ替え / duration 入力 / 表示 / 削除、変更を自動 POST |
| `tools/make_builtin.py` | assets/builtin.png → `src/builtin_image.h`(240x240 JPEG PROGMEM) |
| `src/builtin_image.h` | 自動生成物(13327B JPEG) |
| `main.cpp` | SLIDESHOW 中に player::update()、ダイヤル→navigate、タッチ→togglePause |

## 巡回状態機械(player)

- **AUTO**: 現在項目の `duration_s`(既定 5 秒)経過で次へ(wrap)。描画時刻 `g_item_shown_ms`
  を renderCurrent で更新し、update() が経過を判定。
- **MANUAL**: ダイヤル操作/ジャンプ直後に入る。`millis()+10s` まで自動送りを止め、
  経過後に AUTO へ復帰(復帰時点から計時)。
- **pause**: AUTO/MANUAL と直交。タッチでトグル。一時停止中は上部中央に小さな pause
  アイコン(二本線)を表示。再開時に現在項目の計時をリセット。
- ダイヤル操作時はブザーで短いクリック音(config での無効化は P6)。

## API メモ

- `/api/playlist` POST: ボディ `{"items":[{id,duration_s?,loops?},...]}` を
  `applyPlaylistJson()` で適用。並び順を受信順に再構成し、POST に無い既存項目は末尾に維持
  (削除は DELETE 専用)。受信バッファ用の一時 Item 配列(~3KB)は static 確保。
- `/api/show/<id>` POST: `player::showById()` で MANUAL ジャンプ。
- 削除・並べ替え後は `player::requestRedraw()` で端末表示を即更新。

## 空状態画面

- `drawJpg(kBuiltinImage, len, 0, 0)` で組み込み画像を全画面表示し、下部に暗い帯 +
  「QR で画像を送ってね」「BtnA 長押しで QR」。
- builtin_image.h は make_builtin.py で生成(PIL 使用)。ビルドの pre スクリプトには
  せず生成物をコミットする(penv に PIL が無いためビルドを壊さない)。

## ビルド結果

- RAM 21.4%(70092 B)、Flash 37.7%(1185261 B)。
- 起動ヒープ 304KB、初期化後 244KB 空き。

## 起動ログ

```text
=== M5Dial-Badge P3 ===
[SYS] free heap: 304932 bytes
[FS] mounted: used=81920 total=5111808 bytes
[CS] loaded 2 items
[NET] SoftAP up: ssid=M5Dial-Badge ip=192.168.4.1 (open)
[WEB] HTTP server started on port 80
[SYS] ready
```

- **P2 の永続化を間接確認**: P2 でアップした 2 件が再起動後に `loaded 2 items` で復元。

## ユーザー指摘への対応(2026-07-10)

実機テスト中に 2 点の指摘:

1. **「全削除がない」** → `content_store::clearAll()` と `/api/clear`(POST)を追加。
   Web UI の一覧下部に「全削除」ボタン(確認ダイアログ付き)。全削除で端末は空状態画面に。
2. **「動画が選択できない」** → 動画は P4 の範囲(現状 UI は `accept="image/*"` で画像限定)。
   別途 P4 で対応(ベンチ実施済み。[2026-07-10-p4-benchmark.md](2026-07-10-p4-benchmark.md))。

## 実機確認(未完了 — ユーザー依頼中)

- [ ] 3 件以上で自動巡回(各 duration_s で切替)
- [ ] ダイヤルで次/前へ即切替 → 10 秒無操作で自動巡回に復帰
- [ ] タップで一時停止 ↔ 再開(pause アイコン表示)
- [ ] Web UI: ▲▼ 並べ替え・duration 変更・「表示」・削除が端末に反映
- [ ] コンテンツ全削除で空状態画面(組み込み画像 + 案内)が出る
