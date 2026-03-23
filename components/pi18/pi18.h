#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"

#include <vector>
#include <string>
#include <functional>

namespace esphome {
namespace pi18 {

// ─── CRC ─────────────────────────────────────────────────────────────────────
uint16_t pi18_crc(const uint8_t *data, size_t len);

// ─── PI18 switch helper ───────────────────────────────────────────────────────
class PI18Component;

class PI18Switch : public switch_::Switch {
 public:
  void set_parent(PI18Component *parent) { parent_ = parent; }
  void set_command_on(std::string cmd) { command_on_ = std::move(cmd); }
  void set_command_off(std::string cmd) { command_off_ = std::move(cmd); }

 protected:
  void write_state(bool state) override;
  PI18Component *parent_{nullptr};
  std::string command_on_;
  std::string command_off_;
};

// ─── Main component ───────────────────────────────────────────────────────────
class PI18Component : public uart::UARTDevice, public PollingComponent {
 public:
  // ── Config setters ──────────────────────────────────────────────────────────
  void set_watchdog_pin(GPIOPin *pin) { watchdog_pin_ = pin; }
  void set_watchdog_interval(uint32_t ms) { watchdog_interval_ = ms; }
  void set_parallel_units(uint8_t n) { parallel_units_ = n; }

  // ── Lifecycle ───────────────────────────────────────────────────────────────
  void setup() override;
  void loop() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // ── Send a raw PI18 set command (used by switches/selects/numbers) ──────────
  void send_set_command(const std::string &cmd);

  // ── ^P005GS sensors ─────────────────────────────────────────────────────────
  SUB_SENSOR(grid_voltage)
  SUB_SENSOR(grid_frequency)
  SUB_SENSOR(ac_output_voltage)
  SUB_SENSOR(ac_output_frequency)
  SUB_SENSOR(ac_output_apparent_power)
  SUB_SENSOR(ac_output_active_power)
  SUB_SENSOR(output_load_percent)
  SUB_SENSOR(battery_voltage)
  SUB_SENSOR(battery_voltage_scc1)
  SUB_SENSOR(battery_voltage_scc2)
  SUB_SENSOR(battery_discharge_current)
  SUB_SENSOR(battery_charging_current)
  SUB_SENSOR(battery_capacity)
  SUB_SENSOR(inverter_heatsink_temperature)
  SUB_SENSOR(mppt1_charger_temperature)
  SUB_SENSOR(mppt2_charger_temperature)
  SUB_SENSOR(pv1_input_power)
  SUB_SENSOR(pv2_input_power)
  SUB_SENSOR(pv1_input_voltage)
  SUB_SENSOR(pv2_input_voltage)
  SUB_SENSOR(mppt1_charger_status)
  SUB_SENSOR(mppt2_charger_status)
  SUB_SENSOR(battery_power_direction)
  SUB_SENSOR(dc_ac_power_direction)
  SUB_SENSOR(line_power_direction)
  SUB_SENSOR(local_parallel_id)

  // ── ^P007PIRI sensors ────────────────────────────────────────────────────────
  SUB_SENSOR(grid_rating_voltage)
  SUB_SENSOR(grid_rating_current)
  SUB_SENSOR(ac_output_rating_voltage)
  SUB_SENSOR(ac_output_rating_frequency)
  SUB_SENSOR(ac_output_rating_current)
  SUB_SENSOR(ac_output_rating_apparent_power)
  SUB_SENSOR(ac_output_rating_active_power)
  SUB_SENSOR(battery_rating_voltage)
  SUB_SENSOR(battery_recharge_voltage)
  SUB_SENSOR(battery_redischarge_voltage)
  SUB_SENSOR(battery_under_voltage)
  SUB_SENSOR(battery_bulk_voltage)
  SUB_SENSOR(battery_float_voltage)
  SUB_SENSOR(battery_type)
  SUB_SENSOR(max_ac_charging_current)
  SUB_SENSOR(max_charging_current)
  SUB_SENSOR(input_voltage_range)
  SUB_SENSOR(output_source_priority)
  SUB_SENSOR(charger_source_priority)
  SUB_SENSOR(parallel_max_num)
  SUB_SENSOR(machine_type)
  SUB_SENSOR(topology)
  SUB_SENSOR(output_mode)
  SUB_SENSOR(solar_power_priority)
  SUB_SENSOR(mppt_string_count)

  // ── ^P007PGSn sensors (parallel group, unit 0) ───────────────────────────────
  SUB_SENSOR(pgs_grid_voltage)
  SUB_SENSOR(pgs_grid_frequency)
  SUB_SENSOR(pgs_ac_output_voltage)
  SUB_SENSOR(pgs_ac_output_frequency)
  SUB_SENSOR(pgs_ac_output_apparent_power)
  SUB_SENSOR(pgs_ac_output_active_power)
  SUB_SENSOR(pgs_total_ac_output_apparent_power)
  SUB_SENSOR(pgs_total_ac_output_active_power)
  SUB_SENSOR(pgs_output_load_percent)
  SUB_SENSOR(pgs_total_output_load_percent)
  SUB_SENSOR(pgs_battery_voltage)
  SUB_SENSOR(pgs_battery_discharge_current)
  SUB_SENSOR(pgs_battery_charging_current)
  SUB_SENSOR(pgs_total_battery_charging_current)
  SUB_SENSOR(pgs_battery_capacity)
  SUB_SENSOR(pgs_pv1_input_power)
  SUB_SENSOR(pgs_pv2_input_power)
  SUB_SENSOR(pgs_pv1_input_voltage)
  SUB_SENSOR(pgs_pv2_input_voltage)
  SUB_SENSOR(pgs_max_temperature)

  // ── ^P005ET sensor ───────────────────────────────────────────────────────────
  SUB_SENSOR(total_generated_energy)

  // ── Binary sensors ^P005GS ───────────────────────────────────────────────────
  SUB_BINARY_SENSOR(setting_changed)
  SUB_BINARY_SENSOR(load_connected)

  // ── Binary sensors ^P005FWS ──────────────────────────────────────────────────
  SUB_BINARY_SENSOR(warning_line_fail)
  SUB_BINARY_SENSOR(warning_output_circuit_short)
  SUB_BINARY_SENSOR(warning_over_temperature)
  SUB_BINARY_SENSOR(warning_fan_lock)
  SUB_BINARY_SENSOR(warning_battery_voltage_high)
  SUB_BINARY_SENSOR(warning_battery_low)
  SUB_BINARY_SENSOR(warning_battery_under)
  SUB_BINARY_SENSOR(warning_over_load)
  SUB_BINARY_SENSOR(warning_eeprom_fail)
  SUB_BINARY_SENSOR(warning_power_limit)
  SUB_BINARY_SENSOR(warning_pv1_voltage_high)
  SUB_BINARY_SENSOR(warning_pv2_voltage_high)
  SUB_BINARY_SENSOR(warning_mppt1_overload)
  SUB_BINARY_SENSOR(warning_mppt2_overload)
  SUB_BINARY_SENSOR(warning_scc1_battery_too_low)
  SUB_BINARY_SENSOR(warning_scc2_battery_too_low)

  // ── Binary sensors ^P007FLAG ─────────────────────────────────────────────────
  SUB_BINARY_SENSOR(flag_silence_buzzer)
  SUB_BINARY_SENSOR(flag_overload_bypass)
  SUB_BINARY_SENSOR(flag_lcd_escape_default)
  SUB_BINARY_SENSOR(flag_overload_restart)
  SUB_BINARY_SENSOR(flag_over_temp_restart)
  SUB_BINARY_SENSOR(flag_backlight_on)
  SUB_BINARY_SENSOR(flag_alarm_primary_interrupt)
  SUB_BINARY_SENSOR(flag_fault_code_record)

  // ── Text sensors ─────────────────────────────────────────────────────────────
  SUB_TEXT_SENSOR(device_mode)
  SUB_TEXT_SENSOR(fault_code)
  SUB_TEXT_SENSOR(battery_power_direction_text)
  SUB_TEXT_SENSOR(dc_ac_power_direction_text)
  SUB_TEXT_SENSOR(line_power_direction_text)

 protected:
  // ── Internals ────────────────────────────────────────────────────────────────
  enum class State {
    IDLE,
    WAITING,
  };

  // Watchdog
  GPIOPin *watchdog_pin_{nullptr};
  uint32_t watchdog_interval_{1000};
  uint32_t last_watchdog_{0};
  bool watchdog_state_{false};

  // Parallel config
  uint8_t parallel_units_{1};

  // Poll state
  State state_{State::IDLE};
  std::vector<std::string> poll_commands_;
  size_t poll_index_{0};
  uint32_t last_tx_{0};
  uint32_t tx_timeout_{2000};  // ms to wait for response

  // RX buffer
  std::string rx_buf_;

  // Command queue for set commands (fire-and-forget)
  std::vector<std::string> set_queue_;

  // Helpers
  void build_poll_commands_();
  std::string build_frame_(const std::string &cmd);
  void send_frame_(const std::string &cmd);
  bool try_read_response_();
  void dispatch_response_(const std::string &cmd, const std::string &payload);

  // Decoders
  void decode_gs_(const std::vector<std::string> &f);
  void decode_piri_(const std::vector<std::string> &f);
  void decode_fws_(const std::vector<std::string> &f);
  void decode_mod_(const std::vector<std::string> &f);
  void decode_flag_(const std::vector<std::string> &f);
  void decode_pgs_(const std::vector<std::string> &f);
  void decode_et_(const std::vector<std::string> &f);

  // Parser helpers
  static std::vector<std::string> split_csv_(const std::string &s);
  static float parse_float_(const std::string &s, float scale = 1.0f);
  static int parse_int_(const std::string &s);

  // Fault code to string
  static std::string fault_code_str_(int code);
};

}  // namespace pi18
}  // namespace esphome
