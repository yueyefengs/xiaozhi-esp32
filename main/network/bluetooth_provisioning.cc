#include "bluetooth_provisioning.h"

#if CONFIG_USE_BLUETOOTH_PROVISIONING

#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "nvs_flash.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "BluetoothProvisioning";

BluetoothProvisioning* BluetoothProvisioning::instance_ = nullptr;

BluetoothProvisioning& BluetoothProvisioning::GetInstance() {
    if (!instance_) {
        instance_ = new BluetoothProvisioning();
    }
    return *instance_;
}

void BluetoothProvisioning::Start() {
    if (is_started_) return;
    
    ESP_LOGI(TAG, "Starting Bluetooth provisioning");
    InitBluetooth();
    CreateGattService();
    StartAdvertising();
    is_started_ = true;
}

void BluetoothProvisioning::InitBluetooth() {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    
    esp_bluedroid_init();
    esp_bluedroid_enable();
    
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    
    esp_ble_gatts_app_register(0);
}

void BluetoothProvisioning::StartAdvertising() {
    esp_ble_adv_data_t adv_data = {};
    adv_data.set_scan_rsp = false;
    adv_data.include_name = true;
    adv_data.include_txpower = true;
    adv_data.min_interval = 0x0006;
    adv_data.max_interval = 0x0010;
    adv_data.appearance = 0x00;
    adv_data.manufacturer_len = 0;
    adv_data.p_manufacturer_data = nullptr;
    adv_data.service_data_len = 0;
    adv_data.p_service_data = nullptr;
    adv_data.service_uuid_len = 0;
    adv_data.p_service_uuid = nullptr;
    adv_data.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    
    esp_ble_gap_config_adv_data(&adv_data);
    
    esp_ble_adv_params_t adv_params = {};
    adv_params.adv_int_min = 0x20;
    adv_params.adv_int_max = 0x40;
    adv_params.adv_type = ADV_TYPE_IND;
    adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    adv_params.channel_map = ADV_CHNL_ALL;
    adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
    
    esp_ble_gap_start_advertising(&adv_params);
}

void BluetoothProvisioning::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    auto& instance = GetInstance();
    
    switch (event) {
        case ESP_GATTS_WRITE_EVT: {
            if (param->write.len > 0) {
                std::string data((char*)param->write.value, param->write.len);
                
                // 根据特征UUID处理数据
                if (param->write.handle == /* SSID特征句柄 */) {
                    instance.received_ssid_ = data;
                    ESP_LOGI(TAG, "Received SSID: %s", data.c_str());
                } else if (param->write.handle == /* Password特征句柄 */) {
                    instance.received_password_ = data;
                    ESP_LOGI(TAG, "Received Password");
                    
                    // 收到完整凭据，触发回调
                    if (instance.on_credentials_received_ && !instance.received_ssid_.empty()) {
                        instance.on_credentials_received_(instance.received_ssid_, instance.received_password_);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void BluetoothProvisioning::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising data set complete");
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising started");
            break;
        default:
            break;
    }
}

void BluetoothProvisioning::SendConnectionStatus(bool success) {
    // 通过GATT特征通知手机连接状态
    std::string status = success ? "SUCCESS" : "FAILED";
    // 实现GATT通知逻辑
}

#endif // CONFIG_USE_BLUETOOTH_PROVISIONING