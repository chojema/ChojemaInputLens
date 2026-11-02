#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUTPUT="$ROOT/build/macos"
APP="$OUTPUT/Chojema InputLens.app"
VERSION="$(sed -nE 's/^project\(ChojemaInputLens VERSION ([0-9.]+).*/\1/p' "$ROOT/CMakeLists.txt")"
[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "Invalid project version" >&2; exit 1; }
MODE="${1:-build}"
case "$MODE" in build|package) ;; *) echo "Usage: $0 [build|package]" >&2; exit 2 ;; esac

mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
xcrun --sdk macosx clang++ -std=c++20 -O2 -DNDEBUG -fobjc-arc -Wall -Wextra -Wpedantic \
    -arch arm64 -arch x86_64 -mmacosx-version-min=12.0 -ffile-prefix-map="$ROOT"=. \
    -I "$ROOT/src/core" -I "$ROOT/src/mac" \
    "$ROOT/src/mac/main.mm" "$ROOT/src/mac/key_labels.mm" "$ROOT/src/mac/overlay.mm" \
    "$ROOT/src/core/input_tracker.cpp" -framework Cocoa -framework Carbon \
    -o "$OUTPUT/ChojemaInputLens"
strip -x "$OUTPUT/ChojemaInputLens"
cp -X "$OUTPUT/ChojemaInputLens" "$APP/Contents/MacOS/ChojemaInputLens"
cp -X "$ROOT/assets/icon/ChojemaInputLens.icns" "$APP/Contents/Resources/"
cp -X "$ROOT/LICENSE" "$APP/Contents/Resources/LICENSE.txt"
cp -X "$ROOT/src/mac/INSTALL.md" "$APP/Contents/Resources/INSTALL.md"
sed "s/@PROJECT_VERSION@/$VERSION/g" "$ROOT/src/mac/Info.plist.in" > "$APP/Contents/Info.plist"
plutil -lint "$APP/Contents/Info.plist"
codesign --force --sign - --identifier com.chojema.InputLens "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"
lipo -info "$APP/Contents/MacOS/ChojemaInputLens"

if [[ "$MODE" == package ]]; then
    STAGE="$(mktemp -d "$OUTPUT/package.XXXXXX")"
    ditto --norsrc --noextattr "$APP" "$STAGE/Chojema InputLens.app"
    cp -X "$ROOT/LICENSE" "$STAGE/LICENSE.txt"
    cp -X "$ROOT/src/mac/INSTALL.md" "$STAGE/INSTALL.md"
    ditto --norsrc --noextattr -c -k --keepParent "$APP" "$OUTPUT/ChojemaInputLens-$VERSION-mac-universal.zip"
    ln -s /Applications "$STAGE/Applications"
    hdiutil create -volname "Chojema InputLens $VERSION" -srcfolder "$STAGE" \
        -ov -format UDZO "$OUTPUT/ChojemaInputLens-$VERSION-mac-universal.dmg"
    shasum -a 256 "$OUTPUT/ChojemaInputLens-$VERSION-mac-universal.zip" \
        "$OUTPUT/ChojemaInputLens-$VERSION-mac-universal.dmg"
fi
