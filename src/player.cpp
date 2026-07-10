#include "player.h"

#include <LittleFS.h>
#include <M5Dial.h>

#include "content_store.h"
#include "ui_screens.h"

namespace player {
namespace {

constexpr uint32_t kManualRevertMs = 10000;  // 手動操作後の自動巡回復帰
constexpr uint16_t kDefaultDurationS = 5;
constexpr uint16_t kBeepFreq = 4000;
constexpr uint32_t kBeepMs = 20;

enum class Mode : uint8_t { AUTO, MANUAL };

int g_index = 0;
Mode g_mode = Mode::AUTO;
bool g_paused = false;
bool g_pending = false;
uint32_t g_item_shown_ms = 0;   // 現在項目を描画した時刻
uint32_t g_manual_until_ms = 0; // この時刻まで手動モード

void clampIndex() {
  const int n = content_store::count();
  if (n == 0) g_index = 0;
  else if (g_index >= n) g_index = n - 1;
  else if (g_index < 0) g_index = 0;
}

// 一時停止アイコン(小さな二本線)を上部中央に描く。
void drawPauseIcon() {
  auto& d = M5Dial.Display;
  const int cx = 120, y = 26, w = 5, h = 16, gap = 5;
  d.fillRect(cx - gap - w, y, w, h, TFT_WHITE);
  d.fillRect(cx + gap, y, w, h, TFT_WHITE);
}

}  // namespace

void begin() {
  g_index = 0;
  g_mode = Mode::AUTO;
  g_paused = false;
  g_item_shown_ms = millis();
}

void renderCurrent() {
  const int n = content_store::count();
  if (n == 0) {
    ui_screens::drawEmptyState();
    return;
  }
  clampIndex();

  content_store::Item it;
  if (!content_store::getByIndex(g_index, it)) {
    ui_screens::drawEmptyState();
    return;
  }

  char path[48];
  content_store::imagePath(it.id, path, sizeof(path));

  auto& d = M5Dial.Display;
  d.fillScreen(TFT_BLACK);
  if (!d.drawJpgFile(LittleFS, path, 0, 0)) {
    d.setTextDatum(middle_center);
    d.setTextColor(TFT_RED, TFT_BLACK);
    d.setFont(&fonts::lgfxJapanGothicP_16);
    d.drawString("画像を表示できません", 120, 120);
    Serial.printf("[PLR] drawJpgFile failed: %s\n", path);
  }
  if (g_paused) drawPauseIcon();

  g_item_shown_ms = millis();
}

void update() {
  const int n = content_store::count();
  if (n == 0 || g_paused) return;

  const uint32_t now = millis();

  if (g_mode == Mode::MANUAL) {
    if (now >= g_manual_until_ms) {
      g_mode = Mode::AUTO;
      g_item_shown_ms = now;  // AUTO 復帰時点から計時
    }
    return;
  }

  // AUTO: 現在項目の表示時間が過ぎたら次へ。
  content_store::Item it;
  if (!content_store::getByIndex(g_index, it)) return;
  const uint16_t dur = it.duration_s ? it.duration_s : kDefaultDurationS;
  if (now - g_item_shown_ms >= static_cast<uint32_t>(dur) * 1000) {
    g_index = (g_index + 1) % n;
    g_pending = true;
  }
}

void navigate(int delta) {
  const int n = content_store::count();
  if (n == 0 || delta == 0) return;
  const int step = (delta > 0) ? 1 : -1;
  g_index = (g_index + step + n) % n;
  g_mode = Mode::MANUAL;
  g_manual_until_ms = millis() + kManualRevertMs;
  g_pending = true;
  M5Dial.Speaker.tone(kBeepFreq, kBeepMs);
}

void togglePause() {
  g_paused = !g_paused;
  if (!g_paused) g_item_shown_ms = millis();  // 再開時は計時をリセット
  g_pending = true;
}

bool showById(const char* id) {
  content_store::Item it;
  int idx = -1;
  const int n = content_store::count();
  for (int i = 0; i < n; ++i) {
    if (content_store::getByIndex(i, it) && strcmp(it.id, id) == 0) {
      idx = i;
      break;
    }
  }
  if (idx < 0) return false;
  g_index = idx;
  g_mode = Mode::MANUAL;
  g_manual_until_ms = millis() + kManualRevertMs;
  g_paused = false;
  g_pending = true;
  return true;
}

void notifyNewContent() {
  const int n = content_store::count();
  g_index = (n > 0) ? (n - 1) : 0;
  g_mode = Mode::MANUAL;
  g_manual_until_ms = millis() + kManualRevertMs;
  g_pending = true;
}

void requestRedraw() {
  clampIndex();
  g_pending = true;
}

bool takePending() {
  if (!g_pending) return false;
  g_pending = false;
  return true;
}

}  // namespace player
