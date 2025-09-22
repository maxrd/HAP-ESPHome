#pragma once
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "esphome/components/daikin_s21/climate/daikin_s21_climate.h"

namespace esphome {
namespace homekit {

class ClimateAccessory {
 public:
  ClimateAccessory(daikin_s21::DaikinS21Climate *climate);
  void setup();
  void loop();

 protected:
  daikin_s21::DaikinS21Climate *climate_{nullptr};
  hap_acc_t *acc_{nullptr};
};

}  // namespace homekit
}  // namespace esphome
