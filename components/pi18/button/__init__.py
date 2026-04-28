import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from .. import PI18Component, CONF_PI18_ID, pi18_ns

DEPENDENCIES = ["pi18"]

PI18Button = pi18_ns.class_("PI18Button", button.Button)

PI18SetDateTimeButton = pi18_ns.class_("PI18SetDateTimeButton", button.Button)

BUTTONS = {
    "restore_defaults": "PF",
    "clear_energy":     "CLE",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{
            cv.Optional(key): button.button_schema(PI18Button)
            for key in BUTTONS
        },
        cv.Optional("set_date_time"): button.button_schema(PI18SetDateTimeButton),
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
    if "set_date_time" in config:
        var = await button.new_button(config["set_date_time"])
        cg.add(var.set_parent(parent))
