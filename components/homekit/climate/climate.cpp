#include "climate.hpp"
#include "esphome/core/log.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/daikin_s21/climate/daikin_s21_climate.h"

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>

namespace esphome {
namespace homekit {

static const char *const TAG = "homekit.climate";

ClimateAccessory::ClimateAccessory(daikin_s21::DaikinS21Climate *climate)
    : climate_(climate) {}

void ClimateAccessory::setup() {
  ESP_LOGI(TAG, "Setting up HomeKit Climate accessory");

  hap_acc_cfg_t cfg = {
      .name = this->climate_->get_name().c_str(),
      .manufacturer = "Daikin",
      .model = "S21",
      .serial_num = "42424245",
      .fw_rev = "1.0.0",
      .hw_rev = nullptr,
      .pv = "1.1.0",
      .cid = HAP_CID_THERMOSTAT,
  };

  this->acc_ = hap_acc_create(&cfg);
  hap_serv_t *thermostat = hap_serv_thermostat_create(
      this->climate_->current_temperature, this->climate_->target_temperature,
      this->climate_->mode == climate::CLIMATE_MODE_COOL ? 1 : 0,
      this->climate_->mode == climate::CLIMATE_MODE_OFF ? 0 : 1);

  hap_serv_add_char(thermostat, hap_char_name_create(this->climate_->get_name().c_str()));

  hap_acc_add_serv(this->acc_, thermostat);

  hap_add_accessory(this->acc_);
}

void ClimateAccessory::loop() {
  if (!this->climate_)
    return;

  float temp = this->climate_->current_temperature;
  float target = this->climate_->target_temperature;
  auto mode = this->climate_->mode;

  hap_val_t cur_temp;
  cur_temp.f = temp;
  hap_serv_update_val_by_uuid(this->acc_, HAP_CHAR_CURRENT_TEMPERATURE, &cur_temp);

  hap_val_t tgt_temp;
  tgt_temp.f = target;
  hap_serv_update_val_by_uuid(this->acc_, HAP_CHAR_TARGET_TEMPERATURE, &tgt_temp);

  hap_val_t tgt_mode;
  tgt_mode.i = (mode == climate::CLIMATE_MODE_COOL) ? 1 :
               (mode == climate::CLIMATE_MODE_HEAT) ? 2 :
               (mode == climate::CLIMATE_MODE_DRY) ? 3 :
               (mode == climate::CLIMATE_MODE_FAN_ONLY) ? 4 : 0;
  hap_serv_update_val_by_uuid(this->acc_, HAP_CHAR_TARGET_HEATING_COOLING, &tgt_mode);
}

}  // namespace homekit
}  // namespace esphome
