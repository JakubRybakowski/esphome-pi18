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
