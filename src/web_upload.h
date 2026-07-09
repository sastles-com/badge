// web_upload: /api/upload の multipart チャンク受信
//
// ブラウザは 1 リクエストで 2 パートを送る:
//   image = 240x240 JPEG  /  thumb = 80x80 JPEG
// query: type=image、name=<任意>
// 受信完了で id を採番し content_store に登録、player へ新着通知する。

#pragma once

#include <WebServer.h>

namespace web_upload {

// WebServer への参照を保持する。web_server::begin() から呼ぶ。
void begin(WebServer* server);

// /api/upload のチャンクハンドラ(server.on の第 4 引数)。
void handleChunk();

// /api/upload の完了ハンドラ(server.on の第 3 引数)。
void handleFinish();

}  // namespace web_upload
