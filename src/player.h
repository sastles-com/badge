// player: スライドショー状態機械 / 静止画表示
//
// 巡回モード:
//   AUTO   … 各項目を duration_s 表示して自動で次へ
//   MANUAL … ダイヤル操作直後。10 秒無操作で AUTO に復帰
//   一時停止(pause)は AUTO/MANUAL と直交。タッチでトグル。
// 動画(MJPG)再生は P4 で追加する。

#pragma once

namespace player {

// content_store::begin() の後で呼ぶ。現在位置を初期化する。
void begin();

// 巡回タイミング処理。SLIDESHOW 中は毎ループ呼ぶ(自動送り / 手動復帰)。
void update();

// 現在のコンテンツ(なければ空状態画面)を LCD に描画する。
void renderCurrent();

// ダイヤル送り/戻し(delta の符号方向へ 1 項目)。手動モードに入る。
void navigate(int delta);

// 一時停止 / 再開のトグル(タッチ)。
void togglePause();

// 指定 id へ即ジャンプ(/api/show)。見つからなければ false。
bool showById(const char* id);

// アップロード完了時に呼ぶ。最新コンテンツへ移動し再描画要求を立てる。
void notifyNewContent();

// 現在位置を保ったまま再描画要求を立てる(削除・並べ替え後の反映)。
void requestRedraw();

// 再描画要求があれば true を返して消費する(main が SLIDESHOW を再描画)。
bool takePending();

}  // namespace player
