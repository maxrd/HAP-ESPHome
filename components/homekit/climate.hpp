#pragma once
#include <esphome/core/defines.h>
#ifdef USE_CLIMATE
#include <esphome/core/application.h>
#include <esphome/components/climate/climate_mode.h>
#include <esphome/components/climate/climate_traits.h>
#include <esphome/components/climate/climate.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"
#include "const.h"

namespace esphome {
namespace homekit {

class ClimateEntity : public HapEntity {
 public:
  ClimateEntity() : accessory_(nullptr), service_(nullptr) {}

  void setup() override {
    // 防呆檢查
    if (!this) {
      ESP_LOGW("homekit.climate", "ClimateEntity pointer NULL, skipping setup");
      return;
    }

    // 初始化 HomeKit accessory
    hap_acc_cfg_t acc_cfg{};
    acc_cfg.category = HAP_CID_THERMOSTAT;
    acc_cfg.pmk = nullptr; // 可以自定義 PMK
    acc_cfg.sn = acc_serial; // 設定 serial
    accessory_ = hap_acc_create(&acc_cfg);
    if (!accessory_) {
      ESP_LOGW("homekit.climate", "hap_acc_create failed, skipping accessory setup");
      return;
    }

    // 初始化 HomeKit Thermostat Service
    service_ = hap_serv_thermostat_create();
    if (!service_) {
      ESP_LOGW("homekit.climate", "hap_serv_thermostat_create failed, skipping service add");
      return;
    }

    hap_acc_add_serv(accessory_, service_);

    // 不要用 if(!hap_add_bridged_accessory(...))，它返回 void
    hap_add_bridged_accessory(accessory_, hap_get_unique_aid(acc_serial));
  }

  void loop() override {
    // 防呆檢查
    if (!accessory_ || !service_) return;
    // 可放你的 climate loop 處理
  }

 private:
  hap_acc_t* accessory_;
  hap_serv_t* service_;
};

}  // namespace homekit
}  // namespace esphome
#endif
