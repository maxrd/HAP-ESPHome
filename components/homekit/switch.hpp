#pragma once
#include <esphome/core/defines.h>
#ifdef USE_SWITCH
#include <esphome/core/application.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"

namespace esphome {
namespace homekit {

class SwitchEntity : public HAPEntity {
 private:
  static constexpr const char* TAG = "SwitchEntity";
  switch_::Switch* switchPtr;

  // HomeKit write callback
  static int switch_write(hap_write_data_t write_data[], int count, void* serv_priv, void* write_priv) {
    switch_::Switch* sw = (switch_::Switch*)serv_priv;
    if (!sw) return HAP_SUCCESS;

    ESP_LOGD(TAG, "Write called for Accessory %s (%s)",
             std::to_string(sw->get_object_id_hash()).c_str(),
             sw->get_name().c_str());

    for (int i = 0; i < count; i++) {
      hap_write_data_t* write = &write_data[i];
      if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
        ESP_LOGD(TAG, "Received Write for switch '%s' -> %s", sw->get_name().c_str(),
                 write->val.b ? "On" : "Off");

        write->val.b ? sw->turn_on() : sw->turn_off();
        hap_char_update_val(write->hc, &(write->val));
        *(write->status) = HAP_STATUS_SUCCESS;
      } else {
        *(write->status) = HAP_STATUS_RES_ABSENT;
      }
    }
    return HAP_SUCCESS;
  }

  // switch 狀態更新
  static void on_switch_update(switch_::Switch* sw, bool v) {
    ESP_LOGD(TAG, "%s state: %d", sw->get_name().c_str(), v);
    hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(sw->get_object_id_hash()).c_str()));
    if (!acc) return;
    hap_serv_t* hs = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_SWITCH);
    if (!hs) return;
    hap_char_t* on_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_ON);
    if (!on_char) return;

    hap_val_t state;
    state.b = v;
    hap_char_update_val(on_char, &state);
  }

  // identify callback
  static int acc_identify(hap_acc_t* ha) {
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
  }

 public:
  SwitchEntity(switch_::Switch* ptr) : HAPEntity({{MODEL, "HAP-SWITCH"}}), switchPtr(ptr) {}

  void setup() override {
    if (!switchPtr) {
      ESP_LOGW(TAG, "switchPtr is nullptr");
      return;
    }

    hap_acc_cfg_t acc_cfg = {
        .model = strdup(accessory_info[MODEL]),
        .manufacturer = strdup(accessory_info[MANUFACTURER]),
        .fw_rev = strdup(accessory_info[FW_REV]),
        .hw_rev = NULL,
        .pv = strdup("1.1.0"),
        .cid = HAP_CID_BRIDGE,
        .identify_routine = acc_identify,
    };

    std::string accessory_name = switchPtr->get_name();
    acc_cfg.name = strdup(accessory_name.c_str());
    acc_cfg.serial_num = strdup(std::to_string(switchPtr->get_object_id_hash()).c_str());

    // 建立 accessory
    hap_acc_t* accessory = hap_acc_create(&acc_cfg);

    // 建立 switch service
    hap_serv_t* service = hap_serv_switch_create(accessory_name.c_str());
    hap_serv_set_priv(service, switchPtr);
    hap_serv_set_write_cb(service, switch_write);

    hap_acc_add_serv(accessory, service);
    hap_add_bridged_accessory(accessory, hap_get_unique_aid(std::to_string(switchPtr->get_object_id_hash()).c_str()));

    if (!switchPtr->is_internal()) {
      switchPtr->add_on_state_callback([this](bool v) { SwitchEntity::on_switch_update(switchPtr, v); });
    }

    ESP_LOGI(TAG, "Switch '%s' linked to HomeKit", accessory_name.c_str());
  }
};

}  // namespace homekit
}  // namespace esphome
#endif
