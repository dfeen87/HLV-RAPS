import random
from typing import Dict, Any

HLV_CONSTANTS = {
    "MAX_CURVATURE_THRESHOLD_RMAX": 1.0e-12,
    "MIN_ACCEPTABLE_A_T": 0.80,
    "MAX_TCC_COUPLING_J": 1.0e4,
}

def hlv_predict_At(current_state: Dict[str, Any], command_set: Dict[str, Any]) -> float:
    throttle = command_set.get("throttle_pct", 100.0) / 100.0
    baseline = 1.0 - (throttle * 0.1)

    valve = command_set.get("valve_adjust", 0.0)
    if valve < -0.04:
        return random.uniform(0.70, 0.85)

    return baseline + random.uniform(-0.02, 0.02)

def hlv_predict_J(current_state: Dict[str, Any], command_set: Dict[str, Any]) -> float:
    throttle = command_set.get("throttle_pct", 100.0)
    pressure = current_state.get("pressure_chamber_a", 0.0)

    if throttle > 99.0 and pressure < 2100.0:
        return HLV_CONSTANTS["MAX_TCC_COUPLING_J"] * 1.5

    return random.uniform(50.0, 500.0)
