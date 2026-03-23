import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from .. import PI18Component, CONF_PI18_ID, pi18_ns

DEPENDENCIES = ["pi18"]

PI18Switch = pi18_ns.class_("PI18Switch", switch.Switch)

# (config_key, command_on, command_off)
SWITCHES = {
    # Output source priority  0=Solar-Utility-Battery  1=Solar-Battery-Utility
    "output_source_priority_sub": ("POP1", "POP0"),
    # Solar power priority  0=Battery-Load-Utility  1=Load-Battery-Utility
    "solar_power_priority_load_first": ("PSP1", "PSP0"),
    # Charger source priority
    "charger_solar_first": ("PCP0,0", "PCP0,1"),   # 0=solar first
    "charger_utility": ("PCP0,1", "PCP0,0"),
    "charger_solar_only": ("PCP0,2", "PCP0,0"),
    # Flags  (PE=enable, PD=disable)
    "flag_silence_buzzer_sw": ("PEA", "PDA"),
    "flag_overload_bypass_sw": ("PEB", "PDB"),
    "flag_lcd_escape_sw": ("PEC", "PDC"),
    "flag_overload_restart_sw": ("PED", "PDD"),
    "flag_over_temp_restart_sw": ("PEE", "PDE"),
    "flag_backlight_sw": ("PEF", "PDF"),
    "flag_alarm_primary_sw": ("PEG", "PDG"),
    "flag_fault_record_sw": ("PEH", "PDH"),
    # Load on/off
    "load_on": ("LON1", "LON0"),
    # AC output frequency (50 Hz = on, 60 Hz = off)
    "ac_freq_50hz": ("F50", "F60"),
    # Grid-tie mode
    "grid_tie_mode": ("PEI", "PDI"),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{
            cv.Optional(k): switch.switch_schema(PI18Switch)
            for k in SWITCHES
        },
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PI18_ID])
    for key, (cmd_on, cmd_off) in SWITCHES.items():
        if key in config:
            s = await switch.new_switch(config[key])
            cg.add(s.set_parent(parent))
            cg.add(s.set_command_on(cmd_on))
            cg.add(s.set_command_off(cmd_off))
