cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686.cmake
cmake --build build -j "$(getconf _NPROCESSORS_ONLN)"
