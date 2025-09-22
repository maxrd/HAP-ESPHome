#pragma once
#include <hap.h>
#include <hap_apple_servs.h>
#include "hap_entity.h"

namespace esphome {
namespace homekit {

class SwitchEntity : public HapEntity {
 public:
  SwitchEntity() : accessory_(nullptr), service_(nullptr) {}

  void setup() override {
    if (!this) {
      ESP_LOGW("homekit.switch", "SwitchEntity pointer NULL, skipping setup");
      return;
    }

    // 初始化 HomeKit accessory
    hap_acc_cfg_t acc_cfg{};
    acc_cfg.category = HAP_CID_SWITCH;
    acc_cfg.sn = acc_serial;  // 你的序號
    accessory_ = hap_acc_create(&acc_cfg);
    if (!accessory_) {
      ESP_LOGW("homekit.switch", "hap_acc_create failed, skipping accessory setup");
      return;
    }

    // 初始化 Switch Service
    service_ = hap_serv_switch_create(name.c_str());
    if (!service_) {
      ESP_LOGW("homekit.switch", "hap_serv_switch_create failed, skipping service add");
      return;
    }

    hap_acc_add_serv(accessory_, service_);
    hap_add_bridged_accessory(accessory_, hap_get_unique_aid(acc_serial));
  }

 private:
  hap_acc_t* accessory_;
  hap_serv_t* service_;
};

}  // namespace homekit
}  // namespace esphome
