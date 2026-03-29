#include "satel_kpd.h"
#include "esphome/core/log.h"

namespace esphome {
namespace satel_kpd {

static const char *const TAG = "satel_kpd";
static const uint8_t DATA_FRAME_SIZE = 32;

void SatelKPD::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Satel KPD component...");
  if (this->data_pin_ != nullptr) {
    this->data_pin_->setup();
    this->isr_data_pin_ = this->data_pin_->to_isr(); 
  }
  if (this->ckl_pin_ != nullptr) {
    this->ckl_pin_->setup();
    this->isr_ckl_pin_ = this->ckl_pin_->to_isr();
    this->ckl_pin_->attach_interrupt(SatelKPD::isr, this, gpio::INTERRUPT_ANY_EDGE);
  }
  if (this->prs_pin_ != nullptr) {
    this->prs_pin_->setup();
    this->prs_pin_->digital_write(false); 
    this->isr_prs_pin_ = this->prs_pin_->to_isr();
    this->has_prs_pin_ = true;
  }

  if (this->trouble_text_sensor_ != nullptr) {
    if (this->trouble_lang_pl_) {
      this->trouble_text_sensor_->publish_state("Brak danych");
    } else {
      this->trouble_text_sensor_->publish_state("No data");
    }
  }
}

void SatelKPD::press_sequence(std::string sequence) {
  if (!this->has_prs_pin_) {
    ESP_LOGW(TAG, "Pin PRS nie jest skonfigurowany. Wysylanie komend zostalo zablokowane.");
    return;
  }

  uint8_t len = 0;
  for (char key : sequence) {
    if (len >= 6) break;
    uint32_t mask = 0;
    
    if (key == '1') mask = (1UL << 2) | (1UL << 14);
    else if (key == '2') mask = (1UL << 4) | (1UL << 14);
    else if (key == '3') mask = (1UL << 6) | (1UL << 14);
    else if (key == '4') mask = (1UL << 2) | (1UL << 12);
    else if (key == '5') mask = (1UL << 4) | (1UL << 12);
    else if (key == '6') mask = (1UL << 6) | (1UL << 12);
    else if (key == '7') mask = (1UL << 2) | (1UL << 10);
    else if (key == '8') mask = (1UL << 4) | (1UL << 10);
    else if (key == '9') mask = (1UL << 6) | (1UL << 10);
    else if (key == '*') mask = (1UL << 2) | (1UL << 8);
    else if (key == '0') mask = (1UL << 4) | (1UL << 8);
    else if (key == '#') mask = (1UL << 6) | (1UL << 8);

    if (mask != 0) this->sequence_masks_[len++] = mask;
  }

  if (len > 0) {
    ESP_LOGI(TAG, "Starting sequence transmission: %s", sequence.c_str());
    this->hold_key_continuous_ = false;
    this->sequence_length_ = len;
    this->sequence_index_ = 0;
    this->active_bits_ = this->sequence_masks_[0];
    this->frames_to_hold_ = 3;
    this->frames_to_release_ = 3;
  }
}

void SatelKPD::trigger_trouble_check() {
  if (!this->has_prs_pin_) {
    ESP_LOGW(TAG, "Pin PRS nie jest skonfigurowany. Diagnostyka zostala zablokowana.");
    return;
  }

  ESP_LOGI(TAG, "Starting trouble check procedure (holding key 7)...");
  this->active_bits_ = (1UL << 2) | (1UL << 10); 
  this->hold_key_continuous_ = true;             
  this->trouble_check_state_ = 1;                
  this->trouble_state_timer_ = millis();
}

void IRAM_ATTR SatelKPD::isr(SatelKPD *arg) {
  uint32_t interrupt_time = micros();

  if ((interrupt_time - arg->last_interrupt_time_ > 2000) && (arg->last_interrupt_time_ != 0)) {
    arg->bit_count_ = 0;
    arg->data_ready_ = false;
    if (arg->has_prs_pin_) arg->isr_prs_pin_.digital_write(false);
  }

  arg->last_interrupt_time_ = interrupt_time;
  arg->prev_clk_state_ = arg->clk_state_;
  arg->clk_state_ = arg->isr_ckl_pin_.digital_read();

  if ((arg->prev_clk_state_ == arg->clk_state_) && (arg->bit_count_ > 0)) {
    arg->bit_count_ = DATA_FRAME_SIZE + 10; 
    arg->data_ready_ = false;
    if (arg->has_prs_pin_) arg->isr_prs_pin_.digital_write(false);
  }
  else {
    bool pin_out = false;

    if (arg->hold_key_continuous_ || arg->frames_to_hold_ > 0) {
      if (arg->bit_count_ < DATA_FRAME_SIZE && (arg->active_bits_ & (1UL << arg->bit_count_))) {
        pin_out = true;
      }
    }

    if (arg->simulated_keypad_) {
      if (arg->bit_count_ == 0 || arg->bit_count_ == 24 || arg->bit_count_ == 28) {
        pin_out = true;
      }
    }

    if (arg->has_prs_pin_) arg->isr_prs_pin_.digital_write(pin_out);

    if (arg->bit_count_ < DATA_FRAME_SIZE) {
      delayMicroseconds(100); 
      arg->data_result_[arg->bit_count_] = arg->isr_data_pin_.digital_read();
    }

    arg->bit_count_++;

    if (arg->bit_count_ == DATA_FRAME_SIZE) {
      arg->data_ready_ = true;
      
      for (int i = 0; i < DATA_FRAME_SIZE; i++) {
        arg->finalized_data_[i] = arg->data_result_[i];
      }
      
      if (arg->simulated_keypad_) {
          arg->finalized_data_[0] = false;
          arg->finalized_data_[24] = false;
          arg->finalized_data_[28] = false;
      }

      if (arg->has_prs_pin_) arg->isr_prs_pin_.digital_write(false); 
      
      if (!arg->hold_key_continuous_) {
        if (arg->frames_to_hold_ > 0) {
          arg->frames_to_hold_--;
          if (arg->frames_to_hold_ == 0) arg->active_bits_ = 0; 
        } else if (arg->frames_to_release_ > 0) {
          arg->frames_to_release_--;
          if (arg->frames_to_release_ == 0) {
            arg->sequence_index_++;
            if (arg->sequence_index_ < arg->sequence_length_) {
              arg->active_bits_ = arg->sequence_masks_[arg->sequence_index_];
              arg->frames_to_hold_ = 3;
              arg->frames_to_release_ = 3;
            } else {
              arg->sequence_length_ = 0; 
            }
          }
        }
      }
    }
  }
}

void SatelKPD::loop() {
  uint32_t current_time = millis();

  if (this->trouble_check_state_ == 1) { 
    if (current_time - this->trouble_state_timer_ > 3500) { 
      this->hold_key_continuous_ = false;
      this->active_bits_ = 0;
      this->trouble_check_state_ = 2; 
      this->trouble_state_timer_ = current_time;
      ESP_LOGI(TAG, "Key 7 released. Waiting for trouble status on LEDs...");
    }
  } 
  else if (this->trouble_check_state_ == 2) { 
    if (current_time - this->trouble_state_timer_ > 500) { 
      this->trouble_check_state_ = 3; 
    }
  }

  if (this->data_ready_) {
    this->data_ready_ = false;

    if (this->trouble_check_state_ == 3) {
      std::string trouble_msg = "";
      
      if (this->trouble_lang_pl_) {
        if (this->finalized_data_[31]) trouble_msg += "Awaria wyjscia 1, ";
        if (this->finalized_data_[29]) trouble_msg += "Awaria wyjscia 2, ";
        if (this->finalized_data_[27]) trouble_msg += "Awaria wyjscia 3, ";
        if (this->finalized_data_[25]) trouble_msg += "Brak zasil. 220V, ";
        if (this->finalized_data_[15]) trouble_msg += "Awaria aku., ";
        if (this->finalized_data_[13]) trouble_msg += "Awaria zasil. manip., ";
        if (this->finalized_data_[11]) trouble_msg += "Utrata zegara, ";
        if (this->finalized_data_[9])  trouble_msg += "Brak lacznosci MS, ";
        if (this->finalized_data_[7])  trouble_msg += "Brak nap. linii tel., ";
        if (this->finalized_data_[5])  trouble_msg += "Awaria linii tel., ";
        if (this->finalized_data_[3])  trouble_msg += "Brak sygnalu tel., ";
        if (this->finalized_data_[1])  trouble_msg += "Blad pamieci, ";

        if (trouble_msg.empty()) trouble_msg = "Brak awarii";
        else { trouble_msg.pop_back(); trouble_msg.pop_back(); }
      } else {
        if (this->finalized_data_[31]) trouble_msg += "Output 1 trouble, ";
        if (this->finalized_data_[29]) trouble_msg += "Output 2 trouble, ";
        if (this->finalized_data_[27]) trouble_msg += "Output 3 trouble, ";
        if (this->finalized_data_[25]) trouble_msg += "AC loss, ";
        if (this->finalized_data_[15]) trouble_msg += "Battery trouble, ";
        if (this->finalized_data_[13]) trouble_msg += "KPD power trouble, ";
        if (this->finalized_data_[11]) trouble_msg += "Clock loss, ";
        if (this->finalized_data_[9])  trouble_msg += "MS link trouble, ";
        if (this->finalized_data_[7])  trouble_msg += "Tel. voltage loss, ";
        if (this->finalized_data_[5])  trouble_msg += "Tel. line trouble, ";
        if (this->finalized_data_[3])  trouble_msg += "No tel. signal, ";
        if (this->finalized_data_[1])  trouble_msg += "Memory error, ";

        if (trouble_msg.empty()) trouble_msg = "No troubles";
        else { trouble_msg.pop_back(); trouble_msg.pop_back(); }
      }

      if (this->trouble_text_sensor_ != nullptr) {
          this->trouble_text_sensor_->publish_state(trouble_msg);
      }
      ESP_LOGI(TAG, "Troubles: %s", trouble_msg.c_str());

      this->press_sequence("*");
      this->trouble_check_state_ = 0; 
    } 
    else if (this->trouble_check_state_ == 0) { 

      for (int i = 0; i < DATA_FRAME_SIZE; i++) {
        if (this->bit_sensors_[i] != nullptr) this->bit_sensors_[i]->publish_state(this->finalized_data_[i]);
      }
      
      if (this->armed_a_sensor_) this->armed_a_sensor_->publish_state(this->finalized_data_[7]);
      if (this->armed_b_sensor_) this->armed_b_sensor_->publish_state(this->finalized_data_[5]);
      if (this->alarm_a_sensor_) this->alarm_a_sensor_->publish_state(this->finalized_data_[3]);
      if (this->alarm_b_sensor_) this->alarm_b_sensor_->publish_state(this->finalized_data_[1]);
      if (this->trouble_sensor_) this->trouble_sensor_->publish_state(this->finalized_data_[19]);
      if (this->buzzer_sensor_)  this->buzzer_sensor_->publish_state(this->finalized_data_[17]);
      if (this->phone_sensor_)   this->phone_sensor_->publish_state(this->finalized_data_[21]);
      if (this->power_sensor_)   this->power_sensor_->publish_state(this->finalized_data_[23]);

      if (this->input_sensors_[0]) this->input_sensors_[0]->publish_state(this->finalized_data_[31]);
      if (this->input_sensors_[1]) this->input_sensors_[1]->publish_state(this->finalized_data_[29]);
      if (this->input_sensors_[2]) this->input_sensors_[2]->publish_state(this->finalized_data_[27]);
      if (this->input_sensors_[3]) this->input_sensors_[3]->publish_state(this->finalized_data_[25]);
      if (this->input_sensors_[4]) this->input_sensors_[4]->publish_state(this->finalized_data_[15]);
      if (this->input_sensors_[5]) this->input_sensors_[5]->publish_state(this->finalized_data_[13]);
      if (this->input_sensors_[6]) this->input_sensors_[6]->publish_state(this->finalized_data_[11]);
      if (this->input_sensors_[7]) this->input_sensors_[7]->publish_state(this->finalized_data_[9]);
    }
  }
}

void SatelKPD::dump_config() {
  ESP_LOGCONFIG(TAG, "Satel KPD Component:");
  LOG_PIN("  DATA Pin: ", this->data_pin_);
  LOG_PIN("  CKL Pin: ", this->ckl_pin_);
  if (this->has_prs_pin_) {
    LOG_PIN("  PRS Pin: ", this->prs_pin_);
  } else {
    ESP_LOGCONFIG(TAG, "  PRS Pin: NOT CONFIGURED (Emulation disabled)");
  }
  ESP_LOGCONFIG(TAG, "  Simulated Keypad (ACK Mode): %s", this->simulated_keypad_ ? "YES" : "NO");
}

}  // namespace satel_kpd
}  // namespace esphome