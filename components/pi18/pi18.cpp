#include "pi18.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cstdlib>
#include <cstring>

namespace esphome {
namespace pi18 {

static const char *const TAG = "pi18";

// ─── CRC-16/CCITT (poly 0x1021, init 0x0000) ─────────────────────────────────
uint16_t pi18_crc(const uint8_t *data, size_t len) {
  uint16_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= ((uint16_t)data[i] << 8);
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }
  return crc;
}

#ifdef USE_BUTTON
// ─── PI18Button ───────────────────────────────────────────────────────────────
void PI18Button::press_action() {
  if (!command_.empty() && parent_ != nullptr)
    parent_->send_set_command(command_);
}
#endif  // USE_BUTTON

// ─── PI18Switch ───────────────────────────────────────────────────────────────
void PI18Switch::write_state(bool state) {
  const std::string &cmd = state ? command_on_ : command_off_;
  if (!cmd.empty() && parent_ != nullptr) {
    parent_->send_set_command(cmd);
    this->publish_state(state);
  }
}

#ifdef USE_SELECT
// ─── PI18Select ───────────────────────────────────────────────────────────────
void PI18Select::control(const std::string &value) {
  auto it = mappings_.find(value);
  if (it == mappings_.end() || parent_ == nullptr) {
    ESP_LOGW(TAG, "PI18Select: unknown option '%s'", value.c_str());
    return;
  }
  if (multi_unit_) {
    uint8_t units = parent_->get_parallel_units();
    for (uint8_t u = 0; u < units; u++) {
      char buf[32];
      snprintf(buf, sizeof(buf), it->second.c_str(), (unsigned) u);
      parent_->send_set_command(buf);
    }
  } else {
    parent_->send_set_command(it->second);
  }
  this->publish_state(value);
}
#endif  // USE_SELECT

#ifdef USE_NUMBER
// ─── PI18Number ───────────────────────────────────────────────────────────────
void PI18Number::control(float value) {
  if (parent_ == nullptr) return;
  switch (pair_role_) {
    case 1: parent_->handle_bulk_voltage(value); break;
    case 2: parent_->handle_float_voltage(value); break;
    case 3: parent_->handle_recharge_voltage(value); break;
    case 4: parent_->handle_redischarge_voltage(value); break;
    default: {
      char buf[32];
      snprintf(buf, sizeof(buf), command_template_.c_str(), (int) (value * multiplier_ + 0.5f));
      parent_->send_set_command(buf);
      break;
    }
  }
  this->publish_state(value);
}

// ─── PI18Component: paired voltage handlers ───────────────────────────────────
void PI18Component::handle_bulk_voltage(float v) {
  stored_bulk_voltage_ = v;
  stored_bulk_valid_ = true;
  if (battery_bulk_voltage_number_ != nullptr)
    battery_bulk_voltage_number_->publish_state(v);
  if (stored_float_valid_) {
    char buf[32];
    snprintf(buf, sizeof(buf), "MCHGV%03d,%03d",
             (int) (stored_bulk_voltage_ * 10 + 0.5f),
             (int) (stored_float_voltage_ * 10 + 0.5f));
    send_set_command(buf);
  }
}

void PI18Component::handle_float_voltage(float v) {
  stored_float_voltage_ = v;
  stored_float_valid_ = true;
  if (battery_float_voltage_number_ != nullptr)
    battery_float_voltage_number_->publish_state(v);
  if (stored_bulk_valid_) {
    char buf[32];
    snprintf(buf, sizeof(buf), "MCHGV%03d,%03d",
             (int) (stored_bulk_voltage_ * 10 + 0.5f),
             (int) (stored_float_voltage_ * 10 + 0.5f));
    send_set_command(buf);
  }
}

void PI18Component::handle_recharge_voltage(float v) {
  stored_recharge_voltage_ = v;
  stored_recharge_valid_ = true;
  if (battery_recharge_voltage_number_ != nullptr)
    battery_recharge_voltage_number_->publish_state(v);
  if (stored_redischarge_valid_) {
    char buf[32];
    snprintf(buf, sizeof(buf), "BUCD%03d,%03d",
             (int) (stored_recharge_voltage_ * 10 + 0.5f),
             (int) (stored_redischarge_voltage_ * 10 + 0.5f));
    send_set_command(buf);
  }
}

void PI18Component::handle_redischarge_voltage(float v) {
  stored_redischarge_voltage_ = v;
  stored_redischarge_valid_ = true;
  if (battery_redischarge_voltage_number_ != nullptr)
    battery_redischarge_voltage_number_->publish_state(v);
  if (stored_recharge_valid_) {
    char buf[32];
    snprintf(buf, sizeof(buf), "BUCD%03d,%03d",
             (int) (stored_recharge_voltage_ * 10 + 0.5f),
             (int) (stored_redischarge_voltage_ * 10 + 0.5f));
    send_set_command(buf);
  }
}
#endif  // USE_NUMBER

// ─── Setup ────────────────────────────────────────────────────────────────────
void PI18Component::setup() {
  if (watchdog_pin_ != nullptr) {
    watchdog_pin_->setup();
    watchdog_pin_->digital_write(false);
  }
  build_poll_commands_();
  ESP_LOGCONFIG(TAG, "PI18 component ready. %zu poll commands, %u parallel unit(s).",
                poll_commands_.size(), parallel_units_);
}

// ─── Loop ─────────────────────────────────────────────────────────────────────
void PI18Component::loop() {
  uint32_t now = millis();

  // ── Watchdog toggle ──────────────────────────────────────────────────────────
  if (watchdog_pin_ != nullptr && (now - last_watchdog_) >= watchdog_interval_) {
    watchdog_state_ = !watchdog_state_;
    watchdog_pin_->digital_write(watchdog_state_);
    last_watchdog_ = now;
  }

  // ── Process set command queue (fire-and-forget, one per loop) ────────────────
  if (state_ == State::IDLE && !set_queue_.empty()) {
    std::string cmd = set_queue_.front();
    set_queue_.erase(set_queue_.begin());
    send_frame_(cmd);
    // We don't wait for ACK — just drain any response bytes later
    last_tx_ = now;
    state_ = State::WAITING;
    return;
  }

  // ── State machine ────────────────────────────────────────────────────────────
  switch (state_) {
    case State::IDLE:
      break;

    case State::WAITING: {
      // Try to read a complete frame (terminated by 0x0D)
      while (available()) {
        uint8_t b;
        read_byte(&b);
        if (b == 0x0D) {
          // Got end of frame
          std::string response = rx_buf_;
          rx_buf_.clear();
          state_ = State::IDLE;

          ESP_LOGV(TAG, "RX raw: %s", response.c_str());

          // Verify CRC (last 2 bytes before 0x0D are CRC)
          if (response.size() < 4) {
            ESP_LOGW(TAG, "Response too short: %zu bytes", response.size());
            break;
          }
          uint16_t rx_crc = ((uint8_t)response[response.size() - 2] << 8) |
                             (uint8_t)response[response.size() - 1];
          uint16_t calc = pi18_crc((const uint8_t *)response.c_str(), response.size() - 2);
          if (rx_crc != calc) {
            ESP_LOGW(TAG, "CRC mismatch: got 0x%04X expected 0x%04X", rx_crc, calc);
            break;
          }
          ESP_LOGV(TAG, "CRC OK");

          // Strip CRC bytes → payload
          std::string payload = response.substr(0, response.size() - 2);

          // Which command did we just send?
          if (!poll_commands_.empty()) {
            dispatch_response_(poll_commands_[(poll_index_ == 0 ? poll_commands_.size() : poll_index_) - 1],
                               payload);
          }
          break;
        }
        if (b != 0x0A) {  // ignore LF
          rx_buf_ += (char)b;
        }
      }

      // Timeout
      if ((now - last_tx_) > tx_timeout_ && state_ == State::WAITING) {
        ESP_LOGW(TAG, "Timeout waiting for response to command");
        rx_buf_.clear();
        state_ = State::IDLE;
      }
      break;
    }
  }
}

// ─── Update (called on polling interval) ─────────────────────────────────────
void PI18Component::update() {
  if (poll_commands_.empty()) return;

  if (state_ != State::IDLE) {
    ESP_LOGD(TAG, "Still waiting for previous response, skipping poll tick");
    return;
  }

  const std::string &cmd = poll_commands_[poll_index_];
  ESP_LOGD(TAG, "Polling [%zu/%zu]: %s", poll_index_ + 1, poll_commands_.size(), cmd.c_str());
  send_frame_(cmd);
  last_tx_ = millis();
  state_ = State::WAITING;
  poll_index_ = (poll_index_ + 1) % poll_commands_.size();
}

// ─── Build poll command list ──────────────────────────────────────────────────
void PI18Component::build_poll_commands_() {
  poll_commands_.clear();
  // Always poll these:
  poll_commands_.push_back("GS");    // General status
  poll_commands_.push_back("PIRI");  // Rated info
  poll_commands_.push_back("FWS");   // Faults & warnings
  poll_commands_.push_back("MOD");   // Working mode
  poll_commands_.push_back("FLAG");  // Flags
  poll_commands_.push_back("ET");    // Total energy

  // Parallel group status for each unit
  for (uint8_t i = 0; i < parallel_units_; i++) {
    poll_commands_.push_back(std::string("PGS") + (char)('0' + i));
  }
}

// ─── Frame builder ────────────────────────────────────────────────────────────
// Format: ^P<nnn><CMD><CRC_HI><CRC_LO><CR>
// nnn = len(CMD) + 2 (CRC) + 1 (CR)
std::string PI18Component::build_frame_(const std::string &cmd) {
  std::string body = "^P";
  uint8_t nnn = (uint8_t)(cmd.size() + 3);  // cmd + 2 CRC bytes + 1 CR
  char nnn_str[4];
  snprintf(nnn_str, sizeof(nnn_str), "%03u", nnn);
  body += nnn_str;
  body += cmd;

  uint16_t crc = pi18_crc((const uint8_t *)body.c_str(), body.size());
  body += (char)((crc >> 8) & 0xFF);
  body += (char)(crc & 0xFF);
  body += '\r';
  return body;
}

void PI18Component::send_frame_(const std::string &cmd) {
  std::string frame = build_frame_(cmd);
  ESP_LOGV(TAG, "TX: ^P%03zu%s", cmd.size() + 3, cmd.c_str());
  write_array((const uint8_t *)frame.data(), frame.size());
}

// ─── Public: queue a set command ──────────────────────────────────────────────
void PI18Component::send_set_command(const std::string &cmd) {
  // Set commands use ^S prefix
  std::string body = "^S";
  uint8_t nnn = (uint8_t)(cmd.size() + 3);
  char nnn_str[4];
  snprintf(nnn_str, sizeof(nnn_str), "%03u", nnn);
  body += nnn_str;
  body += cmd;
  uint16_t crc = pi18_crc((const uint8_t *)body.c_str(), body.size());
  body += (char)((crc >> 8) & 0xFF);
  body += (char)(crc & 0xFF);
  body += '\r';
  set_queue_.push_back(body);
  ESP_LOGD(TAG, "Queued set command: %s", cmd.c_str());
}

// ─── Dispatch response by command name ───────────────────────────────────────
void PI18Component::dispatch_response_(const std::string &cmd, const std::string &payload) {
  // payload starts with ^D<nnn><data>
  if (payload.size() < 5 || payload[0] != '^' || payload[1] != 'D') {
    // Could be ^1 (ACK) or ^0 (NAK) for set commands
    if (payload.size() >= 2 && payload[0] == '^') {
      if (payload[1] == '1') {
        ESP_LOGD(TAG, "Set command ACK");
      } else if (payload[1] == '0') {
        ESP_LOGW(TAG, "Set command NAK (refused by inverter)");
      }
    } else {
      ESP_LOGW(TAG, "Unexpected response format: %s", payload.c_str());
    }
    return;
  }

  // Extract data (after ^D<nnn>)
  std::string data = payload.substr(5);  // skip ^D + 3 digit length
  ESP_LOGV(TAG, "CMD=%s DATA=%s", cmd.c_str(), data.c_str());

  auto fields = split_csv_(data);

  if (cmd == "GS") {
    decode_gs_(fields);
  } else if (cmd == "PIRI") {
    decode_piri_(fields);
  } else if (cmd == "FWS") {
    decode_fws_(fields);
  } else if (cmd == "MOD") {
    decode_mod_(fields);
  } else if (cmd == "FLAG") {
    decode_flag_(fields);
  } else if (cmd == "ET") {
    decode_et_(fields);
  } else if (cmd.size() == 4 && cmd.substr(0, 3) == "PGS") {
    decode_pgs_(fields);
  }
}

// ─── CSV splitter ─────────────────────────────────────────────────────────────
std::vector<std::string> PI18Component::split_csv_(const std::string &s) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == ',') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  out.push_back(cur);
  return out;
}

float PI18Component::parse_float_(const std::string &s, float scale) {
  if (s.empty()) return 0.0f;
  return (float)atoi(s.c_str()) * scale;
}

int PI18Component::parse_int_(const std::string &s) {
  if (s.empty()) return 0;
  return atoi(s.c_str());
}

// ─── Decoder: ^P005GS — General Status (28 fields) ───────────────────────────
void PI18Component::decode_gs_(const std::vector<std::string> &f) {
  if (f.size() < 28) {
    ESP_LOGW(TAG, "GS: expected 28 fields, got %zu. Raw may differ in 3-phase mode.", f.size());
    // Log raw fields for debugging
    for (size_t i = 0; i < f.size(); i++) {
      ESP_LOGD(TAG, "  GS[%zu] = '%s'", i, f[i].c_str());
    }
    return;
  }

  // Log all fields at VERBOSE level for debugging
  for (size_t i = 0; i < f.size(); i++) {
    ESP_LOGV(TAG, "  GS[%zu] = '%s'", i, f[i].c_str());
  }

  if (grid_voltage_sensor_ != nullptr)
    grid_voltage_sensor_->publish_state(parse_float_(f[0], 0.1f));
  if (grid_frequency_sensor_ != nullptr)
    grid_frequency_sensor_->publish_state(parse_float_(f[1], 0.1f));
  if (ac_output_voltage_sensor_ != nullptr)
    ac_output_voltage_sensor_->publish_state(parse_float_(f[2], 0.1f));
  if (ac_output_frequency_sensor_ != nullptr)
    ac_output_frequency_sensor_->publish_state(parse_float_(f[3], 0.1f));
  if (ac_output_apparent_power_sensor_ != nullptr)
    ac_output_apparent_power_sensor_->publish_state(parse_float_(f[4]));
  if (ac_output_active_power_sensor_ != nullptr)
    ac_output_active_power_sensor_->publish_state(parse_float_(f[5]));
  if (output_load_percent_sensor_ != nullptr)
    output_load_percent_sensor_->publish_state(parse_float_(f[6]));
  if (battery_voltage_sensor_ != nullptr)
    battery_voltage_sensor_->publish_state(parse_float_(f[7], 0.1f));
  if (battery_voltage_scc1_sensor_ != nullptr)
    battery_voltage_scc1_sensor_->publish_state(parse_float_(f[8], 0.1f));
  if (battery_voltage_scc2_sensor_ != nullptr)
    battery_voltage_scc2_sensor_->publish_state(parse_float_(f[9], 0.1f));
  if (battery_discharge_current_sensor_ != nullptr)
    battery_discharge_current_sensor_->publish_state(parse_float_(f[10]));
  if (battery_charging_current_sensor_ != nullptr)
    battery_charging_current_sensor_->publish_state(parse_float_(f[11]));
  if (battery_capacity_sensor_ != nullptr)
    battery_capacity_sensor_->publish_state(parse_float_(f[12]));
  if (inverter_heatsink_temperature_sensor_ != nullptr)
    inverter_heatsink_temperature_sensor_->publish_state(parse_float_(f[13]));
  if (mppt1_charger_temperature_sensor_ != nullptr)
    mppt1_charger_temperature_sensor_->publish_state(parse_float_(f[14]));
  if (mppt2_charger_temperature_sensor_ != nullptr)
    mppt2_charger_temperature_sensor_->publish_state(parse_float_(f[15]));
  if (pv1_input_power_sensor_ != nullptr)
    pv1_input_power_sensor_->publish_state(parse_float_(f[16]));
  if (pv2_input_power_sensor_ != nullptr)
    pv2_input_power_sensor_->publish_state(parse_float_(f[17]));
  if (pv1_input_voltage_sensor_ != nullptr)
    pv1_input_voltage_sensor_->publish_state(parse_float_(f[18], 0.1f));
  if (pv2_input_voltage_sensor_ != nullptr)
    pv2_input_voltage_sensor_->publish_state(parse_float_(f[19], 0.1f));

  // f[20] = setting_changed (0/1)
  if (setting_changed_binary_sensor_ != nullptr)
    setting_changed_binary_sensor_->publish_state(parse_int_(f[20]) == 1);

  if (mppt1_charger_status_sensor_ != nullptr)
    mppt1_charger_status_sensor_->publish_state(parse_float_(f[21]));
  if (mppt2_charger_status_sensor_ != nullptr)
    mppt2_charger_status_sensor_->publish_state(parse_float_(f[22]));

  if (load_connected_binary_sensor_ != nullptr)
    load_connected_binary_sensor_->publish_state(parse_int_(f[23]) == 1);

  // f[24] battery_power_direction
  int bpd = parse_int_(f[24]);
  if (battery_power_direction_sensor_ != nullptr)
    battery_power_direction_sensor_->publish_state((float)bpd);
  if (battery_power_direction_text_text_sensor_ != nullptr) {
    const char *s = bpd == 0 ? "None" : bpd == 1 ? "Charge" : bpd == 2 ? "Discharge" : "Unknown";
    battery_power_direction_text_text_sensor_->publish_state(s);
  }

  // f[25] dc_ac_power_direction
  int dapd = parse_int_(f[25]);
  if (dc_ac_power_direction_sensor_ != nullptr)
    dc_ac_power_direction_sensor_->publish_state((float)dapd);
  if (dc_ac_power_direction_text_text_sensor_ != nullptr) {
    const char *s = dapd == 0 ? "None" : dapd == 1 ? "AC->DC" : dapd == 2 ? "DC->AC" : "Unknown";
    dc_ac_power_direction_text_text_sensor_->publish_state(s);
  }

  // f[26] line_power_direction
  int lpd = parse_int_(f[26]);
  if (line_power_direction_sensor_ != nullptr)
    line_power_direction_sensor_->publish_state((float)lpd);
  if (line_power_direction_text_text_sensor_ != nullptr) {
    const char *s = lpd == 0 ? "None" : lpd == 1 ? "Input" : lpd == 2 ? "Output" : "Unknown";
    line_power_direction_text_text_sensor_->publish_state(s);
  }

  // f[27] local_parallel_id
  if (local_parallel_id_sensor_ != nullptr)
    local_parallel_id_sensor_->publish_state(parse_float_(f[27]));
}

// ─── Decoder: ^P007PIRI — Rated Information (25 fields) ──────────────────────
void PI18Component::decode_piri_(const std::vector<std::string> &f) {
  if (f.size() < 25) {
    ESP_LOGW(TAG, "PIRI: expected 25 fields, got %zu", f.size());
    for (size_t i = 0; i < f.size(); i++)
      ESP_LOGD(TAG, "  PIRI[%zu] = '%s'", i, f[i].c_str());
    return;
  }
  for (size_t i = 0; i < f.size(); i++)
    ESP_LOGV(TAG, "  PIRI[%zu] = '%s'", i, f[i].c_str());

  if (grid_rating_voltage_sensor_ != nullptr)
    grid_rating_voltage_sensor_->publish_state(parse_float_(f[0], 0.1f));
  if (grid_rating_current_sensor_ != nullptr)
    grid_rating_current_sensor_->publish_state(parse_float_(f[1], 0.1f));
  if (ac_output_rating_voltage_sensor_ != nullptr)
    ac_output_rating_voltage_sensor_->publish_state(parse_float_(f[2], 0.1f));
  if (ac_output_rating_frequency_sensor_ != nullptr)
    ac_output_rating_frequency_sensor_->publish_state(parse_float_(f[3], 0.1f));
  if (ac_output_rating_current_sensor_ != nullptr)
    ac_output_rating_current_sensor_->publish_state(parse_float_(f[4], 0.1f));
  if (ac_output_rating_apparent_power_sensor_ != nullptr)
    ac_output_rating_apparent_power_sensor_->publish_state(parse_float_(f[5]));
  if (ac_output_rating_active_power_sensor_ != nullptr)
    ac_output_rating_active_power_sensor_->publish_state(parse_float_(f[6]));
  if (battery_rating_voltage_sensor_ != nullptr)
    battery_rating_voltage_sensor_->publish_state(parse_float_(f[7], 0.1f));
  // f[8] = battery recharge voltage
  float recharge_v = parse_float_(f[8], 0.1f);
  if (battery_recharge_voltage_sensor_ != nullptr)
    battery_recharge_voltage_sensor_->publish_state(recharge_v);
#ifdef USE_NUMBER
  if (!stored_recharge_valid_) {
    stored_recharge_voltage_ = recharge_v;
    stored_recharge_valid_ = true;
  }
  if (battery_recharge_voltage_number_ != nullptr)
    battery_recharge_voltage_number_->publish_state(recharge_v);
#endif

  // f[9] = battery redischarge voltage
  float redischarge_v = parse_float_(f[9], 0.1f);
  if (battery_redischarge_voltage_sensor_ != nullptr)
    battery_redischarge_voltage_sensor_->publish_state(redischarge_v);
#ifdef USE_NUMBER
  if (!stored_redischarge_valid_) {
    stored_redischarge_voltage_ = redischarge_v;
    stored_redischarge_valid_ = true;
  }
  if (battery_redischarge_voltage_number_ != nullptr)
    battery_redischarge_voltage_number_->publish_state(redischarge_v);
#endif

  if (battery_under_voltage_sensor_ != nullptr)
    battery_under_voltage_sensor_->publish_state(parse_float_(f[10], 0.1f));

  // f[11] = bulk voltage
  float bulk_v = parse_float_(f[11], 0.1f);
  if (battery_bulk_voltage_sensor_ != nullptr)
    battery_bulk_voltage_sensor_->publish_state(bulk_v);
#ifdef USE_NUMBER
  if (!stored_bulk_valid_) {
    stored_bulk_voltage_ = bulk_v;
    stored_bulk_valid_ = true;
  }
  if (battery_bulk_voltage_number_ != nullptr)
    battery_bulk_voltage_number_->publish_state(bulk_v);
#endif

  // f[12] = float voltage
  float float_v = parse_float_(f[12], 0.1f);
  if (battery_float_voltage_sensor_ != nullptr)
    battery_float_voltage_sensor_->publish_state(float_v);
#ifdef USE_NUMBER
  if (!stored_float_valid_) {
    stored_float_voltage_ = float_v;
    stored_float_valid_ = true;
  }
  if (battery_float_voltage_number_ != nullptr)
    battery_float_voltage_number_->publish_state(float_v);
#endif
  if (battery_type_sensor_ != nullptr)
    battery_type_sensor_->publish_state(parse_float_(f[13]));
  if (max_ac_charging_current_sensor_ != nullptr)
    max_ac_charging_current_sensor_->publish_state(parse_float_(f[14]));
  if (max_charging_current_sensor_ != nullptr)
    max_charging_current_sensor_->publish_state(parse_float_(f[15]));
  if (input_voltage_range_sensor_ != nullptr)
    input_voltage_range_sensor_->publish_state(parse_float_(f[16]));
  if (output_source_priority_sensor_ != nullptr)
    output_source_priority_sensor_->publish_state(parse_float_(f[17]));
  if (charger_source_priority_sensor_ != nullptr)
    charger_source_priority_sensor_->publish_state(parse_float_(f[18]));
  if (parallel_max_num_sensor_ != nullptr)
    parallel_max_num_sensor_->publish_state(parse_float_(f[19]));
  if (machine_type_sensor_ != nullptr)
    machine_type_sensor_->publish_state(parse_float_(f[20]));
  if (topology_sensor_ != nullptr)
    topology_sensor_->publish_state(parse_float_(f[21]));
  if (output_mode_sensor_ != nullptr)
    output_mode_sensor_->publish_state(parse_float_(f[22]));
  if (solar_power_priority_sensor_ != nullptr)
    solar_power_priority_sensor_->publish_state(parse_float_(f[23]));
  if (mppt_string_count_sensor_ != nullptr)
    mppt_string_count_sensor_->publish_state(parse_float_(f[24]));
}

// ─── Decoder: ^P005FWS — Fault & Warning Status (17 fields) ──────────────────
void PI18Component::decode_fws_(const std::vector<std::string> &f) {
  if (f.size() < 17) {
    ESP_LOGW(TAG, "FWS: expected 17 fields, got %zu", f.size());
    return;
  }

  int fault_code = parse_int_(f[0]);
  if (fault_code_text_sensor_ != nullptr)
    fault_code_text_sensor_->publish_state(fault_code_str_(fault_code));

  auto pub = [](binary_sensor::BinarySensor *bs, const std::string &v) {
    if (bs != nullptr) bs->publish_state(v == "1");
  };
  pub(warning_line_fail_binary_sensor_, f[1]);
  pub(warning_output_circuit_short_binary_sensor_, f[2]);
  pub(warning_over_temperature_binary_sensor_, f[3]);
  pub(warning_fan_lock_binary_sensor_, f[4]);
  pub(warning_battery_voltage_high_binary_sensor_, f[5]);
  pub(warning_battery_low_binary_sensor_, f[6]);
  pub(warning_battery_under_binary_sensor_, f[7]);
  pub(warning_over_load_binary_sensor_, f[8]);
  pub(warning_eeprom_fail_binary_sensor_, f[9]);
  pub(warning_power_limit_binary_sensor_, f[10]);
  pub(warning_pv1_voltage_high_binary_sensor_, f[11]);
  pub(warning_pv2_voltage_high_binary_sensor_, f[12]);
  pub(warning_mppt1_overload_binary_sensor_, f[13]);
  pub(warning_mppt2_overload_binary_sensor_, f[14]);
  pub(warning_scc1_battery_too_low_binary_sensor_, f[15]);
  pub(warning_scc2_battery_too_low_binary_sensor_, f[16]);
}

// ─── Decoder: ^P006MOD — Working Mode ────────────────────────────────────────
void PI18Component::decode_mod_(const std::vector<std::string> &f) {
  if (f.empty()) return;
  int mode = parse_int_(f[0]);
  const char *mode_str;
  switch (mode) {
    case 0: mode_str = "Power on"; break;
    case 1: mode_str = "Standby"; break;
    case 2: mode_str = "Bypass"; break;
    case 3: mode_str = "Battery"; break;
    case 4: mode_str = "Fault"; break;
    case 5: mode_str = "Hybrid"; break;
    default: mode_str = "Unknown"; break;
  }
  if (device_mode_text_sensor_ != nullptr)
    device_mode_text_sensor_->publish_state(mode_str);
}

// ─── Decoder: ^P007FLAG — Flags (9 fields) ───────────────────────────────────
void PI18Component::decode_flag_(const std::vector<std::string> &f) {
  if (f.size() < 8) {
    ESP_LOGW(TAG, "FLAG: expected 8+ fields, got %zu", f.size());
    return;
  }
  auto pub = [](binary_sensor::BinarySensor *bs, const std::string &v) {
    if (bs != nullptr) bs->publish_state(v == "1");
  };
  pub(flag_silence_buzzer_binary_sensor_, f[0]);
  pub(flag_overload_bypass_binary_sensor_, f[1]);
  pub(flag_lcd_escape_default_binary_sensor_, f[2]);
  pub(flag_overload_restart_binary_sensor_, f[3]);
  pub(flag_over_temp_restart_binary_sensor_, f[4]);
  pub(flag_backlight_on_binary_sensor_, f[5]);
  pub(flag_alarm_primary_interrupt_binary_sensor_, f[6]);
  pub(flag_fault_code_record_binary_sensor_, f[7]);
}

// ─── Decoder: ^P007PGSn — Parallel Group Status (29 fields) ──────────────────
void PI18Component::decode_pgs_(const std::vector<std::string> &f) {
  if (f.size() < 29) {
    ESP_LOGW(TAG, "PGS: expected 29 fields, got %zu", f.size());
    for (size_t i = 0; i < f.size(); i++)
      ESP_LOGD(TAG, "  PGS[%zu] = '%s'", i, f[i].c_str());
    return;
  }
  for (size_t i = 0; i < f.size(); i++)
    ESP_LOGV(TAG, "  PGS[%zu] = '%s'", i, f[i].c_str());

  // f[0]  = parallel ID connection status
  // f[1]  = work mode
  // f[2]  = fault code
  // f[3]  = grid voltage         (0.1V)
  // f[4]  = grid frequency       (0.1Hz)
  // f[5]  = ac output voltage    (0.1V)
  // f[6]  = ac output frequency  (0.1Hz)
  // f[7]  = ac output apparent power  (VA)
  // f[8]  = ac output active power    (W)
  // f[9]  = total apparent power      (VA)
  // f[10] = total active power        (W)
  // f[11] = output load percent (this unit)
  // f[12] = total output load percent
  // f[13] = battery voltage      (0.1V)
  // f[14] = battery discharge current (A)
  // f[15] = battery charging current  (A)
  // f[16] = total battery charging current (A)
  // f[17] = battery capacity     (%)
  // f[18] = pv1 input power      (W)
  // f[19] = pv2 input power      (W)
  // f[20] = pv1 input voltage    (0.1V)
  // f[21] = pv2 input voltage    (0.1V)
  // f[22] = mppt1 charger status
  // f[23] = mppt2 charger status
  // f[24] = load connection
  // f[25] = battery power direction
  // f[26] = dc/ac power direction
  // f[27] = line power direction
  // f[28] = max temperature (°C)

  if (pgs_grid_voltage_sensor_ != nullptr)
    pgs_grid_voltage_sensor_->publish_state(parse_float_(f[3], 0.1f));
  if (pgs_grid_frequency_sensor_ != nullptr)
    pgs_grid_frequency_sensor_->publish_state(parse_float_(f[4], 0.1f));
  if (pgs_ac_output_voltage_sensor_ != nullptr)
    pgs_ac_output_voltage_sensor_->publish_state(parse_float_(f[5], 0.1f));
  if (pgs_ac_output_frequency_sensor_ != nullptr)
    pgs_ac_output_frequency_sensor_->publish_state(parse_float_(f[6], 0.1f));
  if (pgs_ac_output_apparent_power_sensor_ != nullptr)
    pgs_ac_output_apparent_power_sensor_->publish_state(parse_float_(f[7]));
  if (pgs_ac_output_active_power_sensor_ != nullptr)
    pgs_ac_output_active_power_sensor_->publish_state(parse_float_(f[8]));
  if (pgs_total_ac_output_apparent_power_sensor_ != nullptr)
    pgs_total_ac_output_apparent_power_sensor_->publish_state(parse_float_(f[9]));
  if (pgs_total_ac_output_active_power_sensor_ != nullptr)
    pgs_total_ac_output_active_power_sensor_->publish_state(parse_float_(f[10]));
  if (pgs_output_load_percent_sensor_ != nullptr)
    pgs_output_load_percent_sensor_->publish_state(parse_float_(f[11]));
  if (pgs_total_output_load_percent_sensor_ != nullptr)
    pgs_total_output_load_percent_sensor_->publish_state(parse_float_(f[12]));
  if (pgs_battery_voltage_sensor_ != nullptr)
    pgs_battery_voltage_sensor_->publish_state(parse_float_(f[13], 0.1f));
  if (pgs_battery_discharge_current_sensor_ != nullptr)
    pgs_battery_discharge_current_sensor_->publish_state(parse_float_(f[14]));
  if (pgs_battery_charging_current_sensor_ != nullptr)
    pgs_battery_charging_current_sensor_->publish_state(parse_float_(f[15]));
  if (pgs_total_battery_charging_current_sensor_ != nullptr)
    pgs_total_battery_charging_current_sensor_->publish_state(parse_float_(f[16]));
  if (pgs_battery_capacity_sensor_ != nullptr)
    pgs_battery_capacity_sensor_->publish_state(parse_float_(f[17]));
  if (pgs_pv1_input_power_sensor_ != nullptr)
    pgs_pv1_input_power_sensor_->publish_state(parse_float_(f[18]));
  if (pgs_pv2_input_power_sensor_ != nullptr)
    pgs_pv2_input_power_sensor_->publish_state(parse_float_(f[19]));
  if (pgs_pv1_input_voltage_sensor_ != nullptr)
    pgs_pv1_input_voltage_sensor_->publish_state(parse_float_(f[20], 0.1f));
  if (pgs_pv2_input_voltage_sensor_ != nullptr)
    pgs_pv2_input_voltage_sensor_->publish_state(parse_float_(f[21], 0.1f));
  if (pgs_max_temperature_sensor_ != nullptr)
    pgs_max_temperature_sensor_->publish_state(parse_float_(f[28]));
}

// ─── Decoder: ^P005ET — Total Generated Energy ───────────────────────────────
void PI18Component::decode_et_(const std::vector<std::string> &f) {
  if (f.empty()) return;
  if (total_generated_energy_sensor_ != nullptr)
    total_generated_energy_sensor_->publish_state(parse_float_(f[0]));
}

// ─── Fault code to string ─────────────────────────────────────────────────────
std::string PI18Component::fault_code_str_(int code) {
  switch (code) {
    case 0:  return "No fault";
    case 1:  return "Fan locked";
    case 2:  return "Over temperature";
    case 3:  return "Battery voltage too high";
    case 4:  return "Battery voltage too low";
    case 5:  return "Output short or over temperature";
    case 6:  return "Output voltage too high";
    case 7:  return "Overload timeout";
    case 8:  return "Bus voltage too high";
    case 9:  return "Bus soft start failed";
    case 11: return "Main relay failed";
    case 51: return "Over current inverter";
    case 52: return "Bus soft start failed";
    case 53: return "Inverter soft start failed";
    case 54: return "Self-test failed";
    case 55: return "Over DC voltage on inverter output";
    case 56: return "Battery connection open";
    case 57: return "Current sensor failed";
    case 58: return "Output voltage too low";
    case 60: return "Inverter negative power";
    case 71: return "Parallel version different";
    case 72: return "Output circuit failed";
    case 80: return "CAN communication failed";
    case 81: return "Parallel host line lost";
    case 82: return "Parallel sync signal lost";
    case 83: return "Parallel battery voltage detect different";
    case 84: return "Parallel line voltage/frequency different";
    case 85: return "Parallel line input current unbalanced";
    case 86: return "Parallel output setting different";
    default: return "Unknown fault " + to_string(code);
  }
}

}  // namespace pi18
}  // namespace esphome
