#ifndef BLUETOOTH_PROVISIONING_H
#define BLUETOOTH_PROVISIONING_H

#if CONFIG_USE_BLUETOOTH_PROVISIONING

#include <string>
#include <functional>
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

// 服务和特征UUID
#define WIFI_PROV_SERVICE_UUID          0x1234
#define WIFI_SSID_CHAR_UUID            0x1235
#define WIFI_PASSWORD_CHAR_UUID        0x1236
#define WIFI_STATUS_CHAR_UUID          0x1237

class BluetoothProvisioning {
private:
    static BluetoothProvisioning* instance_;
    std::string device_name_ = "BuddyPal";
    bool is_started_ = false;
    std::string received_ssid_;
    std::string received_password_;
    
    std::function<void(const std::string&, const std::string&)> on_credentials_received_;
    
    static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    
    void InitBluetooth();
    void StartAdvertising();
    void CreateGattService();
    
public:
    static BluetoothProvisioning& GetInstance();
    
    void SetDeviceName(const std::string& name) { device_name_ = name; }
    std::string GetDeviceName() const { return device_name_; }
    
    void Start();
    void Stop();
    bool IsStarted() const { return is_started_; }
    
    void OnCredentialsReceived(std::function<void(const std::string&, const std::string&)> callback) {
        on_credentials_received_ = callback;
    }
    
    void SendConnectionStatus(bool success);
};

#endif // CONFIG_USE_BLUETOOTH_PROVISIONING
#endif // BLUETOOTH_PROVISIONING_H