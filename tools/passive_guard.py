from pathlib import Path
import subprocess

from SCons.Script import Exit, Import

Import("env")


def _define_value(cppdefines, name, default="0"):
    for item in cppdefines:
        if isinstance(item, tuple) and item and item[0] == name:
            return str(item[1])
        if isinstance(item, str):
            if item == name:
                return "1"
            prefix = f"{name}="
            if item.startswith(prefix):
                return item[len(prefix):]
    return default


def _truthy(value):
    return value not in ("", "0", "false", "False", "FALSE")


cppdefines = list(env.get("CPPDEFINES", []))
parsed_flags = env.ParseFlags(env.GetProjectOption("build_flags"))
cppdefines.extend(parsed_flags.get("CPPDEFINES", []))
env_name = env.subst("$PIOENV")
is_passive = env_name.endswith("_passive") or _truthy(
    _define_value(cppdefines, "BOARD_CSM_PROFILE_PASSIVE_PRODUCT")
)

if is_passive:
    def _check_passive_symbols(target, source, env):
        elf = Path(str(target[0]))
        nm = env.subst("$NM")
        if not nm or nm == "$NM":
            nm = "arm-none-eabi-nm"
        if not elf.exists():
            print(f"Passive guard failed: ELF not found: {elf}")
            Exit(1)
        try:
            output = subprocess.check_output(
                f'"{nm}" -C "{elf}"',
                shell=True,
                text=True,
                errors="ignore",
            )
        except Exception as exc:
            print(f"Passive guard failed: unable to inspect symbols: {exc}")
            Exit(1)

        forbidden = [
            "HostDownlinkParser",
            "handle_host_can_tx_request",
            "handle_host_heartbeat",
            "handle_host_control_session",
            "handle_host_clear_fault_lockout",
            "start_mcp2515_tx_audit",
            "service_mcp2515_tx_test",
            "service_builtin_can_tx_test",
            "NVIC_SystemReset",
            "MCP2515::setNormalMode",
        ]
        hits = [symbol for symbol in forbidden if symbol in output]
        if hits:
            print("Passive guard failed: forbidden active symbols linked:")
            for symbol in hits:
                print(f"  - {symbol}")
            Exit(1)

    env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", _check_passive_symbols)
