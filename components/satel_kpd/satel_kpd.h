#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <string>

namespace esphome {
namespace satel_kpd {

enum SatelVariant {
  SATEL_VARIANT_CA6,
  SATEL_VARIANT_CA10
};

class SatelKPD : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_data_pin(InternalGPIOPin *pin) { data_pin_ = pin; }
  void set_ckl_pin(InternalGPIOPin *pin) { ckl_pin_ = pin; }
  void set_prs_pin(InternalGPIOPin *pin) { prs_pin_ = pin; }
  
  void set_simulated_keypad(bool sim) { simulated_keypad_ = sim; }
  void set_trouble_lang(const std::string &lang) {
    if (lang == "pl") trouble_lang_pl_ = true;
    else trouble_lang_pl_ = false;
  }
  void set_variant(const std::string &variant) {
    if (variant == "ca10") variant_ = SATEL_VARIANT_CA10;
    else variant_ = SATEL_VARIANT_CA6;
  }

  void set_bit_sensor(uint8_t index, binary_sensor::BinarySensor *sensor) {
    if (index < 32) bit_sensors_[index] = sensor;
  }

  void set_armed_a_sensor(binary_sensor::BinarySensor *s) { armed_a_sensor_ = s; }
  void set_armed_b_sensor(binary_sensor::BinarySensor *s) { armed_b_sensor_ = s; }
  void set_alarm_a_sensor(binary_sensor::BinarySensor *s) { alarm_a_sensor_ = s; }
  void set_alarm_b_sensor(binary_sensor::BinarySensor *s) { alarm_b_sensor_ = s; }
  void set_trouble_sensor(binary_sensor::BinarySensor *s) { trouble_sensor_ = s; }
  void set_buzzer_sensor(binary_sensor::BinarySensor *s) { buzzer_sensor_ = s; }
  void set_phone_sensor(binary_sensor::BinarySensor *s) { phone_sensor_ = s; }
  void set_power_sensor(binary_sensor::BinarySensor *s) { power_sensor_ = s; }

  void set_input_1_sensor(binary_sensor::BinarySensor *s) { input_sensors_[0] = s; }
  void set_input_2_sensor(binary_sensor::BinarySensor *s) { input_sensors_[1] = s; }
  void set_input_3_sensor(binary_sensor::BinarySensor *s) { input_sensors_[2] = s; }
  void set_input_4_sensor(binary_sensor::BinarySensor *s) { input_sensors_[3] = s; }
  void set_input_5_sensor(binary_sensor::BinarySensor *s) { input_sensors_[4] = s; }
  void set_input_6_sensor(binary_sensor::BinarySensor *s) { input_sensors_[5] = s; }
  void set_input_7_sensor(binary_sensor::BinarySensor *s) { input_sensors_[6] = s; }
  void set_input_8_sensor(binary_sensor::BinarySensor *s) { input_sensors_[7] = s; }
  void set_input_9_sensor(binary_sensor::BinarySensor *s) { input_sensors_[8] = s; }
  void set_input_10_sensor(binary_sensor::BinarySensor *s) { input_sensors_[9] = s; }
  void set_input_11_sensor(binary_sensor::BinarySensor *s) { input_sensors_[10] = s; }
  void set_input_12_sensor(binary_sensor::BinarySensor *s) { input_sensors_[11] = s; }

  void set_trouble_text_sensor(text_sensor::TextSensor *s) { trouble_text_sensor_ = s; }

  void press_sequence(std::string sequence);
  void trigger_trouble_check();

  static void IRAM_ATTR isr(SatelKPD *arg);

 protected:
  InternalGPIOPin *data_pin_{nullptr};
  InternalGPIOPin *ckl_pin_{nullptr};
  InternalGPIOPin *prs_pin_{nullptr};

  ISRInternalGPIOPin isr_data_pin_;
  ISRInternalGPIOPin isr_ckl_pin_;
  ISRInternalGPIOPin isr_prs_pin_;

  bool simulated_keypad_{false};
  bool trouble_lang_pl_{false};
  bool has_prs_pin_{false};
  SatelVariant variant_{SATEL_VARIANT_CA6};

  binary_sensor::BinarySensor *bit_sensors_[32]{nullptr};
  binary_sensor::BinarySensor *armed_a_sensor_{nullptr};
  binary_sensor::BinarySensor *armed_b_sensor_{nullptr};
  binary_sensor::BinarySensor *alarm_a_sensor_{nullptr};
  binary_sensor::BinarySensor *alarm_b_sensor_{nullptr};
  binary_sensor::BinarySensor *trouble_sensor_{nullptr};
  binary_sensor::BinarySensor *buzzer_sensor_{nullptr};
  binary_sensor::BinarySensor *phone_sensor_{nullptr};
  binary_sensor::BinarySensor *power_sensor_{nullptr};
  binary_sensor::BinarySensor *input_sensors_[12]{nullptr};
  
  text_sensor::TextSensor *trouble_text_sensor_{nullptr};

  volatile uint32_t last_interrupt_time_{0};
  volatile uint8_t bit_count_{0};
  volatile bool data_ready_{false};
  volatile bool clk_state_{false};
  volatile bool prev_clk_state_{false};
  
  volatile bool data_result_[32];
  volatile bool finalized_data_[32];

  volatile uint32_t sequence_masks_[6]{0};
  volatile uint8_t sequence_length_{0};
  volatile uint8_t sequence_index_{0};

  volatile uint32_t active_bits_{0};   
  volatile uint8_t frames_to_hold_{0};    
  volatile uint8_t frames_to_release_{0}; 
  volatile bool hold_key_continuous_{false}; 

  uint8_t trouble_check_state_{0}; 
  uint32_t trouble_state_timer_{0};
};

}  // namespace satel_kpd
}  // namespace esphome
