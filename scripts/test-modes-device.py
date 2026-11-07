"""Integration checks for the simulated ESP firmware; never use with live outputs."""
import argparse
import json
from pathlib import Path
import re
import time

import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="COM5")
    parser.add_argument("--timing", action="store_true", help="Also observe two real 50-second reversal phases")
    args = parser.parse_args()
    log = []
    port = serial.Serial(port=None, baudrate=115200, timeout=0.2)
    port.port = args.port
    port.dtr = False
    port.rts = False
    rooms = ["wohnzimmer", "schlafzimmer", "bad", "buero"]

    def collect(seconds):
        result = []
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            value = port.readline().decode("utf-8", "replace").strip()
            if value:
                result.append(value)
                log.append(value)
        return result

    def command(value, wait=0.5):
        port.write(value.encode("utf-8") + b"\n")
        return collect(wait)

    def require(condition, message):
        if not condition:
            raise RuntimeError(message)

    def snapshot():
        lines = command("status", 1)
        state_lines = [s for s in lines if s.startswith("STATE ")]
        require(bool(state_lines), "Firmware did not report a room state")
        state = dict(re.findall(r"([A-Z_]+)=([^ ]+)", state_lines[-1]))
        require(state.get("SIMULATION") == "1", "Refusing to test physical fan outputs")
        reported = [s for s in lines if s.startswith("ROOM=")]
        require(len(reported) >= 4, "Missing room state reports")
        state["directions"] = [re.search(r" DIRECTION=(\w+)", s).group(1) for s in reported[-4:]]
        require(state["directions"].count("Zuluft") == 2, "Unbalanced room assignment")
        require(state["directions"].count("Abluft") == 2, "Unbalanced room assignment")
        return state

    def supply(mask):
        selected = [rooms[i] for i in range(4) if mask & (1 << i)]
        require(len(selected) == 2, "Test requires a balanced layout")
        command("supply " + " ".join(selected))

    def reset():
        port.rts = True
        time.sleep(0.15)
        port.rts = False
        return collect(10)

    initial = None
    results = {}
    try:
        port.open()
        collect(1)
        initial = snapshot()
        results["initial_state"] = {k: initial[k] for k in ("STAGE", "MODE", "SUPPLY_MASK")}
        for stage in range(4):
            command(f"stage {stage}")
            require(snapshot()["STAGE"] == str(stage), f"Stage {stage} failed")
        results["four_stages"] = True
        command("mode durchzug")
        require(snapshot()["MODE"] == "cross", "Cross mode failed")
        for mask in (3, 5, 9, 6, 10, 12):
            supply(mask)
            state = snapshot()
            require(int(state["SUPPLY_MASK"]) == mask, "Layout was not applied")
            expected = ["Zuluft" if mask & (1 << i) else "Abluft" for i in range(4)]
            require(state["directions"] == expected, "Room directions differ from layout")
        results["all_six_layouts"] = True
        before = snapshot()
        require(any("ERROR:" in s for s in command("supply bad bad")), "Duplicate rooms accepted")
        require(snapshot()["SUPPLY_MASK"] == before["SUPPLY_MASK"], "Invalid layout changed configuration")
        require(any("ERROR:" in s for s in command("stage 4")), "Invalid stage accepted")
        results["invalid_requests_rejected"] = True
        if args.timing:
            supply(3)
            command("stage Stark")
            command("mode durchzug")
            command("mode wechsel")
            started = time.monotonic()
            baseline = snapshot()
            print("Observing two real 50-second reversal phases...", flush=True)
            collect(max(0, 51.5 - (time.monotonic() - started)))
            first = snapshot()
            require(first["REVERSED"] == "1", "First reversal did not happen")
            require(all(a != b for a, b in zip(baseline["directions"], first["directions"])), "Not all rooms reversed")
            print("First phase: all four directions reversed, still 2 supply / 2 exhaust", flush=True)
            collect(max(0, 101.5 - (time.monotonic() - started)))
            second = snapshot()
            require(second["REVERSED"] == "0", "Second reversal did not happen")
            require(second["directions"] == baseline["directions"], "Baseline was not restored")
            results["two_real_direction_reversals"] = True
        command("mode durchzug")
        supply(12)
        command("stage Stark")
        boot = reset()
        after = snapshot()
        require(after["STAGE"] == "0", "Device resumed a stored active stage at boot")
        require(after["MODE"] == "cross" and after["SUPPLY_MASK"] == "12", "Configuration was not persistent")
        require(any("MATTER_CONTROLS_READY" in s for s in boot), "Matter controls did not start")
        status = command("status", 1)
        require(any("WIFI=connected MATTER=commissioned" in s for s in status), "Existing Matter fabric was lost")
        results["configuration_and_fabric_persisted"] = True
        results["boot_stays_off"] = True
    finally:
        if port.is_open:
            if initial:
                try:
                    command("stage Aus")
                    supply(int(initial["SUPPLY_MASK"]))
                    command("mode wechsel" if initial["MODE"] == "alternating" else "mode durchzug")
                    results["final_state"] = snapshot()
                except Exception:
                    print("Cleanup incomplete: verify that the simulated stage is Aus", flush=True)
            port.close()
        output = Path(__file__).resolve().parents[1] / ".tools"
        output.mkdir(exist_ok=True)
        (output / "device-modes-test.log").write_text("\n".join(log), encoding="utf-8")
        (output / "device-modes-test.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    require(not any(any(t in s.lower() for t in ("guru meditation", "backtrace:", "abort()")) for s in log), "Firmware panic detected")
    require(not any("Matter state publication failed" in s or "persistence failed" in s for s in log), "State publication/persistence failed")
    print(json.dumps(results, indent=2), flush=True)
    print("PASS: simulated device controls", flush=True)


if __name__ == "__main__":
    main()
