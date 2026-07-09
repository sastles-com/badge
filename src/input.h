// input: ダイヤル / タッチ / BtnA のイベント化
//
// BtnA の短押し・長押し(2 秒)判定は splatoon から移植。
// loop() から poll() を毎回呼び、返ったイベントで状態遷移する。

#pragma once

#include <Arduino.h>

namespace input {

// 1 周期分の入力イベント。
struct Events {
  int encoder_delta = 0;  // ダイヤル回転量(符号付き)
  bool btn_short = false; // BtnA 短押し(離した瞬間、長押し未成立時)
  bool btn_long = false;  // BtnA 長押し 2 秒成立(押下中に一度だけ true)
  bool touch_tap = false; // 画面タップ(離した瞬間)
};

// 初期化(エンコーダ基準値の取り込み)。M5Dial.begin() の後で呼ぶ。
void begin();

// M5Dial.update() の後で呼び、この周期のイベントを返す。
Events poll();

}  // namespace input
