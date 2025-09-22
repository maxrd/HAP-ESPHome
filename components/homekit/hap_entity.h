#pragma once
#include "const.h"
#include "esphome/core/entity_base.h"
#include <esp_log.h>
#include <map>
#include <string>

namespace esphome {
namespace homekit {

class HAPEntity {
 private:
  static constexpr const char* TAG = "HAPEntity";

 protected:
  std::map<AInfo, std::string> accessory_info = {
      {NAME, ""},
      {MODEL, "HAP-GENERIC"},
      {SN, ""},
      {MANUFACTURER, "rednblkx"},
      {FW_REV, "0.1"}};

 public:
  HAPEntity() {}

  explicit HAPEntity(const std::map<AInfo, std::string>& info) { setInfo(info); }

  void setInfo(const std::map<AInfo, std::string>& info) {
    for (const auto& pair : info) {
      if (accessory_info.find(pair.first) != accessory_info.end()) {
        accessory_info[pair.first] = pair.second;
      }
    }
  }

  virtual void setup() {
    ESP_LOGI(TAG, "Setup not implemented for this entity!");
  }

  std::string getInfo(AInfo key) const {
    auto it = accessory_info.find(key);
    if (it != accessory_info.end())
      return it->second;
    return "";
  }
};

}  // namespace homekit
}  // namespace esphome
