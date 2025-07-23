#include "bluetooth_provisioning.h"
#include <esp_log.h>

static const char* TAG = "BluetoothProvisioning";
BluetoothProvisioning* BluetoothProvisioning::instance_ = nullptr;

#if CONFIG_USE_BLUETOOTH_PROVISIONING

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "nvs_flash.h"
#include <cstring>
#include <inttypes.h>

BluetoothProvisioning::BluetoothProvisioning() 
    : gatts_if_(ESP_GATT_IF_NONE), service_handle_(0), 
      ssid_char_handle_(0), password_char_handle_(0), 
      status_char_handle_(0), conn_id_(0), is_connected_(false) {
}

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

void BluetoothProvisioning::CreateGattService() {
    ESP_LOGI(TAG, "Creating GATT service");
    
    // 创建服务
    esp_gatt_srvc_id_t service_id = {};
    service_id.is_primary = true;
    service_id.id.inst_id = 0x00;
    service_id.id.uuid.len = ESP_UUID_LEN_16;
    service_id.id.uuid.uuid.uuid16 = WIFI_PROV_SERVICE_UUID;
    
    esp_ble_gatts_create_service(gatts_if_, &service_id, 8); // 需要8个handle
}

void BluetoothProvisioning::HandleWrite(esp_ble_gatts_cb_param_t *param) {
    if (param->write.len > 0) {
        std::string data((char*)param->write.value, param->write.len);
        
        ESP_LOGI(TAG, "Received data: %s on handle %d", data.c_str(), param->write.handle);
        
        if (param->write.handle == ssid_char_handle_) {
            received_ssid_ = data;
            ESP_LOGI(TAG, "Received SSID: %s", data.c_str());
        } else if (param->write.handle == password_char_handle_) {
            received_password_ = data;
            ESP_LOGI(TAG, "Received Password");
            
            // 收到完整凭据，触发回调
            if (on_credentials_received_ && !received_ssid_.empty()) {
                on_credentials_received_(received_ssid_, received_password_);
            }
        }
        
        // 发送写入响应
        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if_, param->write.conn_id,
                                      param->write.trans_id, ESP_GATT_OK, nullptr);
        }
    }
}

void BluetoothProvisioning::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    auto& instance = GetInstance();
    
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            ESP_LOGI(TAG, "GATTS_REG_EVT, app_id %04x", param->reg.app_id);
            instance.gatts_if_ = gatts_if;
            
            // 设置设备名称
            esp_ble_gap_set_device_name(instance.device_name_.c_str());
            
            // 创建服务
            instance.CreateGattService();
            break;
        }
        
        case ESP_GATTS_CREATE_EVT: {
            ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d, service_handle %d", 
                    param->create.status, param->create.service_handle);
            
            instance.service_handle_ = param->create.service_handle;
            
            // 启动服务
            esp_ble_gatts_start_service(instance.service_handle_);
            
            // 添加SSID特征
            esp_bt_uuid_t ssid_char_uuid = {
                .len = ESP_UUID_LEN_16,
                .uuid = {.uuid16 = WIFI_SSID_CHAR_UUID},
            };
            
            esp_ble_gatts_add_char(instance.service_handle_, &ssid_char_uuid,
                                 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                 ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                                 nullptr, nullptr);
            break;
        }
        
        case ESP_GATTS_ADD_CHAR_EVT: {
            ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d, attr_handle %d, service_handle %d",
                    param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
            
            static int char_count = 0;
            char_count++;
            
            if (char_count == 1) {
                instance.ssid_char_handle_ = param->add_char.attr_handle;
                
                // 添加Password特征
                esp_bt_uuid_t password_char_uuid = {
                    .len = ESP_UUID_LEN_16,
                    .uuid = {.uuid16 = WIFI_PASSWORD_CHAR_UUID},
                };
                
                esp_ble_gatts_add_char(instance.service_handle_, &password_char_uuid,
                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                     ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                                     nullptr, nullptr);
            } else if (char_count == 2) {
                instance.password_char_handle_ = param->add_char.attr_handle;
                
                // 添加Status特征
                esp_bt_uuid_t status_char_uuid = {
                    .len = ESP_UUID_LEN_16,
                    .uuid = {.uuid16 = WIFI_STATUS_CHAR_UUID},
                };
                
                esp_ble_gatts_add_char(instance.service_handle_, &status_char_uuid,
                                     ESP_GATT_PERM_READ,
                                     ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                     nullptr, nullptr);
            } else if (char_count == 3) {
                instance.status_char_handle_ = param->add_char.attr_handle;
                ESP_LOGI(TAG, "All characteristics added successfully");
            }
            break;
        }
        
        case ESP_GATTS_CONNECT_EVT: {
            ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d", param->connect.conn_id);
            instance.conn_id_ = param->connect.conn_id;
            instance.is_connected_ = true;
            
            if (instance.on_device_connected_) {
                instance.on_device_connected_(true);
            }
            break;
        }
        
        case ESP_GATTS_DISCONNECT_EVT: {
            ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, conn_id %d", param->disconnect.conn_id);
            instance.is_connected_ = false;
            instance.conn_id_ = 0;
            
            if (instance.on_device_connected_) {
                instance.on_device_connected_(false);
            }
            
            // 重新开始广播
            instance.StartAdvertising();
            break;
        }
        
        case ESP_GATTS_WRITE_EVT: {
            ESP_LOGI(TAG, "GATTS_WRITE_EVT, conn_id %" PRIu16 ", trans_id %" PRIu32 ", handle %" PRIu16,
                    param->write.conn_id, param->write.trans_id, param->write.handle);
            instance.HandleWrite(param);
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
    if (!is_connected_ || status_char_handle_ == 0) {
        ESP_LOGW(TAG, "Cannot send status - device not connected or characteristic not ready");
        return;
    }
    
    std::string status = success ? "SUCCESS" : "FAILED";
    ESP_LOGI(TAG, "Sending connection status: %s", status.c_str());
    
    esp_ble_gatts_send_indicate(gatts_if_, conn_id_, status_char_handle_,
                               status.length(), (uint8_t*)status.c_str(), false);
}

void BluetoothProvisioning::SendDebugMessage(const std::string& message) {
    if (!is_connected_ || status_char_handle_ == 0) {
        return;
    }
    
    std::string debug_msg = "DEBUG: " + message;
    ESP_LOGI(TAG, "Sending debug message: %s", debug_msg.c_str());
    
    esp_ble_gatts_send_indicate(gatts_if_, conn_id_, status_char_handle_,
                               debug_msg.length(), (uint8_t*)debug_msg.c_str(), false);
}

void BluetoothProvisioning::Stop() {
    if (!is_started_) return;
    
    ESP_LOGI(TAG, "Stopping Bluetooth provisioning");
    
    if (is_connected_) {
        esp_ble_gatts_close(gatts_if_, conn_id_);
    }
    
    esp_ble_gap_stop_advertising();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    
    is_started_ = false;
    is_connected_ = false;
}

#else // CONFIG_USE_BLUETOOTH_PROVISIONING

BluetoothProvisioning& BluetoothProvisioning::GetInstance() {
    if (!instance_) {
        instance_ = new BluetoothProvisioning();
    }
    return *instance_;
}

void BluetoothProvisioning::SetDeviceName(const std::string& name) {
    ESP_LOGD(TAG, "Bluetooth provisioning disabled - SetDeviceName ignored");
    device_name_ = name;
}

std::string BluetoothProvisioning::GetDeviceName() const {
    return device_name_;
}

void BluetoothProvisioning::Start() {
    ESP_LOGD(TAG, "Bluetooth provisioning disabled - Start ignored");
}

void BluetoothProvisioning::Stop() {
    ESP_LOGD(TAG, "Bluetooth provisioning disabled - Stop ignored");
}

bool BluetoothProvisioning::IsStarted() const {
    return is_started_;
}

bool BluetoothProvisioning::IsConnected() const {
    return is_connected_;
}

void BluetoothProvisioning::OnCredentialsReceived(std::function<void(const std::string&, const std::string&)> callback) {
    ESP_LOGD(TAG, "Bluetooth provisioning disabled - OnCredentialsReceived ignored");
}

void BluetoothProvisioning::OnDeviceConnected(std::function<void(bool)> callback) {
    ESP_LOGD(TAG, "Bluetooth provisioning disabled - OnDeviceConnected ignored");  
}

void BluetoothProvisioning::SendConnectionStatus(bool success) {
    ESP_LOGD(TAG, "Bluetooth provisioning disabled - SendConnectionStatus ignored");
}

void BluetoothProvisioning::SendDebugMessage(const std::string& message) {
    ESP_LOGD(TAG, "Bluetooth provisioning disabled - SendDebugMessage ignored");
}

#endif // CONFIG_USE_BLUETOOTH_PROVISIONING