# ________________________________________
# RAPS GOVERNOR WITH HLV (REFERENCE)
# ________________________________________

import time
import threading
import random
import secrets
from typing import Dict, Any, Optional

from itl_reference import (
    itl_commit,
    start_itl_flusher,
    shutdown
)

from hlv_math_stubs import (
    hlv_predict_At,
    hlv_predict_J,
    HLV_CONSTANTS
)

# --- CONFIG / CONSTANTS ---

DECISION_HORIZON_MS = 300
WATCHDOG_MS = 120
MAX_ACCEPTABLE_UNCERTAINTY = 0.25
MIN_CONFIDENCE_FOR_EXECUTION = 0.85

STOPSUPERVISOR = threading.Event()

# --- UTILITIES ---

def now_ms() -> int:
    return int(time.monotonic() * 1000)

def metric_emit(name: str, value: Any, tags: Dict[str, str] = None):
    print(f"[METRIC] {name}={value} tags={tags}")

# --- DATA STRUCTURES ---

class PredictionResult:
    def __init__(
        self,
        status: str,
        mean_state: Dict[str, Any],
        cov: Dict[str, Any],
        model_version: str,
        confidence: float,
        pid: str,
        evidence: Dict[str, Any],
    ):
        self.status = status
        self.mean_state = mean_state
        self.cov = cov
        self.model_version = model_version
        self.confidence = confidence
        self.id = pid
        self.evidence = evidence

class Policy:
    def __init__(
        self,
        pid: str,
        command_set: Dict[str, Any],
        cost: float,
        preconditions: Dict[str, Any],
        rollback: Dict[str, Any],
    ):
        self.id = pid
        self.command_set = command_set
        self.cost = cost
        self.preconditions = preconditions
        self.rollback = rollback

# --- PLATFORM STUBS ---

def fast_state_snapshot() -> Dict[str, Any]:
    return {
        "pressure_chamber_a": random.uniform(2000.0, 2500.0),
        "temp_nozzle_b": random.uniform(800.0, 900.0),
        "timestamp_ms": now_ms(),
    }

def pdt_infer(snapshot: Dict[str, Any], horizon_ms: int) -> PredictionResult:
    if random.random() < 0.15:
        return PredictionResult(
            status="PREDICTED_ESE",
            mean_state={"pressure": 2650.0, "temp": 950.0},
            cov={"norm_sigma": 0.15},
            model_version="v2.1_stochastic",
            confidence=0.92,
            pid=f"ESE_{now_ms()}",
            evidence={"sensor_code": "PC_A"},
        )

    return PredictionResult(
        status="NOMINAL",
        mean_state={"pressure": 2400.0, "temp": 860.0},
        cov={"norm_sigma": 0.05},
        model_version="v2.1_stochastic",
        confidence=0.99,
        pid=f"NOM_{now_ms()}",
        evidence={},
    )

def uncertaintymetric(cov: Dict[str, Any]) -> float:
    return float(cov.get("norm_sigma", 0.0))

# --- SAFETY MONITOR (HLV-AWARE) ---

def safety_monitor_validate(
    policy_preflight: Dict[str, Any],
    current_state: Dict[str, Any]
) -> bool:

    cmd = policy_preflight.get("command_set") or {}

    # --- HLV Pillar 2: A(t) ---
    predicted_A_t = hlv_predict_At(current_state, cmd)
    if predicted_A_t < HLV_CONSTANTS["MIN_ACCEPTABLE_A_T"]:
        itl_commit({
            "type": "safety_check_fail_hlv",
            "reason": "Predicted A(t) violates stability window",
            "predicted_A_t": predicted_A_t,
            "timestamp_ms": now_ms(),
        })
        return False

    # --- HLV Pillar 5: TCC Coupling ---
    predicted_J = hlv_predict_J(current_state, cmd)
    if predicted_J > HLV_CONSTANTS["MAX_TCC_COUPLING_J"]:
        itl_commit({
            "type": "safety_check_fail_hlv",
            "reason": "Predicted J violates TCC coupling limit",
            "predicted_J": predicted_J,
            "timestamp_ms": now_ms(),
        })
        return False

    return True

# --- GOVERNANCE LOOP ---

def raps_governance_cycle_once():
    fast_state = fast_state_snapshot()
    snap_id = itl_commit({
        "type": "state_snapshot",
        "state": fast_state,
        "timestamp_ms": now_ms(),
    })

    pred = pdt_infer(fast_state, DECISION_HORIZON_MS)

    itl_commit({
        "type": "prediction_commit",
        "prediction_id": pred.id,
        "confidence": pred.confidence,
        "uncertainty": uncertaintymetric(pred.cov),
        "ref_snapshot": snap_id,
        "timestamp_ms": now_ms(),
    })

    if pred.status != "PREDICTED_ESE":
        return

    if pred.confidence < MIN_CONFIDENCE_FOR_EXECUTION:
        return

    policy = Policy(
        pid="POL_REDUCE_THRUST",
        command_set={"throttle_pct": 96.0},
        cost=0.9,
        preconditions={},
        rollback={"throttle_pct": 100.0},
    )

    preflight = {
        "policy_id": policy.id,
        "command_set": policy.command_set,
        "rollback": policy.rollback,
        "timestamp_ms": now_ms(),
    }

    if not safety_monitor_validate(preflight, fast_state):
        return

    itl_commit({
        "type": "command_commit",
        "policy_id": policy.id,
        "timestamp_ms": now_ms(),
    })

# --- DEMO ENTRYPOINT ---

def demo_main(run_seconds: float = 3.0):
    start_itl_flusher()
    start = time.time()

    while time.time() - start < run_seconds:
        raps_governance_cycle_once()
        time.sleep(0.1)

    shutdown()

if __name__ == "__main__":
    demo_main()
