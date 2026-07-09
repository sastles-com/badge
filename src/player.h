// player: スライドショー状態機械 / 静止画表示
//
// P2 では単一画像の全画面表示のみ。巡回・動画は P3/P4 で拡張する。

#pragma once

namespace player {

// content_store::begin() の後で呼ぶ。現在位置を初期化する。
void begin();

// 現在のコンテンツ(なければ空状態画面)を LCD に描画する。
void renderCurrent();

// アップロード完了時に呼ぶ。最新コンテンツへ移動し、再描画要求を立てる。
void notifyNewContent();

// 再描画要求があれば true を返して消費する(main が SLIDESHOW を再描画する)。
bool takePending();

// P3 用: 次 / 前のコンテンツへ。
void next();
void prev();

}  // namespace player
