// M5Dial-Badge P1: Wi-Fi + QR + Web 基盤
//
// setup/loop と状態機械の統括のみをここに置く。各機能は分割モジュールへ:
//   net_manager  SoftAP + DNS 全リダイレクト
//   web_server   仮ページ / キャプティブポータル案内 / /api/status
//   ui_screens   スライドショー仮画面 / 2 段階 QR / ステータス
//   input        ダイヤル / タッチ / BtnA(短押し・長押し 2 秒)

#include <Arduino.h>
#include <LittleFS.h>
#include <M5Dial.h>

#include "app_state.h"
#include "content_store.h"
#include "input.h"
#include "net_manager.h"
#include "player.h"
#include "ui_screens.h"
#include "web_server.h"

namespace {

// --- 定数 ---
constexpr uint32_t kSerialBaud = 115200;
constexpr uint8_t kBrightness = 120;
constexpr uint32_t kStatusHoldMs = 5000;   // STATUS 画面の自動復帰時間
constexpr uint32_t kHeapLogIntervalMs = 5000;

// LittleFS(P0 での知見: partitions.csv の Name と一致させる)
constexpr bool kFormatLittleFsOnFail = true;
constexpr const char* kLittleFsBasePath = "/littlefs";
constexpr const char* kLittleFsPartitionLabel = "littlefs";
constexpr uint8_t kLittleFsMaxOpenFiles = 10;

// --- 状態 ---
AppState g_app;
uint32_t g_last_heap_log_ms = 0;

void initLittleFs() {
  const bool ok = LittleFS.begin(kFormatLittleFsOnFail, kLittleFsBasePath,
                                 kLittleFsMaxOpenFiles, kLittleFsPartitionLabel);
  if (ok) {
    Serial.printf("[FS] mounted: used=%u total=%u bytes\n",
                  static_cast<unsigned>(LittleFS.usedBytes()),
                  static_cast<unsigned>(LittleFS.totalBytes()));
  } else {
    Serial.println("[FS] mount FAILED");
  }
}

// 現在のモードに応じて画面を描く。
void render() {
  switch (g_app.mode) {
    case AppMode::QR:
      ui_screens::drawQr(g_app.qr_page);
      break;
    case AppMode::STATUS:
      ui_screens::drawStatus();
      break;
    case AppMode::SLIDESHOW:
    default:
      player::renderCurrent();
      break;
  }
  g_app.needs_redraw = false;
}

void enterMode(AppMode mode) {
  g_app.mode = mode;
  g_app.needs_redraw = true;
}

// 入力イベントに応じて状態遷移する。
void handleInput(const input::Events& ev) {
  switch (g_app.mode) {
    case AppMode::SLIDESHOW:
      if (ev.btn_long) {
        g_app.qr_page = QrPage::WIFI;
        enterMode(AppMode::QR);
      } else if (ev.btn_short) {
        g_app.status_until_ms = millis() + kStatusHoldMs;
        enterMode(AppMode::STATUS);
      }
      break;

    case AppMode::QR:
      if (ev.btn_short) {
        enterMode(AppMode::SLIDESHOW);
      } else if (net_manager::mode() == WifiMode::AP &&
                 (ev.encoder_delta != 0 || ev.touch_tap)) {
        // AP 時のみ Wi-Fi QR ↔ URL QR を切り替え。
        g_app.qr_page =
            (g_app.qr_page == QrPage::WIFI) ? QrPage::URL : QrPage::WIFI;
        g_app.needs_redraw = true;
      }
      break;

    case AppMode::STATUS:
      if (ev.btn_short || ev.btn_long) {
        enterMode(AppMode::SLIDESHOW);
      }
      break;

    default:
      break;
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);  // enableEncoder=true, enableRFID=false

  Serial.begin(kSerialBaud);
  delay(200);
  Serial.println();
  Serial.println("=== M5Dial-Badge P2 ===");
  Serial.printf("[SYS] free heap: %u bytes\n", ESP.getFreeHeap());

  M5Dial.Display.setBrightness(kBrightness);

  initLittleFs();
  content_store::begin();
  player::begin();
  net_manager::begin();
  web_server::begin();
  input::begin();

  render();
  Serial.println("[SYS] ready");
}

void loop() {
  M5Dial.update();
  net_manager::loop();
  web_server::loop();

  const input::Events ev = input::poll();
  handleInput(ev);

  // アップロード完了 → 新着コンテンツを表示(SLIDESHOW に戻して再描画)。
  if (player::takePending()) {
    g_app.mode = AppMode::SLIDESHOW;
    g_app.needs_redraw = true;
  }

  // STATUS 画面の自動復帰。
  if (g_app.mode == AppMode::STATUS && millis() >= g_app.status_until_ms) {
    enterMode(AppMode::SLIDESHOW);
  }

  if (g_app.needs_redraw) {
    render();
  }

  const uint32_t now = millis();
  if (now - g_last_heap_log_ms >= kHeapLogIntervalMs) {
    g_last_heap_log_ms = now;
    Serial.printf("[SYS] uptime=%lus heap=%u\n",
                  static_cast<unsigned long>(now / 1000), ESP.getFreeHeap());
  }
}
