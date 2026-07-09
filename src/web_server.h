// web_server: HTTP ルーティング / キャプティブポータル / REST API
//
// P1 では仮のトップページ、OS 接続性チェック URL への案内、/api/status。
// Web UI 本体(gzip HTML)・アップロード・プレイリスト API は P2 以降。

#pragma once

namespace web_server {

// WebServer を起動し、ルーティングを登録する。net_manager::begin() の後で呼ぶ。
void begin();

// クライアント処理。loop() から毎回呼ぶ。
void loop();

}  // namespace web_server
