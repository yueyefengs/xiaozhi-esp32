#ifndef BLUETOOTH_PROVISIONING_H
#define BLUETOOTH_PROVISIONING_H

#include "sdkconfig.h"

#if CONFIG_USE_BLUETOOTH_PROVISIONING

#include <string>
#include <functional>
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

// 服务和特征UUID (128-bit UUIDs for better compatibility)
#define WIFI_PROV_SERVICE_UUID_128      {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}
#define WIFI_SSID_CHAR_UUID_128         {0x12, 0x35, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}
#define WIFI_PASSWORD_CHAR_UUID_128     {0x12, 0x36, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}
#define WIFI_STATUS_CHAR_UUID_128       {0x12, 0x37, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}

// 16-bit UUIDs for compatibility with current app
#define WIFI_PROV_SERVICE_UUID          0x1234
#define WIFI_SSID_CHAR_UUID            0x1235
#define WIFI_PASSWORD_CHAR_UUID        0x1236
#define WIFI_STATUS_CHAR_UUID          0x1237

// GATT handles
#define WIFI_PROV_SERVICE_HANDLE        1
#define WIFI_SSID_CHAR_HANDLE          2
#define WIFI_PASSWORD_CHAR_HANDLE      3
#define WIFI_STATUS_CHAR_HANDLE        4

class BluetoothProvisioning {
private:
    static BluetoothProvisioning* instance_;
    std::string device_name_ = "BuddyPal";
    bool is_started_ = false;
    std::string received_ssid_;
    std::string received_password_;

    // GATT相关
    esp_gatt_if_t gatts_if_;
    uint16_t service_handle_;
    uint16_t ssid_char_handle_;
    uint16_t password_char_handle_;
    uint16_t status_char_handle_;
    uint16_t conn_id_;
    bool is_connected_;

    std::function<void(const std::string&, const std::string&)> on_credentials_received_;
    std::function<void(bool)> on_device_connected_;

    static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

    void InitBluetooth();
    void StartAdvertising();
    void CreateGattService();
    void HandleWrite(esp_ble_gatts_cb_param_t *param);

    // Constructor
    BluetoothProvisioning();

public:
    static BluetoothProvisioning& GetInstance();
    
    void SetDeviceName(const std::string& name) { device_name_ = name; }
    std::string GetDeviceName() const { return device_name_; }
    
    void Start();
    void Stop();
    bool IsStarted() const { return is_started_; }
    bool IsConnected() const { return is_connected_; }
    
    void OnCredentialsReceived(std::function<void(const std::string&, const std::string&)> callback) {
        on_credentials_received_ = callback;
    }
    
    void OnDeviceConnected(std::function<void(bool)> callback) {
        on_device_connected_ = callback;
    }
    
    void SendConnectionStatus(bool success);
    void SendDebugMessage(const std::string& message);
};

#else // CONFIG_USE_BLUETOOTH_PROVISIONING

#include <string>
#include <functional>

// 声明，当蓝牙配网功能被禁用时使用
class BluetoothProvisioning {
private:
    static BluetoothProvisioning* instance_;
    std::string device_name_ = "BuddyPal";
    bool is_started_ = false;
    bool is_connected_ = false;
    uint16_t gatts_if_ = 0;
    uint16_t conn_id_ = 0;
    uint16_t status_char_handle_ = 0;
    
public:
    static BluetoothProvisioning& GetInstance();
    
    void SetDeviceName(const std::string& name);
    std::string GetDeviceName() const;
    
    void Start();
    void Stop();
    bool IsStarted() const;
    bool IsConnected() const;
    
    void OnCredentialsReceived(std::function<void(const std::string&, const std::string&)> callback);
    void OnDeviceConnected(std::function<void(bool)> callback);
    
    void SendConnectionStatus(bool success);
    void SendDebugMessage(const std::string& message);
};

#endif // CONFIG_USE_BLUETOOTH_PROVISIONING
#endif // BLUETOOTH_PROVISIONING_H