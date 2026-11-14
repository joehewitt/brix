set -euo pipefail

export ZIG_GLOBAL_CACHE_DIR="$PWD/zig-cache"
for package in "$SOURCES"/zig-*; do
    "$ZIG_PREFIX/zig" fetch "$package" > /dev/null
done

cd "$SOURCES/ghostty"
"$ZIG_PREFIX/zig" build -Demit-lib-vt -Demit-xcframework=false -Doptimize=ReleaseFast --prefix "$PREFIX"
rm -rf "$PREFIX/include/ghostty" "$PREFIX"/lib/*.dylib
mkdir -p "$PREFIX/include/ghostty"
rsync -a --include='*/' --include='*.h' --exclude='*' include/ghostty/ "$PREFIX/include/ghostty/"
cp LICENSE "$PREFIX/LICENSE"
