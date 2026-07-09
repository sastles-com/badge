// M5Dial-Badge 全体状態
//
// アプリのモード遷移と、モジュール間で共有する状態をここに集約する。
// 実際の状態遷移ロジックは main.cpp が持つ。

#pragma once

#include <Arduino.h>

// ファームウェアバージョン(/api/status などで返す)
constexpr const char* kFwVersion = "0.1.0";

// アプリのモード(LCD が今どの画面を出しているか)。
// P1 では SLIDESHOW(仮)/ QR / STATUS を使う。
// UPLOADING / ERROR は P2 以降で使用する。
enum class AppMode : uint8_t {
  SLIDESHOW,
  QR,
  STATUS,
  UPLOADING,
  ERROR,
};

// Wi-Fi 動作モード。P1 は AP 固定、STA は P5 で追加。
enum class WifiMode : uint8_t {
  AP,
  STA,
};

// QR 画面のページ(AP モード時のみ 2 段階)。
enum class QrPage : uint8_t {
  WIFI = 0,  // Wi-Fi 参加 QR
  URL = 1,   // ブラウザで開く URL QR
};

// モジュール間で共有する実行時状態。
struct AppState {
  AppMode mode = AppMode::SLIDESHOW;
  QrPage qr_page = QrPage::WIFI;
  uint32_t status_until_ms = 0;  // STATUS 画面を自動で閉じる時刻(millis 基準)
  bool needs_redraw = true;      // 画面再描画が必要か
};
