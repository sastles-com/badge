#include "player.h"

#include <LittleFS.h>
#include <M5Dial.h>

#include "config_store.h"
#include "content_store.h"
#include "mjpg_reader.h"
#include "ui_screens.h"

namespace player {
namespace {

constexpr uint32_t kManualRevertMs = 10000;  // 手動操作後の自動巡回復帰
constexpr uint16_t kDefaultDurationS = 5;
constexpr uint16_t kBeepFreq = 4000;
constexpr uint32_t kBeepMs = 20;
constexpr size_t kFrameBufMax = 48 * 1024;    // 最大フレームバッファ(起動時に一度確保)
constexpr uint8_t kMinFps = 1, kMaxFps = 30, kFallbackFps = 10;

enum class Mode : uint8_t { AUTO, MANUAL };

int g_index = 0;
Mode g_mode = Mode::AUTO;
bool g_paused = false;
bool g_pending = false;
uint32_t g_item_shown_ms = 0;   // 静止画を描画した時刻
uint32_t g_manual_until_ms = 0; // この時刻まで手動モード

// 動画再生状態
uint8_t* g_frame_buf = nullptr;
mjpg::Reader g_video;
bool g_is_video = false;
int g_loops_target = 1;
int g_loops_done = 0;
uint32_t g_next_frame_ms = 0;
uint16_t g_frame_interval_ms = 100;

void clampIndex() {
  const int n = content_store::count();
  if (n == 0) g_index = 0;
  else if (g_index >= n) g_index = n - 1;
  else if (g_index < 0) g_index = 0;
}

void drawPauseIcon() {
  auto& d = M5Dial.Display;
  const int cx = 120, y = 26, w = 5, h = 16, gap = 5;
  d.fillRect(cx - gap - w, y, w, h, TFT_WHITE);
  d.fillRect(cx + gap, y, w, h, TFT_WHITE);
}

void closeVideo() {
  if (g_is_video) {
    g_video.close();
    g_is_video = false;
  }
}

// 現在のフレームを 1 枚読み込んで描画する。成功で true。
bool drawNextVideoFrame() {
  size_t len = 0;
  if (!g_video.nextFrame(g_frame_buf, kFrameBufMax, len)) return false;
  M5Dial.Display.drawJpg(g_frame_buf, len, 0, 0);
  return true;
}

}  // namespace

void begin() {
  g_index = 0;
  g_mode = Mode::AUTO;
  g_paused = false;
  g_item_shown_ms = millis();
  if (!g_frame_buf) {
    g_frame_buf = static_cast<uint8_t*>(malloc(kFrameBufMax));
    if (!g_frame_buf) Serial.println("[PLR] frame buffer alloc failed (video disabled)");
  }
}

void renderCurrent() {
  closeVideo();

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

  auto& d = M5Dial.Display;

  if (strcmp(it.type, "video") == 0 && g_frame_buf) {
    char path[48];
    content_store::mjpgPath(it.id, path, sizeof(path));
    if (g_video.open(LittleFS, path)) {
      g_is_video = true;
      g_loops_target = it.loops ? it.loops : 1;
      g_loops_done = 0;
      const uint8_t fps =
          (g_video.header().fps < kMinFps || g_video.header().fps > kMaxFps)
              ? kFallbackFps
              : g_video.header().fps;
      g_frame_interval_ms = 1000 / fps;
      d.fillScreen(TFT_BLACK);
      g_video.rewind();
      drawNextVideoFrame();
      g_next_frame_ms = millis() + g_frame_interval_ms;
      return;
    }
    // 開けない場合はエラー表示にフォールバック。
    Serial.printf("[PLR] video open failed: %s\n", path);
  }

  // 静止画
  char path[48];
  content_store::imagePath(it.id, path, sizeof(path));
  d.fillScreen(TFT_BLACK);
  if (!d.drawJpgFile(LittleFS, path, 0, 0)) {
    d.setTextDatum(middle_center);
    d.setTextColor(TFT_RED, TFT_BLACK);
    d.setFont(&fonts::lgfxJapanGothicP_16);
    d.drawString("表示できません", 120, 120);
    Serial.printf("[PLR] drawJpgFile failed: %s\n", path);
  }
  g_item_shown_ms = millis();
}

// 動画再生 1 周終了時に次へ進むか、ループ/手動保持で先頭へ戻すか判断。
void advanceAfterVideoLoop() {
  const int n = content_store::count();
  const uint32_t now = millis();
  const bool infinite = (n == 1);  // 動画 1 件のみは無限ループ(SPEC 3.5)
  const bool manual_hold = (g_mode == Mode::MANUAL && now < g_manual_until_ms);

  g_loops_done++;
  if (infinite || manual_hold || g_loops_done < g_loops_target) {
    g_video.rewind();
    g_next_frame_ms = now + g_frame_interval_ms;
  } else {
    g_index = (g_index + 1) % n;
    g_pending = true;  // 次コンテンツを renderCurrent で開く
  }
}

void update() {
  const int n = content_store::count();
  if (n == 0 || g_paused) return;

  const uint32_t now = millis();

  if (g_is_video) {
    if (now >= g_next_frame_ms) {
      if (drawNextVideoFrame()) {
        g_next_frame_ms = now + g_frame_interval_ms;
      } else {
        advanceAfterVideoLoop();
      }
    }
    return;
  }

  // 静止画: 手動モード中は自動送りしない。
  if (g_mode == Mode::MANUAL) {
    if (now >= g_manual_until_ms) {
      g_mode = Mode::AUTO;
      g_item_shown_ms = now;
    }
    return;
  }
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
  g_paused = false;
  g_pending = true;
  if (config_store::buzzer()) M5Dial.Speaker.tone(kBeepFreq, kBeepMs);
}

void togglePause() {
  g_paused = !g_paused;
  if (g_paused) {
    drawPauseIcon();  // 現在の画面に重ねる(再描画しない)
  } else {
    const uint32_t now = millis();
    g_item_shown_ms = now;
    g_next_frame_ms = now;
    g_pending = true;  // アイコンを消すため再描画
  }
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
  g_paused = false;
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
