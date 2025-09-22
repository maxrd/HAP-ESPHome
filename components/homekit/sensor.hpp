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
namespace esphome
{
  namespace homekit
  {
    class SensorEntity : public HAPEntity
    {
    private:
      static constexpr const char* TAG = "SensorEntity";
      sensor::Sensor* sensorPtr;
      static void on_sensor_update(sensor::Sensor* obj, float v) {
        ESP_LOGD(TAG, "%s value: %.2f", obj->get_name().c_str(), v);
        hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(obj->get_object_id_hash()).c_str()));
        if (acc) {
          hap_serv_t* hs = hap_serv_get_next(hap_acc_get_first_serv(acc));
          if (hs) {
            hap_char_t* on_char = hap_serv_get_first_char(hs);
            ESP_LOGD(TAG, "HAP CURRENT VALUE - f:%.2f u:%lu", hap_char_get_val(on_char)->f, hap_char_get_val(on_char)->u);
            hap_val_t state;
            if (ceilf(v) == v) {
              state.u = v;
            } else {
              state.f = v;
            }
            hap_char_update_val(on_char, &state);
          }
        }
        return;
      }
      static int sensor_read(hap_char_t* hc, hap_status_t* status_code, void* serv_priv, void* read_priv) {
        if (serv_priv) {
          sensor::Sensor* sensorPtr = (sensor::Sensor*)serv_priv;
          ESP_LOGD(TAG, "Read called for Accessory %s (%s)", std::to_string(sensorPtr->get_object_id_hash()).c_str(), sensorPtr->get_name().c_str());
          hap_val_t sensorValue;
          float v = sensorPtr->get_state();
          if (ceilf(v) == v) {
            sensorValue.u = v;
          } else {
            sensorValue.f = v;
          }
          hap_char_update_val(hc, &sensorValue);
          return HAP_SUCCESS;
        }
        return HAP_FAIL;
      }
      static int acc_identify(hap_acc_t* ha) {
        ESP_LOGI(TAG, "Accessory identified");
        return HAP_SUCCESS;
      }
    public:
      SensorEntity(sensor::Sensor* sensorPtr) : HAPEntity({{MODEL, "HAP-SENSOR"}}), sensorPtr(sensorPtr) {}
      void setup() {
        hap_serv_t* service = nullptr;

        std::string device_class = sensorPtr->get_device_class();
        if (std::equal(device_class.begin(), device_class.end(), strdup("temperature"))) {
          service = hap_serv_temperature_sensor_create(sensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("humidity"))) {
          service = hap_serv_humidity_sensor_create(sensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("illuminance"))) {
          service = hap_serv_light_sensor_create(sensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("aqi"))) {
          service = hap_serv_air_quality_sensor_create(sensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("carbon_dioxide"))) {
          service = hap_serv_carbon_dioxide_sensor_create(false);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("carbon_monoxide"))) {
          service = hap_serv_carbon_monoxide_sensor_create(false);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("pm10"))) {
          service = hap_serv_create(HAP_SERV_UUID_AIR_QUALITY_SENSOR);
          hap_serv_add_char(service, hap_char_pm_10_density_create(sensorPtr->state));
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("pm25"))) {
          service = hap_serv_create(HAP_SERV_UUID_AIR_QUALITY_SENSOR);
          hap_serv_add_char(service, hap_char_pm_2_5_density_create(sensorPtr->state));
        }
        if (service) {
          hap_acc_cfg_t acc_cfg = {
              .model = strdup(accessory_info[MODEL]),
              .manufacturer = strdup(accessory_info[MANUFACTURER]),
              .fw_rev = strdup(accessory_info[FW_REV]),
              .hw_rev = NULL,
              .pv = strdup("1.1.0"),
              .cid = HAP_CID_BRIDGE,
              .identify_routine = acc_identify,
          };
          hap_acc_t* accessory = nullptr;
          std::string accessory_name = sensorPtr->get_name();
          if (accessory_info[NAME] == NULL) {
            acc_cfg.name = strdup(accessory_name.c_str());
          }
          else {
          acc_cfg.name = strdup(accessory_info[NAME]);
          }
          if (accessory_info[SN] == NULL) {
            acc_cfg.serial_num = strdup(std::to_string(sensorPtr->get_object_id_hash()).c_str());
          }
          else {
            acc_cfg.serial_num = strdup(accessory_info[SN]);
          }
          accessory = hap_acc_create(&acc_cfg);
          ESP_LOGD(TAG, "ID HASH: %lu", sensorPtr->get_object_id_hash());
          hap_serv_set_priv(service, sensorPtr);

          /* Set the read callback for the service */
          hap_serv_set_read_cb(service, sensor_read);

          /* Add the Sensor Service to the Accessory Object */
          hap_acc_add_serv(accessory, service);


          /* Add the Accessory to the HomeKit Database */
          hap_add_bridged_accessory(accessory, hap_get_unique_aid(std::to_string(sensorPtr->get_object_id_hash()).c_str()));
          if (!sensorPtr->is_internal())
            sensorPtr->add_on_state_callback([this](float v) { SensorEntity::on_sensor_update(sensorPtr, v); });
          ESP_LOGI(TAG, "Sensor '%s' linked to HomeKit", accessory_name.c_str());
        }
      }
    };
  }
}
#endif