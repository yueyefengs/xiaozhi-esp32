#!/usr/bin/env python3
"""
快速ESP32固件合并脚本
直接使用esptool合并5个文件为一个完整固件
"""

import os
import sys
import subprocess
from pathlib import Path

def main():
    print("=" * 50)
    print("Quick ESP32 Firmware Merge Tool")
    print("=" * 50)
    
    # 检查所有必需文件
    required_files = [
        "build/bootloader/bootloader.bin",
        "build/partition_table/partition-table.bin", 
        "build/ota_data_initial.bin",
        "build/srmodels/srmodels.bin",
        "build/xiaozhi.bin"
    ]
    
    print("Checking files...")
    missing_files = []
    for file_path in required_files:
        if not Path(file_path).exists():
            missing_files.append(file_path)
            print(f"  ❌ {file_path}")
        else:
            print(f"  ✅ {file_path}")
    
    if missing_files:
        print(f"\n❌ Error: {len(missing_files)} files missing!")
        print("Please run 'idf.py build' first")
        input("Press Enter to exit...")
        sys.exit(1)
    
    print("\n✅ All files found!")
    
    # 使用esptool合并
    print("\n🔧 Merging firmware using esptool...")
    
    cmd = [
        "python", "-m", "esptool", "--chip", "esp32s3", "merge-bin",
        "--flash-mode", "dio",
        "--flash-freq", "80m", 
        "--flash-size", "16MB",
        "-o", "build/merged-binary.bin",
        "0x0", "build/bootloader/bootloader.bin",
        "0x8000", "build/partition_table/partition-table.bin",
        "0xd000", "build/ota_data_initial.bin", 
        "0x10000", "build/srmodels/srmodels.bin",
        "0x100000", "build/xiaozhi.bin"
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        
        print("✅ Merge successful!")
        
        # 检查输出文件
        output_file = Path("build/merged-binary.bin")
        if output_file.exists():
            file_size = output_file.stat().st_size
            print(f"📁 Output: build/merged-binary.bin")
            print(f"📏 Size: {file_size:,} bytes ({file_size/1024/1024:.2f} MB)")
            
            print("\n" + "=" * 50)
            print("🎉 SUCCESS! Ready to flash!")
            print("=" * 50)
            print("💡 Flash command:")
            print("   esptool.py -p COMx -b 921600 write_flash 0x0 build/merged-binary.bin")
            print("\n💡 Examples:")
            print("   esptool.py -p COM3 -b 921600 write_flash 0x0 build/merged-binary.bin")
            print("   esptool.py -p COM20 -b 921600 write_flash 0x0 build/merged-binary.bin")
            print("\n💡 ESP32 Flash Download Tool:")
            print("   File: build/merged-binary.bin")
            print("   Address: 0x0")
            print("   SPI Mode: DIO")
            print("   SPI Speed: 80MHz")
            print("   Flash Size: 16MB")
            print("=" * 50)
        else:
            print("❌ Warning: Command succeeded but output file not found")
            
    except subprocess.CalledProcessError as e:
        print(f"❌ Merge failed: {e}")
        if e.stderr:
            print(f"Error details: {e.stderr}")
            
    except FileNotFoundError:
        print("❌ Error: esptool not found!")
        print("Please install esptool: pip install esptool")
        
    print("\n")
    input("Press Enter to exit...")

if __name__ == "__main__":
    main()
