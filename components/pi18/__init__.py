import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
from esphome import pins

CODEOWNERS = ["@JakubRybakowski"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor", "switch"]
MULTI_CONF = False

CONF_PI18_ID = "pi18_id"
CONF_WATCHDOG_PIN = "watchdog_pin"
CONF_WATCHDOG_INTERVAL = "watchdog_interval"
CONF_PARALLEL_UNITS = "parallel_units"

pi18_ns = cg.esphome_ns.namespace("pi18")
PI18Component = pi18_ns.class_("PI18Component", uart.UARTDevice, cg.PollingComponent)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PI18Component),
            cv.Optional(CONF_WATCHDOG_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_WATCHDOG_INTERVAL, default="1s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_PARALLEL_UNITS, default=1): cv.int_range(min=1, max=3),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("10s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_WATCHDOG_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_WATCHDOG_PIN])
        cg.add(var.set_watchdog_pin(pin))

    cg.add(var.set_watchdog_interval(config[CONF_WATCHDOG_INTERVAL]))
    cg.add(var.set_parallel_units(config[CONF_PARALLEL_UNITS]))
