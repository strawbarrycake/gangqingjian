#!/bin/bash
# ============================================================
#  《高中的一周》macOS 版 —— 一键编译打包脚本（在 GitHub Actions / Mac 上跑）
#
#  产物：
#      高中的一周.app          双击即可运行
#      高中的一周-macOS.zip    发给别人的就是这一个文件
#
#  本地手动跑：
#      chmod +x build_mac.sh && ./build_mac.sh
# ============================================================
set -e
cd "$(dirname "$0")"

APP="高中的一周.app"
EXE="AWeek"
SRC="src/高中的一周.cpp"
DEFS="-DUNICODE -D_UNICODE -DHSW_MAC"
FW="-framework Cocoa -framework CoreGraphics -framework CoreText \
    -framework ImageIO -framework AVFoundation -framework QuartzCore \
    -framework CoreFoundation"

ARCHS="${ARCHS:-x86_64 arm64}"
ARCH_FLAGS=""
for a in $ARCHS; do ARCH_FLAGS="$ARCH_FLAGS -arch $a"; done

echo "==> 1/5 编译兼容层 shim_mac.mm（架构：$ARCHS）..."
clang++ -std=c++17 -O2 -fobjc-arc -mmacosx-version-min=11.0 $ARCH_FLAGS \
    $DEFS -I. \
    -x objective-c++ shim_mac.mm -c -o shim.o

echo "==> 2/5 编译游戏源码 ..."
clang++ -std=c++17 -O2 -mmacosx-version-min=11.0 $ARCH_FLAGS \
    $DEFS -I. \
    -x c++ "$SRC" -c -o game.o

echo "==> 3/5 链接 $EXE ..."
clang++ -mmacosx-version-min=11.0 $ARCH_FLAGS shim.o game.o -o "$EXE" $FW

echo "==> 4/5 组装 $APP ..."
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS"
mkdir -p "$APP/Contents/Resources"
cp "$EXE" "$APP/Contents/MacOS/$EXE"
cp Info.plist "$APP/Contents/Info.plist"
if [ -d resources ]; then
    cp -R resources/. "$APP/Contents/Resources/"
fi
if [ -d fonts ]; then
    mkdir -p "$APP/Contents/Resources/fonts"
    cp -R fonts/. "$APP/Contents/Resources/fonts/"
fi
rm -f shim.o game.o "$EXE"

echo "==> 校验 ..."
if [ ! -f "$APP/Contents/MacOS/$EXE" ]; then echo "打包失败：可执行文件缺失"; exit 1; fi
echo "    架构：$(lipo -archs "$APP/Contents/MacOS/$EXE")"
echo "    资源：$(du -sh "$APP/Contents/Resources" | cut -f1)"

echo "==> 5/5 打成 zip ..."
rm -f "HighSchoolWeek-macOS.zip"
ditto -c -k --sequesterRsrc --keepParent "$APP" "HighSchoolWeek-macOS.zip"

echo ""
echo "完成！"
echo "  双击运行：  $(pwd)/$APP"
echo "  发给别人：  $(pwd)/HighSchoolWeek-macOS.zip"
