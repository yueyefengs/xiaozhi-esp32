# 小智ESP32硬件API接口文档

## 概述

本文档详细描述了小智ESP32硬件设备与服务器之间的所有API接口，包括OTA更新、设备控制、语音交互、图像分析等功能。

## 目录

1. [OTA固件更新接口](#1-ota固件更新接口)
2. [设备激活接口](#2-设备激活接口)
3. [摄像头图像分析接口](#3-摄像头图像分析接口)
4. [WebSocket通信接口](#4-websocket通信接口)
5. [MCP协议接口](#5-mcp协议接口)
6. [设备状态查询接口](#6-设备状态查询接口)
7. [常用MCP工具](#7-常用mcp工具)

---

## 1. OTA固件更新接口

### 接口信息
- **接口地址**: 配置的OTA服务器URL（如 `https://xiaozhi.me/ota`）
- **请求方法**: POST/GET
- **描述**: 检查固件更新、获取服务器配置信息

### 请求头
```
Activation-Version: 1或2
Device-Id: {设备MAC地址}
Client-Id: {设备UUID}
Serial-Number: {序列号} (可选)
User-Agent: {设备名称}/{固件版本}
Accept-Language: {语言代码}
Content-Type: application/json
```

### 请求体 (POST时)
```json
{
  "type": "wifi",
  "name": "ESP32-S3-BOX3",
  "ssid": "WiFi名称",
  "rssi": -60,
  "channel": 6,
  "ip": "192.168.1.100",
  "mac": "AA:BB:CC:DD:EE:FF",
  "board": {
    "type": "wifi",
    "name": "ESP32-S3-BOX3",
    "ssid": "WiFi名称",
    "rssi": -60,
    "channel": 6,
    "ip": "192.168.1.100",
    "mac": "AA:BB:CC:DD:EE:FF"
  },
  "audio_codec": {
    "name": "ES8311",
    "output_volume": 70
  },
  "display": {
    "name": "ST7789",
    "width": 240,
    "height": 320,
    "theme": "light"
  },
  "led": {
    "name": "WS2812",
    "count": 8
  },
  "backlight": {
    "name": "PWM",
    "brightness": 100
  },
  "battery": {
    "level": 50,
    "charging": true
  },
  "chip": {
    "name": "ESP32-S3",
    "temperature": 25.5
  }
}
```

### 返回结构
```json
{
  "firmware": {
    "version": "1.0.0",
    "url": "https://example.com/firmware.bin",
    "force": 0
  },
  "activation": {
    "message": "激活消息",
    "code": "激活码",
    "challenge": "挑战码",
    "timeout_ms": 30000
  },
  "mqtt": {
    "broker": "mqtt.example.com",
    "port": 1883,
    "username": "user",
    "password": "pass"
  },
  "websocket": {
    "url": "wss://example.com/ws",
    "token": "access_token"
  },
  "server_time": {
    "timestamp": 1640995200000,
    "timezone_offset": 480
  }
}
```

### 字段说明
- `firmware`: 固件更新信息
  - `version`: 新固件版本号
  - `url`: 固件下载地址
  - `force`: 是否强制更新（1=强制，0=可选）
- `activation`: 设备激活信息
  - `message`: 激活提示消息
  - `code`: 激活码
  - `challenge`: 激活挑战码
  - `timeout_ms`: 激活超时时间
- `mqtt`: MQTT服务器配置
- `websocket`: WebSocket服务器配置
- `server_time`: 服务器时间同步信息

---

## 2. 设备激活接口

### 接口信息
- **接口地址**: `{OTA_URL}/activate`
- **请求方法**: POST
- **描述**: 设备激活验证

### 请求体
```json
{
  "algorithm": "hmac-sha256",
  "serial_number": "设备序列号",
  "challenge": "服务器下发的挑战码",
  "hmac": "HMAC签名"
}
```

### 返回状态
- `200`: 激活成功
- `202`: 激活超时
- `其他`: 激活失败

---

## 3. 摄像头图像分析接口

### 接口信息
- **接口地址**: 通过MCP协议配置的vision URL
- **请求方法**: POST
- **描述**: 将摄像头图像发送到AI服务器进行分析

### 请求头
```
Device-Id: {设备MAC地址}
Client-Id: {设备UUID}
Authorization: Bearer {token}
Content-Type: multipart/form-data; boundary=----ESP32_CAMERA_BOUNDARY
Transfer-Encoding: chunked
```

### 请求体 (multipart/form-data)
```
----ESP32_CAMERA_BOUNDARY
Content-Disposition: form-data; name="question"

{分析问题}
----ESP32_CAMERA_BOUNDARY
Content-Disposition: form-data; name="file"; filename="camera.jpg"
Content-Type: image/jpeg

{JPEG图像数据}
----ESP32_CAMERA_BOUNDARY--
```

### 返回结构
```json
{
  "success": true,
  "result": "图像分析结果"
}
```

### 错误返回
```json
{
  "success": false,
  "message": "错误信息"
}
```

---

## 4. WebSocket通信接口

### 连接信息
- **连接地址**: 配置的WebSocket URL
- **协议**: WebSocket (ws:// 或 wss://)

### 连接头
```
Authorization: Bearer {token}
Protocol-Version: {协议版本}
Device-Id: {设备MAC地址}
Client-Id: {设备UUID}
```

### 4.1 Hello消息

**设备发送**:
```json
{
  "type": "hello",
  "version": 1,
  "features": {
    "mcp": true
  },
  "transport": "websocket",
  "audio_params": {
    "format": "opus",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```

**服务器回复**:
```json
{
  "type": "hello",
  "transport": "websocket",
  "session_id": "会话ID",
  "audio_params": {
    "format": "opus",
    "sample_rate": 24000,
    "channels": 1,
    "frame_duration": 60
  }
}
```

### 4.2 监听控制

**开始监听**:
```json
{
  "session_id": "会话ID",
  "type": "listen",
  "state": "start",
  "mode": "manual"
}
```

**停止监听**:
```json
{
  "session_id": "会话ID",
  "type": "listen",
  "state": "stop"
}
```

**唤醒检测**:
```json
{
  "session_id": "会话ID",
  "type": "listen",
  "state": "detect",
  "text": "唤醒词文本"
}
```

### 4.3 中断操作
```json
{
  "session_id": "会话ID",
  "type": "abort",
  "reason": "wake_word_detected"
}
```

### 4.4 MCP协议消息
```json
{
  "session_id": "会话ID",
  "type": "mcp",
  "payload": {
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
      "content": [
        {"type": "text", "text": "执行结果"}
      ],
      "isError": false
    }
  }
}
```

### 4.5 服务器消息类型

**STT语音识别结果**:
```json
{
  "session_id": "会话ID",
  "type": "stt",
  "text": "识别的文本内容"
}
```

**LLM大模型回复**:
```json
{
  "session_id": "会话ID",
  "type": "llm",
  "text": "AI回复内容"
}
```

**TTS语音合成**:
```json
{
  "session_id": "会话ID",
  "type": "tts",
  "state": "start|end"
}
```

---

## 5. MCP协议接口

MCP (Model Context Protocol) 是用于设备能力发现和工具调用的标准协议。

### 5.1 初始化

**方法**: `initialize`

**请求**:
```json
{
  "jsonrpc": "2.0",
  "method": "initialize",
  "params": {
    "capabilities": {
      "vision": {
        "url": "图像分析服务器地址",
        "token": "访问令牌"
      }
    }
  },
  "id": 1
}
```

**响应**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "protocolVersion": "2024-11-05",
    "capabilities": {
      "tools": {}
    },
    "serverInfo": {
      "name": "小智ESP32",
      "version": "1.0.0"
    }
  }
}
```

### 5.2 工具列表

**方法**: `tools/list`

**请求**:
```json
{
  "jsonrpc": "2.0",
  "method": "tools/list",
  "params": {
    "cursor": "分页游标"
  },
  "id": 2
}
```

**响应**:
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "tools": [
      {
        "name": "self.get_device_status",
        "description": "获取设备状态信息",
        "inputSchema": {
          "type": "object",
          "properties": {}
        }
      }
    ],
    "nextCursor": "下一页游标"
  }
}
```

### 5.3 工具调用

**方法**: `tools/call`

**请求**:
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "工具名称",
    "arguments": {
      "参数名": "参数值"
    },
    "stackSize": 6144
  },
  "id": 3
}
```

**响应**:
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "执行结果"
      }
    ],
    "isError": false
  }
}
```

---

## 6. 设备状态查询接口

### 返回结构
```json
{
  "audio_speaker": {
    "volume": 70
  },
  "screen": {
    "brightness": 100,
    "theme": "light"
  },
  "battery": {
    "level": 50,
    "charging": true
  },
  "network": {
    "type": "wifi",
    "ssid": "WiFi名称",
    "signal": "strong"
  },
  "chip": {
    "temperature": 25.5
  }
}
```

### 字段说明
- `audio_speaker`: 音频扬声器状态
  - `volume`: 音量级别 (0-100)
- `screen`: 屏幕状态
  - `brightness`: 亮度级别 (0-100)
  - `theme`: 主题 (light/dark)
- `battery`: 电池状态
  - `level`: 电量百分比 (0-100)
  - `charging`: 是否正在充电
- `network`: 网络状态
  - `type`: 网络类型 (wifi/ml307)
  - `ssid`: WiFi名称
  - `signal`: 信号强度 (strong/medium/weak)
- `chip`: 芯片状态
  - `temperature`: 芯片温度

---

## 7. 常用MCP工具

### 7.1 获取设备状态

- **工具名**: `self.get_device_status`
- **描述**: 获取设备实时状态信息，包括音频、屏幕、电池、网络、芯片等状态
- **参数**: 无
- **返回**: 设备状态JSON字符串
- **使用场景**: 
  - 回答关于设备当前状态的问题
  - 作为设备控制的第一步，先获取当前状态

### 7.2 设置音量

- **工具名**: `self.audio_speaker.set_volume`
- **描述**: 设置音频扬声器的音量
- **参数**: 
  ```json
  {
    "volume": {
      "type": "integer",
      "minimum": 0,
      "maximum": 100,
      "description": "音量级别"
    }
  }
  ```
- **返回**: true/false
- **使用场景**: 响应用户的"调高音量"、"调低音量"等指令

### 7.3 设置屏幕亮度

- **工具名**: `self.screen.set_brightness`
- **描述**: 设置屏幕亮度
- **参数**: 
  ```json
  {
    "brightness": {
      "type": "integer",
      "minimum": 0,
      "maximum": 100,
      "description": "亮度级别"
    }
  }
  ```
- **返回**: true/false
- **使用场景**: 响应用户的"调亮屏幕"、"调暗屏幕"等指令

### 7.4 设置主题

- **工具名**: `self.screen.set_theme`
- **描述**: 设置屏幕主题（仅支持LCD显示屏）
- **参数**: 
  ```json
  {
    "theme": {
      "type": "string",
      "enum": ["light", "dark"],
      "description": "主题类型"
    }
  }
  ```
- **返回**: true/false
- **使用场景**: 响应用户的"切换到深色主题"、"切换到浅色主题"等指令

### 7.5 拍照分析

- **工具名**: `self.camera.take_photo`
- **描述**: 拍照并对图像进行AI分析
- **参数**: 
  ```json
  {
    "question": {
      "type": "string",
      "description": "关于图像的分析问题"
    }
  }
  ```
- **返回**: 图像分析结果JSON
- **使用场景**: 
  - 响应用户的"看看周围有什么"、"识别这个物体"等指令
  - 需要视觉感知的场景

### 7.6 设备特定工具

不同开发板可能还包含特定的工具，例如：

#### ESP-HI机器狗工具
- `self.dog.forward`: 前进
- `self.dog.backward`: 后退
- `self.dog.turn_left`: 左转
- `self.dog.turn_right`: 右转
- `self.dog.stop`: 停止

#### LED控制工具
- `self.led.set_color`: 设置LED颜色
- `self.led.set_pattern`: 设置LED图案

#### GPIO控制工具
- `self.gpio.set_pin`: 设置GPIO引脚状态
- `self.gpio.get_pin`: 获取GPIO引脚状态

---

## 错误处理

### 常见错误码
- `400`: 请求参数错误
- `401`: 认证失败
- `403`: 权限不足
- `404`: 资源不存在
- `500`: 服务器内部错误
- `503`: 服务不可用

### 错误响应格式
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32601,
    "message": "方法不存在",
    "data": {
      "method": "unknown_method"
    }
  }
}
```

---

## 开发注意事项

1. **认证机制**: 所有API都需要通过Device-Id和Client-Id进行设备认证
2. **超时处理**: 网络请求建议设置合理的超时时间
3. **重试机制**: 对于关键操作建议实现重试机制
4. **错误处理**: 需要妥善处理各种错误情况
5. **数据格式**: 严格遵循JSON格式规范
6. **版本兼容**: 注意API版本兼容性问题

---

## 更新日志

- **v1.0.0** (2024-01-01): 初始版本，包含基础API接口
- **v1.1.0** (2024-01-15): 添加MCP协议支持
- **v1.2.0** (2024-02-01): 完善摄像头图像分析接口
- **v1.3.0** (2024-02-15): 添加设备特定工具说明

---

*本文档基于小智ESP32项目代码分析整理，如有疑问请参考源代码或联系开发团队。* 