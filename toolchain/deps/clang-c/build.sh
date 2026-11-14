set -euo pipefail

mkdir -p "$PREFIX/include"
cp -R "$SOURCES/clang/include/clang-c" "$PREFIX/include/clang-c"
cp "$SOURCES/clang/LICENSE.TXT" "$PREFIX/LICENSE.TXT"
