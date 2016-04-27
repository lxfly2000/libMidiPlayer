@echo off
cd midifile
rd /s /q Debug
rd /s /q lib
rd /s /q lib_d
rd /s /q lib_x64
rd /s /q lib_x64_d
rd /s /q Release
rd /s /q x64
cd..
rd /s /q Debug
rd /s /q lib
rd /s /q lib_d
rd /s /q lib_x64
rd /s /q lib_x64_d
rd /s /q Release
rd /s /q TestMidiPlayer_x64
rd /s /q x64
echo Íê³É¡£
ping -n 2 127.8>nul
