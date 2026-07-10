#include "video_bench.h"

#include <LittleFS.h>
#include <M5Dial.h>

#include "mjpg_reader.h"

namespace video_bench {
namespace {

constexpr const char* kBenchPath = "/bench.mjpg";
constexpr size_t kFrameBufMax = 48 * 1024;  // SPEC: 最大フレームバッファ 48KB
constexpr int kPasses = 3;

}  // namespace

void runIfPresent() {
  if (!LittleFS.exists(kBenchPath)) return;

  mjpg::Reader reader;
  if (!reader.open(LittleFS, kBenchPath)) return;

  uint8_t* buf = static_cast<uint8_t*>(malloc(kFrameBufMax));
  if (!buf) {
    Serial.println("[BENCH] malloc 48KB failed");
    reader.close();
    return;
  }

  const int frames = reader.frameCount();
  uint64_t read_us = 0, draw_us = 0;
  int drawn = 0, draw_fail = 0;
  size_t max_frame = 0;

  Serial.printf("[BENCH] start: %d frames x %d passes\n", frames, kPasses);

  for (int pass = 0; pass < kPasses; ++pass) {
    reader.rewind();
    size_t len = 0;
    while (true) {
      const uint32_t t0 = micros();
      if (!reader.nextFrame(buf, kFrameBufMax, len)) break;
      const uint32_t t1 = micros();
      const bool ok = M5Dial.Display.drawJpg(buf, len, 0, 0);
      const uint32_t t2 = micros();

      read_us += (t1 - t0);
      draw_us += (t2 - t1);
      if (ok) drawn++; else draw_fail++;
      if (len > max_frame) max_frame = len;
    }
  }

  free(buf);
  reader.close();

  if (drawn == 0) {
    Serial.println("[BENCH] no frames drawn");
    return;
  }

  const double avg_read_ms = (read_us / 1000.0) / drawn;
  const double avg_draw_ms = (draw_us / 1000.0) / drawn;
  const double avg_total_ms = avg_read_ms + avg_draw_ms;
  const double fps_draw = 1000.0 / avg_draw_ms;
  const double fps_total = 1000.0 / avg_total_ms;

  Serial.println("[BENCH] ==== MJPG playback benchmark ====");
  Serial.printf("[BENCH] frames drawn=%d fail=%d max_frame=%uB\n", drawn,
                draw_fail, static_cast<unsigned>(max_frame));
  Serial.printf("[BENCH] avg read = %.2f ms/frame\n", avg_read_ms);
  Serial.printf("[BENCH] avg draw = %.2f ms/frame (decode+blit)\n", avg_draw_ms);
  Serial.printf("[BENCH] avg total= %.2f ms/frame\n", avg_total_ms);
  Serial.printf("[BENCH] fps (decode+draw only) = %.1f\n", fps_draw);
  Serial.printf("[BENCH] fps (read+decode+draw)  = %.1f\n", fps_total);
  Serial.println("[BENCH] target 10fps / floor 8fps (SPEC 4)");
  Serial.println("[BENCH] =================================");
}

}  // namespace video_bench
