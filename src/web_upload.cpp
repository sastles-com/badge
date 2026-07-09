#include "web_upload.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "content_store.h"
#include "player.h"

namespace web_upload {
namespace {

// 空き容量がこれを下回る状態でのアップロードは拒否する(断片化/満杯対策)。
constexpr size_t kMinFreeBytes = 32 * 1024;

WebServer* s_server = nullptr;

// リクエスト単位の状態。
char s_id[content_store::kIdLen + 1] = {0};
File s_file;             // 現在書き込み中のパートのファイル
bool s_failed = false;
const char* s_error = "";
size_t s_image_bytes = 0;

void tempImagePath(char* out, size_t n) {
  snprintf(out, n, "/content/%s.jpg", s_id);
}
void tempThumbPath(char* out, size_t n) {
  snprintf(out, n, "/content/%s.thm", s_id);
}

// 書きかけファイルを削除する。
void cleanup() {
  if (s_file) s_file.close();
  if (s_id[0]) {
    char path[48];
    tempImagePath(path, sizeof(path));
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

    // パート名(image / thumb)で保存先を分ける。
    char path[48];
    if (up.name == "thumb") {
      tempThumbPath(path, sizeof(path));
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
    if (up.name != "thumb") s_image_bytes += w;

  } else if (up.status == UPLOAD_FILE_END) {
    if (s_file) s_file.close();

  } else if (up.status == UPLOAD_FILE_ABORTED) {
    fail("upload aborted");
    cleanup();
  }
}

void handleFinish() {
  if (s_file) s_file.close();

  if (s_failed) {
    cleanup();
    s_server->send(400, "text/plain", s_error);
    s_id[0] = '\0';
    return;
  }

  if (s_id[0] == '\0' || s_image_bytes == 0) {
    cleanup();
    s_server->send(400, "text/plain", "no image data");
    s_id[0] = '\0';
    return;
  }

  const String name = s_server->hasArg("name") ? s_server->arg("name") : String();
  if (!content_store::addImage(s_id, name.c_str(), 5)) {
    cleanup();
    s_server->send(500, "text/plain", "failed to register content");
    s_id[0] = '\0';
    return;
  }

  Serial.printf("[UP] stored id=%s bytes=%u name=%s\n", s_id,
                static_cast<unsigned>(s_image_bytes), name.c_str());

  // 端末側に即反映(新着コンテンツを表示)。
  player::notifyNewContent();

  char json[64];
  snprintf(json, sizeof(json), "{\"id\":\"%s\"}", s_id);
  s_server->send(200, "application/json", json);
  s_id[0] = '\0';
}

}  // namespace web_upload
