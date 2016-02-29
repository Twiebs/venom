@echo off

set link_flags= user32.lib winmm.lib opengl32.lib imm32.lib gdi32.lib

pushd ..\build
cl ../src/platform.cpp -ovenom.exe /link %link_flags%
popd
