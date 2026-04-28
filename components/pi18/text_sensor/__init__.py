import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from .. import PI18Component, CONF_PI18_ID

DEPENDENCIES = ["pi18"]

TEXT_SENSORS = {
    "device_mode":                   "set_device_mode_text_sensor",
    "fault_code":                    "set_fault_code_text_sensor",
    "battery_power_direction_text":  "set_battery_power_direction_text_text_sensor",
    "dc_ac_power_direction_text":    "set_dc_ac_power_direction_text_text_sensor",
    "line_power_direction_text":     "set_line_power_direction_text_text_sensor",
    # Informational (polled once, rarely changes)
    "protocol_id":                   "set_protocol_id_text_sensor",
    "serial_number":                 "set_serial_number_text_sensor",
    "firmware_version":              "set_firmware_version_text_sensor",
    # PGS (parallel group, unit 0)
    "pgs_work_mode":                 "set_pgs_work_mode_text_sensor",
    "pgs_connection_status":         "set_pgs_connection_status_text_sensor",
    "pgs_battery_power_direction":   "set_pgs_battery_power_direction_text_sensor",
    "pgs_dc_ac_power_direction":     "set_pgs_dc_ac_power_direction_text_sensor",
    "pgs_line_power_direction":      "set_pgs_line_power_direction_text_sensor",
    # Per-phase L1
    "l1_work_mode":                  "set_l1_work_mode_text_sensor",
    "l1_battery_power_direction":    "set_l1_battery_power_direction_text_sensor",
    "l1_dc_ac_power_direction":      "set_l1_dc_ac_power_direction_text_sensor",
    "l1_line_power_direction":       "set_l1_line_power_direction_text_sensor",
    # Per-phase L2
    "l2_work_mode":                  "set_l2_work_mode_text_sensor",
    "l2_battery_power_direction":    "set_l2_battery_power_direction_text_sensor",
    "l2_dc_ac_power_direction":      "set_l2_dc_ac_power_direction_text_sensor",
    "l2_line_power_direction":       "set_l2_line_power_direction_text_sensor",
    # Per-phase L3
    "l3_work_mode":                  "set_l3_work_mode_text_sensor",
    "l3_battery_power_direction":    "set_l3_battery_power_direction_text_sensor",
    "l3_dc_ac_power_direction":      "set_l3_dc_ac_power_direction_text_sensor",
    "l3_line_power_direction":       "set_l3_line_power_direction_text_sensor",
    # AC charge / supply load time buckets
    "ac_charge_time_bucket":         "set_ac_charge_time_bucket_text_sensor",
    "ac_supply_load_time_bucket":    "set_ac_supply_load_time_bucket_text_sensor",
    # Current device time
    "device_time":                   "set_device_time_text_sensor",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{cv.Optional(k): text_sensor.text_sensor_schema() for k in TEXT_SENSORS},
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PI18_ID])
    for key, setter in TEXT_SENSORS.items():
        if key in config:
            s = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(parent, setter)(s))
