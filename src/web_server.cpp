#include "web_server.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>

#include "app_state.h"
#include "net_manager.h"

namespace web_server {
namespace {

constexpr uint16_t kHttpPort = 80;

WebServer g_server(kHttpPort);

// P1 の仮トップページ(P2 で gzip 済み Web UI に差し替える)。
const char kPlaceholderPage[] PROGMEM =
    "<!doctype html><html lang=\"ja\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>M5Dial-Badge</title>"
    "<style>body{font-family:sans-serif;margin:2em;line-height:1.6;color:#222}"
    "h1{font-size:1.4em}code{background:#eee;padding:.1em .3em;border-radius:3px}"
    "</style></head><body>"
    "<h1>M5Dial-Badge</h1>"
    "<p>接続できました。Web UI は準備中です(P2 で実装)。</p>"
    "<p><a href=\"/api/status\">/api/status</a></p>"
    "</body></html>";

// 接続性チェック URL などに返す案内ページ。
// キャプティブポータルの疑似ブラウザではなく、標準ブラウザで開くよう促す。
const char kCaptivePage[] PROGMEM =
    "<!doctype html><html lang=\"ja\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>M5Dial-Badge</title>"
    "<style>body{font-family:sans-serif;margin:2em;line-height:1.7;color:#222}"
    "a{font-size:1.2em}</style></head><body>"
    "<h1>M5Dial-Badge に接続中</h1>"
    "<p>ブラウザで下のリンクを開いてください。</p>"
    "<p><a href=\"http://192.168.4.1/\">http://192.168.4.1/</a></p>"
    "</body></html>";

void handleRoot() {
  g_server.send_P(200, "text/html; charset=utf-8", kPlaceholderPage);
}

// OS の接続性チェック URL 用。案内ページを 200 で返す(疑似ブラウザ対策)。
void handleCaptive() {
  g_server.send_P(200, "text/html; charset=utf-8", kCaptivePage);
}

void handleStatus() {
  char ip[16];
  net_manager::ipString(ip, sizeof(ip));

  const size_t total = LittleFS.totalBytes();
  const size_t used = LittleFS.usedBytes();
  const size_t freeBytes = (total > used) ? (total - used) : 0;
  const int content_count = 0;  // P2 で content_store から取得する

  char json[256];
  snprintf(json, sizeof(json),
           "{\"fw\":\"%s\",\"mode\":\"%s\",\"ssid\":\"%s\",\"ip\":\"%s\","
           "\"totalBytes\":%u,\"freeBytes\":%u,\"contentCount\":%d}",
           kFwVersion,
           net_manager::mode() == WifiMode::AP ? "AP" : "STA",
           net_manager::ssid(), ip,
           static_cast<unsigned>(total), static_cast<unsigned>(freeBytes),
           content_count);
  g_server.send(200, "application/json", json);
}

}  // namespace

void begin() {
  g_server.on("/", HTTP_GET, handleRoot);

  // OS のキャプティブポータル判定 URL。
  g_server.on("/generate_204", HTTP_GET, handleCaptive);              // Android
  g_server.on("/gen_204", HTTP_GET, handleCaptive);                   // Android 変種
  g_server.on("/hotspot-detect.html", HTTP_GET, handleCaptive);       // iOS/macOS
  g_server.on("/library/test/success.html", HTTP_GET, handleCaptive); // iOS/macOS
  g_server.on("/ncsi.txt", HTTP_GET, handleCaptive);                  // Windows
  g_server.on("/connecttest.txt", HTTP_GET, handleCaptive);           // Windows

  g_server.on("/api/status", HTTP_GET, handleStatus);

  // それ以外はすべて案内ページへ(DNS で自 IP に来た未知 URL を拾う)。
  g_server.onNotFound(handleCaptive);

  g_server.begin();
  Serial.printf("[WEB] HTTP server started on port %u\n", kHttpPort);
}

void loop() { g_server.handleClient(); }

}  // namespace web_server
