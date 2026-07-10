#include "config_store.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <M5Dial.h>

namespace config_store {
namespace {

constexpr const char* kConfigPath = "/config.json";
constexpr uint8_t kMinBrightness = 10;  // 消灯防止の下限

Values g_v;

void clamp() {
  if (g_v.brightness < kMinBrightness) g_v.brightness = kMinBrightness;
  if (g_v.default_duration_s < 1) g_v.default_duration_s = 1;
  if (g_v.default_duration_s > 600) g_v.default_duration_s = 600;
  if (g_v.default_loops < 1) g_v.default_loops = 1;
}

void save() {
  StaticJsonDocument<256> doc;
  doc["brightness"] = g_v.brightness;
  doc["default_duration_s"] = g_v.default_duration_s;
  doc["default_loops"] = g_v.default_loops;
  doc["buzzer"] = g_v.buzzer;
  File f = LittleFS.open(kConfigPath, "w");
  if (!f) {
    Serial.println("[CFG] save open failed");
    return;
  }
  serializeJson(doc, f);
  f.close();
}

void load() {
  if (!LittleFS.exists(kConfigPath)) {
    Serial.println("[CFG] no config.json (defaults)");
    return;
  }
  File f = LittleFS.open(kConfigPath, "r");
  if (!f) return;
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, f)) {
    f.close();
    Serial.println("[CFG] parse error (defaults)");
    return;
  }
  f.close();
  g_v.brightness = doc["brightness"] | g_v.brightness;
  g_v.default_duration_s = doc["default_duration_s"] | g_v.default_duration_s;
  g_v.default_loops = doc["default_loops"] | g_v.default_loops;
  g_v.buzzer = doc["buzzer"] | g_v.buzzer;
  clamp();
  Serial.printf("[CFG] loaded: bright=%u dur=%u loops=%u buzzer=%d\n",
                g_v.brightness, g_v.default_duration_s, g_v.default_loops, g_v.buzzer);
}

}  // namespace

void begin() {
  load();
  applyBrightness();
}

const Values& get() { return g_v; }
uint8_t brightness() { return g_v.brightness; }
uint16_t defaultDurationS() { return g_v.default_duration_s; }
uint8_t defaultLoops() { return g_v.default_loops; }
bool buzzer() { return g_v.buzzer; }

void applyBrightness() { M5Dial.Display.setBrightness(g_v.brightness); }

bool applyJson(const char* body, size_t len) {
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, body, len)) return false;
  if (doc.containsKey("brightness")) g_v.brightness = doc["brightness"];
  if (doc.containsKey("default_duration_s")) g_v.default_duration_s = doc["default_duration_s"];
  if (doc.containsKey("default_loops")) g_v.default_loops = doc["default_loops"];
  if (doc.containsKey("buzzer")) g_v.buzzer = doc["buzzer"];
  clamp();
  applyBrightness();
  save();
  return true;
}

bool toJson(char* out, size_t out_size) {
  StaticJsonDocument<256> doc;
  doc["brightness"] = g_v.brightness;
  doc["default_duration_s"] = g_v.default_duration_s;
  doc["default_loops"] = g_v.default_loops;
  doc["buzzer"] = g_v.buzzer;
  const size_t n = serializeJson(doc, out, out_size);
  return n > 0 && n < out_size;
}

}  // namespace config_store
