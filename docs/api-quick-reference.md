# 小智ESP32 API接口速查表

## 快速索引

| 接口类型 | 接口名称 | 方法 | 用途 |
|---------|---------|------|------|
| OTA | 固件更新检查 | POST/GET | 检查固件更新、获取配置 |
| OTA | 设备激活 | POST | 设备激活验证 |
| 摄像头 | 图像分析 | POST | AI图像分析 |
| WebSocket | 连接建立 | WS | 实时通信 |
| MCP | 工具列表 | JSON-RPC | 获取设备能力 |
| MCP | 工具调用 | JSON-RPC | 执行设备操作 |

---

## 1. OTA固件更新接口

**URL**: `{OTA_URL}`  
**方法**: POST/GET

### 请求头
```
Device-Id: {MAC地址}
Client-Id: {UUID}
Authorization: Bearer {token}
```

### 请求示例
```json
{
  "type": "wifi",
  "name": "ESP32-S3-BOX3",
  "ssid": "WiFi名称",
  "rssi": -60,
  "mac": "AA:BB:CC:DD:EE:FF"
}
```

### 响应示例
```json
{
  "firmware": {
    "version": "1.0.0",
    "url": "https://example.com/firmware.bin"
  },
  "websocket": {
    "url": "wss://example.com/ws",
    "token": "access_token"
  }
}
```

---

## 2. 设备激活接口

**URL**: `{OTA_URL}/activate`  
**方法**: POST

### 请求示例
```json
{
  "algorithm": "hmac-sha256",
  "serial_number": "设备序列号",
  "challenge": "挑战码",
  "hmac": "HMAC签名"
}
```

---

## 3. 摄像头图像分析接口

**URL**: 通过MCP配置的vision URL  
**方法**: POST

### 请求头
```
Content-Type: multipart/form-data
Authorization: Bearer {token}
```

### 请求体
```
--boundary
Content-Disposition: form-data; name="question"
分析问题
--boundary
Content-Disposition: form-data; name="file"; filename="camera.jpg"
Content-Type: image/jpeg
{JPEG图像数据}
--boundary--
```

### 响应示例
```json
{
  "success": true,
  "result": "图像分析结果"
}
```

---

## 4. WebSocket通信接口

### 连接建立
**URL**: 配置的WebSocket URL

### Hello消息
```json
{
  "type": "hello",
  "version": 1,
  "features": {"mcp": true},
  "transport": "websocket",
  "audio_params": {
    "format": "opus",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```

### 监听控制
```json
{
  "session_id": "会话ID",
  "type": "listen",
  "state": "start|stop|detect",
  "mode": "manual"
}
```

---

## 5. MCP协议接口

### 5.1 初始化
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

### 5.2 工具列表
```json
{
  "jsonrpc": "2.0",
  "method": "tools/list",
  "params": {"cursor": "分页游标"},
  "id": 2
}
```

### 5.3 工具调用
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "工具名称",
    "arguments": {"参数名": "参数值"}
  },
  "id": 3
}
```

---

## 6. 常用MCP工具

| 工具名 | 参数 | 返回 | 说明 |
|--------|------|------|------|
| `self.get_device_status` | 无 | JSON | 获取设备状态 |
| `self.audio_speaker.set_volume` | `{"volume": 0-100}` | bool | 设置音量 |
| `self.screen.set_brightness` | `{"brightness": 0-100}` | bool | 设置亮度 |
| `self.screen.set_theme` | `{"theme": "light\|dark"}` | bool | 设置主题 |
| `self.camera.take_photo` | `{"question": "问题"}` | JSON | 拍照分析 |

---

## 7. 设备状态JSON结构

```json
{
  "audio_speaker": {"volume": 70},
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
  "chip": {"temperature": 25.5}
}
```

---

## 8. 错误码速查

| 错误码 | 说明 | 处理建议 |
|--------|------|----------|
| 200 | 成功 | 正常处理 |
| 202 | 超时 | 重试或等待 |
| 400 | 参数错误 | 检查请求参数 |
| 401 | 认证失败 | 检查token |
| 403 | 权限不足 | 检查设备权限 |
| 404 | 资源不存在 | 检查URL |
| 500 | 服务器错误 | 联系技术支持 |

---

## 9. 开发提示

### 认证信息
- **Device-Id**: 设备MAC地址
- **Client-Id**: 设备UUID
- **Authorization**: Bearer token

### 常用请求头
```
Content-Type: application/json
Accept-Language: zh-CN
User-Agent: 设备名称/版本号
```

### 超时设置
- HTTP请求: 30秒
- WebSocket连接: 10秒
- 工具调用: 60秒

---

*更多详细信息请参考完整API文档: [hardware-api-interfaces.md](./hardware-api-interfaces.md)* 