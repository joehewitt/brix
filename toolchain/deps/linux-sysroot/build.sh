set -euo pipefail

for package in "$SOURCES"/*; do
    cp -R "$package/." "$PREFIX/"
done
rm -rf "$PREFIX/etc" "$PREFIX/var" "$PREFIX/usr/bin" "$PREFIX/usr/sbin" "$PREFIX/usr/libexec" \
    "$PREFIX/usr/share/doc" "$PREFIX/usr/share/info" "$PREFIX/usr/share/locale" "$PREFIX/usr/share/man" \
    "$PREFIX/usr/lib64/gconv" "$PREFIX/usr/lib64/audit"
ln -s usr/lib "$PREFIX/lib"
ln -s usr/lib64 "$PREFIX/lib64"
