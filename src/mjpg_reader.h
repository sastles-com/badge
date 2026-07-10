// mjpg_reader: 独自 MJPG コンテナのパース・逐次フレーム読み出し
//
// コンテナ(リトルエンディアン、SPEC 3.3):
//   magic "MJPG"(4) / version u8 / fps u8 / frames u16 / reserved u32
//   以降: [u32 size][JPEG(240x240)] の繰り返し
// フレーム全体をメモリに展開せず、1 フレームずつ順次読み出す(シーク不要)。

#pragma once

#include <Arduino.h>
#include <FS.h>

namespace mjpg {

struct Header {
  uint8_t version = 0;
  uint8_t fps = 0;
  uint16_t frames = 0;
};

class Reader {
 public:
  // ヘッダを検証して開く。失敗で false。
  bool open(fs::FS& fs, const char* path);
  bool valid() const { return valid_; }
  const Header& header() const { return header_; }
  int frameCount() const { return header_.frames; }

  // 先頭フレームへ戻す。
  void rewind();

  // 次フレームを buf(容量 cap)に読み込む。成功で true & len にサイズ。
  // 全フレーム読み終わり / エラーで false。
  bool nextFrame(uint8_t* buf, size_t cap, size_t& len);

  void close();

 private:
  File file_;
  Header header_{};
  bool valid_ = false;
  size_t data_start_ = 0;  // 最初のフレームのファイルオフセット
  int read_index_ = 0;
};

}  // namespace mjpg
