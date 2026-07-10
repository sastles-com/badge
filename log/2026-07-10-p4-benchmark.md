# 2026-07-10 P4 タスク1: 再生 fps ベンチマーク

CLAUDE.md「P4 は最初に必ず再生 fps のベンチマークを行い、結果を報告してから本実装に進む」
に対応。本実装の前に MJPG 再生性能を実機計測した。

## 方法

- `tools/mjpg_tool.py gen --src assets/builtin.png --frames 30 --fps 10 --quality 0.8`
  で 240×240 の実画像ベース MJPG を生成(30 フレーム、各 ~11.7KB)。
  → 合成グラデーション(~5KB)は圧縮が効きすぎて楽観的なため、実写に近い実画像で計測。
- `data/bench.mjpg` を `pio run -t uploadfs` で LittleFS に書き込み
  (`board_build.filesystem = littlefs` を platformio.ini に追加)。
- `src/mjpg_reader.{h,cpp}`(ヘッダ検証 + 逐次フレーム読み出し)と
  `src/video_bench.{h,cpp}`(起動時に /bench.mjpg があれば計測)を追加。
- 48KB フレームバッファを malloc し、30 フレーム × 3 周 = 90 フレームを
  「LittleFS 読み出し → drawJpg(デコード+描画)」で計測(micros 単位)。
- 計測は net_manager 起動前に実施(Wi-Fi/Web の負荷なしのクリーン計測)。

## 結果(実機 M5Dial / ESP32-S3、240×240)

| 項目 | 実測 |
| --- | --- |
| LittleFS 読み出し | 3.95 ms/フレーム |
| JPEG デコード + 描画 | 45.87 ms/フレーム |
| 合計 | 49.82 ms/フレーム |
| fps(デコード+描画のみ) | 21.8 |
| **fps(読み+デコード+描画)** | **20.1** |

- frames drawn=90 / fail=0 / max_frame=11756B。

## 判断

- **目標 10fps / 下限 8fps(SPEC 4)に対し約 2 倍の余裕**。fps 既定値を下げる必要なし。
- フレームは 240×240 固定でデコード時間は内容にほぼ依存しないため、実写でも同水準の見込み。
- Wi-Fi/Web と CPU を共有する実再生でも、フレーム間ウェイトで 10fps ペーシングすれば
  余裕を持って達成可能。→ **本実装(ブラウザ変換・端末再生)へ進む**。

## メモ / 後片付け

- `uploadfs` は LittleFS 全体を上書きするため、既存の /content と playlist.json は消える。
  実機動作確認時は再アップロードが必要。
- 現状 /bench.mjpg があると起動のたびにベンチが走る。**P4 本実装完了時に
  `video_bench::runIfPresent()` の呼び出し(と関連ファイル)を撤去する**。
- `data/bench.mjpg` は .gitignore 済み(350KB のバイナリはコミットしない。
  `tools/mjpg_tool.py` で再生成可能)。
