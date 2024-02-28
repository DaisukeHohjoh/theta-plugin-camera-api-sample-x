cmake_minimum_required(VERSION 3.4.1)

add_library(
  # 識別用ライブラリ名を指定
  dngLib

  # 共有ライブラリとしてビルドさせる
  SHARED

  # C/C++ソースへの相対パス指定
  src/main/cpp/dngLib.cpp
)

target_link_libraries(
        dngLib
        android
        log
)
