set -euo pipefail

(cd "$SOURCES/ninja" && python3 configure.py --bootstrap)
cmake="$SOURCES/cmake/CMake.app/Contents/bin/cmake"
ninja="$SOURCES/ninja/ninja"
options=(
    -DCMAKE_MAKE_PROGRAM="$ninja"
    -DCMAKE_BUILD_TYPE=Release
    -DLLVM_TARGETS_TO_BUILD=AArch64
    -DLLVM_INCLUDE_TESTS=OFF
    -DLLVM_INCLUDE_BENCHMARKS=OFF
    -DLLVM_INCLUDE_EXAMPLES=OFF
    -DLLVM_INCLUDE_DOCS=OFF
    -DLLVM_ENABLE_ZLIB=OFF
    -DLLVM_ENABLE_ZSTD=OFF
    -DLLVM_ENABLE_LIBXML2=OFF
    -DLLVM_ENABLE_LIBEDIT=OFF
)
if [ -n "${TARGET_SYSROOT:-}" ]; then
    compile_flags="--gcc-install-dir=$TARGET_GCC_INSTALL_DIR"
    link_flags="--ld-path=$TARGET_LINKER -static-libstdc++ -static-libgcc -Wl,--strip-all"
    "$cmake" -S "$SOURCES/llvm/llvm" -B build -G Ninja "${options[@]}" \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_C_COMPILER_TARGET="$TARGET_TRIPLE" \
        -DCMAKE_CXX_COMPILER_TARGET="$TARGET_TRIPLE" \
        -DCMAKE_SYSROOT="$TARGET_SYSROOT" \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
        -DCMAKE_C_FLAGS="$compile_flags" \
        -DCMAKE_CXX_FLAGS="$compile_flags" \
        -DCMAKE_EXE_LINKER_FLAGS="$link_flags" \
        -DCMAKE_SHARED_LINKER_FLAGS="$link_flags" \
        -DCMAKE_MODULE_LINKER_FLAGS="$link_flags" \
        -DLLVM_HOST_TRIPLE=aarch64-unknown-linux-gnu \
        -DLLVM_ENABLE_PROJECTS="clang;lld" \
        -DCLANG_ENABLE_ARCMT=OFF \
        -DCLANG_ENABLE_STATIC_ANALYZER=OFF \
        -DCLANG_PLUGIN_SUPPORT=OFF \
        -DCLANG_HAVE_DLFCN_H=1 \
        -DCLANG_HAVE_DLADDR=1 \
        -DCROSS_TOOLCHAIN_FLAGS_NATIVE="-DCMAKE_C_COMPILER=clang;-DCMAKE_CXX_COMPILER=clang++"
    "$ninja" -C build -j"$JOBS" lld libclang clang-resource-headers
    mkdir -p "$PREFIX/bin" "$PREFIX/lib"
    cp build/bin/lld "$PREFIX/bin/"
    ln -s lld "$PREFIX/bin/ld.lld"
    cp -P build/lib/libclang.so* "$PREFIX/lib/"
    cp -R build/lib/clang "$PREFIX/lib/"
    cp "$SOURCES/llvm/clang/LICENSE.TXT" "$PREFIX/LICENSE.TXT"
else
    "$cmake" -S "$SOURCES/llvm/llvm" -B build -G Ninja "${options[@]}" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DLLVM_ENABLE_PROJECTS=lld
    "$ninja" -C build -j"$JOBS" lld llvm-readobj llvm-objdump
    mkdir -p "$PREFIX/bin"
    cp build/bin/lld build/bin/llvm-readobj build/bin/llvm-objdump "$PREFIX/bin/"
    ln -s lld "$PREFIX/bin/ld.lld"
    ln -s lld "$PREFIX/bin/ld64.lld"
    ln -s llvm-readobj "$PREFIX/bin/llvm-readelf"
    cp "$SOURCES/llvm/lld/LICENSE.TXT" "$PREFIX/LICENSE.TXT"
fi
