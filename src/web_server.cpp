#include "web_server.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <uri/UriBraces.h>

#include "app_state.h"
#include "content_store.h"
#include "net_manager.h"
#include "web_ui.h"
#include "web_upload.h"

namespace web_server {
namespace {

constexpr uint16_t kHttpPort = 80;

WebServer g_server(kHttpPort);

// 接続性チェック URL などに返す案内ページ。
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
  g_server.sendHeader("Content-Encoding", "gzip");
  g_server.send_P(200, "text/html; charset=utf-8",
                  reinterpret_cast<const char*>(kWebUiHtml), kWebUiHtmlLen);
}

void handleCaptive() {
  g_server.send_P(200, "text/html; charset=utf-8", kCaptivePage);
}

void handleStatus() {
  char ip[16];
  net_manager::ipString(ip, sizeof(ip));

  char json[256];
  snprintf(json, sizeof(json),
           "{\"fw\":\"%s\",\"mode\":\"%s\",\"ssid\":\"%s\",\"ip\":\"%s\","
           "\"totalBytes\":%u,\"freeBytes\":%u,\"contentCount\":%d}",
           kFwVersion,
           net_manager::mode() == WifiMode::AP ? "AP" : "STA",
           net_manager::ssid(), ip,
           static_cast<unsigned>(content_store::totalBytes()),
           static_cast<unsigned>(content_store::freeBytes()),
           content_store::count());
  g_server.send(200, "application/json", json);
}

void handlePlaylist() {
  // 大きめ(最大 ~6KB)なのでスタックではなく静的確保する。
  static char json[content_store::kMaxItems * 96 + 64];
  if (!content_store::toJson(json, sizeof(json))) {
    g_server.send(500, "text/plain", "playlist too large");
    return;
  }
  g_server.send(200, "application/json", json);
}

// サムネイル / 本体 JPEG のストリーミング配信。
void streamJpeg(const char* path) {
  if (!LittleFS.exists(path)) {
    g_server.send(404, "text/plain", "not found");
    return;
  }
  File f = LittleFS.open(path, "r");
  if (!f) {
    g_server.send(500, "text/plain", "open failed");
    return;
  }
  g_server.streamFile(f, "image/jpeg");
  f.close();
}

void handleThumb() {
  char path[48];
  content_store::thumbPath(g_server.pathArg(0).c_str(), path, sizeof(path));
  streamJpeg(path);
}

void handleDelete() {
  const String id = g_server.pathArg(0);
  if (content_store::remove(id.c_str())) {
    g_server.send(200, "text/plain", "deleted");
  } else {
    g_server.send(404, "text/plain", "not found");
  }
}

}  // namespace

void begin() {
  web_upload::begin(&g_server);

  g_server.on("/", HTTP_GET, handleRoot);

  // OS のキャプティブポータル判定 URL。
  g_server.on("/generate_204", HTTP_GET, handleCaptive);              // Android
  g_server.on("/gen_204", HTTP_GET, handleCaptive);                   // Android 変種
  g_server.on("/hotspot-detect.html", HTTP_GET, handleCaptive);       // iOS/macOS
  g_server.on("/library/test/success.html", HTTP_GET, handleCaptive); // iOS/macOS
  g_server.on("/ncsi.txt", HTTP_GET, handleCaptive);                  // Windows
  g_server.on("/connecttest.txt", HTTP_GET, handleCaptive);           // Windows

  g_server.on("/api/status", HTTP_GET, handleStatus);
  g_server.on("/api/playlist", HTTP_GET, handlePlaylist);
  g_server.on(UriBraces("/api/thumb/{}"), HTTP_GET, handleThumb);
  g_server.on(UriBraces("/api/content/{}"), HTTP_DELETE, handleDelete);

  // アップロード(multipart チャンク)。
  g_server.on("/api/upload", HTTP_POST, web_upload::handleFinish,
              web_upload::handleChunk);

  g_server.onNotFound(handleCaptive);

  g_server.begin();
  Serial.printf("[WEB] HTTP server started on port %u\n", kHttpPort);
}

void loop() { g_server.handleClient(); }

}  // namespace web_server
