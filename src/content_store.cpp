#include "content_store.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <esp_random.h>

namespace content_store {
namespace {

constexpr const char* kPlaylistPath = "/playlist.json";
constexpr const char* kContentDir = "/content";
constexpr uint16_t kDefaultDurationS = 5;
constexpr size_t kJsonCapacity = 8192;  // 64 項目 + 余裕(固定サイズ)

Item g_items[kMaxItems];
int g_count = 0;

// JSON ドキュメントは 8KB と大きいためスタックに置かず静的確保する
// (loopTask のスタックは 8KB しかなくオーバーフローするため)。単一スレッドで再利用。
StaticJsonDocument<kJsonCapacity> g_doc;

void save() {
  g_doc.clear();
  JsonArray arr = g_doc.createNestedArray("items");
  for (int i = 0; i < g_count; ++i) {
    JsonObject o = arr.createNestedObject();
    o["id"] = g_items[i].id;
    o["type"] = g_items[i].type;
    o["name"] = g_items[i].name;
    if (strcmp(g_items[i].type, "video") == 0) {
      o["loops"] = g_items[i].loops;
    } else {
      o["duration_s"] = g_items[i].duration_s;
    }
  }

  File f = LittleFS.open(kPlaylistPath, "w");
  if (!f) {
    Serial.println("[CS] save: open failed");
    return;
  }
  serializeJson(g_doc, f);
  f.close();
}

void load() {
  g_count = 0;
  if (!LittleFS.exists(kPlaylistPath)) {
    Serial.println("[CS] no playlist.json (empty)");
    return;
  }
  File f = LittleFS.open(kPlaylistPath, "r");
  if (!f) {
    Serial.println("[CS] playlist.json open failed");
    return;
  }
  g_doc.clear();
  DeserializationError err = deserializeJson(g_doc, f);
  f.close();
  if (err) {
    Serial.printf("[CS] playlist parse error: %s\n", err.c_str());
    return;
  }

  for (JsonObject o : g_doc["items"].as<JsonArray>()) {
    if (g_count >= kMaxItems) break;
    Item& it = g_items[g_count];
    strlcpy(it.id, o["id"] | "", sizeof(it.id));
    strlcpy(it.type, o["type"] | "image", sizeof(it.type));
    strlcpy(it.name, o["name"] | "", sizeof(it.name));
    it.duration_s = o["duration_s"] | kDefaultDurationS;
    it.loops = o["loops"] | 1;
    if (it.id[0] != '\0') g_count++;
  }
  Serial.printf("[CS] loaded %d items\n", g_count);
}

int indexOf(const char* id) {
  for (int i = 0; i < g_count; ++i) {
    if (strcmp(g_items[i].id, id) == 0) return i;
  }
  return -1;
}

}  // namespace

void begin() {
  if (!LittleFS.exists(kContentDir)) {
    LittleFS.mkdir(kContentDir);
  }
  load();
}

int count() { return g_count; }

bool getByIndex(int index, Item& out) {
  if (index < 0 || index >= g_count) return false;
  out = g_items[index];
  return true;
}

bool getById(const char* id, Item& out) {
  const int i = indexOf(id);
  if (i < 0) return false;
  out = g_items[i];
  return true;
}

bool generateId(char* out, size_t out_size) {
  if (out_size < kIdLen + 1) return false;
  for (int attempt = 0; attempt < 8; ++attempt) {
    const uint32_t r = esp_random();
    snprintf(out, out_size, "%08x", r);
    if (indexOf(out) < 0) return true;
  }
  return false;
}

bool addImage(const char* id, const char* name, uint16_t duration_s) {
  if (g_count >= kMaxItems) {
    Serial.println("[CS] addImage: playlist full");
    return false;
  }
  if (indexOf(id) >= 0) return false;

  Item& it = g_items[g_count];
  strlcpy(it.id, id, sizeof(it.id));
  strlcpy(it.type, "image", sizeof(it.type));
  strlcpy(it.name, (name && name[0]) ? name : id, sizeof(it.name));
  it.duration_s = duration_s;
  it.loops = 1;
  g_count++;
  save();
  return true;
}

bool addVideo(const char* id, const char* name, uint8_t loops) {
  if (g_count >= kMaxItems) {
    Serial.println("[CS] addVideo: playlist full");
    return false;
  }
  if (indexOf(id) >= 0) return false;

  Item& it = g_items[g_count];
  strlcpy(it.id, id, sizeof(it.id));
  strlcpy(it.type, "video", sizeof(it.type));
  strlcpy(it.name, (name && name[0]) ? name : id, sizeof(it.name));
  it.duration_s = 0;
  it.loops = loops ? loops : 1;
  g_count++;
  save();
  return true;
}

// id に紐づく実ファイル(.jpg / .mjp / .thm)をすべて削除する。
void removeFiles(const char* id) {
  char path[48];
  imagePath(id, path, sizeof(path));
  if (LittleFS.exists(path)) LittleFS.remove(path);
  mjpgPath(id, path, sizeof(path));
  if (LittleFS.exists(path)) LittleFS.remove(path);
  thumbPath(id, path, sizeof(path));
  if (LittleFS.exists(path)) LittleFS.remove(path);
}

bool remove(const char* id) {
  const int i = indexOf(id);
  if (i < 0) return false;

  removeFiles(id);
  for (int k = i; k < g_count - 1; ++k) g_items[k] = g_items[k + 1];
  g_count--;
  save();
  return true;
}

bool applyPlaylistJson(const char* body, size_t len) {
  g_doc.clear();
  DeserializationError err = deserializeJson(g_doc, body, len);
  if (err) {
    Serial.printf("[CS] playlist POST parse error: %s\n", err.c_str());
    return false;
  }

  static Item new_items[kMaxItems];  // ~3KB。スタックを避けて静的確保。
  int new_count = 0;
  bool used[kMaxItems] = {false};

  for (JsonObject o : g_doc["items"].as<JsonArray>()) {
    const char* id = o["id"] | "";
    const int idx = indexOf(id);
    if (idx < 0 || used[idx] || new_count >= kMaxItems) continue;
    used[idx] = true;
    Item it = g_items[idx];
    if (o.containsKey("duration_s")) it.duration_s = o["duration_s"] | it.duration_s;
    if (o.containsKey("loops")) it.loops = o["loops"] | it.loops;
    new_items[new_count++] = it;
  }
  // POST に含まれなかった既存項目は末尾に維持(削除は DELETE で行う)。
  for (int i = 0; i < g_count; ++i) {
    if (!used[i] && new_count < kMaxItems) new_items[new_count++] = g_items[i];
  }

  for (int i = 0; i < new_count; ++i) g_items[i] = new_items[i];
  g_count = new_count;
  save();
  return true;
}

int clearAll() {
  char path[48];
  const int removed = g_count;
  for (int i = 0; i < g_count; ++i) {
    imagePath(g_items[i].id, path, sizeof(path));
    if (LittleFS.exists(path)) LittleFS.remove(path);
    mjpgPath(g_items[i].id, path, sizeof(path));
    if (LittleFS.exists(path)) LittleFS.remove(path);
    thumbPath(g_items[i].id, path, sizeof(path));
    if (LittleFS.exists(path)) LittleFS.remove(path);
  }
  g_count = 0;
  save();
  return removed;
}

void imagePath(const char* id, char* out, size_t out_size) {
  snprintf(out, out_size, "%s/%s.jpg", kContentDir, id);
}

void mjpgPath(const char* id, char* out, size_t out_size) {
  snprintf(out, out_size, "%s/%s.mjp", kContentDir, id);
}

void thumbPath(const char* id, char* out, size_t out_size) {
  snprintf(out, out_size, "%s/%s.thm", kContentDir, id);
}

size_t totalBytes() { return LittleFS.totalBytes(); }

size_t freeBytes() {
  const size_t total = LittleFS.totalBytes();
  const size_t used = LittleFS.usedBytes();
  return (total > used) ? (total - used) : 0;
}

bool toJson(char* out, size_t out_size) {
  g_doc.clear();
  JsonArray arr = g_doc.createNestedArray("items");
  for (int i = 0; i < g_count; ++i) {
    JsonObject o = arr.createNestedObject();
    o["id"] = g_items[i].id;
    o["type"] = g_items[i].type;
    o["name"] = g_items[i].name;
    if (strcmp(g_items[i].type, "video") == 0) {
      o["loops"] = g_items[i].loops;
    } else {
      o["duration_s"] = g_items[i].duration_s;
    }
  }
  const size_t n = serializeJson(g_doc, out, out_size);
  return n > 0 && n < out_size;
}

}  // namespace content_store
