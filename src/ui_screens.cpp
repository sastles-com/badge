#include "ui_screens.h"

#include <LittleFS.h>
#include <M5Dial.h>

#include "builtin_image.h"
#include "net_manager.h"

namespace ui_screens {
namespace {

// --- 配色(RGB565) ---
constexpr int kBgColor = 0x0821;      // 濃紺
constexpr int kFgColor = TFT_WHITE;
constexpr int kAccentColor = 0x051F;  // 明るい青

// --- レイアウト ---
constexpr int kCx = 120;              // 画面中心 X
constexpr int kQrSize = 160;          // QR 一辺(内接正方形に収める)
constexpr int kQrX = (240 - kQrSize) / 2;
constexpr int kQrY = 40;
constexpr uint8_t kQrVersion = 4;     // 33x33 モジュール。Wi-Fi/URL 両方に十分
constexpr int kTopCaptionY = 16;
constexpr int kBottomCaptionY = 216;

void setJpFont() { M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_16); }

}  // namespace

void drawSlideshowPlaceholder() {
  auto& d = M5Dial.Display;
  d.fillScreen(kBgColor);
  d.drawCircle(kCx, kCx, 116, kAccentColor);
  d.drawCircle(kCx, kCx, 112, kAccentColor);

  d.setTextDatum(middle_center);
  d.setTextColor(kFgColor, kBgColor);
  d.setFont(&fonts::Font4);
  d.drawString("M5Dial-Badge", kCx, 100);

  setJpFont();
  d.setTextColor(kAccentColor, kBgColor);
  d.drawString("BtnA 長押しで QR", kCx, 140);
  d.setTextColor(kFgColor, kBgColor);
  d.drawString("BtnA 短押しで情報", kCx, 162);
}

void drawEmptyState() {
  auto& d = M5Dial.Display;
  d.fillScreen(TFT_BLACK);
  // 組み込みデフォルト画像(240x240 JPEG)を全画面表示。
  if (!d.drawJpg(kBuiltinImage, kBuiltinImageLen, 0, 0)) {
    d.fillScreen(kBgColor);
  }

  // 下部に案内を重ねる(円内に収まる位置に暗い帯)。
  d.fillRect(0, 186, 240, 40, kBgColor);
  setJpFont();
  d.setTextDatum(middle_center);
  d.setTextColor(kFgColor, kBgColor);
  d.drawString("QR で画像を送ってね", kCx, 198);
  d.setTextColor(kAccentColor, kBgColor);
  d.drawString("BtnA 長押しで QR", kCx, 216);
}

void drawQr(QrPage page) {
  auto& d = M5Dial.Display;
  d.fillScreen(kBgColor);

  char payload[96];
  const char* top_caption = nullptr;
  char bottom_caption[48];

  if (page == QrPage::WIFI) {
    net_manager::buildWifiQrPayload(payload, sizeof(payload));
    top_caption = "1/2 Wi-Fi に接続";
    snprintf(bottom_caption, sizeof(bottom_caption), "SSID: %s",
             net_manager::ssid());
  } else {
    net_manager::buildUrl(payload, sizeof(payload));
    top_caption = "2/2 ブラウザで開く";
    net_manager::buildUrl(bottom_caption, sizeof(bottom_caption));
  }

  // QR 本体(白地・内接正方形)
  d.qrcode(payload, kQrX, kQrY, kQrSize, kQrVersion);

  setJpFont();
  d.setTextDatum(middle_center);
  d.setTextColor(kAccentColor, kBgColor);
  d.drawString(top_caption, kCx, kTopCaptionY);
  d.setTextColor(kFgColor, kBgColor);
  d.drawString(bottom_caption, kCx, kBottomCaptionY);
}

void drawStatus() {
  auto& d = M5Dial.Display;
  d.fillScreen(kBgColor);
  d.drawCircle(kCx, kCx, 116, kAccentColor);

  char ip[16];
  net_manager::ipString(ip, sizeof(ip));
  const size_t total = LittleFS.totalBytes();
  const size_t used = LittleFS.usedBytes();
  const unsigned free_kb =
      static_cast<unsigned>((total > used ? total - used : 0) / 1024);

  setJpFont();
  d.setTextDatum(middle_center);

  d.setTextColor(kAccentColor, kBgColor);
  d.drawString("ステータス", kCx, 44);

  d.setTextColor(kFgColor, kBgColor);
  char line[48];
  snprintf(line, sizeof(line), "モード: %s",
           net_manager::mode() == WifiMode::AP ? "AP" : "STA");
  d.drawString(line, kCx, 84);

  snprintf(line, sizeof(line), "SSID: %s", net_manager::ssid());
  d.drawString(line, kCx, 108);

  snprintf(line, sizeof(line), "IP: %s", ip);
  d.drawString(line, kCx, 132);

  snprintf(line, sizeof(line), "空き: %u KB", free_kb);
  d.drawString(line, kCx, 156);

  snprintf(line, sizeof(line), "コンテンツ: %d 件", 0);  // P2 で実数に
  d.drawString(line, kCx, 180);
}

}  // namespace ui_screens
