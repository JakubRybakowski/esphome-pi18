import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_BATTERY,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_WATT,
    UNIT_HERTZ,
    UNIT_PERCENT,
    UNIT_CELSIUS,
    UNIT_WATT_HOURS,
    UNIT_VOLT_AMPS,
)
from .. import PI18Component, CONF_PI18_ID, pi18_ns

DEPENDENCIES = ["pi18"]

# ── sensor keys ───────────────────────────────────────────────────────────────
# ^P005GS
CONF_GRID_VOLTAGE = "grid_voltage"
CONF_GRID_FREQUENCY = "grid_frequency"
CONF_AC_OUTPUT_VOLTAGE = "ac_output_voltage"
CONF_AC_OUTPUT_FREQUENCY = "ac_output_frequency"
CONF_AC_OUTPUT_APPARENT_POWER = "ac_output_apparent_power"
CONF_AC_OUTPUT_ACTIVE_POWER = "ac_output_active_power"
CONF_OUTPUT_LOAD_PERCENT = "output_load_percent"
CONF_BATTERY_VOLTAGE = "battery_voltage"
CONF_BATTERY_VOLTAGE_SCC1 = "battery_voltage_scc1"
CONF_BATTERY_VOLTAGE_SCC2 = "battery_voltage_scc2"
CONF_BATTERY_DISCHARGE_CURRENT = "battery_discharge_current"
CONF_BATTERY_CHARGING_CURRENT = "battery_charging_current"
CONF_BATTERY_CAPACITY = "battery_capacity"
CONF_INVERTER_HEATSINK_TEMPERATURE = "inverter_heatsink_temperature"
CONF_MPPT1_CHARGER_TEMPERATURE = "mppt1_charger_temperature"
CONF_MPPT2_CHARGER_TEMPERATURE = "mppt2_charger_temperature"
CONF_PV1_INPUT_POWER = "pv1_input_power"
CONF_PV2_INPUT_POWER = "pv2_input_power"
CONF_PV1_INPUT_VOLTAGE = "pv1_input_voltage"
CONF_PV2_INPUT_VOLTAGE = "pv2_input_voltage"
CONF_MPPT1_CHARGER_STATUS = "mppt1_charger_status"
CONF_MPPT2_CHARGER_STATUS = "mppt2_charger_status"
CONF_BATTERY_POWER_DIRECTION = "battery_power_direction"
CONF_DC_AC_POWER_DIRECTION = "dc_ac_power_direction"
CONF_LINE_POWER_DIRECTION = "line_power_direction"
CONF_LOCAL_PARALLEL_ID = "local_parallel_id"

# ^P007PIRI
CONF_GRID_RATING_VOLTAGE = "grid_rating_voltage"
CONF_GRID_RATING_CURRENT = "grid_rating_current"
CONF_AC_OUTPUT_RATING_VOLTAGE = "ac_output_rating_voltage"
CONF_AC_OUTPUT_RATING_FREQUENCY = "ac_output_rating_frequency"
CONF_AC_OUTPUT_RATING_CURRENT = "ac_output_rating_current"
CONF_AC_OUTPUT_RATING_APPARENT_POWER = "ac_output_rating_apparent_power"
CONF_AC_OUTPUT_RATING_ACTIVE_POWER = "ac_output_rating_active_power"
CONF_BATTERY_RATING_VOLTAGE = "battery_rating_voltage"
CONF_BATTERY_RECHARGE_VOLTAGE = "battery_recharge_voltage"
CONF_BATTERY_REDISCHARGE_VOLTAGE = "battery_redischarge_voltage"
CONF_BATTERY_UNDER_VOLTAGE = "battery_under_voltage"
CONF_BATTERY_BULK_VOLTAGE = "battery_bulk_voltage"
CONF_BATTERY_FLOAT_VOLTAGE = "battery_float_voltage"
CONF_BATTERY_TYPE = "battery_type"
CONF_MAX_AC_CHARGING_CURRENT = "max_ac_charging_current"
CONF_MAX_CHARGING_CURRENT = "max_charging_current"
CONF_INPUT_VOLTAGE_RANGE = "input_voltage_range"
CONF_OUTPUT_SOURCE_PRIORITY = "output_source_priority"
CONF_CHARGER_SOURCE_PRIORITY = "charger_source_priority"
CONF_PARALLEL_MAX_NUM = "parallel_max_num"
CONF_MACHINE_TYPE = "machine_type"
CONF_TOPOLOGY = "topology"
CONF_OUTPUT_MODE = "output_mode"
CONF_SOLAR_POWER_PRIORITY = "solar_power_priority"
CONF_MPPT_STRING_COUNT = "mppt_string_count"

# ^P007PGSn
CONF_PGS_GRID_VOLTAGE = "pgs_grid_voltage"
CONF_PGS_GRID_FREQUENCY = "pgs_grid_frequency"
CONF_PGS_AC_OUTPUT_VOLTAGE = "pgs_ac_output_voltage"
CONF_PGS_AC_OUTPUT_FREQUENCY = "pgs_ac_output_frequency"
CONF_PGS_AC_OUTPUT_APPARENT_POWER = "pgs_ac_output_apparent_power"
CONF_PGS_AC_OUTPUT_ACTIVE_POWER = "pgs_ac_output_active_power"
CONF_PGS_TOTAL_AC_OUTPUT_APPARENT_POWER = "pgs_total_ac_output_apparent_power"
CONF_PGS_TOTAL_AC_OUTPUT_ACTIVE_POWER = "pgs_total_ac_output_active_power"
CONF_PGS_OUTPUT_LOAD_PERCENT = "pgs_output_load_percent"
CONF_PGS_TOTAL_OUTPUT_LOAD_PERCENT = "pgs_total_output_load_percent"
CONF_PGS_BATTERY_VOLTAGE = "pgs_battery_voltage"
CONF_PGS_BATTERY_DISCHARGE_CURRENT = "pgs_battery_discharge_current"
CONF_PGS_BATTERY_CHARGING_CURRENT = "pgs_battery_charging_current"
CONF_PGS_TOTAL_BATTERY_CHARGING_CURRENT = "pgs_total_battery_charging_current"
CONF_PGS_BATTERY_CAPACITY = "pgs_battery_capacity"
CONF_PGS_PV1_INPUT_POWER = "pgs_pv1_input_power"
CONF_PGS_PV2_INPUT_POWER = "pgs_pv2_input_power"
CONF_PGS_PV1_INPUT_VOLTAGE = "pgs_pv1_input_voltage"
CONF_PGS_PV2_INPUT_VOLTAGE = "pgs_pv2_input_voltage"
CONF_PGS_MAX_TEMPERATURE = "pgs_max_temperature"

# ^P005ET
CONF_TOTAL_GENERATED_ENERGY = "total_generated_energy"

SENSOR_SCHEMAS = {
    # GS
    CONF_GRID_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_GRID_FREQUENCY: sensor.sensor_schema(unit_of_measurement=UNIT_HERTZ, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_FREQUENCY: sensor.sensor_schema(unit_of_measurement=UNIT_HERTZ, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_APPARENT_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT_AMPS, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_ACTIVE_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT),
    CONF_OUTPUT_LOAD_PERCENT: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_VOLTAGE_SCC1: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_VOLTAGE_SCC2: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_DISCHARGE_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=0, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_CHARGING_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=0, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_CAPACITY: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, device_class=DEVICE_CLASS_BATTERY, state_class=STATE_CLASS_MEASUREMENT),
    CONF_INVERTER_HEATSINK_TEMPERATURE: sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_MPPT1_CHARGER_TEMPERATURE: sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_MPPT2_CHARGER_TEMPERATURE: sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PV1_INPUT_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PV2_INPUT_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PV1_INPUT_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PV2_INPUT_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_MPPT1_CHARGER_STATUS: sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_MPPT2_CHARGER_STATUS: sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_POWER_DIRECTION: sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_DC_AC_POWER_DIRECTION: sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_LINE_POWER_DIRECTION: sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_LOCAL_PARALLEL_ID: sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    # PIRI
    CONF_GRID_RATING_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_GRID_RATING_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=1, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_RATING_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_RATING_FREQUENCY: sensor.sensor_schema(unit_of_measurement=UNIT_HERTZ, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_RATING_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=1, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_RATING_APPARENT_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT_AMPS, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_AC_OUTPUT_RATING_ACTIVE_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_RATING_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_RECHARGE_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_REDISCHARGE_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_UNDER_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_BULK_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_FLOAT_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BATTERY_TYPE: sensor.sensor_schema(accuracy_decimals=0),
    CONF_MAX_AC_CHARGING_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=0, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_MAX_CHARGING_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=0, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_INPUT_VOLTAGE_RANGE: sensor.sensor_schema(accuracy_decimals=0),
    CONF_OUTPUT_SOURCE_PRIORITY: sensor.sensor_schema(accuracy_decimals=0),
    CONF_CHARGER_SOURCE_PRIORITY: sensor.sensor_schema(accuracy_decimals=0),
    CONF_PARALLEL_MAX_NUM: sensor.sensor_schema(accuracy_decimals=0),
    CONF_MACHINE_TYPE: sensor.sensor_schema(accuracy_decimals=0),
    CONF_TOPOLOGY: sensor.sensor_schema(accuracy_decimals=0),
    CONF_OUTPUT_MODE: sensor.sensor_schema(accuracy_decimals=0),
    CONF_SOLAR_POWER_PRIORITY: sensor.sensor_schema(accuracy_decimals=0),
    CONF_MPPT_STRING_COUNT: sensor.sensor_schema(accuracy_decimals=0),
    # PGS
    CONF_PGS_GRID_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_GRID_FREQUENCY: sensor.sensor_schema(unit_of_measurement=UNIT_HERTZ, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_AC_OUTPUT_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_AC_OUTPUT_FREQUENCY: sensor.sensor_schema(unit_of_measurement=UNIT_HERTZ, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_AC_OUTPUT_APPARENT_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT_AMPS, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_AC_OUTPUT_ACTIVE_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_TOTAL_AC_OUTPUT_APPARENT_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT_AMPS, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_TOTAL_AC_OUTPUT_ACTIVE_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_OUTPUT_LOAD_PERCENT: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_TOTAL_OUTPUT_LOAD_PERCENT: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_BATTERY_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_BATTERY_DISCHARGE_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=0, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_BATTERY_CHARGING_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=0, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_TOTAL_BATTERY_CHARGING_CURRENT: sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=0, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_BATTERY_CAPACITY: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, device_class=DEVICE_CLASS_BATTERY, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_PV1_INPUT_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_PV2_INPUT_POWER: sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_PV1_INPUT_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_PV2_INPUT_VOLTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT),
    CONF_PGS_MAX_TEMPERATURE: sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
    # ET
    CONF_TOTAL_GENERATED_ENERGY: sensor.sensor_schema(unit_of_measurement=UNIT_WATT_HOURS, accuracy_decimals=0, device_class=DEVICE_CLASS_ENERGY, state_class=STATE_CLASS_TOTAL_INCREASING),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{cv.Optional(k): v for k, v in SENSOR_SCHEMAS.items()},
    }
)

# mapping from config key to setter method name
SENSOR_SETTERS = {
    CONF_GRID_VOLTAGE: "set_grid_voltage_sensor",
    CONF_GRID_FREQUENCY: "set_grid_frequency_sensor",
    CONF_AC_OUTPUT_VOLTAGE: "set_ac_output_voltage_sensor",
    CONF_AC_OUTPUT_FREQUENCY: "set_ac_output_frequency_sensor",
    CONF_AC_OUTPUT_APPARENT_POWER: "set_ac_output_apparent_power_sensor",
    CONF_AC_OUTPUT_ACTIVE_POWER: "set_ac_output_active_power_sensor",
    CONF_OUTPUT_LOAD_PERCENT: "set_output_load_percent_sensor",
    CONF_BATTERY_VOLTAGE: "set_battery_voltage_sensor",
    CONF_BATTERY_VOLTAGE_SCC1: "set_battery_voltage_scc1_sensor",
    CONF_BATTERY_VOLTAGE_SCC2: "set_battery_voltage_scc2_sensor",
    CONF_BATTERY_DISCHARGE_CURRENT: "set_battery_discharge_current_sensor",
    CONF_BATTERY_CHARGING_CURRENT: "set_battery_charging_current_sensor",
    CONF_BATTERY_CAPACITY: "set_battery_capacity_sensor",
    CONF_INVERTER_HEATSINK_TEMPERATURE: "set_inverter_heatsink_temperature_sensor",
    CONF_MPPT1_CHARGER_TEMPERATURE: "set_mppt1_charger_temperature_sensor",
    CONF_MPPT2_CHARGER_TEMPERATURE: "set_mppt2_charger_temperature_sensor",
    CONF_PV1_INPUT_POWER: "set_pv1_input_power_sensor",
    CONF_PV2_INPUT_POWER: "set_pv2_input_power_sensor",
    CONF_PV1_INPUT_VOLTAGE: "set_pv1_input_voltage_sensor",
    CONF_PV2_INPUT_VOLTAGE: "set_pv2_input_voltage_sensor",
    CONF_MPPT1_CHARGER_STATUS: "set_mppt1_charger_status_sensor",
    CONF_MPPT2_CHARGER_STATUS: "set_mppt2_charger_status_sensor",
    CONF_BATTERY_POWER_DIRECTION: "set_battery_power_direction_sensor",
    CONF_DC_AC_POWER_DIRECTION: "set_dc_ac_power_direction_sensor",
    CONF_LINE_POWER_DIRECTION: "set_line_power_direction_sensor",
    CONF_LOCAL_PARALLEL_ID: "set_local_parallel_id_sensor",
    CONF_GRID_RATING_VOLTAGE: "set_grid_rating_voltage_sensor",
    CONF_GRID_RATING_CURRENT: "set_grid_rating_current_sensor",
    CONF_AC_OUTPUT_RATING_VOLTAGE: "set_ac_output_rating_voltage_sensor",
    CONF_AC_OUTPUT_RATING_FREQUENCY: "set_ac_output_rating_frequency_sensor",
    CONF_AC_OUTPUT_RATING_CURRENT: "set_ac_output_rating_current_sensor",
    CONF_AC_OUTPUT_RATING_APPARENT_POWER: "set_ac_output_rating_apparent_power_sensor",
    CONF_AC_OUTPUT_RATING_ACTIVE_POWER: "set_ac_output_rating_active_power_sensor",
    CONF_BATTERY_RATING_VOLTAGE: "set_battery_rating_voltage_sensor",
    CONF_BATTERY_RECHARGE_VOLTAGE: "set_battery_recharge_voltage_sensor",
    CONF_BATTERY_REDISCHARGE_VOLTAGE: "set_battery_redischarge_voltage_sensor",
    CONF_BATTERY_UNDER_VOLTAGE: "set_battery_under_voltage_sensor",
    CONF_BATTERY_BULK_VOLTAGE: "set_battery_bulk_voltage_sensor",
    CONF_BATTERY_FLOAT_VOLTAGE: "set_battery_float_voltage_sensor",
    CONF_BATTERY_TYPE: "set_battery_type_sensor",
    CONF_MAX_AC_CHARGING_CURRENT: "set_max_ac_charging_current_sensor",
    CONF_MAX_CHARGING_CURRENT: "set_max_charging_current_sensor",
    CONF_INPUT_VOLTAGE_RANGE: "set_input_voltage_range_sensor",
    CONF_OUTPUT_SOURCE_PRIORITY: "set_output_source_priority_sensor",
    CONF_CHARGER_SOURCE_PRIORITY: "set_charger_source_priority_sensor",
    CONF_PARALLEL_MAX_NUM: "set_parallel_max_num_sensor",
    CONF_MACHINE_TYPE: "set_machine_type_sensor",
    CONF_TOPOLOGY: "set_topology_sensor",
    CONF_OUTPUT_MODE: "set_output_mode_sensor",
    CONF_SOLAR_POWER_PRIORITY: "set_solar_power_priority_sensor",
    CONF_MPPT_STRING_COUNT: "set_mppt_string_count_sensor",
    CONF_PGS_GRID_VOLTAGE: "set_pgs_grid_voltage_sensor",
    CONF_PGS_GRID_FREQUENCY: "set_pgs_grid_frequency_sensor",
    CONF_PGS_AC_OUTPUT_VOLTAGE: "set_pgs_ac_output_voltage_sensor",
    CONF_PGS_AC_OUTPUT_FREQUENCY: "set_pgs_ac_output_frequency_sensor",
    CONF_PGS_AC_OUTPUT_APPARENT_POWER: "set_pgs_ac_output_apparent_power_sensor",
    CONF_PGS_AC_OUTPUT_ACTIVE_POWER: "set_pgs_ac_output_active_power_sensor",
    CONF_PGS_TOTAL_AC_OUTPUT_APPARENT_POWER: "set_pgs_total_ac_output_apparent_power_sensor",
    CONF_PGS_TOTAL_AC_OUTPUT_ACTIVE_POWER: "set_pgs_total_ac_output_active_power_sensor",
    CONF_PGS_OUTPUT_LOAD_PERCENT: "set_pgs_output_load_percent_sensor",
    CONF_PGS_TOTAL_OUTPUT_LOAD_PERCENT: "set_pgs_total_output_load_percent_sensor",
    CONF_PGS_BATTERY_VOLTAGE: "set_pgs_battery_voltage_sensor",
    CONF_PGS_BATTERY_DISCHARGE_CURRENT: "set_pgs_battery_discharge_current_sensor",
    CONF_PGS_BATTERY_CHARGING_CURRENT: "set_pgs_battery_charging_current_sensor",
    CONF_PGS_TOTAL_BATTERY_CHARGING_CURRENT: "set_pgs_total_battery_charging_current_sensor",
    CONF_PGS_BATTERY_CAPACITY: "set_pgs_battery_capacity_sensor",
    CONF_PGS_PV1_INPUT_POWER: "set_pgs_pv1_input_power_sensor",
    CONF_PGS_PV2_INPUT_POWER: "set_pgs_pv2_input_power_sensor",
    CONF_PGS_PV1_INPUT_VOLTAGE: "set_pgs_pv1_input_voltage_sensor",
    CONF_PGS_PV2_INPUT_VOLTAGE: "set_pgs_pv2_input_voltage_sensor",
    CONF_PGS_MAX_TEMPERATURE: "set_pgs_max_temperature_sensor",
    CONF_TOTAL_GENERATED_ENERGY: "set_total_generated_energy_sensor",
}


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PI18_ID])
    for key, setter in SENSOR_SETTERS.items():
        if key in config:
            s = await sensor.new_sensor(config[key])
            cg.add(getattr(parent, setter)(s))
