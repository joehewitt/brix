set -euo pipefail

source="$SOURCES/sqlite"
mkdir -p "$PREFIX/include" "$PREFIX/lib"

flags=(
    -O2 -w
    -target "${TARGET_TRIPLE:-arm64-apple-macos}"
    ${SDKROOT:+-isysroot "$SDKROOT"}
    ${TARGET_SYSROOT:+--sysroot="$TARGET_SYSROOT" --gcc-install-dir="$TARGET_GCC_INSTALL_DIR"}
    -DSQLITE_DQS=0
    -DSQLITE_DEFAULT_MEMSTATUS=0
    -DSQLITE_DEFAULT_FOREIGN_KEYS=1
    -DSQLITE_DEFAULT_WAL_SYNCHRONOUS=1
    -DSQLITE_LIKE_DOESNT_MATCH_BLOBS
    -DSQLITE_OMIT_DEPRECATED
    -DSQLITE_OMIT_LOAD_EXTENSION
    -DSQLITE_OMIT_SHARED_CACHE
    -DSQLITE_ENABLE_FTS5
    -DSQLITE_ENABLE_MATH_FUNCTIONS
    -DHAVE_USLEEP=1
    -DHAVE_LOCALTIME_R=1
)

clang "${flags[@]}" -c "$source/sqlite3.c" -o sqlite3.o
ar rcs "$PREFIX/lib/libsqlite3.a" sqlite3.o
cp "$source/sqlite3.h" "$PREFIX/include/sqlite3.h"
printf 'The SQLite source code is in the public domain. See https://sqlite.org/copyright.html\n' > "$PREFIX/LICENSE.md"
