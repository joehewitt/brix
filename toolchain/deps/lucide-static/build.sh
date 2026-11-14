set -euo pipefail

cp -R "$SOURCES/lucide-static/icons" "$PREFIX/icons"
cp "$SOURCES/lucide-static/LICENSE" "$PREFIX/LICENSE"
cp "$SOURCES/lucide-static/tags.json" "$PREFIX/tags.json"
