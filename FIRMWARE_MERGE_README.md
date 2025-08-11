# ESP32固件合并工具使用说明

本项目提供了多种方式将5个独立的二进制文件合并成一个完整的固件文件，方便烧录。

## 🎯 问题解决

**问题**：使用ESP32 Flash Download Tool烧录时设备无法启动，但命令行烧录可以正常工作。

**原因**：GUI工具的配置与项目实际的flash布局不匹配。

**解决方案**：将5个文件合并成一个文件，简化烧录过程。

## 📁 文件说明

项目构建后会生成以下5个文件：
- `build/bootloader/bootloader.bin` @ `0x0` - 引导程序
- `build/partition_table/partition-table.bin` @ `0x8000` - 分区表
- `build/ota_data_initial.bin` @ `0xd000` - OTA数据
- `build/srmodels/srmodels.bin` @ `0x10000` - 语音识别模型
- `build/xiaozhi.bin` @ `0x100000` - 主应用程序

## 🛠️ 合并工具

### 1. 快速合并脚本（推荐）

```bash
python quick_merge.py
```

**特点**：
- ✅ 简单易用，一键合并
- ✅ 自动检查文件完整性
- ✅ 清晰的状态提示
- ✅ 提供详细的烧录说明

### 2. 完整合并脚本

```bash
python merge_firmware.py
```

**特点**：
- 🔄 先尝试使用idf.py，失败后自动使用esptool
- 📋 详细的错误处理和提示

### 3. Windows批处理脚本

```cmd
merge_flash.bat
```

**特点**：
- 🪟 Windows原生支持
- 🔄 自动尝试多种合并方法

### 4. Linux/macOS脚本

```bash
./merge_flash.sh
```

**特点**：
- 🐧 Linux/macOS原生支持
- 🔧 使用bash脚本实现

## 📋 使用步骤

### 1. 准备工作

确保已经构建了项目：
```bash
idf.py build
```

### 2. 安装依赖（如果需要）

```bash
pip install esptool
```

### 3. 运行合并脚本

选择任一合并工具运行，推荐使用快速合并脚本：
```bash
python quick_merge.py
```

### 4. 烧录固件

合并成功后，使用以下任一方法烧录：

#### 方法1：命令行烧录（推荐）
```bash
esptool.py -p COM3 -b 921600 write_flash 0x0 build/merged-binary.bin
```

#### 方法2：ESP32 Flash Download Tool
1. 打开ESP32 Flash Download Tool
2. 选择文件：`build/merged-binary.bin`
3. 设置地址：`0x0`
4. 配置参数：
   - SPI Mode: **DIO**
   - SPI Speed: **80MHz**
   - Flash Size: **16MB**
5. 选择正确的COM端口
6. 点击START开始烧录

## ⚙️ 重要配置

### ESP32 Flash Download Tool正确配置

| 参数 | 值 |
|------|-----|
| 文件 | `build/merged-binary.bin` |
| 地址 | `0x0` |
| SPI Mode | **DIO** (不是QIO!) |
| SPI Speed | **80MHz** |
| Flash Size | **16MB** |
| DoNotChgBin | ✅ 勾选 |

### 常见错误配置

❌ **错误的配置**：
- SPI Mode设置为QIO
- 使用多个文件分别烧录到不同地址
- Flash参数不匹配

✅ **正确的配置**：
- 使用合并后的单个文件
- 地址设置为0x0
- SPI Mode设置为DIO

## 🔍 故障排除

### 1. 文件缺失错误
```
Error: Cannot find build/xiaozhi.bin
```
**解决方案**：先运行 `idf.py build` 构建项目

### 2. esptool未安装
```
Error: esptool not found
```
**解决方案**：运行 `pip install esptool`

### 3. 设备仍无法启动
- 检查串口连接
- 确认芯片型号为ESP32-S3
- 检查电源供应
- 尝试擦除flash后重新烧录：
  ```bash
  esptool.py -p COM3 erase_flash
  esptool.py -p COM3 -b 921600 write_flash 0x0 build/merged-binary.bin
  ```

## 📊 文件大小参考

合并后的固件文件大小约为：**4.38 MB** (4,592,352 bytes)

如果文件大小差异很大，请检查构建是否完整。

## 🎉 成功标志

烧录成功后，设备应该能够：
1. 正常启动
2. 显示启动日志
3. 进入正常工作模式

---

**提示**：建议使用快速合并脚本 `quick_merge.py`，它提供了最佳的用户体验和错误处理。
