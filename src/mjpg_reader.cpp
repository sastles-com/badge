#include "mjpg_reader.h"

namespace mjpg {
namespace {

constexpr size_t kHeaderSize = 12;  // magic(4)+ver(1)+fps(1)+frames(2)+reserved(4)

uint16_t rdU16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}
uint32_t rdU32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

}  // namespace

bool Reader::open(fs::FS& fs, const char* path) {
  close();
  file_ = fs.open(path, "r");
  if (!file_) {
    Serial.printf("[MJPG] open failed: %s\n", path);
    return false;
  }

  uint8_t hdr[kHeaderSize];
  if (file_.read(hdr, kHeaderSize) != static_cast<int>(kHeaderSize)) {
    Serial.println("[MJPG] header too short");
    close();
    return false;
  }
  if (memcmp(hdr, "MJPG", 4) != 0) {
    Serial.println("[MJPG] bad magic");
    close();
    return false;
  }
  header_.version = hdr[4];
  header_.fps = hdr[5];
  header_.frames = rdU16(hdr + 6);
  data_start_ = kHeaderSize;
  read_index_ = 0;
  valid_ = true;
  Serial.printf("[MJPG] ok: v=%u fps=%u frames=%u\n", header_.version,
                header_.fps, header_.frames);
  return true;
}

void Reader::rewind() {
  if (!valid_) return;
  file_.seek(data_start_);
  read_index_ = 0;
}

bool Reader::nextFrame(uint8_t* buf, size_t cap, size_t& len) {
  len = 0;
  if (!valid_ || read_index_ >= header_.frames) return false;

  uint8_t szbuf[4];
  if (file_.read(szbuf, 4) != 4) return false;
  const uint32_t size = rdU32(szbuf);
  if (size == 0 || size > cap) {
    Serial.printf("[MJPG] frame %d too large: %u > %u\n", read_index_,
                  static_cast<unsigned>(size), static_cast<unsigned>(cap));
    return false;
  }
  if (file_.read(buf, size) != static_cast<int>(size)) {
    Serial.printf("[MJPG] frame %d read short\n", read_index_);
    return false;
  }
  len = size;
  read_index_++;
  return true;
}

void Reader::close() {
  if (file_) file_.close();
  valid_ = false;
  read_index_ = 0;
}

}  // namespace mjpg
