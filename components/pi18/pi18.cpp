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

void PI18SetDateTimeButton::press_action() {
  if (parent_ != nullptr)
    parent_->send_set_date_time();
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

void PI18VoltageSelect::control(const std::string &value) {
  if (parent_ == nullptr) return;
  float v = (float) std::atof(value.c_str());
  switch (pair_role_) {
    case 0: {  // cutoff (PSDV mmm — 0.1V scaled)
      char buf[16];
      snprintf(buf, sizeof(buf), "PSDV%03d", (int)(v * 10 + 0.5f));
      parent_->send_set_command(buf);
      break;
    }
    case 1: parent_->handle_bulk_voltage(v); break;
    case 2: parent_->handle_float_voltage(v); break;
    case 3: parent_->handle_recharge_voltage(v); break;
    case 4: parent_->handle_redischarge_voltage(v); break;
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
#endif  // USE_NUMBER

// ─── PI18Component: paired voltage handlers (used by Number AND Voltage Select) ──
void PI18Component::handle_bulk_voltage(float v) {
  stored_bulk_voltage_ = v;
  stored_bulk_valid_ = true;
#ifdef USE_NUMBER
  if (battery_bulk_voltage_number_ != nullptr)
    battery_bulk_voltage_number_->publish_state(v);
#endif
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
#ifdef USE_NUMBER
  if (battery_float_voltage_number_ != nullptr)
    battery_float_voltage_number_->publish_state(v);
#endif
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
#ifdef USE_NUMBER
  if (battery_recharge_voltage_number_ != nullptr)
    battery_recharge_voltage_number_->publish_state(v);
#endif
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
#ifdef USE_NUMBER
  if (battery_redischarge_voltage_number_ != nullptr)
    battery_redischarge_voltage_number_->publish_state(v);
#endif
  if (stored_recharge_valid_) {
    char buf[32];
    snprintf(buf, sizeof(buf), "BUCD%03d,%03d",
             (int) (stored_recharge_voltage_ * 10 + 0.5f),
             (int) (stored_redischarge_voltage_ * 10 + 0.5f));
    send_set_command(buf);
  }
}

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
    std::string frame = set_queue_.front();
    set_queue_.erase(set_queue_.begin());
    // frame is already a fully built ^S<nnn><cmd><CRC><CR> — write directly
    ESP_LOGV(TAG, "TX set frame (%zu bytes)", frame.size());
    write_array((const uint8_t *)frame.data(), frame.size());
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
            ESP_LOGW(TAG, "CRC mismatch: got 0x%04X expected 0x%04X (processing anyway)", rx_crc, calc);
          } else {
            ESP_LOGV(TAG, "CRC OK");
          }

          // Strip CRC bytes → payload
          std::string payload = response.substr(0, response.size() - 2);

          // Which command did we just send? (set by sender in update())
          if (!last_sent_cmd_.empty()) {
            dispatch_response_(last_sent_cmd_, payload);
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
  // Rebuild date-parameterised poll commands when day changes
  if (time_ != nullptr) {
    auto now = time_->now();
    if (now.is_valid() && now.day_of_year != last_built_yday_ &&
        (yearly_energy_sensor_ != nullptr || monthly_energy_sensor_ != nullptr ||
         daily_energy_sensor_ != nullptr)) {
      ESP_LOGI(TAG, "Time-driven rebuild of poll plan (yday %d -> %d)",
               last_built_yday_, now.day_of_year);
      build_poll_commands_();
    }
  }

  if (state_ != State::IDLE) {
    ESP_LOGD(TAG, "Still waiting for previous response, skipping poll tick");
    return;
  }

  cycle_counter_++;

  auto fire = [&](const std::string &cmd, const char *kind) {
    ESP_LOGD(TAG, "Polling [%s]: %s", kind, cmd.c_str());
    last_sent_cmd_ = cmd;
    send_frame_(cmd);
    last_tx_ = millis();
    state_ = State::WAITING;
  };

  // 1) Drain one-shot tasks first (PI, ID, VFW, ACCT, ACLT, T) — at boot only.
  //    Highest priority so user sees static info quickly.
  for (auto &bg : bg_tasks_) {
    if (bg.period_cycles == 0 && !bg.one_shot_done) {
      fire(bg.cmd, "once");
      bg.last_sent_cycle = cycle_counter_;
      bg.one_shot_done = true;
      return;
    }
  }

  // 2) BG slot: every 8th tick is reserved for periodic background tasks (12.5%).
  //    Exception: if any periodic task is severely overdue (> 3x its period),
  //    promote to BG slot immediately — keeps MOD/FWS/FLAG snappy when needed.
  bool bg_overdue = false;
  for (auto &bg : bg_tasks_) {
    if (bg.period_cycles == 0 || bg.last_sent_cycle == 0) continue;
    uint32_t elapsed = cycle_counter_ - bg.last_sent_cycle;
    if (elapsed > 3 * bg.period_cycles) { bg_overdue = true; break; }
  }
  bool bg_slot = (cycle_counter_ % 30 == 0) || bg_overdue;
  if (bg_slot) {
    BgTask *chosen = nullptr;
    uint32_t best_score = 0;  // higher = more overdue / never-sent
    for (auto &bg : bg_tasks_) {
      if (bg.period_cycles == 0) continue;  // one-shot, already drained
      uint32_t score;
      if (bg.last_sent_cycle == 0) {
        score = 0xFFFFFFFFu;  // never sent — top priority
      } else {
        uint32_t elapsed = cycle_counter_ - bg.last_sent_cycle;
        if (elapsed < bg.period_cycles) continue;  // not due
        score = elapsed - bg.period_cycles;        // how overdue
      }
      if (chosen == nullptr || score > best_score) {
        chosen = &bg;
        best_score = score;
      }
    }
    if (chosen != nullptr) {
      fire(chosen->cmd, "bg");
      chosen->last_sent_cycle = cycle_counter_;
      return;
    }
  }

  // 3) LIVE rotation — round-robin every other tick when no BG fires.
  if (live_commands_.empty()) return;
  const std::string &cmd = live_commands_[live_index_];
  fire(cmd, "live");
  live_index_ = (live_index_ + 1) % live_commands_.size();
}

// ─── Build poll command list ──────────────────────────────────────────────────
void PI18Component::build_poll_commands_() {
  // Tiered polling. update_interval = 5s (configurable in YAML).
  //   LIVE (every tick) — current power, currents, directions, batteries
  //   MEDIUM (every 4 ticks) — mode + warnings + flags
  //   SLOW (every 12 ticks) — rated info / settings
  //   HOURLY (every 720 ticks ≈ 1h) — totals / static config
  //   ONCE — info that never changes (serial, firmware, protocol id)
  live_commands_.clear();
  bg_tasks_.clear();

  // ── LIVE: round-robin every tick ────────────────────────────────────────────
  // GS only if user actually configured a GS-only sensor (heat sink temp,
  // MPPT temps, SCC voltages, parallel ID, setting-changed flag). All other
  // GS data is available via PGS0/L1/L2/L3.
  bool needs_gs =
      inverter_heatsink_temperature_sensor_ != nullptr ||
      mppt1_charger_temperature_sensor_ != nullptr ||
      mppt2_charger_temperature_sensor_ != nullptr ||
      battery_voltage_scc1_sensor_ != nullptr ||
      battery_voltage_scc2_sensor_ != nullptr ||
      local_parallel_id_sensor_ != nullptr ||
      setting_changed_binary_sensor_ != nullptr;
  if (needs_gs) live_commands_.push_back("GS");
  for (uint8_t i = 0; i < parallel_units_; i++)
    live_commands_.push_back(std::string("PGS") + (char)('0' + i));

  auto add_bg = [&](const std::string &cmd, uint16_t period) {
    bg_tasks_.push_back({cmd, period, 0, false});
  };

  // Periods are in ticks (= update_interval seconds, default 1s).
  // ── MEDIUM (~60s) ───────────────────────────────────────────────────────────
  add_bg("MOD",  60);
  add_bg("FWS",  60);
  add_bg("FLAG", 60);

  // ── SLOW (~5min) ────────────────────────────────────────────────────────────
  add_bg("PIRI", 300);

  // ── HOURLY — only if sensors configured ────────────────────────────────────
  if (total_generated_energy_sensor_ != nullptr) add_bg("ET", 3600);

  // ── ONCE — read once at boot, rarely changes ───────────────────────────────
  if (ac_charge_time_bucket_text_sensor_ != nullptr) add_bg("ACCT", 0);
  if (ac_supply_load_time_bucket_text_sensor_ != nullptr) add_bg("ACLT", 0);
  if (device_time_text_sensor_ != nullptr) add_bg("T", 0);

  // ── ONCE (one-shot, fired on first tick) ───────────────────────────────────
  if (protocol_id_text_sensor_ != nullptr) add_bg("PI", 0);
  if (serial_number_text_sensor_ != nullptr) add_bg("ID", 0);
  if (firmware_version_text_sensor_ != nullptr) add_bg("VFW", 0);

  // ── Date-parameterised energy queries: rebuild lazily, only if time valid ──
  if (time_ != nullptr) {
    auto now = time_->now();
    if (now.is_valid()) {
      char buf[16];
      if (yearly_energy_sensor_ != nullptr) {
        snprintf(buf, sizeof(buf), "EY%04d", now.year);
        add_bg(buf, 720);
      }
      if (monthly_energy_sensor_ != nullptr) {
        snprintf(buf, sizeof(buf), "EM%04d%02d", now.year, now.month);
        add_bg(buf, 720);
      }
      if (daily_energy_sensor_ != nullptr) {
        snprintf(buf, sizeof(buf), "ED%04d%02d%02d", now.year, now.month, now.day_of_month);
        add_bg(buf, 300);  // ~5 min for daily totals
      }
      last_built_yday_ = now.day_of_year;
    }
  }

  // Compatibility shim: keep poll_commands_ populated for any legacy reference
  poll_commands_ = live_commands_;
  ESP_LOGI(TAG, "Poll plan: %u live commands, %u background tasks",
           (unsigned)live_commands_.size(), (unsigned)bg_tasks_.size());
}

void PI18Component::send_set_date_time() {
  if (time_ == nullptr) {
    ESP_LOGW(TAG, "send_set_date_time: no time component configured");
    return;
  }
  auto now = time_->now();
  if (!now.is_valid()) {
    ESP_LOGW(TAG, "send_set_date_time: time is not valid yet");
    return;
  }
  char buf[24];
  // ^S018DATyymmddhhffss
  snprintf(buf, sizeof(buf), "DAT%02d%02d%02d%02d%02d%02d",
           now.year % 100, now.month, now.day_of_month,
           now.hour, now.minute, now.second);
  ESP_LOGI(TAG, "Setting inverter time: %s", buf);
  send_set_command(buf);
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
    uint8_t phase = (uint8_t)(cmd[3] - '0');
    decode_pgs_(fields, phase);
  } else if (cmd == "PI") {
    if (protocol_id_text_sensor_ != nullptr)
      protocol_id_text_sensor_->publish_state(data);
  } else if (cmd == "ID") {
    if (serial_number_text_sensor_ != nullptr) {
      // Format: LLXXXX...XXXX where LL = valid digits length, max 20 X digits
      if (data.size() >= 2) {
        int valid_len = (data[0] - '0') * 10 + (data[1] - '0');
        if (valid_len > 0 && (size_t)(2 + valid_len) <= data.size())
          serial_number_text_sensor_->publish_state(data.substr(2, valid_len));
        else
          serial_number_text_sensor_->publish_state(data);
      } else {
        serial_number_text_sensor_->publish_state(data);
      }
    }
  } else if (cmd == "VFW") {
    if (firmware_version_text_sensor_ != nullptr)
      firmware_version_text_sensor_->publish_state(data);
  } else if (cmd == "ACCT") {
    decode_acct_(fields);
  } else if (cmd == "ACLT") {
    decode_aclt_(fields);
  } else if (cmd == "T") {
    decode_t_(fields);
  } else if (cmd.size() >= 6 && cmd.substr(0, 2) == "EY") {
    decode_ey_(fields);
  } else if (cmd.size() >= 8 && cmd.substr(0, 2) == "EM") {
    decode_em_(fields);
  } else if (cmd.size() >= 10 && cmd.substr(0, 2) == "ED") {
    decode_ed_(fields);
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
#ifdef USE_SELECT
  if (battery_recharge_voltage_select_ != nullptr) {
    char buf[8]; snprintf(buf, sizeof(buf), "%.1f", recharge_v);
    battery_recharge_voltage_select_->publish_state(buf);
  }
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
#ifdef USE_SELECT
  if (battery_redischarge_voltage_select_ != nullptr) {
    char buf[8]; snprintf(buf, sizeof(buf), "%.1f", redischarge_v);
    battery_redischarge_voltage_select_->publish_state(buf);
  }
#endif

  float under_v = parse_float_(f[10], 0.1f);
  if (battery_under_voltage_sensor_ != nullptr)
    battery_under_voltage_sensor_->publish_state(under_v);
#ifdef USE_NUMBER
  if (battery_cutoff_voltage_number_ != nullptr)
    battery_cutoff_voltage_number_->publish_state(under_v);
#endif
#ifdef USE_SELECT
  if (battery_cutoff_voltage_select_ != nullptr) {
    char buf[8]; snprintf(buf, sizeof(buf), "%.1f", under_v);
    battery_cutoff_voltage_select_->publish_state(buf);
  }
#endif

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
#ifdef USE_SELECT
  if (battery_bulk_voltage_select_ != nullptr) {
    char buf[8]; snprintf(buf, sizeof(buf), "%.1f", bulk_v);
    battery_bulk_voltage_select_->publish_state(buf);
  }
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
#ifdef USE_SELECT
  if (battery_float_voltage_select_ != nullptr) {
    char buf[8]; snprintf(buf, sizeof(buf), "%.1f", float_v);
    battery_float_voltage_select_->publish_state(buf);
  }
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

#ifdef USE_SELECT
  // ── Sync select entities from PIRI numeric values ────────────────────────────
  static const char *OPP[] = {"Solar-Utility-Battery", "Solar-Battery-Utility"};
  static const char *CSP[] = {"Solar-first", "Solar-and-Utility", "Only-solar"};
  static const char *SPP[] = {"Battery-Load", "Load-Battery"};
  static const char *BTP[] = {"AGM", "Flooded", "User-defined", "Pylontech", "WECO", "Soltaro", "LIB-compatible", "Lithium", "LIB-protocol"};
  static const char *IVR[] = {"Appliance", "UPS"};
  static const char *OMD[] = {"Single", "Parallel", "Phase-1 of 3", "Phase-2 of 3", "Phase-3 of 3"};

  auto pub_sel = [](select::Select *s, const char **opts, size_t n, int idx) {
    if (s != nullptr && idx >= 0 && (size_t) idx < n)
      s->publish_state(opts[idx]);
  };
  pub_sel(output_source_priority_select_, OPP, 2, parse_int_(f[17]));
  pub_sel(charger_source_priority_select_, CSP, 3, parse_int_(f[18]));
  pub_sel(solar_power_priority_select_, SPP, 2, parse_int_(f[23]));
  {
    int bt = parse_int_(f[13]);
    ESP_LOGV(TAG, "PIRI battery_type raw=%d", bt);
    pub_sel(battery_type_select_, BTP, 9, bt);
  }
  pub_sel(input_voltage_range_select_, IVR, 2, parse_int_(f[16]));
  pub_sel(output_mode_select_, OMD, 5, parse_int_(f[22]));

  // AC output voltage select — map from 0.1V rating value
  if (ac_output_voltage_select_ != nullptr) {
    int v = parse_int_(f[2]);
    const char *vopt = nullptr;
    if (v == 2080) vopt = "208V";
    else if (v == 2200) vopt = "220V";
    else if (v == 2300) vopt = "230V";
    else if (v == 2400) vopt = "240V";
    ESP_LOGV(TAG, "PIRI ac_output_rating_voltage raw=%d -> %s", v, vopt ? vopt : "unknown");
    if (vopt) ac_output_voltage_select_->publish_state(vopt);
  }

  // Max charging current — format as "50A", "100A" etc.
  if (max_charging_current_select_ != nullptr) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%dA", parse_int_(f[15]));
    max_charging_current_select_->publish_state(buf);
  }
  if (max_ac_charging_current_select_ != nullptr) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%dA", parse_int_(f[14]));
    max_ac_charging_current_select_->publish_state(buf);
  }
#endif  // USE_SELECT
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
void PI18Component::decode_pgs_(const std::vector<std::string> &f, uint8_t phase) {
  if (f.size() < 29) {
    ESP_LOGW(TAG, "PGS%u: expected 29 fields, got %zu", phase, f.size());
    for (size_t i = 0; i < f.size(); i++)
      ESP_LOGD(TAG, "  PGS%u[%zu] = '%s'", phase, i, f[i].c_str());
    return;
  }
  for (size_t i = 0; i < f.size(); i++)
    ESP_LOGV(TAG, "  PGS%u[%zu] = '%s'", phase, i, f[i].c_str());

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

  // All `pgs_*` sensors are master/system-wide views and should come ONLY from
  // PGS0. Otherwise PGS1/PGS2 overwrite them with per-slave views every cycle,
  // which causes flicker (e.g. total power jumping between 200W and 2000W).
  if (phase == 0) {
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

  // Helper string conversions reused across PGS/per-phase
  auto work_mode_str = [](int m) -> const char * {
    switch (m) {
      case 0: return "Power on"; case 1: return "Standby"; case 2: return "Bypass";
      case 3: return "Battery";  case 4: return "Fault";   case 5: return "Hybrid";
      default: return "Unknown";
    }
  };
  auto conn_status_str = [](int s) -> const char * {
    return s == 1 ? "Existent" : s == 0 ? "Not existent" : "Unknown";
  };
  auto bpd_str = [](int v) -> const char * {
    return v == 0 ? "None" : v == 1 ? "Charge" : v == 2 ? "Discharge" : "Unknown";
  };
  auto dapd_str = [](int v) -> const char * {
    return v == 0 ? "None" : v == 1 ? "AC->DC" : v == 2 ? "DC->AC" : "Unknown";
  };
  auto lpd_str = [](int v) -> const char * {
    return v == 0 ? "None" : v == 1 ? "Input" : v == 2 ? "Output" : "Unknown";
  };

  const int f_conn = parse_int_(f[0]);
  const int f_mode = parse_int_(f[1]);
  const int f_bpd  = parse_int_(f[25]);
  const int f_dapd = parse_int_(f[26]);
  const int f_lpd  = parse_int_(f[27]);

  // PGS-only fields (publish from PGS0 only to avoid overwrite by per-phase frames)
  if (phase == 0) {
    if (pgs_fault_code_sensor_ != nullptr)
      pgs_fault_code_sensor_->publish_state(parse_float_(f[2]));
    if (pgs_mppt1_charger_status_sensor_ != nullptr)
      pgs_mppt1_charger_status_sensor_->publish_state(parse_float_(f[22]));
    if (pgs_mppt2_charger_status_sensor_ != nullptr)
      pgs_mppt2_charger_status_sensor_->publish_state(parse_float_(f[23]));
    if (pgs_load_connected_binary_sensor_ != nullptr)
      pgs_load_connected_binary_sensor_->publish_state(parse_int_(f[24]) == 1);
    if (pgs_work_mode_text_sensor_ != nullptr)
      pgs_work_mode_text_sensor_->publish_state(work_mode_str(f_mode));
    if (pgs_connection_status_text_sensor_ != nullptr)
      pgs_connection_status_text_sensor_->publish_state(conn_status_str(f_conn));
    if (pgs_battery_power_direction_text_sensor_ != nullptr)
      pgs_battery_power_direction_text_sensor_->publish_state(bpd_str(f_bpd));
    if (pgs_dc_ac_power_direction_text_sensor_ != nullptr)
      pgs_dc_ac_power_direction_text_sensor_->publish_state(dapd_str(f_dapd));
    if (pgs_line_power_direction_text_sensor_ != nullptr)
      pgs_line_power_direction_text_sensor_->publish_state(lpd_str(f_lpd));
  }

  if (phase < 3) {
    binary_sensor::BinarySensor *ps_load_conn[3] = {l1_load_connected_binary_sensor_, l2_load_connected_binary_sensor_, l3_load_connected_binary_sensor_};
    if (ps_load_conn[phase])
      ps_load_conn[phase]->publish_state(parse_int_(f[24]) == 1);

    text_sensor::TextSensor *ps_work_mode[3] = {l1_work_mode_text_sensor_, l2_work_mode_text_sensor_, l3_work_mode_text_sensor_};
    text_sensor::TextSensor *ps_bpd[3]       = {l1_battery_power_direction_text_sensor_, l2_battery_power_direction_text_sensor_, l3_battery_power_direction_text_sensor_};
    text_sensor::TextSensor *ps_dapd[3]      = {l1_dc_ac_power_direction_text_sensor_, l2_dc_ac_power_direction_text_sensor_, l3_dc_ac_power_direction_text_sensor_};
    text_sensor::TextSensor *ps_lpd[3]       = {l1_line_power_direction_text_sensor_, l2_line_power_direction_text_sensor_, l3_line_power_direction_text_sensor_};
    if (ps_work_mode[phase]) ps_work_mode[phase]->publish_state(work_mode_str(f_mode));
    if (ps_bpd[phase])       ps_bpd[phase]->publish_state(bpd_str(f_bpd));
    if (ps_dapd[phase])      ps_dapd[phase]->publish_state(dapd_str(f_dapd));
    if (ps_lpd[phase])       ps_lpd[phase]->publish_state(lpd_str(f_lpd));
  }

  // Per-phase (L1/L2/L3) sensors
  if (phase < 3) {
    sensor::Sensor *ps_ac_voltage[3]     = {l1_ac_output_voltage_sensor_,     l2_ac_output_voltage_sensor_,     l3_ac_output_voltage_sensor_};
    sensor::Sensor *ps_ac_frequency[3]   = {l1_ac_output_frequency_sensor_,   l2_ac_output_frequency_sensor_,   l3_ac_output_frequency_sensor_};
    sensor::Sensor *ps_apparent[3]       = {l1_ac_output_apparent_power_sensor_, l2_ac_output_apparent_power_sensor_, l3_ac_output_apparent_power_sensor_};
    sensor::Sensor *ps_active[3]         = {l1_ac_output_active_power_sensor_, l2_ac_output_active_power_sensor_, l3_ac_output_active_power_sensor_};
    sensor::Sensor *ps_load_pct[3]       = {l1_output_load_percent_sensor_,   l2_output_load_percent_sensor_,   l3_output_load_percent_sensor_};
    sensor::Sensor *ps_pv1_power[3]      = {l1_pv1_input_power_sensor_,       l2_pv1_input_power_sensor_,       l3_pv1_input_power_sensor_};
    sensor::Sensor *ps_pv2_power[3]      = {l1_pv2_input_power_sensor_,       l2_pv2_input_power_sensor_,       l3_pv2_input_power_sensor_};
    sensor::Sensor *ps_pv1_voltage[3]    = {l1_pv1_input_voltage_sensor_,     l2_pv1_input_voltage_sensor_,     l3_pv1_input_voltage_sensor_};
    sensor::Sensor *ps_pv2_voltage[3]    = {l1_pv2_input_voltage_sensor_,     l2_pv2_input_voltage_sensor_,     l3_pv2_input_voltage_sensor_};
    sensor::Sensor *ps_temperature[3]    = {l1_max_temperature_sensor_,       l2_max_temperature_sensor_,       l3_max_temperature_sensor_};
    sensor::Sensor *ps_batt_discharge[3] = {l1_battery_discharge_current_sensor_, l2_battery_discharge_current_sensor_, l3_battery_discharge_current_sensor_};
    sensor::Sensor *ps_batt_charge[3]    = {l1_battery_charging_current_sensor_,  l2_battery_charging_current_sensor_,  l3_battery_charging_current_sensor_};

    if (ps_ac_voltage[phase])    ps_ac_voltage[phase]->publish_state(parse_float_(f[5], 0.1f));
    if (ps_ac_frequency[phase])  ps_ac_frequency[phase]->publish_state(parse_float_(f[6], 0.1f));
    if (ps_apparent[phase])      ps_apparent[phase]->publish_state(parse_float_(f[7]));
    if (ps_active[phase])        ps_active[phase]->publish_state(parse_float_(f[8]));
    if (ps_load_pct[phase])      ps_load_pct[phase]->publish_state(parse_float_(f[11]));
    if (ps_pv1_power[phase])     ps_pv1_power[phase]->publish_state(parse_float_(f[18]));
    if (ps_pv2_power[phase])     ps_pv2_power[phase]->publish_state(parse_float_(f[19]));
    if (ps_pv1_voltage[phase])   ps_pv1_voltage[phase]->publish_state(parse_float_(f[20], 0.1f));
    if (ps_pv2_voltage[phase])   ps_pv2_voltage[phase]->publish_state(parse_float_(f[21], 0.1f));
    if (ps_temperature[phase])   ps_temperature[phase]->publish_state(parse_float_(f[28]));
    if (ps_batt_discharge[phase]) ps_batt_discharge[phase]->publish_state(parse_float_(f[14]));
    if (ps_batt_charge[phase])   ps_batt_charge[phase]->publish_state(parse_float_(f[15]));
  }
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

// ─── Decoder: ^P009EYyyyy — Yearly generated energy (kWh) ────────────────────
void PI18Component::decode_ey_(const std::vector<std::string> &f) {
  if (f.empty()) return;
  if (yearly_energy_sensor_ != nullptr)
    yearly_energy_sensor_->publish_state(parse_float_(f[0]));
}

// ─── Decoder: ^P011EMyyyymm — Monthly generated energy (kWh) ─────────────────
void PI18Component::decode_em_(const std::vector<std::string> &f) {
  if (f.empty()) return;
  if (monthly_energy_sensor_ != nullptr)
    monthly_energy_sensor_->publish_state(parse_float_(f[0]));
}

// ─── Decoder: ^P013EDyyyymmdd — Daily generated energy (Wh) ──────────────────
void PI18Component::decode_ed_(const std::vector<std::string> &f) {
  if (f.empty()) return;
  if (daily_energy_sensor_ != nullptr)
    daily_energy_sensor_->publish_state(parse_float_(f[0]));
}

// ─── Decoder: ^P005ACCT — AC charge time bucket (HH:MM,HH:MM) ────────────────
void PI18Component::decode_acct_(const std::vector<std::string> &f) {
  if (f.size() < 2) return;
  if (ac_charge_time_bucket_text_sensor_ == nullptr) return;
  // Each field is "HHMM"; format as "HH:MM-HH:MM"
  auto fmt = [](const std::string &s) -> std::string {
    if (s.size() != 4) return s;
    return s.substr(0, 2) + ":" + s.substr(2, 2);
  };
  ac_charge_time_bucket_text_sensor_->publish_state(fmt(f[0]) + "-" + fmt(f[1]));
}

// ─── Decoder: ^P005ACLT — AC supply load time bucket ─────────────────────────
void PI18Component::decode_aclt_(const std::vector<std::string> &f) {
  if (f.size() < 2) return;
  if (ac_supply_load_time_bucket_text_sensor_ == nullptr) return;
  auto fmt = [](const std::string &s) -> std::string {
    if (s.size() != 4) return s;
    return s.substr(0, 2) + ":" + s.substr(2, 2);
  };
  ac_supply_load_time_bucket_text_sensor_->publish_state(fmt(f[0]) + "-" + fmt(f[1]));
}

// ─── Decoder: ^P004T — Device time YYYYMMDDHHFFSS ────────────────────────────
void PI18Component::decode_t_(const std::vector<std::string> &f) {
  if (f.empty()) return;
  if (device_time_text_sensor_ == nullptr) return;
  const std::string &s = f[0];
  if (s.size() != 14) {
    device_time_text_sensor_->publish_state(s);
    return;
  }
  std::string out = s.substr(0, 4) + "-" + s.substr(4, 2) + "-" + s.substr(6, 2) +
                    " " + s.substr(8, 2) + ":" + s.substr(10, 2) + ":" + s.substr(12, 2);
  device_time_text_sensor_->publish_state(out);
}

}  // namespace pi18
}  // namespace esphome
