#pragma once
#include <esphome/core/defines.h>
#ifdef USE_SENSOR
#include <map>
#include "const.h"
#include <esphome/core/application.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"

namespace esphome {
namespace homekit {

class SensorEntity : public HAPEntity {
 private:
  static constexpr const char* TAG = "SensorEntity";
  sensor::Sensor* sensorPtr_;
  hap_acc_t* accessory_;
  hap_serv_t* service_;

  static void on_sensor_update(sensor::Sensor* obj, float v) {
    if (!obj) return;
    hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(obj->get_object_id_hash()).c_str()));
    if (!acc) return;
    hap_serv_t* hs = hap_serv_get_next(hap_acc_get_first_serv(acc));
    if (!hs) return;
    hap_char_t* on_char = hap_serv_get_first_char(hs);
    if (!on_char) return;
    hap_val_t state{};
    if (ceilf(v) == v) {
      state.u = v;
    } else {
      state.f = v;
    }
    hap_char_update_val(on_char, &state);
  }

  static int sensor_read(hap_char_t* hc, hap_status_t* status_code, void* serv_priv, void* read_priv) {
    if (!serv_priv || !hc) return HAP_FAIL;
    sensor::Sensor* sensorPtr = static_cast<sensor::Sensor*>(serv_priv);
    float v = sensorPtr->get_state();
    hap_val_t sensorValue{};
    if (ceilf(v) == v) {
      sensorValue.u = v;
    } else {
      sensorValue.f = v;
    }
    hap_char_update_val(hc, &sensorValue);
    return HAP_SUCCESS;
  }

  static int acc_identify(hap_acc_t* ha) {
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
  }

 public:
  SensorEntity(sensor::Sensor* sensorPtr)
      : HAPEntity({{MODEL, "HAP-SENSOR"}}), sensorPtr_(sensorPtr), accessory_(nullptr), service_(nullptr) {}

  void setup() override {
    if (!sensorPtr_) {
      ESP_LOGW(TAG, "sensorPtr is NULL, skipping setup");
      return;
    }

    // 判斷 device_class
    std::string device_class = sensorPtr_->get_device_class();
    if (device_class == "temperature") {
      service_ = hap_serv_temperature_sensor_create(sensorPtr_->state);
    } else if (device_class == "humidity") {
      service_ = hap_serv_humidity_sensor_create(sensorPtr_->state);
    } else if (device_class == "illuminance") {
      service_ = hap_serv_light_sensor_create(sensorPtr_->state);
    } else if (device_class == "aqi") {
      service_ = hap_serv_air_quality_sensor_create(sensorPtr_->state);
    } else if (device_class == "carbon_dioxide") {
      service_ = hap_serv_carbon_dioxide_sensor_create(false);
    } else if (device_class == "carbon_monoxide") {
      service_ = hap_serv_carbon_monoxide_sensor_create(false);
    } else if (device_class == "pm10") {
      service_ = hap_serv_create(HAP_SERV_UUID_AIR_QUALITY_SENSOR);
      hap_serv_add_char(service_, hap_char_pm_10_density_create(sensorPtr_->state));
    } else if (device_class == "pm25") {
      service_ = hap_serv_create(HAP_SERV_UUID_AIR_QUALITY_SENSOR);
      hap_serv_add_char(service_, hap_char_pm_2_5_density_create(sensorPtr_->state));
    }

    if (!service_) return;

    hap_acc_cfg_t acc_cfg{};
    acc_cfg.model = strdup(accessory_info[MODEL]);
    acc_cfg.manufacturer = strdup(accessory_info[MANUFACTURER]);
    acc_cfg.fw_rev = strdup(accessory_info[FW_REV]);
    acc_cfg.hw_rev = nullptr;
    acc_cfg.pv = strdup("1.1.0");
    acc_cfg.cid = HAP_CID_BRIDGE;
    acc_cfg.identify_routine = acc_identify;

    std::string accessory_name = sensorPtr_->get_name();
    acc_cfg.name = accessory_info[NAME] ? strdup(accessory_info[NAME]) : strdup(accessory_name.c_str());
    acc_cfg.serial_num = accessory_info[SN] ? strdup(accessory_info[SN]) : strdup(std::to_string(sensorPtr_->get_object_id_hash()).c_str());

    accessory_ = hap_acc_create(&acc_cfg);
    if (!accessory_) {
      ESP_LOGW(TAG, "hap_acc_create failed, skipping accessory setup");
      return;
    }

    hap_serv_set_priv(service_, sensorPtr_);
    hap_serv_set_read_cb(service_, sensor_read);
    hap_acc_add_serv(accessory_, service_);
    hap_add_bridged_accessory(accessory_, hap_get_unique_aid(std::to_string(sensorPtr_->get_object_id_hash()).c_str()));

    if (!sensorPtr_->is_internal())
      sensorPtr_->add_on_state_callback([this](float v) { on_sensor_update(sensorPtr_, v); });

    ESP_LOGI(TAG, "Sensor '%s' linked to HomeKit", accessory_name.c_str());
  }
};

}  // namespace homekit
}  // namespace esphome
#endif
