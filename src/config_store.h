// config_store: 表示設定の保持と永続化(/config.json)
//
// 明るさ / デフォルト表示秒数 / 動画ループ既定 / ブザー(SPEC 3.7)。

#pragma once

#include <Arduino.h>

namespace config_store {

struct Values {
  uint8_t brightness = 120;       // 0-255
  uint16_t default_duration_s = 5;
  uint8_t default_loops = 1;
  bool buzzer = true;
};

// /config.json を読み込み(無ければ既定)、明るさを適用する。M5Dial.begin() 後に呼ぶ。
void begin();

const Values& get();
uint8_t brightness();
uint16_t defaultDurationS();
uint8_t defaultLoops();
bool buzzer();

// LCD 明るさを現在値で適用する。
void applyBrightness();

// /api/config POST の JSON を反映して保存・適用する。
bool applyJson(const char* body, size_t len);

// 現在値を JSON にして out に書く。
bool toJson(char* out, size_t out_size);

}  // namespace config_store
