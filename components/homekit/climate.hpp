#pragma once
#include <esphome/core/defines.h>
#ifdef USE_CLIMATE
#include <esphome/core/application.h>
#include <esphome/components/climate/climate_mode.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "const.h"
#include "hap_entity.h"

namespace esphome
{
  namespace homekit
  {
    class ClimateEntity : public HAPEntity
    {
    private:
      static constexpr const char* TAG = "ClimateEntity";
      climate::Climate* climatePtr;

      static void on_climate_update(climate::Climate* obj) {
        if (!obj) return;
        ESP_LOGI(TAG, "%s Mode: %d Action: %d CTemp: %.2f TTemp: %.2f CHum: %.2f THum: %.2f",
                 obj->get_name().c_str(), obj->mode, obj->action, obj->current_temperature, obj->target_temperature,
                 obj->current_humidity, obj->target_humidity);
        hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(obj->get_object_id_hash()).c_str()));
        if (acc) {
          hap_serv_t* hs = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_THERMOSTAT);
          if (!hs) return;
          hap_char_t* cur_temp = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_CURRENT_TEMPERATURE);
          hap_char_t* tar_temp = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_TARGET_TEMPERATURE);
          hap_char_t* cur_mode = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_CURRENT_HEATING_COOLING_STATE);
          hap_char_t* tar_mode = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_TARGET_HEATING_COOLING_STATE);
          hap_val_t state{};
          state.f = obj->current_temperature;
          hap_char_update_val(cur_temp, &state);
          state.f = obj->target_temperature;
          hap_char_update_val(tar_temp, &state);
          state.i = obj->action == climate::CLIMATE_ACTION_HEATING ? 1 :
                    obj->action == climate::CLIMATE_ACTION_COOLING ? 2 : 0;
          hap_char_update_val(cur_mode, &state);
          state.i = obj->mode == climate::CLIMATE_MODE_HEAT ? 1 :
                    obj->mode == climate::CLIMATE_MODE_COOL ? 2 :
                    obj->mode == climate::CLIMATE_MODE_AUTO ? 3 : 0;
          hap_char_update_val(tar_mode, &state);
          if (obj->get_traits().get_supports_current_humidity()) {
            hap_char_t* cur_hum = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_CURRENT_RELATIVE_HUMIDITY);
            state.f = obj->current_humidity;
            hap_char_update_val(cur_hum, &state);
          }
        }
      }

      static int climate_read(hap_char_t* hc, hap_status_t* status_code, void* serv_priv, void* read_priv) {
        auto obj = reinterpret_cast<climate::Climate*>(serv_priv);
        if (!obj) return HAP_SUCCESS;

        hap_val_t state{};
        const char* type = hap_char_get_type_uuid(hc);
        if (!strcmp(type, HAP_CHAR_UUID_CURRENT_HEATING_COOLING_STATE)) {
          state.i = obj->action == climate::CLIMATE_ACTION_HEATING ? 1 :
                    obj->action == climate::CLIMATE_ACTION_COOLING ? 2 : 0;
        } else if (!strcmp(type, HAP_CHAR_UUID_CURRENT_TEMPERATURE)) {
          state.f = obj->current_temperature;
        } else if (!strcmp(type, HAP_CHAR_UUID_CURRENT_RELATIVE_HUMIDITY)) {
          state.f = obj->current_humidity;
        }
        hap_char_update_val(hc, &state);
        return HAP_SUCCESS;
      }

      static int climate_write(hap_write_data_t write_data[], int count, void* serv_priv, void* write_priv) {
        auto obj = reinterpret_cast<climate::Climate*>(serv_priv);
        if (!obj) return HAP_SUCCESS;

        for (int i = 0; i < count; i++) {
          hap_write_data_t* write = &write_data[i];
          const char* type = hap_char_get_type_uuid(write->hc);
          if (!strcmp(type, HAP_CHAR_UUID_TARGET_HEATING_COOLING_STATE)) {
            switch (write->val.i) {
              case 0: obj->make_call().set_mode(climate::CLIMATE_MODE_OFF).perform(); break;
              case 1: obj->make_call().set_mode(climate::CLIMATE_MODE_HEAT).perform(); break;
              case 2: obj->make_call().set_mode(climate::CLIMATE_MODE_COOL).perform(); break;
              case 3: obj->make_call().set_mode(climate::CLIMATE_MODE_AUTO).perform(); break;
              default: break;
            }
          } else if (!strcmp(type, HAP_CHAR_UUID_TARGET_TEMPERATURE)) {
            obj->make_call().set_target_temperature(write->val.f).perform();
          } else if (!strcmp(type, HAP_CHAR_UUID_TARGET_RELATIVE_HUMIDITY)) {
            obj->make_call().set_target_humidity(write->val.f).perform();
          }
        }
        return HAP_SUCCESS;
      }

      static int acc_identify(hap_acc_t* ha) {
        ESP_LOGI(TAG, "Accessory identified");
        return HAP_SUCCESS;
      }

    public:
      ClimateEntity(climate::Climate* climatePtr)
          : HAPEntity({{MODEL, "HAP-CLIMATE"}}), climatePtr(climatePtr) {}

      void setup(TemperatureUnits units = CELSIUS) {
        if (!climatePtr) return;

        hap_acc_cfg_t acc_cfg{};
        static char acc_name[32];
        static char acc_serial[32];

        snprintf(acc_name, sizeof(acc_name), "%s", climatePtr->get_name().c_str());
        snprintf(acc_serial, sizeof(acc_serial), "%lu", climatePtr->get_object_id_hash());

        acc_cfg.name = acc_name;
        acc_cfg.serial_num = acc_serial;
        strcpy(acc_cfg.model, "ESP-CLIMATE");
        strcpy(acc_cfg.manufacturer, "rednblkx");
        strcpy(acc_cfg.fw_rev, "0.1.0");
        strcpy(acc_cfg.pv, "1.1.0");
        acc_cfg.hw_rev = nullptr;
        acc_cfg.cid = HAP_CID_BRIDGE;
        acc_cfg.identify_routine = acc_identify;

        uint8_t current_mode = 0, target_mode = 0;
        switch (climatePtr->action) {
          case climate::CLIMATE_ACTION_HEATING: current_mode = 1; break;
          case climate::CLIMATE_ACTION_COOLING: current_mode = 2; break;
          default: current_mode = 0; break;
        }
        switch (climatePtr->mode) {
          case climate::CLIMATE_MODE_HEAT: target_mode = 1; break;
          case climate::CLIMATE_MODE_COOL: target_mode = 2; break;
          case climate::CLIMATE_MODE_AUTO: target_mode = 3; break;
          default: target_mode = 0; break;
        }

        hap_serv_t* service = hap_serv_thermostat_create(current_mode, target_mode,
                                                         climatePtr->current_temperature,
                                                         climatePtr->target_temperature,
                                                         units);

        climate::ClimateTraits traits = climatePtr->get_traits();
        if (traits.get_supports_current_humidity()) {
          hap_serv_add_char(service, hap_char_current_relative_humidity_create(climatePtr->current_humidity));
        }
        if (traits.get_supports_target_humidity()) {
          hap_serv_add_char(service, hap_char_target_relative_humidity_create(climatePtr->target_humidity));
        }

        hap_acc_t* accessory = hap_acc_create(&acc_cfg);
        hap_serv_set_priv(service, climatePtr); // 直接傳物件指標
        hap_serv_set_write_cb(service, climate_write);
        hap_serv_set_read_cb(service, climate_read);
        hap_acc_add_serv(accessory, service);
        hap_add_bridged_accessory(accessory, hap_get_unique_aid(acc_serial));

        // 綁定更新 callback
        climatePtr->add_on_state_callback([this](bool) { ClimateEntity::on_climate_update(climatePtr); });

        ESP_LOGI(TAG, "Climate '%s' linked to HomeKit", climatePtr->get_name().c_str());
      }
    };
  }
}
#endif
