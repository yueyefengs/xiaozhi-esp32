#!/usr/bin/env python3
"""
合并ESP32固件文件脚本
将5个独立的二进制文件合并成一个完整的固件文件，方便烧录
"""

import os
import sys
import json
import subprocess
import argparse
from pathlib import Path

def get_project_root():
    """获取项目根目录"""
    script_dir = Path(__file__).parent
    return script_dir.parent

def check_build_files():
    """检查构建文件是否存在"""
    project_root = get_project_root()
    build_dir = project_root / "build"
    
    required_files = [
        "bootloader/bootloader.bin",
        "partition_table/partition-table.bin", 
        "ota_data_initial.bin",
        "srmodels/srmodels.bin",
        "xiaozhi.bin"
    ]
    
    missing_files = []
    for file_path in required_files:
        full_path = build_dir / file_path
        if not full_path.exists():
            missing_files.append(str(full_path))
    
    if missing_files:
        print("错误：以下构建文件不存在，请先运行 'idf.py build':")
        for file in missing_files:
            print(f"  - {file}")
        return False
    
    return True

def read_flasher_args():
    """读取flasher_args.json获取烧录配置"""
    project_root = get_project_root()
    flasher_args_path = project_root / "build" / "flasher_args.json"
    
    if not flasher_args_path.exists():
        print(f"错误：找不到 {flasher_args_path}")
        return None
    
    with open(flasher_args_path, 'r', encoding='utf-8') as f:
        return json.load(f)

def merge_using_idf_py():
    """使用idf.py merge-bin命令合并文件"""
    project_root = get_project_root()
    os.chdir(project_root)
    
    print("使用 idf.py merge-bin 合并固件文件...")
    
    try:
        # 运行idf.py merge-bin命令
        result = subprocess.run(
            ["idf.py", "merge-bin"],
            capture_output=True,
            text=True,
            check=True
        )
        
        print("合并成功！")
        print(f"输出文件：build/merged-binary.bin")
        
        # 检查输出文件
        merged_file = project_root / "build" / "merged-binary.bin"
        if merged_file.exists():
            file_size = merged_file.stat().st_size
            print(f"文件大小：{file_size:,} 字节 ({file_size/1024/1024:.2f} MB)")
            return str(merged_file)
        else:
            print("警告：未找到合并后的文件")
            return None
            
    except subprocess.CalledProcessError as e:
        print(f"合并失败：{e}")
        print(f"错误输出：{e.stderr}")
        return None
    except FileNotFoundError:
        print("错误：找不到 idf.py 命令，请确保ESP-IDF环境已正确配置")
        return None

def merge_using_esptool():
    """使用esptool.py手动合并文件（备用方案）"""
    project_root = get_project_root()
    
    # 读取烧录配置
    flasher_args = read_flasher_args()
    if not flasher_args:
        return None
    
    # 构建esptool命令
    flash_files = flasher_args.get("flash_files", {})
    write_flash_args = flasher_args.get("write_flash_args", [])
    
    cmd = ["python", "-m", "esptool", "merge_bin"]
    
    # 添加flash参数
    for arg in write_flash_args:
        cmd.append(arg)
    
    # 输出文件
    output_file = project_root / "build" / "merged-binary-manual.bin"
    cmd.extend(["-o", str(output_file)])
    
    # 添加文件和地址
    build_dir = project_root / "build"
    for address, filename in flash_files.items():
        cmd.append(address)
        cmd.append(str(build_dir / filename))
    
    print("使用 esptool.py 手动合并固件文件...")
    print(f"命令：{' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print("合并成功！")
        print(f"输出文件：{output_file}")
        
        if output_file.exists():
            file_size = output_file.stat().st_size
            print(f"文件大小：{file_size:,} 字节 ({file_size/1024/1024:.2f} MB)")
            return str(output_file)
        
    except subprocess.CalledProcessError as e:
        print(f"合并失败：{e}")
        print(f"错误输出：{e.stderr}")
        return None
    except FileNotFoundError:
        print("错误：找不到 esptool.py，请安装 esptool")
        return None

def show_flash_command(merged_file):
    """显示烧录命令"""
    if not merged_file:
        return
    
    print("\n" + "="*60)
    print("合并完成！现在您可以使用以下命令烧录：")
    print("="*60)
    print(f"esptool.py -p COM端口 -b 921600 write_flash 0x0 {merged_file}")
    print("\n例如：")
    print(f"esptool.py -p COM3 -b 921600 write_flash 0x0 {merged_file}")
    print("\n或者在ESP32 Flash Download Tool中：")
    print(f"  文件：{merged_file}")
    print("  地址：0x0")
    print("  其他设置保持默认")
    print("="*60)

def main():
    parser = argparse.ArgumentParser(description="合并ESP32固件文件")
    parser.add_argument("--method", choices=["idf", "esptool"], default="idf",
                       help="合并方法：idf (使用idf.py) 或 esptool (使用esptool.py)")
    
    args = parser.parse_args()
    
    print("ESP32固件合并工具")
    print("="*40)
    
    # 检查构建文件
    if not check_build_files():
        sys.exit(1)
    
    # 根据选择的方法合并文件
    if args.method == "idf":
        merged_file = merge_using_idf_py()
        if not merged_file and input("\nidf.py方法失败，是否尝试esptool方法？(y/N): ").lower() == 'y':
            merged_file = merge_using_esptool()
    else:
        merged_file = merge_using_esptool()
    
    # 显示烧录命令
    show_flash_command(merged_file)

if __name__ == "__main__":
    main()
