#include "player.h"

#include <LittleFS.h>
#include <M5Dial.h>

#include "content_store.h"
#include "ui_screens.h"

namespace player {
namespace {

int g_index = 0;       // 現在表示中のプレイリスト位置
bool g_pending = false; // 再描画要求(新着など)

void clampIndex() {
  const int n = content_store::count();
  if (n == 0) {
    g_index = 0;
  } else if (g_index >= n) {
    g_index = n - 1;
  } else if (g_index < 0) {
    g_index = 0;
  }
}

}  // namespace

void begin() { g_index = 0; }

void renderCurrent() {
  const int n = content_store::count();
  if (n == 0) {
    ui_screens::drawSlideshowPlaceholder();  // 空状態(P3 で組み込み画像に差し替え)
    return;
  }
  clampIndex();

  content_store::Item it;
  if (!content_store::getByIndex(g_index, it)) {
    ui_screens::drawSlideshowPlaceholder();
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
}

void notifyNewContent() {
  const int n = content_store::count();
  g_index = (n > 0) ? (n - 1) : 0;
  g_pending = true;
}

bool takePending() {
  if (!g_pending) return false;
  g_pending = false;
  return true;
}

void next() {
  const int n = content_store::count();
  if (n == 0) return;
  g_index = (g_index + 1) % n;
  g_pending = true;
}

void prev() {
  const int n = content_store::count();
  if (n == 0) return;
  g_index = (g_index - 1 + n) % n;
  g_pending = true;
}

}  // namespace player
