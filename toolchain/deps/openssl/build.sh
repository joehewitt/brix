set -euo pipefail

INSTALLED=/usr/local
STAGED="$PWD/staged"
TOOLS="$PWD/tools"

cd "$SOURCES/openssl"
if [ -n "${TARGET_SYSROOT:-}" ]; then
    mkdir -p "$TOOLS"
    printf '#!/bin/sh\nexec clang -target %s --sysroot=%s --gcc-install-dir=%s "$@"\n' \
        "'$TARGET_TRIPLE'" "'$TARGET_SYSROOT'" "'$TARGET_GCC_INSTALL_DIR'" > "$TOOLS/$TARGET_TRIPLE-cc"
    chmod +x "$TOOLS/$TARGET_TRIPLE-cc"
    export PATH="$TOOLS:$PATH" CC="$TARGET_TRIPLE-cc" AR="$TARGET_ARCHIVER" RANLIB="${TARGET_ARCHIVER%/*}/llvm-ranlib"
    ./Configure linux-aarch64 no-module no-shared no-tests --prefix="$INSTALLED" --libdir=lib --openssldir=/etc/ssl
    make -j"$JOBS" build_libs
    make install_dev DESTDIR="$STAGED"
else
    ./Configure darwin64-arm64-cc no-module no-shared no-tests --prefix="$INSTALLED" --libdir=lib --openssldir=/etc/ssl
    make -j"$JOBS"
    make install_sw DESTDIR="$STAGED"
fi
cp -R "$STAGED$INSTALLED/." "$PREFIX"
cp LICENSE.txt "$PREFIX/LICENSE.txt"
