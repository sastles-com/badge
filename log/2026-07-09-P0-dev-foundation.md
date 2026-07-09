# P0: 開発基盤 実装ログ

日付: 2026-07-09
担当: Claude (Opus 4.8)

## 実装内容

- `platformio.ini`: m5dial 環境(board=m5stack-stamps3、M5Dial ライブラリ、OTA なしパーティション)
- `partitions.csv`: app 3MB / LittleFS 約 4.9MB
- `src/main.cpp`(P0 スケルトン):
  1. M5Dial 初期化 + "Hello Badge" スプラッシュ表示(円形画面の内接円内に描画)
  2. エンコーダ回転 / タッチ press・release / BtnA press・release・長押し2秒 をシリアル出力
  3. LittleFS マウント(失敗時フォーマット)+ 使用量/全体を画面と シリアルに表示
  4. 定期ヒープ監視(1秒ごとに uptime/heap をシリアル出力)
  - エンコーダ回転時にクリック音(P0 の動作確認用)

## ビルド結果

- `pio run -e m5dial` 成功(57秒、ライブラリ初回取得含む)
- RAM: 6.8% (22300 / 327680 bytes)
- Flash: 16.4% (517113 / 3145728 bytes)

## 実機確認: 未実施(デバイス未接続)

`/dev/cu.usbmodem*` が見つからず、書き込み・実機確認は未実施。
M5Dial を USB 接続して以下を実行し、結果を本ログに追記すること。

```bash
# 書き込み(ポートは ls /dev/cu.* で確認)
/Users/katano/Library/Python/3.9/bin/pio run -e m5dial -t upload --upload-port /dev/cu.usbmodemXXXX

# シリアルモニタ
/Users/katano/Library/Python/3.9/bin/pio device monitor -b 115200
```

### 確認項目(チェックリスト)

- [ ] LCD に "Hello Badge" と "M5Dial-Badge P0"、下部に "FS xxx/xxxx KB" が表示される
- [ ] ダイヤルを回すと `[ENC] value=... delta=...` がシリアルに出る + クリック音が鳴る
- [ ] 画面をタッチすると `[TOUCH] pressed/released x=.. y=..` が出る
- [ ] ダイヤルを押し込むと `[BTN] BtnA pressed/released`、2秒押しで long-press が出る
- [ ] `[FS] mounted: used=.. total=..` が出る(mount FAILED でないこと)
- [ ] `[SYS] uptime=.. heap=..` が1秒ごとに出て、heap が急減しない

## メモ / 要検証

- M5Dial ライブラリの API 名(`M5Dial.Encoder.read()`, `M5Dial.Touch.getDetail()`,
  `M5Dial.BtnA`, `M5Dial.Speaker.tone()`)はビルドは通ったが実機挙動は要確認。
- タッチ座標が円形パネルで期待通りか(P3 のタップ一時停止で使うため)実機で確認したい。
