// net_manager: Wi-Fi 起動シーケンスとネットワーク情報の提供
//
// P1 では SoftAP(オープン AP)+ DNS 全リダイレクト(キャプティブポータル)。
// STA 試行 / フォールバック / mDNS は P5 で追加する。

#pragma once

#include <Arduino.h>
#include <IPAddress.h>

#include "app_state.h"

namespace net_manager {

// SoftAP を起動し、DNS 全リダイレクトを開始する。
void begin();

// DNS リクエスト処理。loop() から毎回呼ぶ。
void loop();

// 現在の Wi-Fi モード(P1 は常に AP)。
WifiMode mode();

// 接続先 SSID。
const char* ssid();

// 現在の IP アドレス。
IPAddress ip();

// IP を "192.168.4.1" 形式で out に書き込む。
void ipString(char* out, size_t out_size);

// オープン AP(パスワードなし)かどうか。
bool isOpenAp();

// Wi-Fi 参加 QR のペイロード("WIFI:T:nopass;S:...;;")を書き込む。
void buildWifiQrPayload(char* out, size_t out_size);

// Web UI の URL("http://192.168.4.1/")を書き込む。
void buildUrl(char* out, size_t out_size);

}  // namespace net_manager
