cmake_minimum_required(VERSION 3.10)

# プロジェクト名を設定
project(BlackHoleSim)

# C++のバージョンを C++17 に設定 (新しい機能を使えるようにするため)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ソースコードの場所を指定して実行可能ファイルを作成
# add_executable(実行ファイル名 ソースコードファイル)
add_executable(main src/main.cpp)
