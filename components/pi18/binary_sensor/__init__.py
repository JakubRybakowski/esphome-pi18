import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from .. import PI18Component, CONF_PI18_ID

DEPENDENCIES = ["pi18"]

BINARY_SENSORS = {
    # ^P005GS
    "setting_changed": "set_setting_changed_binary_sensor",
    "load_connected": "set_load_connected_binary_sensor",
    # ^P005FWS
    "warning_line_fail": "set_warning_line_fail_binary_sensor",
    "warning_output_circuit_short": "set_warning_output_circuit_short_binary_sensor",
    "warning_over_temperature": "set_warning_over_temperature_binary_sensor",
    "warning_fan_lock": "set_warning_fan_lock_binary_sensor",
    "warning_battery_voltage_high": "set_warning_battery_voltage_high_binary_sensor",
    "warning_battery_low": "set_warning_battery_low_binary_sensor",
    "warning_battery_under": "set_warning_battery_under_binary_sensor",
    "warning_over_load": "set_warning_over_load_binary_sensor",
    "warning_eeprom_fail": "set_warning_eeprom_fail_binary_sensor",
    "warning_power_limit": "set_warning_power_limit_binary_sensor",
    "warning_pv1_voltage_high": "set_warning_pv1_voltage_high_binary_sensor",
    "warning_pv2_voltage_high": "set_warning_pv2_voltage_high_binary_sensor",
    "warning_mppt1_overload": "set_warning_mppt1_overload_binary_sensor",
    "warning_mppt2_overload": "set_warning_mppt2_overload_binary_sensor",
    "warning_scc1_battery_too_low": "set_warning_scc1_battery_too_low_binary_sensor",
    "warning_scc2_battery_too_low": "set_warning_scc2_battery_too_low_binary_sensor",
    # ^P007FLAG
    "flag_silence_buzzer": "set_flag_silence_buzzer_binary_sensor",
    "flag_overload_bypass": "set_flag_overload_bypass_binary_sensor",
    "flag_lcd_escape_default": "set_flag_lcd_escape_default_binary_sensor",
    "flag_overload_restart": "set_flag_overload_restart_binary_sensor",
    "flag_over_temp_restart": "set_flag_over_temp_restart_binary_sensor",
    "flag_backlight_on": "set_flag_backlight_on_binary_sensor",
    "flag_alarm_primary_interrupt": "set_flag_alarm_primary_interrupt_binary_sensor",
    "flag_fault_code_record": "set_flag_fault_code_record_binary_sensor",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{cv.Optional(k): binary_sensor.binary_sensor_schema() for k in BINARY_SENSORS},
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PI18_ID])
    for key, setter in BINARY_SENSORS.items():
        if key in config:
            s = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(parent, setter)(s))
