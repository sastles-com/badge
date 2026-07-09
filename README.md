# M5Dial-Badge

M5Dial を「電子バッヂ」にするプロジェクトです。

胸元や机の上に置いた M5Dial の円形 LCD に、好きな画像や短い動画をループ表示します。表示するコンテンツはスマホから差し替えられます — M5Dial 本体が Web アプリを提供しており、LCD に表示される QR コードを標準カメラで読むだけでアクセスできます。PC・専用アプリ・クラウドは不要です。

![組み込みデフォルト画像](assets/builtin.png)

## 使い方(イメージ)

1. M5Dial の電源を入れる(スライドショーが始まる)
2. ボタンを 2 秒長押しすると QR コードが表示される
3. スマホのカメラで **① Wi-Fi QR** を読む → タップして `M5Dial-Badge` に接続(パスワード不要)
4. ダイヤルを回して **② URL QR** を読む → ブラウザで Web アプリが開く
5. 画像・動画を選んでアップロード → 変換はブラウザ内で自動処理され、LCD にすぐ反映される

- **ダイヤル回転**: コンテンツの送り/戻し
- **タッチ**: 一時停止 / 再開
- **ボタン短押し**: ステータス表示(Wi-Fi モード / IP / 空き容量)

自宅では Web アプリの設定画面から家の Wi-Fi に参加させることもできます(STA モード)。接続に失敗した場合は自動で AP モードに戻ります。

## 主な特徴

- **QR 2 段階誘導**: キャプティブポータルの機種依存問題(iOS/Android で開かない・LTE に逃げる)を回避し、カメラで QR を 2 回読むだけの確実な導線
- **動画対応**: ESP32 では H.264 をデコードできないため、ブラウザ側で動画を 240×240 の JPEG 連番(独自 MJPG コンテナ)に変換して転送し、端末でループ再生
- **複数コンテンツのスライドショー**: LittleFS(約 5MB)に複数の画像・動画を保存し、Web アプリで順序・表示秒数・ループ回数を管理

## ハードウェア

| 項目 | 内容 |
| --- | --- |
| 本体 | [M5Stack M5Dial](https://docs.m5stack.com/en/core/M5Dial)(M5StampS3 / ESP32-S3FN8) |
| ディスプレイ | 1.28 インチ円形 240×240(GC9A01)、タッチ対応 |
| 制約 | PSRAM なし・SD カードなし・Flash 8MB |

## ドキュメント

| ドキュメント | 内容 |
| --- | --- |
| [docs/PLAN.md](docs/PLAN.md) | 計画書 — 目的・スコープ・技術判断の理由・マイルストーン |
| [docs/SPEC.md](docs/SPEC.md) | 仕様書 — Wi-Fi 動作・QR・コンテナ形式・REST API・画面・操作 |
| [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md) | 実装計画書 — ソース構成・フェーズ別タスク・テスト計画 |
| [CLAUDE.md](CLAUDE.md) | AI 実装エージェント向けガイド |
| [log/](log/) | 仕様検討の経緯(Wi-Fi 方式の前例調査など) |

## 開発

PlatformIO + Arduino + [M5Dial ライブラリ](https://github.com/m5stack/M5Dial)(M5Unified ベース)で開発します。

```bash
# ビルド
pio run -e m5dial

# 書き込み(ポートは ls /dev/cu.usbmodem* で確認)
pio run -e m5dial -t upload --upload-port /dev/cu.usbmodem1101

# シリアルモニタ
pio device monitor -b 115200
```

実装は [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md) のフェーズ P0(開発基盤)〜 P6(仕上げ)の順に進めます。

## ステータス

- [x] 仕様策定(2026-07-09)
- [ ] P0: 開発基盤
- [ ] P1: Wi-Fi + QR + Web 基盤
- [ ] P2: 画像アップロード・表示
- [ ] P3: プレイリスト・スライドショー
- [ ] P4: 動画対応
- [ ] P5: STA ハイブリッド
- [ ] P6: 仕上げ・配布物

## 参考

- 先行プロジェクト: AtomS3 Splatoon プレートプリンタ(SoftAP + QR + Web アップロードの原型)
- [WLED](https://kno.wled.ge/basics/getting-started/) / [Tasmota](https://tasmota.github.io/docs/) — Wi-Fi ハイブリッド構成(AP フォールバック)の参考
