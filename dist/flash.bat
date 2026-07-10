@echo off
rem M5Dial-Badge 統合バイナリ書き込みスクリプト(Windows)
rem
rem 使い方: flash.bat <ポート>
rem   例:    flash.bat COM3
rem ポートは「デバイスマネージャー」や `pio device list` で確認。
rem
rem 事前に esptool が必要: python -m pip install --user esptool

if "%~1"=="" (
  echo usage: flash.bat ^<port^>   例: flash.bat COM3
  exit /b 1
)

python -m esptool --chip esp32s3 --port %1 --baud 460800 write_flash 0x0 "%~dp0m5dial-badge-merged.bin"

echo 書き込み完了。M5Dial が再起動してスライドショーが始まります。
