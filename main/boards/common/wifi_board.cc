#include "wifi_board.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <string>
#include <cJSON.h>

#if CONFIG_USE_BLUETOOTH_PROVISIONING
#include "network/bluetooth_provisioning.h"
#endif

#include "wifi_configuration_ap.h"

#include "display.h"
#include "application.h"
#include "system_info.h"
#include "font_awesome_symbols.h"
#include "settings.h"
#include "assets/lang_config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_http.h>
#include <esp_mqtt.h>
#include <esp_udp.h>
#include <tcp_transport.h>
#include <tls_transport.h>
#include <web_socket.h>
#include <esp_log.h>

#include <wifi_station.h>
#include <wifi_configuration_ap.h>
#include <ssid_manager.h>
#include "afsk_demod.h"

static const char *TAG = "WifiBoard";

WifiBoard::WifiBoard() {
    Settings settings("wifi", true);
    wifi_config_mode_ = settings.GetInt("force_ap") == 1;
    if (wifi_config_mode_) {
        ESP_LOGI(TAG, "force_ap is set to 1, reset to 0");
        settings.SetInt("force_ap", 0);
    }
}

std::string WifiBoard::GetBoardType() {
    return "wifi";
}

void WifiBoard::EnterWifiConfigMode() {
    auto& application = Application::GetInstance();
    application.SetDeviceState(kDeviceStateWifiConfiguring);

#if CONFIG_USE_BLUETOOTH_PROVISIONING && (CONFIG_WIFI_PROVISIONING_BLUETOOTH || CONFIG_WIFI_PROVISIONING_DUAL)
    // 启动蓝牙配网
    ESP_LOGI(TAG, "Starting Bluetooth provisioning");
    auto& ble_prov = BluetoothProvisioning::GetInstance();
    
    // 设置设备名称
#ifdef CONFIG_BLE_DEVICE_NAME
    ble_prov.SetDeviceName(CONFIG_BLE_DEVICE_NAME);
#endif
    
    // 设置凭据接收回调
    ble_prov.OnCredentialsReceived([this](const std::string& ssid, const std::string& password) {
        ESP_LOGI(TAG, "Received WiFi credentials via BLE: %s", ssid.c_str());
        this->ConnectToWifi(ssid, password);
    });
    
    // 设置设备连接状态回调
    ble_prov.OnDeviceConnected([](bool connected) {
        auto display = Board::GetInstance().GetDisplay();
        if (connected) {
            ESP_LOGI(TAG, "BLE device connected");
            display->ShowNotification("BLE设备已连接", 5000);
        } else {
            ESP_LOGI(TAG, "BLE device disconnected");
        }
    });
    
    // 启动蓝牙配网
    ble_prov.Start();
    
    // 显示蓝牙配网提示
    std::string ble_hint = "蓝牙配网已启动\n设备名: ";
    ble_hint += ble_prov.GetDeviceName();
    ble_hint += "\n请使用手机App连接";
    application.Alert("蓝牙配网模式", ble_hint.c_str(), "", "");
#endif

#if CONFIG_WIFI_PROVISIONING_AP || CONFIG_WIFI_PROVISIONING_DUAL
    // 启动AP配网（原有方式）
    auto& wifi_ap = WifiConfigurationAp::GetInstance();
    wifi_ap.SetLanguage(Lang::CODE);
    wifi_ap.SetSsidPrefix("BuddyPal");
    wifi_ap.Start();

    // 显示 WiFi 配置 AP 的 SSID 和 Web 服务器 URL
    std::string hint = Lang::Strings::CONNECT_TO_HOTSPOT;
    hint += wifi_ap.GetSsid();
    hint += Lang::Strings::ACCESS_VIA_BROWSER;
    hint += wifi_ap.GetWebServerUrl();
    hint += "\n\n";
    
    // 播报配置 WiFi 的提示
    application.Alert(Lang::Strings::WIFI_CONFIG_MODE, hint.c_str(), "", Lang::Sounds::P3_WIFICONFIG);
#endif

    #if USE_ACOUSTIC_WIFI_PROVISIONING
    audio_wifi_config::ReceiveWifiCredentialsFromAudio(&application, &wifi_ap);
    #endif
    
    // Wait forever until reset after configuration
    while (true) {
        int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
        ESP_LOGI(TAG, "Free internal: %u minimal internal: %u", free_sram, min_free_sram);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void WifiBoard::ConnectToWifi(const std::string& ssid, const std::string& password) {
    ESP_LOGI(TAG, "Attempting to connect to WiFi: %s", ssid.c_str());
    
    // 发送BLE调试信息
#if CONFIG_USE_BLUETOOTH_PROVISIONING
    auto& ble_prov = BluetoothProvisioning::GetInstance();
    if (ble_prov.IsConnected()) {
        ble_prov.SendDebugMessage("正在连接WiFi: " + ssid);
    }
#endif
    
    // 保存WiFi配置
    auto& ssid_manager = SsidManager::GetInstance();
    ssid_manager.AddSsid(ssid, password);
    
    // 尝试连接
    auto& wifi_station = WifiStation::GetInstance();
    
    // 设置连接回调
    wifi_station.OnConnected([this, ssid](const std::string& connected_ssid) {
        ESP_LOGI(TAG, "WiFi connected successfully: %s", connected_ssid.c_str());
        
        // 发送成功状态
#if CONFIG_USE_BLUETOOTH_PROVISIONING
        auto& ble_prov = BluetoothProvisioning::GetInstance();
        if (ble_prov.IsConnected()) {
            ble_prov.SendConnectionStatus(true);
            ble_prov.SendDebugMessage("WiFi连接成功!");
            
            // 延迟一秒后停止蓝牙配网
            vTaskDelay(pdMS_TO_TICKS(1000));
            ble_prov.Stop();
        }
#endif
        
        // 重启设备以正常模式启动
        ESP_LOGI(TAG, "Restarting device in normal mode...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    });
    
    wifi_station.OnConnect([this, ssid](const std::string& connected_ssid) {
        ESP_LOGE(TAG, "WiFi connection failed: %s", connected_ssid.c_str());
        
        // 发送失败状态
#if CONFIG_USE_BLUETOOTH_PROVISIONING
        auto& ble_prov = BluetoothProvisioning::GetInstance();
        if (ble_prov.IsConnected()) {
            ble_prov.SendConnectionStatus(false);
            ble_prov.SendDebugMessage("WiFi连接失败，请检查密码");
        }
#endif
    });
    
    // 开始连接
    wifi_station.AddAuth(std::move(ssid), std::move(password));
}

void WifiBoard::StartNetwork() {
    // User can press BOOT button while starting to enter WiFi configuration mode
    if (wifi_config_mode_) {
        EnterWifiConfigMode();
        return;
    }

    // If no WiFi SSID is configured, enter WiFi configuration mode
    auto& ssid_manager = SsidManager::GetInstance();
    auto ssid_list = ssid_manager.GetSsidList();
    if (ssid_list.empty()) {
        wifi_config_mode_ = true;
        EnterWifiConfigMode();
        return;
    }

    auto& wifi_station = WifiStation::GetInstance();
    wifi_station.OnScanBegin([this]() {
        auto display = Board::GetInstance().GetDisplay();
        display->ShowNotification(Lang::Strings::SCANNING_WIFI, 30000);
    });
    wifi_station.OnConnect([this](const std::string& ssid) {
        auto display = Board::GetInstance().GetDisplay();
        std::string notification = Lang::Strings::CONNECT_TO;
        notification += ssid;
        notification += "...";
        display->ShowNotification(notification.c_str(), 30000);
    });
    wifi_station.OnConnected([this](const std::string& ssid) {
        auto display = Board::GetInstance().GetDisplay();
        std::string notification = Lang::Strings::CONNECTED_TO;
        notification += ssid;
        display->ShowNotification(notification.c_str(), 30000);
    });
    wifi_station.Start();

    // Try to connect to WiFi, if failed, launch the WiFi configuration AP
    if (!wifi_station.WaitForConnected(60 * 1000)) {
        wifi_station.Stop();
        wifi_config_mode_ = true;
        EnterWifiConfigMode();
        return;
    }
}

Http* WifiBoard::CreateHttp() {
    return new EspHttp();
}

WebSocket* WifiBoard::CreateWebSocket() {
    Settings settings("websocket", false);
    std::string url = settings.GetString("url");
    if (url.find("wss://") == 0) {
        return new WebSocket(new TlsTransport());
    } else {
        return new WebSocket(new TcpTransport());
    }
    return nullptr;
}

Mqtt* WifiBoard::CreateMqtt() {
    return new EspMqtt();
}

Udp* WifiBoard::CreateUdp() {
    return new EspUdp();
}

const char* WifiBoard::GetNetworkStateIcon() {
    if (wifi_config_mode_) {
        return FONT_AWESOME_WIFI;
    }
    auto& wifi_station = WifiStation::GetInstance();
    if (!wifi_station.IsConnected()) {
        return FONT_AWESOME_WIFI_OFF;
    }
    int8_t rssi = wifi_station.GetRssi();
    if (rssi >= -60) {
        return FONT_AWESOME_WIFI;
    } else if (rssi >= -70) {
        return FONT_AWESOME_WIFI_FAIR;
    } else {
        return FONT_AWESOME_WIFI_WEAK;
    }
}

std::string WifiBoard::GetBoardJson() {
    // Set the board type for OTA
    auto& wifi_station = WifiStation::GetInstance();
    std::string board_json = R"({)";
    board_json += R"("type":")" + std::string(BOARD_TYPE) + R"(",)";
    board_json += R"("name":")" + std::string(BOARD_NAME) + R"(",)";
    if (!wifi_config_mode_) {
        board_json += R"("ssid":")" + wifi_station.GetSsid() + R"(",)";
        board_json += R"("rssi":)" + std::to_string(wifi_station.GetRssi()) + R"(,)";
        board_json += R"("channel":)" + std::to_string(wifi_station.GetChannel()) + R"(,)";
        board_json += R"("ip":")" + wifi_station.GetIpAddress() + R"(",)";
    }
    board_json += R"("mac":")" + SystemInfo::GetMacAddress() + R"(")";
    board_json += R"(})";
    return board_json;
}

void WifiBoard::SetPowerSaveMode(bool enabled) {
    auto& wifi_station = WifiStation::GetInstance();
    wifi_station.SetPowerSaveMode(enabled);
}

void WifiBoard::ResetWifiConfiguration() {
    // Set a flag and reboot the device to enter the network configuration mode
    {
        Settings settings("wifi", true);
        settings.SetInt("force_ap", 1);
    }
    GetDisplay()->ShowNotification(Lang::Strings::ENTERING_WIFI_CONFIG_MODE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    // Reboot the device
    esp_restart();
}

std::string WifiBoard::GetDeviceStatusJson() {
    /*
     * 返回设备状态JSON
     * 
     * 返回的JSON结构如下：
     * {
     *     "audio_speaker": {
     *         "volume": 70
     *     },
     *     "screen": {
     *         "brightness": 100,
     *         "theme": "light"
     *     },
     *     "battery": {
     *         "level": 50,
     *         "charging": true
     *     },
     *     "network": {
     *         "type": "wifi",
     *         "ssid": "Xiaozhi",
     *         "rssi": -60
     *     },
     *     "chip": {
     *         "temperature": 25
     *     }
     * }
     */
    auto& board = Board::GetInstance();
    auto root = cJSON_CreateObject();

    // Audio speaker
    auto audio_speaker = cJSON_CreateObject();
    auto audio_codec = board.GetAudioCodec();
    if (audio_codec) {
        cJSON_AddNumberToObject(audio_speaker, "volume", audio_codec->output_volume());
    }
    cJSON_AddItemToObject(root, "audio_speaker", audio_speaker);

    // Screen brightness
    auto backlight = board.GetBacklight();
    auto screen = cJSON_CreateObject();
    if (backlight) {
        cJSON_AddNumberToObject(screen, "brightness", backlight->brightness());
    }
    auto display = board.GetDisplay();
    if (display && display->height() > 64) { // For LCD display only
        cJSON_AddStringToObject(screen, "theme", display->GetTheme().c_str());
    }
    cJSON_AddItemToObject(root, "screen", screen);

    // Battery
    int battery_level = 0;
    bool charging = false;
    bool discharging = false;
    if (board.GetBatteryLevel(battery_level, charging, discharging)) {
        cJSON* battery = cJSON_CreateObject();
        cJSON_AddNumberToObject(battery, "level", battery_level);
        cJSON_AddBoolToObject(battery, "charging", charging);
        cJSON_AddItemToObject(root, "battery", battery);
    }

    // Network
    auto network = cJSON_CreateObject();
    auto& wifi_station = WifiStation::GetInstance();
    cJSON_AddStringToObject(network, "type", "wifi");
    cJSON_AddStringToObject(network, "ssid", wifi_station.GetSsid().c_str());
    int rssi = wifi_station.GetRssi();
    if (rssi >= -60) {
        cJSON_AddStringToObject(network, "signal", "strong");
    } else if (rssi >= -70) {
        cJSON_AddStringToObject(network, "signal", "medium");
    } else {
        cJSON_AddStringToObject(network, "signal", "weak");
    }
    cJSON_AddItemToObject(root, "network", network);

    // Chip
    float esp32temp = 0.0f;
    if (board.GetTemperature(esp32temp)) {
        auto chip = cJSON_CreateObject();
        cJSON_AddNumberToObject(chip, "temperature", esp32temp);
        cJSON_AddItemToObject(root, "chip", chip);
    }

    auto json_str = cJSON_PrintUnformatted(root);
    std::string json(json_str);
    cJSON_free(json_str);
    cJSON_Delete(root);
    return json;
}
