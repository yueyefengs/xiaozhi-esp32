#ifndef ZHENGCHEN_GC9A01_DISPLAY_H
#define ZHENGCHEN_GC9A01_DISPLAY_H

#include "display/lcd_display.h"
#include "lvgl_theme.h"
#include <esp_lvgl_port.h>
#include <esp_log.h>
#include <inttypes.h>

class ZHENGCHEN_Gc9a01Display : public SpiLcdDisplay {
protected:
    lv_obj_t* high_temp_popup_ = nullptr;  // 高温警告弹窗
    lv_obj_t* high_temp_label_ = nullptr;  // 高温警告标签

public:
    // 使用基类构造函数
    using SpiLcdDisplay::SpiLcdDisplay;

    void SetupHighTempWarningPopup() {
        auto lvgl_theme = static_cast<LvglTheme*>(current_theme_);
        auto text_font = lvgl_theme->text_font()->font();
        // 创建高温警告弹窗
        high_temp_popup_ = lv_obj_create(lv_scr_act());
        lv_obj_set_size(high_temp_popup_, 200, 100);
        lv_obj_center(high_temp_popup_);
        lv_obj_set_style_bg_color(high_temp_popup_, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_bg_opa(high_temp_popup_, LV_OPA_80, 0);
        lv_obj_add_flag(high_temp_popup_, LV_OBJ_FLAG_HIDDEN);

        // 创建警告文本
        high_temp_label_ = lv_label_create(high_temp_popup_);
        lv_label_set_text(high_temp_label_, "High Temperature\nWarning!");
        lv_obj_set_style_text_color(high_temp_label_, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(high_temp_label_, text_font, 0);
        lv_obj_center(high_temp_label_);
    }

    void UpdateHighTempWarning(float chip_temp, float threshold = 75.0f) {
        if (high_temp_popup_ == nullptr) {
            ESP_LOGW("ZHENGCHEN_Gc9a01Display", "High temp popup not initialized!");
            return;
        }

        if (chip_temp > threshold) {
            ShowHighTempWarning();
        } else {
            HideHighTempWarning();
        }
    }

    void ShowHighTempWarning() {
        if (high_temp_popup_ != nullptr) {
            lv_obj_clear_flag(high_temp_popup_, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void HideHighTempWarning() {
        if (high_temp_popup_ != nullptr) {
            lv_obj_add_flag(high_temp_popup_, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // 圆形显示器特有的遮罩功能
    void SetupCircularMask() {
        // 为圆形显示器创建圆形遮罩
        lv_obj_t* mask_obj = lv_obj_create(lv_scr_act());
        lv_obj_set_size(mask_obj, 240, 240);
        lv_obj_set_pos(mask_obj, 0, 0);
        lv_obj_set_style_radius(mask_obj, 120, 0);
        lv_obj_set_style_clip_corner(mask_obj, true, 0);
        lv_obj_set_style_bg_opa(mask_obj, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(mask_obj, 0, 0);
    }
};

#endif // ZHENGCHEN_GC9A01_DISPLAY_H