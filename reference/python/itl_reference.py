import threading
import queue
import hashlib
import json
from typing import Dict, Any

ITL_QUEUE_MAXSIZE = 8192
MERKLE_BATCH_SIZE = 32

ITLQUEUE = queue.Queue(maxsize=ITL_QUEUE_MAXSIZE)
ITLSTORAGE: Dict[str, Dict] = {}
MERKLEBUFFER = []

ITL_LOCK = threading.Lock()
MERKLE_LOCK = threading.Lock()
FLUSHERSHUTDOWN = threading.Event()

def now_ms():
    import time
    return int(time.monotonic() * 1000)

def stable_json_hash(obj: Any) -> str:
    return hashlib.sha256(
        json.dumps(obj, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()

def compute_merkle_root(ids: list) -> str:
    nodes = ids[:]
    while len(nodes) > 1:
        it = []
        for i in range(0, len(nodes), 2):
            a = nodes[i]
            b = nodes[i + 1] if i + 1 < len(nodes) else nodes[i]
            it.append(hashlib.sha256((a + b).encode()).hexdigest())
        nodes = it
    return nodes[0] if nodes else ""

def anchor_merkle_root(root: str, batch_ids: list):
    itl_commit({
        "type": "merkle_anchor",
        "merkle_root": root,
        "batch_ids": batch_ids,
        "timestamp_ms": now_ms(),
    })

def itl_background_flusher_loop():
    global MERKLEBUFFER

    while not FLUSHERSHUTDOWN.is_set():
        try:
            entry = ITLQUEUE.get(timeout=0.1)
        except queue.Empty:
            continue

        entry_id = entry["id"]
        payload = entry["payload"]

        with ITL_LOCK:
            ITLSTORAGE[entry_id] = payload

        with MERKLE_LOCK:
            MERKLEBUFFER.append(entry_id)
            if len(MERKLEBUFFER) >= MERKLE_BATCH_SIZE:
                batch = MERKLEBUFFER[:MERKLE_BATCH_SIZE]
                MERKLEBUFFER = MERKLEBUFFER[MERKLE_BATCH_SIZE:]
                root = compute_merkle_root(batch)
                anchor_merkle_root(root, batch)

        ITLQUEUE.task_done()

def start_itl_flusher():
    t = threading.Thread(target=itl_background_flusher_loop, daemon=True)
    t.start()
    return t

def shutdown():
    FLUSHERSHUTDOWN.set()
    with MERKLE_LOCK:
        if MERKLEBUFFER:
            root = compute_merkle_root(MERKLEBUFFER)
            anchor_merkle_root(root, MERKLEBUFFER)
            MERKLEBUFFER.clear()

def itl_commit(payload: Dict[str, Any]) -> str:
    entry_id = stable_json_hash(payload)
    ITLQUEUE.put({"id": entry_id, "payload": payload})
    return entry_id
