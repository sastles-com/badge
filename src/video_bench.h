// video_bench: P4 の再生 fps ベンチマーク(開発用)
//
// LittleFS に /bench.mjpg があれば、読み出し + JPEG デコード + 描画の実測 fps を
// シリアルに出力する。CLAUDE.md の「P4 は最初に必ずベンチ」に対応する一時的な計測。

#pragma once

namespace video_bench {

// /bench.mjpg があればベンチを実行して結果をシリアルに出力する。
void runIfPresent();

}  // namespace video_bench
