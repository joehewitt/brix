set -euo pipefail

cd "$SOURCES/openssl"
if [ -n "${TARGET_SYSROOT:-}" ]; then
    export CC="clang -target $TARGET_TRIPLE --sysroot=$TARGET_SYSROOT --gcc-install-dir=$TARGET_GCC_INSTALL_DIR"
    ./Configure linux-aarch64 no-module no-shared no-tests --prefix="$PREFIX" --libdir=lib --openssldir=/etc/ssl
    make -j"$JOBS" build_libs
    make install_dev
else
    ./Configure darwin64-arm64-cc no-shared no-tests --prefix="$PREFIX" --libdir=lib --openssldir=/etc/ssl
    make -j"$JOBS"
    make install_sw
fi
cp LICENSE.txt "$PREFIX/LICENSE.txt"
