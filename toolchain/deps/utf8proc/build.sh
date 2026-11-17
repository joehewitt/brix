set -euo pipefail

source="$SOURCES/utf8proc"
mkdir -p "$PREFIX/include" "$PREFIX/lib"
clang -O2 -std=c99 -target "${TARGET_TRIPLE:-arm64-apple-macos}" ${SDKROOT:+-isysroot "$SDKROOT"} ${TARGET_SYSROOT:+--sysroot="$TARGET_SYSROOT" --gcc-install-dir="$TARGET_GCC_INSTALL_DIR"} -DUTF8PROC_EXPORTS -c "$source/utf8proc.c" -o utf8proc.o
"${TARGET_ARCHIVER:-ar}" rcs "$PREFIX/lib/libutf8proc.a" utf8proc.o
cp "$source/utf8proc.h" "$PREFIX/include/utf8proc.h"
cp "$source/LICENSE.md" "$PREFIX/LICENSE.md"
