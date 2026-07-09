// content_store: LittleFS 上のコンテンツ / プレイリストの管理
//
// /content/<id>.jpg  静止画  /content/<id>.thm サムネイル
// /playlist.json     順序・名前・種別・表示秒数(SPEC 3.4)
//
// プレイリストは起動時にメモリへ読み込み、変更のたびに再保存する。

#pragma once

#include <Arduino.h>

namespace content_store {

constexpr int kMaxItems = 64;   // プレイリスト上限(RAM 節約のため固定)
constexpr int kIdLen = 8;       // id は 8 桁 hex

struct Item {
  char id[kIdLen + 1];
  char type[6];        // "image" / "video"
  char name[33];
  uint16_t duration_s; // 静止画の表示秒数
  uint8_t loops;       // 動画のループ回数
};

// LittleFS マウント後に呼ぶ。playlist.json を読み込み、書きかけ temp を掃除する。
void begin();

int count();
bool getByIndex(int index, Item& out);
bool getById(const char* id, Item& out);

// 未使用の 8 桁 hex id を生成して out(9 バイト以上)に書く。
bool generateId(char* out, size_t out_size);

// 静止画をプレイリスト末尾に追加して保存する(実ファイルは web_upload が保存済み前提)。
bool addImage(const char* id, const char* name, uint16_t duration_s);

// コンテンツを削除(実ファイル + プレイリストから除去)。
bool remove(const char* id);

// パス生成: "/content/<id>.jpg" / ".thm"
void imagePath(const char* id, char* out, size_t out_size);
void thumbPath(const char* id, char* out, size_t out_size);

// 容量
size_t totalBytes();
size_t freeBytes();

// playlist.json を JSON 文字列として out に書く(/api/playlist 用)。
// 収まらなければ false。
bool toJson(char* out, size_t out_size);

}  // namespace content_store
