#pragma once
#include <esphome/core/defines.h>
#ifdef USE_CLIMATE
#include <esphome/core/application.h>
#include <esphome/components/climate/climate.h>
#include <esphome/components/climate/climate_mode.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "const.h"
#include "hap_entity.h"

namespace esphome {
namespace homekit {

class Climate : public climate::Climate, public HAPEntity {
 public:
  Climate() {}

  void setup() override {
    // 建立 HomeKit Thermostat 服務
    hap_serv_t *service = hap_serv_thermostat();

    // 必要的 Characteristics
    hap_serv_add_char(service, hap_char_current_heating_cooling_state(0));
    hap_serv_add_char(service, hap_char_target_heating_cooling_state(0));
    hap_serv_add_char(service, hap_char_current_temperature(20.0f));
    hap_serv_add_char(service, hap_char_target_temperature(22.0f));
    hap_serv_add_char(service, hap_char_temperature_display_units(0));

    // 掛到 HAP accessory
    this->hap_setup(service);
  }

  void dump_config() override {
    ESP_LOGCONFIG("homekit.climate", "HomeKit Climate");
  }

 protected:
  climate::ClimateTraits traits() override {
    auto t = climate::ClimateTraits();
    t.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_HEAT_COOL,
    });
    t.set_supports_current_temperature(true);
    t.set_supports_target_temperature(true);
    return t;
  }

  void control(const climate::ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      this->mode = *call.get_mode();
      this->publish_state();
    }
    if (call.get_target_temperature().has_value()) {
      this->target_temperature = *call.get_target_temperature();
      this->publish_state();
    }
  }
};

}  // namespace homekit
}  // namespace esphome

#endif
