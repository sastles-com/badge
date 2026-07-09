# M5Dial バッヂプロジェクト 仕様検討ログ

日付: 2026-07-09
参加: katano / Claude (Fable 5)

## プロジェクト概要

M5Dial を「バッヂ」とみなしたガジェットを作る。

- M5Dial 上で Web アプリを立ち上げ、QR コードベースでスマホからアクセス
- 画像・動画をアップロードすると、M5Dial の円形 LCD でその画像・動画を再生
- 参考プロジェクト: `~/Documents/PlatformIO/Projects/splatoon`(AtomS3 Splatoon プレートプリンタ)
- 最終成果物: 計画書・仕様書・実装計画書・CLAUDE.md を作成し、Opus に実装させる

## 参考プロジェクト(splatoon)から流用できる資産

- WiFi softAP 起動 + Wi-Fi 設定 QR(`WIFI:T:...;S:...;P:...;;`)の LCD 表示(`M5.Display.qrcode`)
- キャプティブポータル(Android/iOS/Windows の検出 URL ハンドリング)
- WebServer によるチャンクアップロード(`/api/upload`)
- LittleFS 保存 + 「アップロード画像優先、なければ組み込み画像」の優先順位ロジック
- Web UI(単一 HTML、変換プレビュー付き)
- png2c.py(組み込みデフォルト画像の生成)
- PlatformIO + Arduino + M5Unified 構成

## ハードウェア制約(M5Dial)

- MCU: M5StampS3(ESP32-S3FN8)、**8MB Flash、PSRAM なし**
- LCD: 1.28 インチ円形 240×240(GC9A01)、タッチ、ロータリーエンコーダ、物理ボタン、ブザー、RFID(WS1850S)、RTC
- **SD カードなし** → 保存は LittleFS(約 6MB 確保可能)のみ
- H.264/MP4 のデコードは不可能 → 動画はブラウザ側でフレーム分解して転送する方式が現実解
  - 240×240 JPEG ≈ 15〜30KB/フレーム → 10fps で 20〜40 秒程度が保存上限の目安

## 決定事項

| 項目 | 決定 |
| --- | --- |
| 動画方式 | **ブラウザ側フレーム変換**。Web アプリで動画を 240×240 JPEG 連番に変換して転送、端末で MJPEG 的にループ再生 |
| コンテンツ管理 | アップロード時に複数選択可能にし、選択された画像・動画を**スライドショー的に連続再生**(SD なし、LittleFS 保存) |
| 開発環境 | **PlatformIO + Arduino + M5Unified**(splatoon 踏襲) |

## 検討中(未決定)

### Wi-Fi 接続方式

ユーザー意向: 「スマホが接続している Wi-Fi に M5Dial も接続する」など、気軽に接続できる方式がないか前例を調査したい。

候補:

- AP モード(splatoon 方式): 自己完結だがスマホのネット接続が切れる
- STA モード: 既存 Wi-Fi に参加。プロビジョニング(SSID/パスワードの渡し方)が課題
- 両対応

→ Web リサーチを実施。結果は下記。

### Wi-Fi 接続方式 調査結果(2026-07-09)

**有名 OSS(WLED / Tasmota / ESPHome)はほぼ全員ハイブリッド**:「初回 SoftAP ポータルで資格情報投入 → 以後 STA、繋がらなければ AP に自動フォールバック」が業界標準。

主要な知見:

1. **キャプティブポータルの自動ポップアップは機種依存で信頼できない**
   - iOS: 開くまで 30 秒かかる/開かない報告多数。ポータル内疑似ブラウザ(CNA)はファイルアップロードが不安定
   - Android: 「インターネットなし」判定で LTE にフォールバックし、AP に繋がっていても通信が ESP32 に届かない問題あり
   - → ポータルは「ブラウザでこの URL を開いて」の案内に留め、本番 UI は通常ブラウザで開かせるのが定石
2. **LCD がある機器の定石は「2 段階 QR」**
   - ① Wi-Fi 参加 QR(`WIFI:T:WPA;S:...;P:...;;`)→ iOS 11+/Android 10+ の標準カメラで読むだけで AP 参加
   - ② URL QR(`http://192.168.4.1/`)→ ブラウザを確実に開かせ、ポータル機種依存問題を回避
3. **STA プロビジョニングの代替手段は全滅気味**
   - SmartConfig: 専用アプリ必須+不安定 → 不採用
   - Improv BLE: ESPHome 標準だが BLE スタックの RAM 消費大。PSRAM なし M5Dial で Web サーバー+画像バッファと同居は危険 → 見送り
   - DPP (Easy Connect): iOS 非対応、Arduino 非対応 → 不採用
4. **mDNS (`badge.local`)**: iOS は完全対応、Android は 12 以降のみ。補助手段として併用可
5. **イベント会場では会場 Wi-Fi は当てにしない**(AP isolation でスマホ→バッヂ通信が遮断される)→ SoftAP 直結が唯一確実

**推奨**: ハイブリッド(SoftAP 主軸 + 設定で STA 参加可、STA 失敗時 AP 自動フォールバック)。誘導は LCD の 2 段階 QR。STA 時は取得 IP の URL QR + mDNS 併用。

## 追加決定事項(調査後の第 2 ラウンド)

| 項目 | 決定 |
| --- | --- |
| Wi-Fi 方式 | **ハイブリッド**。デフォルト SoftAP + 2 段階 QR。Web UI から STA 設定可、STA 失敗時は AP に自動フォールバック |
| 端末操作 | **ダイヤル = コンテンツ送り/戻し中心**。回すと手動モード → 数秒で自動再開。長押しで QR 表示、タッチで一時停止/再開 |
| 表示設定 | **Web UI で設定**(表示順・静止画秒数・動画ループ回数など)し端末に保存 |
| プロジェクト名 | **badge / M5Dial-Badge**(SSID: M5Dial-Badge、mDNS: badge.local) |

## 要確認事項への回答(第 3 ラウンド)

| 項目 | 決定 |
| --- | --- |
| AP パスワード | **不要(オープン AP)**。`WIFI:T:nopass;S:M5Dial-Badge;;` の QR で 1 タップ接続。第三者接続リスクは用途上許容 |
| ブザー音 | **あり**(デフォルト ON、設定で OFF 可) |
| STA 時の PIN 保護 | v1 では実装しない |
| 組み込みデフォルト画像 | 仮サンプルを生成(`assets/builtin.png`)。後日差し替え可 |

リポジトリ: <https://github.com/sastles-com/badge.git>

## 成果物

- [docs/PLAN.md](../docs/PLAN.md) — 計画書
- [docs/SPEC.md](../docs/SPEC.md) — 仕様書
- [docs/IMPLEMENTATION.md](../docs/IMPLEMENTATION.md) — 実装計画書
- [CLAUDE.md](../CLAUDE.md) — Opus 実装用ガイド

## 運用メモ

- 議論はこの `log/` フォルダに保存し、後で振り返れるようにする(ユーザー指示)
