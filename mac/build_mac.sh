#!/bin/bash
# ============================================================
#  钢琴键游戏 macOS 版 —— 一键打包脚本
#
#  用法（在一台 Mac 上）：
#    打开「终端」，cd 到本目录，执行
#      chmod +x build_mac.sh
#      ./build_mac.sh
#
#  前置条件：Xcode 命令行工具。没装过的话先执行一次
#      xcode-select --install
#  （不需要装完整的 Xcode，也不需要联网，只用系统自带框架）
#
#  产物：
#      钢琴键游戏.app         双击即可运行，4 首曲子都在里面
#      钢琴键游戏-macOS.zip   给别人的时候发这一个文件就行
# ============================================================
set -e
cd "$(dirname "$0")"

APP="钢琴键游戏.app"
EXE="PianoTiles"

# ---- 架构 ------------------------------------------------------------
# 默认编「通用二进制」：一份 app 里同时含 x86_64 和 arm64，
# Intel 的 Mac 和 M1/M2/M3/M4 的 Mac 都能原生运行，不用 Rosetta 转译。
#
# 只想编某一种架构时，这样跑：
#     ARCHS=arm64  ./build_mac.sh     # 只给 M 系列
#     ARCHS=x86_64 ./build_mac.sh     # 只给老 Intel Mac
#
# 注意：arm64 要求系统至少 macOS 11.0，所以下面统一用 11.0。
# （牺牲了 10.13~10.15 那批很老的机器，现在基本没人用了）
# ---------------------------------------------------------------------
ARCHS="${ARCHS:-x86_64 arm64}"
ARCH_FLAGS=""
for a in $ARCHS; do ARCH_FLAGS="$ARCH_FLAGS -arch $a"; done

echo "==> 1/4 编译 main.mm（架构：$ARCHS）..."
clang++ -std=c++17 -O2 -fobjc-arc -mmacosx-version-min=11.0 \
    $ARCH_FLAGS \
    main.mm -o "$EXE" \
    -framework Cocoa \
    -framework CoreGraphics \
    -framework CoreText \
    -framework CoreVideo \
    -framework QuartzCore \
    -framework AVFoundation \
    -framework ImageIO \
    -framework CoreFoundation

echo "==> 2/4 组装 $APP ..."
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS"
mkdir -p "$APP/Contents/Resources"
cp "$EXE" "$APP/Contents/MacOS/$EXE"
cp Info.plist "$APP/Contents/Info.plist"
cp audio/aud0.mp3 audio/aud1.mp3 audio/aud2.mp3 audio/aud3.mp3 "$APP/Contents/Resources/"

echo "==> 3/4 校验 ..."
if [ ! -f "$APP/Contents/MacOS/$EXE" ]; then echo "打包失败：可执行文件缺失"; exit 1; fi
for i in 0 1 2 3; do
    if [ ! -f "$APP/Contents/Resources/aud$i.mp3" ]; then echo "打包失败：缺 aud$i.mp3"; exit 1; fi
done
echo "    aud0~aud3.mp3 全部就位，共 $(du -sh "$APP/Contents/Resources" | cut -f1)"
echo "    架构：$(lipo -archs "$APP/Contents/MacOS/$EXE")"
echo "    （同时出现 x86_64 和 arm64 就是通用二进制，两种 Mac 都能原生跑）"

echo "==> 4/4 打成 zip ..."
rm -f "钢琴键游戏-macOS.zip"
ditto -c -k --sequesterRsrc --keepParent "$APP" "钢琴键游戏-macOS.zip"
rm -f "$EXE"

echo ""
echo "完成！"
echo "  双击运行：  $(pwd)/$APP"
echo "  发给别人：  $(pwd)/钢琴键游戏-macOS.zip"
