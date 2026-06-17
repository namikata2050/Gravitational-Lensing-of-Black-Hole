@echo off
REM Emscriptenの環境変数を読み込む
call "C:\Users\hiroto yamada\emsdk\emsdk_env.bat"

REM Compile command
REM --bind: Enable Embind
REM --shell-file: Use custom HTML shell
if not exist docs mkdir docs
emcc src/main.cpp -o docs/index.html -s USE_SDL=2 --bind --shell-file src/shell.html -s "EXPORTED_FUNCTIONS=['_malloc', '_free', '_main']" -s "EXPORTED_RUNTIME_METHODS=['HEAPU8']" -s ALLOW_MEMORY_GROWTH=1

REM Copy background image if it exists
if exist build\bg.jpg (
    copy build\bg.jpg docs\bg.jpg >nul
) else if exist src\bg.jpg (
    copy src\bg.jpg docs\bg.jpg >nul
)

REM 成功したらメッセージ表示
if %errorlevel% equ 0 (
    echo Build Successful!
) else (
    echo Build Failed.
)

