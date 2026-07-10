#include "web_upload.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "config_store.h"
#include "content_store.h"
#include "player.h"
#include "ui_screens.h"

namespace web_upload {
namespace {

// 空き容量がこれを下回る状態でのアップロードは拒否する(断片化/満杯対策)。
constexpr size_t kMinFreeBytes = 32 * 1024;
constexpr uint32_t kProgressIntervalMs = 200;  // 進捗描画の間引き

WebServer* s_server = nullptr;

// リクエスト単位の状態。
char s_id[content_store::kIdLen + 1] = {0};
File s_file;             // 現在書き込み中のパートのファイル
bool s_failed = false;
const char* s_error = "";
size_t s_image_bytes = 0;
size_t s_video_bytes = 0;
size_t s_received = 0;           // このリクエストで受信した総バイト
uint32_t s_last_progress_ms = 0;

void tempImagePath(char* out, size_t n) { snprintf(out, n, "/content/%s.jpg", s_id); }
void tempMjpgPath(char* out, size_t n) { snprintf(out, n, "/content/%s.mjp", s_id); }
void tempThumbPath(char* out, size_t n) { snprintf(out, n, "/content/%s.thm", s_id); }

// 書きかけファイルを削除する。
void cleanup() {
  if (s_file) s_file.close();
  if (s_id[0]) {
    char path[48];
    tempImagePath(path, sizeof(path));
    if (LittleFS.exists(path)) LittleFS.remove(path);
    tempMjpgPath(path, sizeof(path));
    if (LittleFS.exists(path)) LittleFS.remove(path);
    tempThumbPath(path, sizeof(path));
    if (LittleFS.exists(path)) LittleFS.remove(path);
  }
}

void fail(const char* msg) {
  s_failed = true;
  s_error = msg;
  if (s_file) s_file.close();
}

}  // namespace

void begin(WebServer* server) { s_server = server; }

void handleChunk() {
  HTTPUpload& up = s_server->upload();

  if (up.status == UPLOAD_FILE_START) {
    // 最初のパートでリクエストを初期化し、id を採番する。
    if (s_id[0] == '\0') {
      s_failed = false;
      s_error = "";
      s_image_bytes = 0;
      s_video_bytes = 0;
      s_received = 0;
      s_last_progress_ms = 0;
      if (content_store::freeBytes() < kMinFreeBytes) {
        fail("storage full");
        return;
      }
      if (!content_store::generateId(s_id, sizeof(s_id))) {
        fail("id allocation failed");
        return;
      }
    }
    if (s_failed) return;

    // パート名(image / video / thumb)で保存先を分ける。
    char path[48];
    if (up.name == "thumb") {
      tempThumbPath(path, sizeof(path));
    } else if (up.name == "video") {
      tempMjpgPath(path, sizeof(path));
    } else {
      tempImagePath(path, sizeof(path));
    }
    if (s_file) s_file.close();
    s_file = LittleFS.open(path, "w");
    if (!s_file) fail("open temp failed");

  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (s_failed || !s_file) return;
    const size_t w = s_file.write(up.buf, up.currentSize);
    if (w != up.currentSize) {
      fail("write failed (storage full?)");
      return;
    }
    if (up.name == "video") s_video_bytes += w;
    else if (up.name != "thumb") s_image_bytes += w;
    s_received += w;

    // アップロード中はスライドショーを止め、進捗を表示(間引き描画)。
    const uint32_t now = millis();
    if (now - s_last_progress_ms >= kProgressIntervalMs) {
      s_last_progress_ms = now;
      ui_screens::drawUploadProgress(s_received, up.totalSize);
    }

  } else if (up.status == UPLOAD_FILE_END) {
    if (s_file) s_file.close();

  } else if (up.status == UPLOAD_FILE_ABORTED) {
    fail("upload aborted");
    cleanup();
  }
}

void finishReset() { s_id[0] = '\0'; }

void handleFinish() {
  if (s_file) s_file.close();

  if (s_failed) {
    cleanup();
    ui_screens::drawError(s_error);  // 端末にもエラー表示(容量不足など)
    s_server->send(400, "text/plain", s_error);
    finishReset();
    return;
  }

  if (s_id[0] == '\0' || (s_image_bytes == 0 && s_video_bytes == 0)) {
    cleanup();
    s_server->send(400, "text/plain", "no content data");
    finishReset();
    return;
  }

  const String name = s_server->hasArg("name") ? s_server->arg("name") : String();
  bool ok;
  if (s_video_bytes > 0) {
    const uint8_t loops = s_server->hasArg("loops")
                              ? static_cast<uint8_t>(s_server->arg("loops").toInt())
                              : config_store::defaultLoops();
    ok = content_store::addVideo(s_id, name.c_str(), loops);
    Serial.printf("[UP] stored VIDEO id=%s bytes=%u loops=%u name=%s\n", s_id,
                  static_cast<unsigned>(s_video_bytes), loops, name.c_str());
  } else {
    ok = content_store::addImage(s_id, name.c_str(), config_store::defaultDurationS());
    Serial.printf("[UP] stored IMAGE id=%s bytes=%u name=%s\n", s_id,
                  static_cast<unsigned>(s_image_bytes), name.c_str());
  }

  if (!ok) {
    cleanup();
    s_server->send(500, "text/plain", "failed to register content");
    finishReset();
    return;
  }

  // 端末側に即反映(新着コンテンツを表示)。
  player::notifyNewContent();

  char json[64];
  snprintf(json, sizeof(json), "{\"id\":\"%s\"}", s_id);
  s_server->send(200, "application/json", json);
  finishReset();
}

}  // namespace web_upload
