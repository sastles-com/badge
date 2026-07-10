# はじめての方へ — クローンして Claude で続きを開発する手順

このプロジェクトを **GitHub から自分のパソコンにコピー(クローン)** して、
**Claude Code(AI ペアプログラマー)** と一緒に続きを開発するための手順書です。
プログラミングにあまり詳しくない方でも進められるよう、コマンドをそのまま貼り付けられる形で説明します。

> **macOS と Windows の両方に対応** しています。各手順に 🍎 **Mac** と 🪟 **Windows** の
> 両方のやり方を書いています。自分の OS の方を見てください。

---

## 全体の流れ(3 ステップ)

1. 道具をそろえる(インストール)
2. プロジェクトをコピーする(クローン)
3. Claude Code を起動して続きを頼む

---

## ステップ 1: 道具をそろえる

まず、コマンドを打つための画面を開きます。

- 🍎 **Mac**: 「ターミナル」アプリ(`⌘ + スペース` で「ターミナル」と検索)
- 🪟 **Windows**: 「ターミナル」または「PowerShell」(スタートボタンを右クリック →「ターミナル」)

以下のコマンドを **1 行ずつ** 貼り付けて実行します。

### 1-1. パッケージ管理ツール(ソフトを入れる道具)

- 🍎 **Mac**: [Homebrew](https://brew.sh/index_ja)

  ```bash
  brew --version   # バージョンが出れば OK。無ければ公式サイトの手順でインストール
  ```

- 🪟 **Windows**: winget(Windows 10/11 に標準搭載)

  ```powershell
  winget --version   # バージョンが出れば OK
  ```

### 1-2. Git(プロジェクトをコピーする道具)

- 🍎 **Mac**

  ```bash
  git --version        # 出れば OK
  brew install git     # 無ければこれ
  ```

- 🪟 **Windows**

  ```powershell
  git --version              # 出れば OK
  winget install Git.Git     # 無ければこれ（入れた後ターミナルを開き直す）
  ```

### 1-3. Python と PlatformIO(マイコンにプログラムを書き込む道具)

- 🍎 **Mac**

  ```bash
  python3 --version
  python3 -m pip install --user platformio
  ```

  → `pio` の場所が分からないときは `~/Library/Python/3.9/bin/pio`(数字はバージョンで変化)。

- 🪟 **Windows**

  ```powershell
  winget install Python.Python.3.12   # 既に入っていれば不要
  python -m pip install --user platformio
  ```

  → `pio` が「見つからない」と言われたら、いったんターミナルを開き直してください。
  それでもダメなら `python -m platformio` と打てば同じことができます。

### 1-4. Claude Code(AI 開発アシスタント)

- 🍎 **Mac**

  ```bash
  brew install --cask claude-code
  ```

- 🪟 **Windows**: [Claude Code のドキュメント](https://docs.claude.com/en/docs/claude-code)
  の手順に従ってインストールしてください(Windows では WSL の利用を推奨する場合があります)。

初回起動時に Anthropic アカウントでのログインが必要です。

---

## ステップ 2: プロジェクトをコピーする(クローン)

作業用のフォルダを作って、その中にプロジェクトをコピーします。

- 🍎 **Mac**

  ```bash
  mkdir -p ~/work
  cd ~/work
  git clone https://github.com/sastles-com/badge.git
  cd badge
  ```

- 🪟 **Windows**(PowerShell)

  ```powershell
  mkdir $HOME\work -Force
  cd $HOME\work
  git clone https://github.com/sastles-com/badge.git
  cd badge
  ```

---

## ステップ 3: Claude Code を起動して続きを頼む

プロジェクトのフォルダ(`badge`)の中で、次を実行します。

```bash
claude
```

Claude Code が起動したら、日本語でお願いごとを書くだけです。まずはこう頼むのがおすすめです:

```
このプロジェクトの現状を教えて。CLAUDE.md と docs/、log/ を読んで、
今どこまで進んでいて、次に何をやればいいか説明して。
```

Claude が資料を読んで、進捗と次の作業(次のフェーズ)を教えてくれます。
続けて、例えばこう頼めます:

```
次のフェーズを実装して。実機の書き込みが必要なところは指示して。
```

> 💡 このプロジェクトには開発ルールが [CLAUDE.md](../CLAUDE.md) に書いてあり、
> Claude は自動でそれに従います(フェーズ順に進める・実機確認はユーザーに依頼する、など)。

---

## マイコン(M5Dial)につないで動かす

M5Dial を USB ケーブルでパソコンにつなぎます。まず接続先(ポート)を調べます。

```bash
# OS 共通: つながっている機器を一覧表示
pio device list
```

- 🍎 **Mac**: `/dev/cu.usbmodem101` のような名前が出ます。
- 🪟 **Windows**: `COM3` のような名前が出ます(「デバイスマネージャー」でも確認できます)。

ビルドと書き込み(`--upload-port` の値は上で調べた名前に置き換える):

- 🍎 **Mac**

  ```bash
  pio run -e m5dial                                              # ビルド
  pio run -e m5dial -t upload --upload-port /dev/cu.usbmodem101  # 書き込み
  pio device monitor -b 115200                                  # 動作ログ（Ctrl+C で終了）
  ```

- 🪟 **Windows**

  ```powershell
  pio run -e m5dial                                    # ビルド
  pio run -e m5dial -t upload --upload-port COM3       # 書き込み
  pio device monitor -b 115200                         # 動作ログ（Ctrl+C で終了）
  ```

これらの操作も、Claude Code に「ビルドして書き込んで」と頼めば代わりにやってくれます。

---

## 変更を GitHub に反映する(コミット & プッシュ)

動作を確認できたら、変更を記録して GitHub に送ります。これも Claude に頼めます:

```
動作確認できたのでコミットしてプッシュして。
```

自分でやる場合はこちら(OS 共通):

```bash
git add -A
git commit -m "変更内容の説明"
git push origin main
```

---

## 困ったときのチェックリスト

| 症状 | 対処 |
| --- | --- |
| `pio` が「見つからない」と言われる | ターミナルを開き直す。それでもダメなら Mac は `~/Library/Python/3.9/bin/pio`、Windows は `python -m platformio` で実行 |
| 書き込みで「port is busy」 | シリアルモニタを開いていたら閉じる(ログ画面で Ctrl+C) |
| 書き込みで「No serial data received」 | 起動失敗中かも。G0(BOOT)ボタンを押しながら RST を押してから再実行(詳細は [log/2026-07-10-p2-image-upload.md](../log/2026-07-10-p2-image-upload.md)) |
| ポート名が分からない | `pio device list` で確認(Mac は `/dev/cu.usbmodem…`、Windows は `COM…`) |
| つまずいた | そのままエラー文を Claude Code に貼って「これ直して」と頼む |

---

## もっと知りたい

| 資料 | 内容 |
| --- | --- |
| [README.md](../README.md) | プロジェクトの概要・使い方 |
| [docs/PLAN.md](PLAN.md) | 何を作るか・なぜその方針か |
| [docs/SPEC.md](SPEC.md) | 詳しい仕様(機能の正本) |
| [docs/IMPLEMENTATION.md](IMPLEMENTATION.md) | 作業のフェーズ分けとタスク |
| [log/](../log/) | これまでの開発記録・ハマりどころ |
