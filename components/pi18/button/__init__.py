import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from .. import PI18Component, CONF_PI18_ID, pi18_ns

DEPENDENCIES = ["pi18"]

PI18Button = pi18_ns.class_("PI18Button", button.Button)

# (key, command)
BUTTONS = {
    "restore_defaults": "PF",    # ^S005PF — reset all settings to factory defaults
    "clear_energy":     "CLE",   # ^S006CLE — clear total generated energy counter
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{
            cv.Optional(key): button.button_schema(
                PI18Button,
                entity_category=cg.EntityCategory.CONFIG,
            )
            for key in BUTTONS
        },
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PI18_ID])
    for key, cmd in BUTTONS.items():
        if key not in config:
            continue
        var = await button.new_button(config[key])
        cg.add(var.set_parent(parent))
        cg.add(var.set_command(cmd))
