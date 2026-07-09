# 2026-07-10 LittleFS マウント失敗(パーティションラベル不一致)

## 症状

P0 スケルトンを書き込んだところ、LCD 下部に `FS mount FAILED` と表示。
シリアルにも `[FS] mount FAILED`。

## 原因

`LittleFS.begin(kFormatLittleFsOnFail)` は第4引数のパーティションラベルを
省略するとデフォルトで `"spiffs"` を探す。
一方 [partitions.csv](../partitions.csv) では Name を `littlefs` にしていたため、
対象パーティションが見つからずマウント(および自動フォーマット)に失敗していた。

```
# partitions.csv
littlefs, data, spiffs,  0x310000, 0x4E0000
```

※ SubType は `spiffs` だが、`LittleFS.begin()` が照合するのは Name(ラベル)。

## 対処

`LittleFS.begin()` にラベルを明示的に渡すよう修正(src/main.cpp)。

```cpp
constexpr const char* kLittleFsBasePath = "/littlefs";
constexpr const char* kLittleFsPartitionLabel = "littlefs";  // partitions.csv の Name と一致
constexpr uint8_t kLittleFsMaxOpenFiles = 10;

g_littlefs_ok = LittleFS.begin(kFormatLittleFsOnFail, kLittleFsBasePath,
                               kLittleFsMaxOpenFiles, kLittleFsPartitionLabel);
```

## 確認

再書き込み後の起動ログ:

```
[FS] mounted: used=8192 total=5111808 bytes
```

約 4.9MB の LittleFS が正常にマウントされることを実機で確認。

## 教訓

partitions.csv のパーティション Name を `spiffs` 以外にする場合、
`LittleFS.begin()` / `SPIFFS.begin()` にラベルを明示すること。
