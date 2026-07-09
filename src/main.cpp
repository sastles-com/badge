// M5Dial-Badge P0: 開発基盤スケルトン
//
// 目的:
//   1. M5Dial の初期化と "Hello Badge" 表示
//   2. エンコーダ回転値 / タッチ座標 / BtnA イベントをシリアルに出力
//   3. LittleFS のマウント確認(失敗時フォーマット)と空き容量表示
//
// この main.cpp は P0 のスケルトン。P1 以降で net_manager / web_server /
// player / ui_screens 等に機能別分割していく(docs/IMPLEMENTATION.md §2)。

#include <Arduino.h>
#include <LittleFS.h>
#include <M5Dial.h>

namespace {

// --- 定数(マジックナンバーはここに集約) ---
constexpr uint32_t kSerialBaud = 115200;
constexpr int kTextColorFg = TFT_WHITE;
constexpr int kBgColor = 0x0821;            // 濃紺(RGB565)
constexpr int kAccentColor = 0x051F;        // 明るい青
constexpr uint32_t kStatusIntervalMs = 1000;  // シリアルへの定期ステータス出力周期
constexpr bool kFormatLittleFsOnFail = true;  // マウント失敗時に自動フォーマットするか
constexpr const char* kLittleFsBasePath = "/littlefs";
constexpr const char* kLittleFsPartitionLabel = "littlefs";  // partitions.csv の Name と一致させる
constexpr uint8_t kLittleFsMaxOpenFiles = 10;

// --- 状態 ---
long g_last_encoder = 0;
uint32_t g_last_status_ms = 0;
uint32_t g_btn_press_ms = 0;
bool g_littlefs_ok = false;

// 画面中央に "Hello Badge" とサブテキストを描く(円形 240x240 の内接円内)。
void drawSplash() {
  auto& d = M5Dial.Display;
  d.fillScreen(kBgColor);

  // 装飾リング
  d.drawCircle(120, 120, 116, kAccentColor);
  d.drawCircle(120, 120, 112, kAccentColor);

  d.setTextDatum(middle_center);
  d.setTextColor(kTextColorFg, kBgColor);
  d.setTextSize(1);
  d.setFont(&fonts::Font4);
  d.drawString("Hello Badge", 120, 96);

  d.setFont(&fonts::Font2);
  d.setTextColor(kAccentColor, kBgColor);
  d.drawString("M5Dial-Badge P0", 120, 128);
}

// LittleFS の使用量 / 全体を画面下部に表示する。
void drawStorageLine() {
  auto& d = M5Dial.Display;
  d.setFont(&fonts::Font2);
  d.setTextDatum(middle_center);
  d.setTextColor(kTextColorFg, kBgColor);

  char line[48];
  if (g_littlefs_ok) {
    const size_t total = LittleFS.totalBytes();
    const size_t used = LittleFS.usedBytes();
    snprintf(line, sizeof(line), "FS %u/%u KB",
             static_cast<unsigned>(used / 1024),
             static_cast<unsigned>(total / 1024));
  } else {
    snprintf(line, sizeof(line), "FS mount FAILED");
  }
  d.drawString(line, 120, 156);
}

// LittleFS をマウント(必要ならフォーマット)し、結果をシリアルに出す。
void initLittleFs() {
  g_littlefs_ok = LittleFS.begin(kFormatLittleFsOnFail, kLittleFsBasePath,
                                 kLittleFsMaxOpenFiles, kLittleFsPartitionLabel);
  if (g_littlefs_ok) {
    Serial.printf("[FS] mounted: used=%u total=%u bytes\n",
                  static_cast<unsigned>(LittleFS.usedBytes()),
                  static_cast<unsigned>(LittleFS.totalBytes()));
  } else {
    Serial.println("[FS] mount FAILED");
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  // M5Dial.begin(cfg, enableEncoder, enableRFID)
  M5Dial.begin(cfg, true, false);

  Serial.begin(kSerialBaud);
  delay(200);
  Serial.println();
  Serial.println("=== M5Dial-Badge P0 ===");
  Serial.printf("[SYS] free heap: %u bytes\n", ESP.getFreeHeap());

  M5Dial.Display.setBrightness(120);
  drawSplash();

  initLittleFs();
  drawStorageLine();

  g_last_encoder = M5Dial.Encoder.read();
  Serial.println("[IN] rotate dial / touch screen / press button to test");
}

void loop() {
  M5Dial.update();

  // --- エンコーダ回転 ---
  const long enc = M5Dial.Encoder.read();
  if (enc != g_last_encoder) {
    Serial.printf("[ENC] value=%ld delta=%ld\n", enc, enc - g_last_encoder);
    g_last_encoder = enc;
    M5Dial.Speaker.tone(4000, 20);  // クリック音(P0 での動作確認用)
  }

  // --- タッチ ---
  auto t = M5Dial.Touch.getDetail();
  if (t.wasPressed()) {
    Serial.printf("[TOUCH] pressed x=%d y=%d\n", t.x, t.y);
  }
  if (t.wasReleased()) {
    Serial.printf("[TOUCH] released x=%d y=%d\n", t.x, t.y);
  }

  // --- BtnA(ダイヤル押し込み) ---
  if (M5Dial.BtnA.wasPressed()) {
    g_btn_press_ms = millis();
    Serial.println("[BTN] BtnA pressed");
  }
  if (M5Dial.BtnA.wasReleased()) {
    Serial.printf("[BTN] BtnA released (held %lu ms)\n",
                  static_cast<unsigned long>(millis() - g_btn_press_ms));
  }
  if (M5Dial.BtnA.pressedFor(2000)) {
    Serial.println("[BTN] BtnA long-press (>=2s)");
  }

  // --- 定期ステータス(ヒープ監視) ---
  const uint32_t now = millis();
  if (now - g_last_status_ms >= kStatusIntervalMs) {
    g_last_status_ms = now;
    Serial.printf("[SYS] uptime=%lus heap=%u\n",
                  static_cast<unsigned long>(now / 1000),
                  ESP.getFreeHeap());
  }
}
