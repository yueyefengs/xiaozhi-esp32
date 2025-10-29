#!/bin/bash
echo "=== 正成科技显示器配置测试 ==="
echo "当前板型配置:"
grep "CONFIG_BOARD_TYPE.*=y" sdkconfig
echo ""
echo "当前显示器配置:"
grep "CONFIG_ZHENGCHEN_DISPLAY.*=y" sdkconfig
echo ""
echo "检查条件编译宏定义:"
if grep -q "CONFIG_ZHENGCHEN_DISPLAY_GC9A01=y" sdkconfig; then
    echo "✓ GC9A01显示器已启用"
    echo "  - 应该使用zhengchen_gc9a01_display.h"
    echo "  - 应该包含esp_lcd_gc9a01.h"
elif grep -q "CONFIG_ZHENGCHEN_DISPLAY_ST7789=y" sdkconfig; then
    echo "✓ ST7789显示器已启用"
    echo "  - 应该使用zhengchen_lcd_display.h"
else
    echo "✗ 未找到有效的显示器配置"
fi
echo ""
echo "检查相关文件是否存在:"
files=(
    "main/boards/zhengchen-1.54tft-wifi/zhengchen_gc9a01_display.h"
    "main/boards/zhengchen-1.54tft-wifi/zhengchen_lcd_display.h"
    "main/boards/zhengchen-1.54tft-wifi/zhengchen-1.54tft-wifi.cc"
    "main/boards/zhengchen-1.54tft-ml307/zhengchen-1.54tft-ml307.cc"
)
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ $file (缺失)"
    fi
done
