#include "net_manager.h"

#include <DNSServer.h>
#include <WiFi.h>

namespace net_manager {
namespace {

// --- 定数 ---
constexpr const char* kApSsid = "M5Dial-Badge";
constexpr const char* kApPassword = nullptr;  // オープン AP(パスワードなし)
constexpr uint8_t kApChannel = 1;
constexpr uint8_t kDnsPort = 53;
// IPAddress は constexpr 構築できないため関数内で生成する。

// --- 状態 ---
DNSServer g_dns;

}  // namespace

void begin() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1),
                    IPAddress(255, 255, 255, 0));

  // kApPassword == nullptr でオープン AP になる。
  const bool ap_ok = WiFi.softAP(kApSsid, kApPassword, kApChannel);
  const IPAddress ip = WiFi.softAPIP();
  if (ap_ok) {
    Serial.printf("[NET] SoftAP up: ssid=%s ip=%u.%u.%u.%u (open)\n",
                  kApSsid, ip[0], ip[1], ip[2], ip[3]);
  } else {
    Serial.println("[NET] SoftAP start FAILED");
  }

  // 全ドメインを自 IP に解決(キャプティブポータル)。
  g_dns.start(kDnsPort, "*", ip);
  Serial.println("[NET] DNS captive redirect started");
}

void loop() { g_dns.processNextRequest(); }

WifiMode mode() { return WifiMode::AP; }

const char* ssid() { return kApSsid; }

IPAddress ip() { return WiFi.softAPIP(); }

void ipString(char* out, size_t out_size) {
  const IPAddress a = WiFi.softAPIP();
  snprintf(out, out_size, "%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
}

bool isOpenAp() { return kApPassword == nullptr; }

void buildWifiQrPayload(char* out, size_t out_size) {
  // Wi-Fi QR フォーマット: WIFI:T:<auth>;S:<ssid>;P:<password>;;
  if (isOpenAp()) {
    snprintf(out, out_size, "WIFI:T:nopass;S:%s;;", kApSsid);
  } else {
    snprintf(out, out_size, "WIFI:T:WPA;S:%s;P:%s;;", kApSsid, kApPassword);
  }
}

void buildUrl(char* out, size_t out_size) {
  const IPAddress a = WiFi.softAPIP();
  snprintf(out, out_size, "http://%u.%u.%u.%u/", a[0], a[1], a[2], a[3]);
}

}  // namespace net_manager
