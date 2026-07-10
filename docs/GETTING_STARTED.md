# はじめての方へ — クローンして Claude で続きを開発する手順

このプロジェクトを **GitHub から自分のパソコンにコピー(クローン)** して、
**Claude Code(AI ペアプログラマー)** と一緒に続きを開発するための手順書です。
プログラミングにあまり詳しくない方でも進められるよう、コマンドをそのまま貼り付けられる形で説明します。

> 対象 OS: **macOS**(この開発は Mac で行っています)。Windows の方は一部読み替えが必要です。

---

## 全体の流れ(3 ステップ)

1. 道具をそろえる(インストール)
2. プロジェクトをコピーする(クローン)
3. Claude Code を起動して続きを頼む

---

## ステップ 1: 道具をそろえる

「ターミナル」というアプリを開いて、下のコマンドを **1 行ずつ** 貼り付けて実行します。
(ターミナルは「Launchpad」→「その他」→「ターミナル」、または `⌘ + スペース` で「terminal」と検索)

### 1-1. Homebrew(ソフトを入れる道具)

```bash
# すでに入っているか確認(バージョンが出れば OK、次へ)
brew --version
```

入っていなければ、[Homebrew 公式サイト](https://brew.sh/index_ja) の手順でインストールしてください。

### 1-2. Git(プロジェクトをコピーする道具)

```bash
git --version   # 出れば OK。無ければ↓
brew install git
```

### 1-3. Python と PlatformIO(マイコンにプログラムを書き込む道具)

```bash
# Python（多くの Mac に最初から入っています）
python3 --version

# PlatformIO（ビルド・書き込みツール）
python3 -m pip install --user platformio
```

> インストール後、`pio` コマンドの場所が分からない場合は
> `~/Library/Python/3.9/bin/pio` のようなパスにあります(数字はバージョンで変わります)。

### 1-4. Claude Code(AI 開発アシスタント)

```bash
brew install --cask claude-code
```

または [Claude Code のドキュメント](https://docs.claude.com/en/docs/claude-code) の手順に従ってください。
初回起動時に Anthropic アカウントでのログインが必要です。

---

## ステップ 2: プロジェクトをコピーする(クローン)

作業用のフォルダを作って、その中にプロジェクトをコピーします。

```bash
# 好きな場所に作業フォルダを作る（例: ホーム直下の work）
mkdir -p ~/work
cd ~/work

# GitHub からコピー（URL はこのリポジトリのもの）
git clone https://github.com/sastles-com/badge.git

# コピーしたフォルダに移動
cd badge
```

---

## ステップ 3: Claude Code を起動して続きを頼む

プロジェクトのフォルダ(`~/work/badge`)の中で、次を実行します。

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

M5Dial を USB ケーブルで Mac につなぎます。

```bash
# つながっているポートを確認（cu.usbmodem... という名前が出る）
ls /dev/cu.usbmodem*

# ビルド（プログラムを組み立てる）
~/Library/Python/3.9/bin/pio run -e m5dial

# 書き込み（--upload-port は上で確認した名前に置き換える）
~/Library/Python/3.9/bin/pio run -e m5dial -t upload --upload-port /dev/cu.usbmodem101

# 動作ログを見る（Ctrl + C で終了）
~/Library/Python/3.9/bin/pio device monitor -b 115200
```

これらの操作も、Claude Code に「ビルドして書き込んで」と頼めば代わりにやってくれます。

---

## 変更を GitHub に反映する(コミット & プッシュ)

動作を確認できたら、変更を記録して GitHub に送ります。これも Claude に頼めます:

```
動作確認できたのでコミットしてプッシュして。
```

自分でやる場合はこちら:

```bash
git add -A
git commit -m "変更内容の説明"
git push origin main
```

---

## 困ったときのチェックリスト

| 症状 | 対処 |
| --- | --- |
| `pio` が「見つからない」と言われる | フルパス `~/Library/Python/3.9/bin/pio` で実行してみる |
| 書き込みで「port is busy」 | シリアルモニタが開いていたら閉じる(`pkill -f "device monitor"`) |
| 書き込みで「No serial data received」 | 起動失敗中かも。G0(BOOT)ボタンを押しながら RST を押してから再実行(詳細は [log/2026-07-10-p2-image-upload.md](../log/2026-07-10-p2-image-upload.md)) |
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
