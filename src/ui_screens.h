// ui_screens: LCD 各画面の描画
//
// 円形 240x240(GC9A01)。四隅は見えないため重要情報は内接円内に描く。
// P1 では スライドショー仮画面 / 2 段階 QR / ステータスを実装。

#pragma once

#include "app_state.h"

namespace ui_screens {

// スライドショー仮画面(ロゴ + 操作案内)。
void drawSlideshowPlaceholder();

// 空状態画面: 組み込みデフォルト画像 + 「QR で画像を送ってね」案内。
void drawEmptyState();

// QR 画面。AP モードでは page に応じて Wi-Fi QR / URL QR を描く。
void drawQr(QrPage page);

// ステータス画面(モード / SSID / IP / 空き容量 / コンテンツ数)。
void drawStatus();

}  // namespace ui_screens
