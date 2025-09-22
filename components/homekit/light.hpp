#pragma once
#include <esphome/core/defines.h>
#ifdef USE_LIGHT
#include <esphome/core/application.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"

namespace esphome {
namespace homekit {

class LightEntity : public HAPEntity {
 private:
  static constexpr const char* TAG = "LightEntity";
  light::LightState* lightPtr_;
  hap_acc_t* accessory_;
  hap_serv_t* service_;

  static int light_write(hap_write_data_t write_data[], int count, void* serv_priv, void* write_priv) {
    if (!serv_priv || !write_data) return HAP_FAIL;
    light::LightState* lightPtr = static_cast<light::LightState*>(serv_priv);
    if (!lightPtr) return HAP_FAIL;
    for (int i = 0; i < count; i++) {
      hap_write_data_t* write = &write_data[i];
      if (!write->hc || !write->status) continue;

      const char* uuid = hap_char_get_type_uuid(write->hc);
      if (!uuid) continue;

      if (!strcmp(uuid, HAP_CHAR_UUID_ON)) {
        write->val.b ? lightPtr->turn_on().set_save(true).perform() : lightPtr->turn_off().set_save(true).perform();
        hap_char_update_val(write->hc, &(write->val));
        *(write->status) = HAP_STATUS_SUCCESS;
      } else if (!strcmp(uuid, HAP_CHAR_UUID_BRIGHTNESS)) {
        lightPtr->make_call().set_save(true).set_brightness((float)(write->val.i) / 100).perform();
        hap_char_update_val(write->hc, &(write->val));
        *(write->status) = HAP_STATUS_SUCCESS;
      } else if (!strcmp(uuid, HAP_CHAR_UUID_HUE) || !strcmp(uuid, HAP_CHAR_UUID_SATURATION)) {
        int hue = 0;
        float saturation = 0, value = 0;
        rgb_to_hsv(lightPtr->remote_values.get_red(), lightPtr->remote_values.get_green(), lightPtr->remote_values.get_blue(), hue, saturation, value);
        float tR = 0, tG = 0, tB = 0;
        if (!strcmp(uuid, HAP_CHAR_UUID_HUE))
          hsv_to_rgb(write->val.f, saturation, value, tR, tG, tB);
        else
          hsv_to_rgb(hue, write->val.f / 100, value, tR, tG, tB);
        lightPtr->make_call().set_rgb(tR, tG, tB).set_save(true).perform();
        hap_char_update_val(write->hc, &(write->val));
        *(write->status) = HAP_STATUS_SUCCESS;
      } else if (!strcmp(uuid, HAP_CHAR_UUID_COLOR_TEMPERATURE)) {
        lightPtr->make_call().set_color_temperature(write->val.u).set_save(true).perform();
        hap_char_update_val(write->hc, &(write->val));
        *(write->status) = HAP_STATUS_SUCCESS;
      } else {
        *(write->status) = HAP_STATUS_RES_ABSENT;
      }
    }
    return HAP_SUCCESS;
  }

  static void on_light_update(light::LightState* obj) {
    if (!obj) return;
    hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(obj->get_object_id_hash()).c_str()));
    if (!acc) return;
    hap_serv_t* hs = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_LIGHTBULB);
    if (!hs) return;

    hap_char_t* on_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_ON);
    if (on_char) {
      hap_val_t state{};
      state.b = obj->current_values.get_state();
      hap_char_update_val(on_char, &state);
    }

    if (obj->get_traits().supports_color_capability(light::ColorCapability::BRIGHTNESS)) {
      hap_char_t* level_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_BRIGHTNESS);
      if (level_char) {
        hap_val_t level{};
        level.i = (int)(obj->current_values.get_brightness() * 100);
        hap_char_update_val(level_char, &level);
      }
    }

    if (obj->current_values.get_color_mode() & light::ColorCapability::RGB) {
      hap_char_t* hue_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_HUE);
      hap_char_t* saturation_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_SATURATION);
      if (hue_char && saturation_char) {
        int hue = 0;
        float saturation = 0, value = 0;
        rgb_to_hsv(obj->current_values.get_red(), obj->current_values.get_green(), obj->current_values.get_blue(), hue, saturation, value);
        hap_val_t h{}, s{};
        h.f = hue;
        s.f = saturation * 100;
        hap_char_update_val(hue_char, &h);
        hap_char_update_val(saturation_char, &s);
      }
    }

    if (obj->current_values.get_color_mode() & (light::ColorCapability::COLOR_TEMPERATURE | light::ColorCapability::COLD_WARM_WHITE)) {
      hap_char_t* temp_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_COLOR_TEMPERATURE);
      if (temp_char) {
        hap_val_t temp{};
        temp.u = obj->current_values.get_color_temperature();
        hap_char_update_val(temp_char, &temp);
      }
    }
  }

  static int acc_identify(hap_acc_t* ha) {
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
  }

 public:
  LightEntity(light::LightState* lightPtr)
      : HAPEntity({{MODEL, "HAP-LIGHT"}}), lightPtr_(lightPtr), accessory_(nullptr), service_(nullptr) {}

  void setup() override {
    if (!lightPtr_) {
      ESP_LOGW(TAG, "lightPtr is NULL, skipping setup");
      return;
    }

    hap_acc_cfg_t acc_cfg{};
    acc_cfg.model = strdup(accessory_info[MODEL]);
    acc_cfg.manufacturer = strdup(accessory_info[MANUFACTURER]);
    acc_cfg.fw_rev = strdup(accessory_info[FW_REV]);
    acc_cfg.hw_rev = nullptr;
    acc_cfg.pv = strdup("1.1.0");
    acc_cfg.cid = HAP_CID_BRIDGE;
    acc_cfg.identify_routine = acc_identify;

    std::string accessory_name = lightPtr_->get_name();
    acc_cfg.name = accessory_info[NAME] ? strdup(accessory_info[NAME]) : strdup(accessory_name.c_str());
    acc_cfg.serial_num = accessory_info[SN] ? strdup(accessory_info[SN]) : strdup(std::to_string(lightPtr_->get_object_id_hash()).c_str());

    accessory_ = hap_acc_create(&acc_cfg);
    if (!accessory_) {
      ESP_LOGW(TAG, "hap_acc_create failed, skipping accessory setup");
      return;
    }

    int hue = 0;
    float saturation = 0, value = 0;
    rgb_to_hsv(lightPtr_->current_values.get_red(), lightPtr_->current_values.get_green(), lightPtr_->current_values.get_blue(), hue, saturation, value);

    service_ = hap_serv_lightbulb_create(lightPtr_->current_values.get_state());
    if (!service_) return;

    hap_serv_add_char(service_, hap_char_name_create(accessory_name.c_str()));
    if (lightPtr_->get_traits().supports_color_capability(light::ColorCapability::BRIGHTNESS))
      hap_serv_add_char(service_, hap_char_brightness_create(lightPtr_->current_values.get_brightness() * 100));
    if (lightPtr_->get_traits().supports_color_capability(light::ColorCapability::RGB)) {
      hap_serv_add_char(service_, hap_char_hue_create(hue));
      hap_serv_add_char(service_, hap_char_saturation_create(saturation * 100));
    }
    if (lightPtr_->get_traits().supports_color_capability(light::ColorCapability::COLOR_TEMPERATURE) ||
        lightPtr_->get_traits().supports_color_capability(light::ColorCapability::COLD_WARM_WHITE))
      hap_serv_add_char(service_, hap_char_color_temperature_create(lightPtr_->current_values.get_color_temperature()));

    hap_serv_set_priv(service_, lightPtr_);
    hap_serv_set_write_cb(service_, light_write);

    hap_acc_add_serv(accessory_, service_);
    hap_add_bridged_accessory(accessory_, hap_get_unique_aid(std::to_string(lightPtr_->get_object_id_hash()).c_str()));

    if (!lightPtr_->is_internal())
      lightPtr_->add_new_target_state_reached_callback([this]() { on_light_update(lightPtr_); });

    ESP_LOGI(TAG, "Light '%s' linked to HomeKit", accessory_name.c_str());
  }
};

}  // namespace homekit
}  // namespace esphome
#endif
