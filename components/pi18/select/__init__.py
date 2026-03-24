import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from .. import PI18Component, CONF_PI18_ID, pi18_ns

DEPENDENCIES = ["pi18"]

PI18Select = pi18_ns.class_("PI18Select", select.Select)

# ── Option → PI18 command mappings ────────────────────────────────────────────
# For commands with unit number: template uses %u (filled per parallel unit)

OUTPUT_SOURCE_PRIORITY_OPTIONS = {
    "Utility-Solar-Battery": "POP0",
    "Solar-Utility-Battery": "POP1",
    "Solar-Battery-Utility": "POP2",
}

# PCPm,n: m=parallel unit (multi_unit), n=priority
CHARGER_SOURCE_PRIORITY_OPTIONS = {
    "Utility-first":  "PCP%u,0",
    "Solar-first":    "PCP%u,1",
    "Solar-Utility":  "PCP%u,2",
    "Solar-only":     "PCP%u,3",
}

SOLAR_POWER_PRIORITY_OPTIONS = {
    "Battery-Load": "PSP0",
    "Load-Battery": "PSP1",
}

BATTERY_TYPE_OPTIONS = {
    "AGM":            "PBT0",
    "Flooded":        "PBT1",
    "User-defined":   "PBT2",
    "Pylontech":      "PBT3",
    "WECO":           "PBT4",
    "Soltaro":        "PBT5",
    "LIB-compatible": "PBT6",
}

INPUT_VOLTAGE_RANGE_OPTIONS = {
    "Appliance": "PGR0",
    "UPS":       "PGR1",
}

# Vnnnn: nnnn = voltage in 0.1V (e.g. 2200 = 220.0 V)
AC_OUTPUT_VOLTAGE_OPTIONS = {
    "208V": "V2080",
    "220V": "V2200",
    "230V": "V2300",
    "240V": "V2400",
}

# MCHGCm,nnn: m=parallel unit (multi_unit), nnn=current
MAX_CHARGING_CURRENT_OPTIONS = {
    "10A":  "MCHGC%u,010",
    "20A":  "MCHGC%u,020",
    "30A":  "MCHGC%u,030",
    "40A":  "MCHGC%u,040",
    "50A":  "MCHGC%u,050",
    "60A":  "MCHGC%u,060",
    "70A":  "MCHGC%u,070",
    "80A":  "MCHGC%u,080",
    "90A":  "MCHGC%u,090",
    "100A": "MCHGC%u,100",
    "110A": "MCHGC%u,110",
    "120A": "MCHGC%u,120",
    "130A": "MCHGC%u,130",
    "140A": "MCHGC%u,140",
}

# MUCHGCm,nnn: m=parallel unit (multi_unit), nnn=current
MAX_AC_CHARGING_CURRENT_OPTIONS = {
    "2A":   "MUCHGC%u,002",
    "10A":  "MUCHGC%u,010",
    "15A":  "MUCHGC%u,015",
    "20A":  "MUCHGC%u,020",
    "25A":  "MUCHGC%u,025",
    "30A":  "MUCHGC%u,030",
    "40A":  "MUCHGC%u,040",
    "50A":  "MUCHGC%u,050",
    "60A":  "MUCHGC%u,060",
    "70A":  "MUCHGC%u,070",
    "80A":  "MUCHGC%u,080",
    "90A":  "MUCHGC%u,090",
    "100A": "MUCHGC%u,100",
    "110A": "MUCHGC%u,110",
    "120A": "MUCHGC%u,120",
    "130A": "MUCHGC%u,130",
    "140A": "MUCHGC%u,140",
}

OUTPUT_MODE_OPTIONS = {
    "Single":       "POPM0,0",
    "Parallel":     "POPM0,1",
    "Phase-1 of 3": "POPM0,2",
    "Phase-2 of 3": "POPM0,3",
    "Phase-3 of 3": "POPM0,4",
}

# ── (key, options_dict, multi_unit) ───────────────────────────────────────────
SELECT_ENTRIES = {
    "output_source_priority_select":   (OUTPUT_SOURCE_PRIORITY_OPTIONS,   False),
    "charger_source_priority_select":  (CHARGER_SOURCE_PRIORITY_OPTIONS,  True),
    "solar_power_priority_select":     (SOLAR_POWER_PRIORITY_OPTIONS,     False),
    "battery_type_select":             (BATTERY_TYPE_OPTIONS,             False),
    "input_voltage_range_select":      (INPUT_VOLTAGE_RANGE_OPTIONS,      False),
    "ac_output_voltage_select":        (AC_OUTPUT_VOLTAGE_OPTIONS,        False),
    "max_charging_current_select":     (MAX_CHARGING_CURRENT_OPTIONS,     True),
    "max_ac_charging_current_select":  (MAX_AC_CHARGING_CURRENT_OPTIONS,  True),
    "output_mode_select":              (OUTPUT_MODE_OPTIONS,              False),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PI18_ID): cv.use_id(PI18Component),
        **{
            cv.Optional(key): select.select_schema(PI18Select)
            for key in SELECT_ENTRIES
        },
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PI18_ID])
    for key, (options, multi_unit) in SELECT_ENTRIES.items():
        if key not in config:
            continue
        var = await select.new_select(config[key], options=list(options.keys()))
        cg.add(var.set_parent(parent))
        cg.add(var.set_multi_unit(multi_unit))
        for option, cmd in options.items():
            cg.add(var.add_mapping(option, cmd))
