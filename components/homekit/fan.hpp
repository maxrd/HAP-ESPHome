#pragma once
#include <esphome/core/defines.h>
#ifdef USE_FAN
#include <esphome/core/application.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"

namespace esphome {
namespace homekit {

class FanEntity : public HAPEntity {
 private:
  static constexpr const char* TAG = "FanEntity";
  fan::Fan* fanPtr_;
  hap_acc_t* accessory_;
  hap_serv_t* service_;

  static int fan_write(hap_write_data_t write_data[], int count, void* serv_priv, void* write_priv) {
    if (!serv_priv || !write_data) return HAP_FAIL;
    fan::Fan* fanPtr = static_cast<fan::Fan*>(serv_priv);
    if (!fanPtr) return HAP_FAIL;

    for (int i = 0; i < count; i++) {
      hap_write_data_t* write = &write_data[i];
      if (!write->hc || !write->status) continue;

      const char* uuid = hap_char_get_type_uuid(write->hc);
      if (!uuid) continue;

      if (!strcmp(uuid, HAP_CHAR_UUID_ON)) {
        fanPtr->make_call().set_state(write->val.b).perform();
        hap_char_update_val(write->hc, &(write->val));
        *(write->status) = HAP_STATUS_SUCCESS;
      } else {
        *(write->status) = HAP_STATUS_RES_ABSENT;
      }
    }
    return HAP_SUCCESS;
  }

  static void on_fan_update(fan::Fan* obj) {
    if (!obj) return;
    hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(obj->get_object_id_hash()).c_str()));
    if (!acc) return;
    hap_serv_t* hs = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_FAN);
    if (!hs) return;
    hap_char_t* on_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_ON);
    if (!on_char) return;

    hap_val_t state{};
    state.b = !!obj->state;
    hap_char_update_val(on_char, &state);
  }

  static int acc_identify(hap_acc_t* ha) {
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
  }

 public:
  FanEntity(fan::Fan* fanPtr) : HAPEntity({{MODEL, "HAP-FAN"}}), fanPtr_(fanPtr), accessory_(nullptr), service_(nullptr) {}

  void setup() override {
    if (!fanPtr_) {
      ESP_LOGW(TAG, "fanPtr is NULL, skipping setup");
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

    std::string accessory_name = fanPtr_->get_name();
    acc_cfg.name = accessory_info[NAME] ? strdup(accessory_info[NAME]) : strdup(accessory_name.c_str());
    acc_cfg.serial_num = accessory_info[SN] ? strdup(accessory_info[SN]) : strdup(std::to_string(fanPtr_->get_object_id_hash()).c_str());

    accessory_ = hap_acc_create(&acc_cfg);
    if (!accessory_) {
      ESP_LOGW(TAG, "hap_acc_create failed, skipping accessory setup");
      return;
    }

    service_ = hap_serv_fan_create(fanPtr_->state);
    if (!service_) return;

    hap_serv_set_priv(service_, fanPtr_);
    hap_serv_set_write_cb(service_, fan_write);

    hap_acc_add_serv(accessory_, service_);
    hap_add_bridged_accessory(accessory_, hap_get_unique_aid(std::to_string(fanPtr_->get_object_id_hash()).c_str()));

    if (!fanPtr_->is_internal())
      fanPtr_->add_on_state_callback([this]() { on_fan_update(fanPtr_); });

    ESP_LOGI(TAG, "Fan '%s' linked to HomeKit", accessory_name.c_str());
  }
};

}  // namespace homekit
}  // namespace esphome
#endif
