#include "input.h"

#include <M5Dial.h>

namespace input {
namespace {

constexpr uint32_t kLongPressMs = 2000;  // BtnA 長押し判定

long g_last_encoder = 0;
uint32_t g_press_start_ms = 0;
bool g_long_handled = false;  // この押下で長押しを処理済みか

}  // namespace

void begin() { g_last_encoder = M5Dial.Encoder.read(); }

Events poll() {
  Events ev;

  // --- エンコーダ ---
  const long enc = M5Dial.Encoder.read();
  if (enc != g_last_encoder) {
    ev.encoder_delta = static_cast<int>(enc - g_last_encoder);
    g_last_encoder = enc;
  }

  // --- BtnA(短押し / 長押し 2 秒) ---
  if (M5Dial.BtnA.wasPressed()) {
    g_press_start_ms = millis();
    g_long_handled = false;
  }
  if (M5Dial.BtnA.isPressed() && !g_long_handled) {
    if (millis() - g_press_start_ms >= kLongPressMs) {
      g_long_handled = true;
      ev.btn_long = true;
    }
  }
  if (M5Dial.BtnA.wasReleased()) {
    if (!g_long_handled) {
      ev.btn_short = true;
    }
  }

  // --- タッチ(タップ) ---
  auto t = M5Dial.Touch.getDetail();
  if (t.wasReleased()) {
    ev.touch_tap = true;
  }

  return ev;
}

}  // namespace input
