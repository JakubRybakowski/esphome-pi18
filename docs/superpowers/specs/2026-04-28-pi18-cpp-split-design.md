# Split `pi18.cpp` into focused translation units

**Date:** 2026-04-28
**Status:** Approved (awaiting user review)
**Scope:** Refactor only — no behaviour change.

## Problem

`components/pi18/pi18.cpp` has grown to ~1100 lines spanning unrelated concerns:
lifecycle, UART protocol, response decoders, HA entity glue, paired-voltage
helpers, and CRC. A single large translation unit hurts readability, slows
compilation, and makes targeted edits riskier.

## Goal

Split `pi18.cpp` into four focused `.cpp` files (~200–450 LOC each) without
changing public API, runtime behaviour, or the YAML interface. Pure
maintainability refactor.

## File Boundaries

```
components/pi18/
├── __init__.py              (no change)
├── pi18.h                   (no change — already declares everything)
├── pi18.cpp                 ~200 LOC — Lifecycle
├── pi18_protocol.cpp        ~200 LOC — UART low-level
├── pi18_decoders.cpp        ~450 LOC — Response parsing
└── pi18_helpers.cpp         ~250 LOC — HA entity glue + paired voltage
```

### `pi18.cpp` — Lifecycle

- `PI18Component::setup()`
- `PI18Component::loop()`
- `PI18Component::update()`
- `PI18Component::build_poll_commands_()`
- `PI18Component::send_set_date_time()`

### `pi18_protocol.cpp` — UART low-level

- `pi18_crc()` (free function)
- `PI18Component::build_frame_()`
- `PI18Component::send_frame_()`
- `PI18Component::send_set_command()`
- `PI18Component::dispatch_response_()`
- `PI18Component::split_csv_()`
- `PI18Component::parse_float_()`
- `PI18Component::parse_int_()`

### `pi18_decoders.cpp` — Parsing

- `decode_gs_`, `decode_piri_`, `decode_fws_`, `decode_mod_`, `decode_flag_`
- `decode_pgs_`, `decode_et_`, `decode_ey_`, `decode_em_`, `decode_ed_`
- `decode_acct_`, `decode_aclt_`, `decode_t_`
- `fault_code_str_`

### `pi18_helpers.cpp` — HA entity glue

- `PI18Switch::write_state`
- `PI18Button::press_action`, `PI18SetDateTimeButton::press_action`
- `PI18Select::control`, `PI18VoltageSelect::control`
- `PI18Number::control` (under `#ifdef USE_NUMBER`)
- `PI18Component::handle_bulk_voltage` / `_float_` / `_recharge_` / `_redischarge_`

## Common Includes

Each `.cpp`:

```cpp
#include "pi18.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pi18 {

static const char *const TAG = "pi18";

// ... implementations ...

}  // namespace pi18
}  // namespace esphome
```

`pi18.h` already declares everything — no header changes needed.

## Migration Plan

Atomic, reversible steps with build validation between each.

| Step | Action | Risk |
|------|--------|------|
| A | Create empty `pi18_protocol.cpp`, `pi18_decoders.cpp`, `pi18_helpers.cpp` with header skeleton. | Trivial |
| B | Move HA entity glue + paired-voltage handlers to `pi18_helpers.cpp`. | Low — bounded scope |
| C | Move CRC, frame builder, dispatch, parser helpers to `pi18_protocol.cpp`. | Low |
| D | Move all `decode_*` and `fault_code_str_` to `pi18_decoders.cpp`. | Low — declarations in header |
| E | `pi18.cpp` shrinks to lifecycle only. | None — leftover after D |

After each step:

```bash
esphome config easun.yaml      # YAML still valid
esphome compile easun.yaml     # full build clean
```

Each step is its own git commit, so `git revert <hash>` rolls back a single
step if regression appears post-flash.

## Build System

ESPHome's PlatformIO build picks up every `.cpp` in the component directory
automatically. **No `__init__.py` changes**, no CMake adjustments. Verified
pattern in mainline ESPHome components (e.g. `bme680_bsec/` has 4-5 `.cpp`
files).

## Validation

### Static

1. `esphome config easun.yaml` → `Configuration is valid!`
2. `esphome compile easun.yaml` → no errors, no new warnings.

### Runtime (after OTA)

1. LIVE round (PGS0/1/2) refreshes every ~1 s as before.
2. BG slot fires MOD/FWS/FLAG/PIRI on cadence.
3. One-shots (PI/ID/VFW/ACCT/ACLT) fire once at boot.
4. SET commands work: pick a Select option → command queued → ACK → next PIRI
   reflects new value.
5. No new CRC mismatches or "still waiting" log spam.

### Regression check (manual)

- All sensors publish at least once after boot.
- `Total AC Output Active Power` updates via PGS0.
- L1/L2/L3 sensors update from PGS0/1/2 respectively.
- Voltage selects (bulk/float/recharge/redischarge/cutoff) sync from PIRI.

## Out of Scope

- No new sensors / commands / features.
- No bug fixes (e.g., 3-phase per-phase quirk, CRC retry, set verification).
- No changes to `pi18.h` (declarations stay).
- No tests added (separate concern — covered by P2 in earlier brainstorm).

## Risks & Mitigations

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Multiple definition link errors (function moved but copy left in old file) | Medium | Atomic per-step `git diff` review before commit |
| Forgotten `#include` in new file | Low | `pi18.h` is the only needed include; verify build after each step |
| Static `TAG` collision across translation units | Low | Each file has its own static (per ESPHome convention) — no collision since static linkage |
| Hidden helper coupling (e.g., decoder uses internal state set by another decoder) | Low | All shared state is on `PI18Component` member; declarations in header |

## Success Criteria

1. After step E:
   - `pi18.cpp` ≤ 250 LOC
   - All four files compile clean
   - Flash + 5 minute soak shows identical behaviour to pre-split build
2. `git log --oneline -5` shows 5 well-scoped commits (A through E).
3. No public API change — no adjustment needed in user's `easun.yaml`.
