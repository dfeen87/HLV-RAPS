import time
import json
import random
import threading
import secrets
from typing import Dict, Any, List

# =====================================================
# Configuration Constants (Reference Mirrors RAPSConfig)
# =====================================================

DECISION_HORIZON_MS = 300
WATCHDOG_MS = 120
MIN_CONFIDENCE_FOR_EXECUTION = 0.85
MAX_ACCEPTABLE_UNCERTAINTY = 0.25

# =====================================================
# Global Stores / Stubs
# =====================================================

ITLSTORAGE: Dict[str, Dict[str, Any]] = {}
ROLLBACKSTORE: Dict[str, Dict[str, Any]] = {}
APPLIEDTX: Dict[str, Dict[str, Any]] = {}

FLUSHERSHUTDOWN = threading.Event()
STOPSUPERVISOR = threading.Event()

# =====================================================
# Utility Functions
# =====================================================

def now_ms() -> int:
    return int(time.time() * 1000)

def metric_emit(name: str, value: float, tags: Dict[str, Any] | None = None):
    pass  # telemetry stub

def itl_commit(entry: Dict[str, Any]) -> str:
    entry_id = f"itl_{len(ITLSTORAGE)}"
    ITLSTORAGE[entry_id] = entry
    return entry_id

def stable_json_hash(obj: Any) -> str:
    return str(hash(json.dumps(obj, sort_keys=True)))

def fast_state_snapshot() -> Dict[str, Any]:
    return {"pressure": 2400.0, "temp": 860.0}

def uncertaintymetric(cov: Dict[str, Any]) -> float:
    return float(cov.get("norm_sigma", 0.0))

# =====================================================
# PDT / APE Reference Implementations
# =====================================================

class PredictionResult:
    def __init__(self, status, mean_state, cov, confidence, model_version, pid, evidence):
        self.status = status
        self.mean_state = mean_state
        self.cov = cov
        self.confidence = confidence
        self.model_version = model_version
        self.id = pid
        self.evidence = evidence

class Policy:
    def __init__(self, pid, command_set, cost, preconditions, rollback):
        self.id = pid
        self.command_set = command_set
        self.cost = cost
        self.preconditions = preconditions
        self.rollback = rollback

def pdt_infer(snapshot: Dict[str, Any], horizon_ms: int) -> PredictionResult:
    if random.random() < 0.15:
        pr = PredictionResult(
            status="PREDICTED_ESE",
            mean_state={"pressure": 2650.0, "temp": 950.0},
            cov={"norm_sigma": 0.15},
            confidence=0.92,
            model_version="v2.1_stochastic",
            pid=f"ESE_{now_ms()}",
            evidence={"sensor_code": "PC_A", "delta": 150}
        )
    else:
        pr = PredictionResult(
            status="NOMINAL",
            mean_state={"pressure": 2400.0, "temp": 860.0},
            cov={"norm_sigma": 0.05},
            confidence=0.99,
            model_version="v2.1_stochastic",
            pid=f"NOM_{now_ms()}",
            evidence={}
        )
    metric_emit("pdt.infer_latency_ms", random.uniform(1.0, 10.0))
    return pr

def ape_generate_candidates(evidence, mean_state) -> List[Policy]:
    return [
        Policy("POL_THROTTLE_ADJUST_001", {"throttle_pct": 98.5}, 1.5, {"min_thrust": 80.0}, {"throttle_pct": 100.0}),
        Policy("POL_VALVE_TRIM_002", {"valve_adjust": -0.08}, 1.2, {"min_thrust": 75.0}, {"valve_adjust": 0.0}),
        Policy("POL_REDUCE_THRUST_003", {"throttle_pct": 96.0}, 0.9, {"min_thrust": 70.0}, {"throttle_pct": 100.0}),
    ]

def ape_get_best_policy(evidence, mean_state) -> Policy:
    candidates = ape_generate_candidates(evidence, mean_state)
    candidates.sort(key=lambda p: p.cost)
    return candidates[0]

# =====================================================
# Actuation / Rollback
# =====================================================

def execute_command_on_actuators(command_set, tx_id, timeout_ms=WATCHDOG_MS) -> bool:
    if tx_id in APPLIEDTX:
        metric_emit("actuator.idempotent_shortcircuit", 1)
        return True

    simulated_latency = random.uniform(0.003, 0.02)
    if simulated_latency * 1000 > timeout_ms:
        return False

    time.sleep(simulated_latency)
    APPLIEDTX[tx_id] = command_set.copy()
    return True

def trigger_fallback_safe_state(reason="safety_fallback"):
    itl_commit({"type": "fallback_triggered", "reason": reason, "timestamp_ms": now_ms()})
    metric_emit("fallback.triggered", 1, {"reason": reason})

def execute_rollback(policy_id: str) -> bool:
    rollback = ROLLBACKSTORE.get(policy_id)
    if not rollback:
        trigger_fallback_safe_state("rollback_missing")
        return False

    tx_id = secrets.token_hex(12)
    success = execute_command_on_actuators(rollback, tx_id)
    if not success:
        trigger_fallback_safe_state("rollback_execution_failure")
        return False

    itl_commit({"type": "rollback_commit", "policy_id": policy_id, "tx_id": tx_id})
    return True

# =====================================================
# Governance Cycle
# =====================================================

def raps_governance_cycle_once():
    loop_ts = now_ms()
    fast_state = fast_state_snapshot()
    snap_id = itl_commit({"type": "state_snapshot", "state": fast_state})

    pred = pdt_infer(fast_state, DECISION_HORIZON_MS)
    itl_commit({"type": "prediction_commit", "prediction_id": pred.id})

    if pred.status == "PREDICTED_ESE" and pred.confidence >= MIN_CONFIDENCE_FOR_EXECUTION:
        policy = ape_get_best_policy(pred.evidence, pred.mean_state)
        tx_id = secrets.token_hex(12)
        success = execute_command_on_actuators(policy.command_set, tx_id)
        if not success:
            execute_rollback(policy.id)
        else:
            ROLLBACKSTORE[policy.id] = policy.rollback.copy()
            itl_commit({"type": "command_commit", "policy_id": policy.id})

# =====================================================
# Demo Entry Point
# =====================================================

def demo_main(run_seconds=3.0, interval_ms=100):
    start = time.time()
    while time.time() - start < run_seconds:
        raps_governance_cycle_once()
        time.sleep(interval_ms / 1000.0)

if __name__ == "__main__":
    demo_main()
