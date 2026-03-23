import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from .. import PI18Component, CONF_PI18_ID, pi18_ns

DEPENDENCIES = ["pi18"]

PI18Select = pi18_ns.class_("PI18Select", select.Select, cg.Component)


class PI18SelectClass(select.Select):
    pass


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        cv.Optional("max_charging_current"): select.select_schema(
            select.Select,
            entity_category=cg.EntityCategory.CONFIG,
        ),
        cv.Optional("max_ac_charging_current"): select.select_schema(
            select.Select,
            entity_category=cg.EntityCategory.CONFIG,
        ),
        cv.Optional("output_source_priority_select"): select.select_schema(
            select.Select,
            entity_category=cg.EntityCategory.CONFIG,
        ),
        cv.Optional("charger_source_priority_select"): select.select_schema(
            select.Select,
            entity_category=cg.EntityCategory.CONFIG,
        ),
        cv.Optional("battery_type_select"): select.select_schema(
            select.Select,
            entity_category=cg.EntityCategory.CONFIG,
        ),
    }
)


async def to_code(config):
    # Selects are mostly informational in this version.
    # Actual set commands are handled through switches/numbers.
    pass
