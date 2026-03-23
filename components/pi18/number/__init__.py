import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from .. import PI18Component, CONF_PI18_ID, pi18_ns

DEPENDENCIES = ["pi18"]

PI18Number = pi18_ns.class_("PI18Number", number.Number, cg.Component)

# ── Number entity definitions ─────────────────────────────────────────────────
# (key, min, max, step, unit, pair_role, multiplier, command_template)
#
# pair_role:
#   0 = simple  (uses command_template with single %d, value * multiplier)
#   1 = bulk voltage    (paired with float via MCHGV command)
#   2 = float voltage   (paired with bulk via MCHGV command)
#   3 = recharge voltage      (paired with redischarge via BUCD command)
#   4 = redischarge voltage   (paired with recharge via BUCD command)
#
# command_template only used when pair_role==0

NUMBER_ENTRIES = {
    # Paired: MCHGV — battery bulk and float voltage (sent together)
    "battery_bulk_voltage": (
        48.0, 58.5, 0.1, "V", 1, 10.0, "",
    ),
    "battery_float_voltage": (
        48.0, 58.5, 0.1, "V", 2, 10.0, "",
    ),
    # Paired: BUCD — battery recharge and redischarge voltage (sent together)
    "battery_recharge_voltage": (
        44.0, 53.0, 0.1, "V", 3, 10.0, "",
    ),
    "battery_redischarge_voltage": (
        0.0, 58.0, 0.1, "V", 4, 10.0, "",
    ),
    # Simple: PSDV — battery cutoff (under) voltage
    # PSDVmmm: mmm = voltage * 10 (e.g. 42.0V → 420 → "420")
    "battery_cutoff_voltage": (
        40.0, 51.0, 0.1, "V", 0, 10.0, "PSDV%03d",
    ),
}

# Setter method names on PI18Component for number pointers
NUMBER_SETTERS = {
    "battery_bulk_voltage":      "set_battery_bulk_voltage_number",
    "battery_float_voltage":     "set_battery_float_voltage_number",
    "battery_recharge_voltage":  "set_battery_recharge_voltage_number",
    "battery_redischarge_voltage": "set_battery_redischarge_voltage_number",
    "battery_cutoff_voltage":    "set_battery_cutoff_voltage_number",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{
            cv.Optional(key): number.number_schema(
                PI18Number,
                entity_category=cg.EntityCategory.CONFIG,
                unit_of_measurement=entry[3],
            )
            for key, entry in NUMBER_ENTRIES.items()
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
        await cg.register_component(var, config[key])
        cg.add(var.set_parent(parent))
        cg.add(var.set_pair_role(pair_role))
        cg.add(var.set_multiplier(multiplier))
        if cmd_tmpl:
            cg.add(var.set_command_template(cmd_tmpl))
        # Register number pointer in parent so PIRI reads can publish state
        setter = NUMBER_SETTERS.get(key)
        if setter:
            cg.add(getattr(parent, setter)(var))
