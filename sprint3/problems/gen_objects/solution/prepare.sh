#!/bin/bash

# Настройка Conan
mkdir -p build && cd build && \
#conan profile new default --detect && \
#conan profile update settings.compiler.libcxx=libstdc++11 default && \
conan install .. --build=missing -s compiler.libcxx=libstdc++11 && \
cd ..

# mkdir -p build-release && cd build-release
# conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
# cmake .. -DCMAKE_BUILD_TYPE=Release
# cd ..

# mkdir -p build-debug && cd build-debug
# conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11
# cmake .. -DCMAKE_BUILD_TYPE=Debug
# cd ..
