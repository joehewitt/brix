set -euo pipefail

objects="$PWD/objects"
cflags=(-O3 -std=gnu11 -target arm64-apple-macos -D_DARWIN_C_SOURCE -w)
cxxflags=(-O3 -std=c++17 -target arm64-apple-macos -D_DARWIN_C_SOURCE -w)

grammars=(
    "c tree-sitter-c src queries"
    "cpp tree-sitter-cpp src queries"
    "javascript tree-sitter-javascript src queries"
    "typescript tree-sitter-typescript typescript/src queries"
    "tsx tree-sitter-typescript tsx/src -"
    "python tree-sitter-python src queries"
    "rust tree-sitter-rust src queries"
    "go tree-sitter-go src queries"
    "json tree-sitter-json src queries"
    "css tree-sitter-css src queries"
    "html tree-sitter-html src queries"
    "bash tree-sitter-bash src queries"
    "markdown tree-sitter-markdown tree-sitter-markdown/src tree-sitter-markdown/queries"
    "objcpp tree-sitter-objcpp src -"
)

write_queries() {
    python3 -I - "$@" <<'PYTHON'
import os
import re
import sys

output, name, *files = sys.argv[1:]
lines = []
for path in files:
    stem = re.sub(r'[^A-Za-z0-9]', '_', os.path.basename(path)[:-len('.scm')])
    with open(path, encoding='utf-8') as handle:
        text = handle.read().replace('\r', '')
    lines.append(f'const char tree_sitter_{name}_query_{stem}[] =')
    body = text.split('\n')
    if body and body[-1] == '':
        body.pop()
    if not body:
        body = ['']
    for line in body:
        escaped = (line.replace('\\', '\\\\').replace('"', '\\"').replace('?', '\\?').replace('\t', '\\t'))
        lines.append(f'    "{escaped}\\n"')
    lines[-1] += ';'
    lines.append('')
with open(output, 'w', encoding='utf-8') as handle:
    handle.write('\n'.join(lines))
PYTHON
}

build_runtime() {
    local work="$objects/runtime"
    mkdir -p "$work" "$PREFIX/include/tree_sitter" "$PREFIX/lib"
    clang "${cflags[@]}" -I "$SOURCES/tree-sitter/lib/include" -I "$SOURCES/tree-sitter/lib/src" \
        -c "$SOURCES/tree-sitter/lib/src/lib.c" -o "$work/lib.o"
    ar rcs "$PREFIX/lib/libtree-sitter.a" "$work/lib.o"
    cp "$SOURCES/tree-sitter/lib/include/tree_sitter/api.h" "$PREFIX/include/tree_sitter/api.h"
    cp "$SOURCES/tree-sitter/LICENSE" "$PREFIX/LICENSE"
}

build_grammar() {
    local name="$1" repository="$2" source="$3" queries="$4"
    local directory="$SOURCES/$repository/$source"
    local work="$objects/$name"
    local members=()
    local queryFiles=()
    mkdir -p "$work"
    clang "${cflags[@]}" -I "$directory" -c "$directory/parser.c" -o "$work/parser.o"
    members+=("$work/parser.o")
    if [ -f "$directory/scanner.c" ]; then
        clang "${cflags[@]}" -I "$directory" -c "$directory/scanner.c" -o "$work/scanner.o"
        members+=("$work/scanner.o")
    elif [ -f "$directory/scanner.cc" ]; then
        clang++ "${cxxflags[@]}" -I "$directory" -c "$directory/scanner.cc" -o "$work/scanner.o"
        members+=("$work/scanner.o")
    fi
    if [ "$queries" != "-" ]; then
        queryFiles+=("$SOURCES/$repository/$queries"/highlights*.scm)
    fi
    if [ -d "$RECIPE/queries/$name" ]; then
        queryFiles+=("$RECIPE/queries/$name"/highlights*.scm)
    fi
    if [ ${#queryFiles[@]} -gt 0 ]; then
        write_queries "$work/queries.c" "$name" "${queryFiles[@]}"
        clang "${cflags[@]}" -c "$work/queries.c" -o "$work/queries.o"
        members+=("$work/queries.o")
    fi
    ar rcs "$PREFIX/lib/libtree-sitter-$name.a" "${members[@]}"
}

build_runtime

pids=()
for entry in "${grammars[@]}"; do
    build_grammar $entry &
    pids+=($!)
done
status=0
for pid in "${pids[@]}"; do
    wait "$pid" || status=1
done
exit $status
