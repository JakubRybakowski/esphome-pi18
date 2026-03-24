import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from .. import PI18Component, CONF_PI18_ID, pi18_ns

DEPENDENCIES = ["pi18"]

PI18Number = pi18_ns.class_("PI18Number", number.Number)

# ── Number entity definitions ─────────────────────────────────────────────────
# (key, min, max, step, unit, pair_role, multiplier, command_template)
NUMBER_ENTRIES = {
    "battery_bulk_voltage":       (48.0, 58.5, 0.1, "V", 1, 10.0, ""),
    "battery_float_voltage":      (48.0, 58.5, 0.1, "V", 2, 10.0, ""),
    "battery_recharge_voltage":   (44.0, 53.0, 0.1, "V", 3, 10.0, ""),
    "battery_redischarge_voltage":(0.0,  58.0, 0.1, "V", 4, 10.0, ""),
    "battery_cutoff_voltage":     (40.0, 51.0, 0.1, "V", 0, 10.0, "PSDV%03d"),
}

NUMBER_SETTERS = {
    "battery_bulk_voltage":       "set_battery_bulk_voltage_number",
    "battery_float_voltage":      "set_battery_float_voltage_number",
    "battery_recharge_voltage":   "set_battery_recharge_voltage_number",
    "battery_redischarge_voltage":"set_battery_redischarge_voltage_number",
    "battery_cutoff_voltage":     "set_battery_cutoff_voltage_number",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{
            cv.Optional(key): number.number_schema(PI18Number)
            for key in NUMBER_ENTRIES
        },
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PI18_ID])
    for key, (min_v, max_v, step, unit, pair_role, multiplier, cmd_tmpl) in NUMBER_ENTRIES.items():
        if key not in config:
            continue
        var = await number.new_number(
            config[key],
            min_value=min_v,
            max_value=max_v,
            step=step,
        )
        cg.add(var.set_parent(parent))
        cg.add(var.set_pair_role(pair_role))
        cg.add(var.set_multiplier(multiplier))
        if cmd_tmpl:
            cg.add(var.set_command_template(cmd_tmpl))
        setter = NUMBER_SETTERS.get(key)
        if setter:
            cg.add(getattr(parent, setter)(var))
