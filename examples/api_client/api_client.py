#!/usr/bin/env python3
"""
HLV-RAPS REST API Client Example

Demonstrates how to query the HLV-RAPS Flight Middleware REST API
for observability and telemetry data.

Usage:
    python api_client.py [host] [port]
    
Example:
    python api_client.py 192.168.1.100 8080
"""

import sys
import json
import time
from typing import Dict, Any, Optional
import urllib.request
import urllib.error


class RapsApiClient:
    """Client for HLV-RAPS REST API"""
    
    def __init__(self, host: str = "localhost", port: int = 8080):
        """Initialize API client
        
        Args:
            host: API server hostname or IP address
            port: API server port
        """
        self.base_url = f"http://{host}:{port}"
        
    def _get(self, endpoint: str) -> Dict[str, Any]:
        """Make GET request to API endpoint
        
        Args:
            endpoint: API endpoint path (e.g., '/health')
            
        Returns:
            Parsed JSON response
            
        Raises:
            urllib.error.URLError: If request fails
        """
        url = f"{self.base_url}{endpoint}"
        
        try:
            with urllib.request.urlopen(url, timeout=5) as response:
                data = response.read()
                return json.loads(data)
        except urllib.error.HTTPError as e:
            error_body = e.read().decode('utf-8')
            try:
                error_data = json.loads(error_body)
                error_msg = error_data.get('error', str(e))
            except json.JSONDecodeError:
                error_msg = str(e)
            raise Exception(f"HTTP {e.code}: {error_msg}")
    
    def health(self) -> Dict[str, Any]:
        """Get health status
        
        Returns:
            Health check response
        """
        return self._get("/health")
    
    def get_state(self) -> Dict[str, Any]:
        """Get current state (Ψ and Φ)
        
        Returns:
            Current physical and informational state
        """
        return self._get("/api/state")
    
    def get_pdt(self) -> Dict[str, Any]:
        """Get PDT predictions
        
        Returns:
            Predictive Digital Twin predictions with confidence/uncertainty
        """
        return self._get("/api/pdt")
    
    def get_dsm(self) -> Dict[str, Any]:
        """Get DSM safety status
        
        Returns:
            Deterministic Safety Monitor status and gates
        """
        return self._get("/api/dsm")
    
    def get_supervisor(self) -> Dict[str, Any]:
        """Get supervisor state
        
        Returns:
            Redundant supervisor state (A/B, failover, mismatch)
        """
        return self._get("/api/supervisor")
    
    def get_rollback(self) -> Dict[str, Any]:
        """Get rollback status
        
        Returns:
            Rollback status and last rollback event
        """
        return self._get("/api/rollback")
    
    def get_itl(self) -> Dict[str, Any]:
        """Get ITL telemetry entries
        
        Returns:
            Latest N telemetry entries from Immutable Telemetry Ledger
        """
        return self._get("/api/itl")


def print_separator(title: str = ""):
    """Print a section separator"""
    if title:
        print(f"\n{'='*60}")
        print(f"  {title}")
        print(f"{'='*60}")
    else:
        print(f"{'='*60}")


def print_health(client: RapsApiClient):
    """Print health check information"""
    print_separator("Health Check")
    
    try:
        health = client.health()
        print(f"Status:             {health.get('status', 'unknown')}")
        print(f"Service:            {health.get('service', 'unknown')}")
        print(f"API Version:        {health.get('api_version', 'unknown')}")
        print(f"Observability Only: {health.get('observability_only', False)}")
    except Exception as e:
        print(f"Error: {e}")


def print_state(client: RapsApiClient):
    """Print current state information"""
    print_separator("Current State (Ψ and Φ)")
    
    try:
        state = client.get_state()
        
        if not state.get('valid', False):
            print("⚠ Warning: State snapshot not valid")
        
        print(f"Timestamp: {state.get('timestamp_ms', 0)} ms")
        
        # Physical state
        physics = state.get('physics_state', {})
        print("\nPhysical State (Ψ):")
        pos = physics.get('position_m', [0, 0, 0])
        vel = physics.get('velocity_m_s', [0, 0, 0])
        print(f"  Position (m):     [{pos[0]:12.2f}, {pos[1]:12.2f}, {pos[2]:12.2f}]")
        print(f"  Velocity (m/s):   [{vel[0]:12.2f}, {vel[1]:12.2f}, {vel[2]:12.2f}]")
        print(f"  Mass (kg):        {physics.get('mass_kg', 0):12.2f}")
        
        # Spacetime state (if available)
        if 'spacetime_state' in state:
            st = state['spacetime_state']
            print("\nInformational State (Φ):")
            print(f"  Warp Field:               {st.get('warp_field_strength', 0):.6f}")
            print(f"  Gravito Flux Bias:        {st.get('gravito_flux_bias', 0):.6f}")
            print(f"  Curvature Magnitude:      {st.get('spacetime_curvature_magnitude', 0):.9e}")
            print(f"  Time Dilation:            {st.get('time_dilation_factor', 1):.9f}")
            print(f"  Stability Index:          {st.get('spacetime_stability_index', 0):.6f}")
            print(f"  Control Authority:        {st.get('control_authority_remaining', 0):.6f}")
            print(f"  Antimatter Remaining:     {st.get('remaining_antimatter_kg', 0):.2f} kg")
            print(f"  Emergency Mode:           {st.get('emergency_mode_active', False)}")
    except Exception as e:
        print(f"Error: {e}")


def print_pdt(client: RapsApiClient):
    """Print PDT prediction information"""
    print_separator("PDT Predictions")
    
    try:
        pdt = client.get_pdt()
        
        if not pdt.get('valid', False):
            print("⚠ Warning: PDT snapshot not valid")
        
        status = pdt.get('status', 'UNKNOWN')
        confidence = pdt.get('confidence', 0)
        uncertainty = pdt.get('uncertainty', 0)
        
        print(f"Timestamp:   {pdt.get('timestamp_ms', 0)} ms")
        print(f"Status:      {status}")
        print(f"Confidence:  {confidence:.4f} ({confidence*100:.2f}%)")
        print(f"Uncertainty: {uncertainty:.4f} ({uncertainty*100:.2f}%)")
        
        # Status indicator
        if status == "NOMINAL":
            print("✓ Prediction nominal")
        elif status == "PREDICTED_ESE":
            print("⚠ WARNING: Predicted Excursion Safety Event (ESE)")
        
        # Predicted end state
        end_state = pdt.get('predicted_end_state', {})
        if end_state:
            print("\nPredicted End State:")
            pos = end_state.get('position_m', [0, 0, 0])
            vel = end_state.get('velocity_m_s', [0, 0, 0])
            print(f"  Position (m):     [{pos[0]:12.2f}, {pos[1]:12.2f}, {pos[2]:12.2f}]")
            print(f"  Velocity (m/s):   [{vel[0]:12.2f}, {vel[1]:12.2f}, {vel[2]:12.2f}]")
            print(f"  Mass (kg):        {end_state.get('mass_kg', 0):12.2f}")
    except Exception as e:
        print(f"Error: {e}")


def print_dsm(client: RapsApiClient):
    """Print DSM safety status"""
    print_separator("DSM Safety Status")
    
    try:
        dsm = client.get_dsm()
        
        if not dsm.get('valid', False):
            print("⚠ Warning: DSM snapshot not valid")
        
        action = dsm.get('current_action', 'UNKNOWN')
        safing = dsm.get('safing_sequence_active', False)
        
        print(f"Timestamp:        {dsm.get('timestamp_ms', 0)} ms")
        print(f"Current Action:   {action}")
        print(f"Safing Active:    {safing}")
        
        # Action indicator
        if action == "NONE":
            print("✓ No safety action required")
        elif action == "ROLLBACK":
            print("⚠ WARNING: ROLLBACK action triggered")
        elif action == "FULL_SHUTDOWN":
            print("🛑 CRITICAL: FULL SHUTDOWN triggered")
        
        print("\nSafety Metrics:")
        print(f"  Curvature (R_max):        {dsm.get('last_estimated_curvature', 0):.9e}")
        print(f"  Time Dilation:            {dsm.get('measured_time_dilation', 1):.9f}")
        print(f"  Oscillatory Prefactor:    {dsm.get('measured_oscillatory_prefactor', 0):.6f}")
        print(f"  TCC Coupling (J):         {dsm.get('measured_tcc_coupling', 0):.2f}")
        print(f"  Resonance Amplitude:      {dsm.get('current_resonance_amplitude', 0):.6f}")
        print(f"  Main Control Healthy:     {dsm.get('main_control_healthy', False)}")
    except Exception as e:
        print(f"Error: {e}")


def print_supervisor(client: RapsApiClient):
    """Print supervisor state"""
    print_separator("Supervisor State")
    
    try:
        supervisor = client.get_supervisor()
        
        if not supervisor.get('valid', False):
            print("⚠ Warning: Supervisor snapshot not valid")
        
        channel = supervisor.get('active_channel', 'UNKNOWN')
        mismatch = supervisor.get('has_prediction_mismatch', False)
        
        print(f"Timestamp:            {supervisor.get('timestamp_ms', 0)} ms")
        print(f"Active Channel:       {channel}")
        print(f"Prediction Mismatch:  {mismatch}")
        print(f"Last Sync:            {supervisor.get('last_sync_timestamp_ms', 0)} ms")
        print(f"Last Confidence:      {supervisor.get('last_prediction_confidence', 0):.4f}")
        print(f"Last Uncertainty:     {supervisor.get('last_prediction_uncertainty', 0):.4f}")
        
        if mismatch:
            print("\n⚠ WARNING: A/B prediction mismatch detected")
    except Exception as e:
        print(f"Error: {e}")


def print_rollback(client: RapsApiClient):
    """Print rollback status"""
    print_separator("Rollback Status")
    
    try:
        rollback = client.get_rollback()
        
        if not rollback.get('valid', False):
            print("⚠ Warning: Rollback snapshot not valid")
        
        has_plan = rollback.get('has_rollback_plan', False)
        count = rollback.get('rollback_count', 0)
        
        print(f"Timestamp:         {rollback.get('timestamp_ms', 0)} ms")
        print(f"Has Rollback Plan: {has_plan}")
        print(f"Rollback Count:    {count}")
        
        if has_plan and 'last_rollback_plan' in rollback:
            plan = rollback['last_rollback_plan']
            print("\nLast Rollback Plan:")
            print(f"  Policy ID:         {plan.get('policy_id', 'unknown')}")
            print(f"  Thrust (kN):       {plan.get('thrust_magnitude_kN', 0):.2f}")
            print(f"  Gimbal Theta:      {plan.get('gimbal_theta_rad', 0):.6f} rad")
            print(f"  Gimbal Phi:        {plan.get('gimbal_phi_rad', 0):.6f} rad")
    except Exception as e:
        print(f"Error: {e}")


def print_itl(client: RapsApiClient):
    """Print ITL telemetry entries"""
    print_separator("ITL Telemetry")
    
    try:
        itl = client.get_itl()
        
        if not itl.get('valid', False):
            print("⚠ Warning: ITL snapshot not valid")
        
        count = itl.get('count', 0)
        print(f"Timestamp:     {itl.get('timestamp_ms', 0)} ms")
        print(f"Entry Count:   {count}")
        
        entries = itl.get('entries', [])
        if entries:
            print("\nRecent Entries:")
            for i, entry in enumerate(entries[:10]):  # Show last 10
                print(f"\n  [{i+1}] Type {entry.get('type', 0)} @ {entry.get('timestamp_ms', 0)} ms")
                print(f"      {entry.get('summary', 'No summary')}")
    except Exception as e:
        print(f"Error: {e}")


def monitor_loop(client: RapsApiClient, interval: float = 2.0):
    """Continuous monitoring loop
    
    Args:
        client: API client instance
        interval: Update interval in seconds
    """
    print_separator("Monitoring Mode")
    print(f"Updating every {interval} seconds. Press Ctrl+C to stop.\n")
    
    try:
        while True:
            # Clear screen (works on most terminals)
            print("\033[2J\033[H", end='')
            
            print(f"HLV-RAPS Telemetry Monitor - {time.strftime('%Y-%m-%d %H:%M:%S')}")
            
            # Quick status
            try:
                pdt = client.get_pdt()
                dsm = client.get_dsm()
                supervisor = client.get_supervisor()
                
                print(f"\nPDT:        {pdt.get('status', 'UNKNOWN')} "
                      f"(confidence: {pdt.get('confidence', 0):.2f})")
                print(f"DSM:        {dsm.get('current_action', 'UNKNOWN')}")
                print(f"Supervisor: Channel {supervisor.get('active_channel', '?')}")
                
                if pdt.get('status') == 'PREDICTED_ESE':
                    print("\n⚠ WARNING: ESE predicted!")
                if dsm.get('current_action') != 'NONE':
                    print(f"\n⚠ WARNING: Safety action: {dsm.get('current_action')}")
                if supervisor.get('has_prediction_mismatch'):
                    print("\n⚠ WARNING: Prediction mismatch!")
                    
            except Exception as e:
                print(f"\nError: {e}")
            
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print("\n\nMonitoring stopped.")


def main():
    """Main entry point"""
    # Parse command line arguments
    host = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080
    
    print(f"Connecting to HLV-RAPS API at {host}:{port}")
    
    # Create client
    client = RapsApiClient(host, port)
    
    # Print all telemetry
    print_health(client)
    print_state(client)
    print_pdt(client)
    print_dsm(client)
    print_supervisor(client)
    print_rollback(client)
    print_itl(client)
    
    print_separator()
    print("\nFor continuous monitoring, uncomment the monitor_loop() call in main()")
    print("Example: monitor_loop(client, interval=2.0)")
    
    # Uncomment to enable continuous monitoring:
    # monitor_loop(client, interval=2.0)


if __name__ == "__main__":
    main()
