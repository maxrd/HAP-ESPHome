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

class ClimateEntity : public HAPEntity {
 public:
  ClimateEntity(climate::Climate *climate) : climate_(climate) {}

  void setup() override {
    if (climate_ == nullptr) {
      ESP_LOGW("homekit.climate", "No climate device bound, skipping setup.");
      return;
    }

    this->service_ = hap_serv_thermostat();
    if (!this->service_) {
      ESP_LOGE("homekit.climate", "Failed to create thermostat service");
      return;
    }

    hap_serv_add_char(this->service_, hap_char_current_heating_cooling_state());
    hap_serv_add_char(this->service_, hap_char_target_heating_cooling_state());
    hap_serv_add_char(this->service_, hap_char_current_temperature());
    hap_serv_add_char(this->service_, hap_char_target_temperature());
    hap_serv_add_char(this->service_, hap_char_temperature_display_units());

    hap_add_serv(this->service_);

    ESP_LOGI("homekit.climate", "HomeKit climate service created");
  }

  void update() override {
    if (climate_ == nullptr) {
      ESP_LOGW("homekit.climate", "update() called but climate is null");
      return;
    }

    auto *s = this->service_;
    if (!s) {
      ESP_LOGW("homekit.climate", "update() called but service is null");
      return;
    }

    hap_val_t v;

    // Current temperature
    if (!isnan(climate_->current_temperature)) {
      v.f = climate_->current_temperature;
      hap_serv_set_val(s, HAP_CHAR_CURRENT_TEMPERATURE, &v);
    } else {
      ESP_LOGW("homekit.climate", "current_temperature is NaN");
    }

    // Target temperature
    if (!isnan(climate_->target_temperature)) {
      v.f = climate_->target_temperature;
      hap_serv_set_val(s, HAP_CHAR_TARGET_TEMPERATURE, &v);
    } else {
      ESP_LOGW("homekit.climate", "target_temperature is NaN");
    }

    // Current mode
    v.u = (uint8_t) climate_to_homekit_mode(climate_->mode);
    hap_serv_set_val(s, HAP_CHAR_CURRENT_HEATING_COOLING_STATE, &v);
    hap_serv_set_val(s, HAP_CHAR_TARGET_HEATING_COOLING_STATE, &v);

    // Units (always Celsius for ESPHome)
    v.u = 0;
    hap_serv_set_val(s, HAP_CHAR_TEMPERATURE_DISPLAY_UNITS, &v);
  }

 private:
  climate::Climate *climate_{nullptr};

  static uint8_t climate_to_homekit_mode(climate::ClimateMode mode) {
    switch (mode) {
      case climate::CLIMATE_MODE_HEAT: 
        return 1;  // Heat
      case climate::CLIMATE_MODE_COOL: 
        return 2;  // Cool
      case climate::CLIMATE_MODE_HEAT_COOL: 
        return 3;  // Auto
      case climate::CLIMATE_MODE_OFF:
      default: 
        return 0;  // Off
    }
  }
};

}  // namespace homekit
}  // namespace esphome

#endif
