set -euo pipefail

source="$SOURCES/pcre2/src"
work="$PWD/objects"
mkdir -p "$work" "$PREFIX/include" "$PREFIX/lib"
cp "$source/config.h.generic" "$work/config.h"
cp "$source/pcre2.h.generic" "$work/pcre2.h"
cp "$source/pcre2_chartables.c.dist" "$work/pcre2_chartables.c"

flags=(
    -O2 -std=c99 -w
    -target "${TARGET_TRIPLE:-arm64-apple-macos}"
    ${SDKROOT:+-isysroot "$SDKROOT"}
    -I "$work" -I "$source"
    -DHAVE_CONFIG_H
    -DPCRE2_CODE_UNIT_WIDTH=8
    -DPCRE2_STATIC
    -DSUPPORT_PCRE2_8
    -DSUPPORT_UNICODE
    -DHAVE_BUILTIN_MUL_OVERFLOW
)

units=(
    pcre2_auto_possess pcre2_chkdint pcre2_compile pcre2_compile_cgroup pcre2_compile_class
    pcre2_config pcre2_context pcre2_convert pcre2_dfa_match pcre2_error pcre2_extuni
    pcre2_find_bracket pcre2_jit_compile pcre2_maketables pcre2_match pcre2_match_data
    pcre2_match_next pcre2_newline pcre2_ord2utf pcre2_pattern_info pcre2_script_run
    pcre2_serialize pcre2_string_utils pcre2_study pcre2_substitute pcre2_substring
    pcre2_tables pcre2_ucd pcre2_valid_utf pcre2_xclass
)

objects=()
for unit in "${units[@]}"; do
    clang "${flags[@]}" -c "$source/$unit.c" -o "$work/$unit.o"
    objects+=("$work/$unit.o")
done
clang "${flags[@]}" -c "$work/pcre2_chartables.c" -o "$work/pcre2_chartables.o"
objects+=("$work/pcre2_chartables.o")

ar rcs "$PREFIX/lib/libpcre2-8.a" "${objects[@]}"
cp "$work/pcre2.h" "$PREFIX/include/pcre2.h"
cp "$SOURCES/pcre2/LICENCE.md" "$PREFIX/LICENSE.md"
