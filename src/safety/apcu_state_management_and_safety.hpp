// Diagnostic Metrics (Must be computed AFTER core state changes)
//
=====================================================================
====
current_propulsion_state_.field_coupling_stress = compute_field_coupling_stress();
current_propulsion_state_.spacetime_stability_index = compute_stability_index();
current_propulsion_state_.control_authority_remaining = compute_control_authority();
current_propulsion_state_.emergency_mode_active = emergency_mode_active_;
//
=====================================================================
====
// State Management & Safety
//
=====================================================================
====
current_propulsion_state_.timestamp_ms += elapsed_ms;
current_propulsion_state_.state_hash = calculate_state_hash(current_propulsion_state_);
// Save safe state periodically
if (is_state_safe_to_save(current_propulsion_state_) &&
(current_propulsion_state_.timestamp_ms - last_safe_state_timestamp_ms_ > 1000)) {
save_safe_state();
}
// Check for emergency conditions
if (!is_operational_state_safe() && !emergency_mode_active_) {
enter_emergency_mode();
}
// Emit comprehensive metrics
PlatformHAL::metric_emit("apcu.power_draw_GW",
current_propulsion_state_.power_draw_GW);
PlatformHAL::metric_emit("apcu.warp_strength",
current_propulsion_state_.warp_field_strength);
PlatformHAL::metric_emit("apcu.flux_bias", current_propulsion_state_.gravito_flux_bias);
PlatformHAL::metric_emit("apcu.curvature_mag",
current_propulsion_state_.spacetime_curvature_magnitude);
PlatformHAL::metric_emit("apcu.time_dilation_factor",
current_propulsion_state_.time_dilation_factor);
PlatformHAL::metric_emit("apcu.induced_gravity_g",
current_propulsion_state_.induced_gravity_g);
PlatformHAL::metric_emit("apcu.subspace_efficiency_pct",
current_propulsion_state_.subspace_efficiency_pct);
PlatformHAL::metric_emit("apcu.total_displacement_km",
(float)current_propulsion_state_.total_displacement_km);
PlatformHAL::metric_emit("apcu.antimatter_kg",
current_propulsion_state_.remaining_antimatter_kg);
PlatformHAL::metric_emit("apcu.quantum_fluid_L",
current_propulsion_state_.quantum_fluid_level);
PlatformHAL::metric_emit("apcu.coupling_stress",
current_propulsion_state_.field_coupling_stress);
PlatformHAL::metric_emit("apcu.stability_index",
current_propulsion_state_.spacetime_stability_index);
PlatformHAL::metric_emit("apcu.control_authority",
current_propulsion_state_.control_authority_remaining);
}
void AdvancedPropulsionControlUnit::save_safe_state() {
last_safe_state_ = current_propulsion_state_;
last_safe_state_timestamp_ms_ = current_propulsion_state_.timestamp_ms;
PlatformHAL::metric_emit("apcu.safe_state_saved", 1.0f);
}
bool AdvancedPropulsionControlUnit::is_state_safe_to_save(
const SpacetimeModulationState& state) const {
return state.remaining_antimatter_kg >
RAPSConfig::EMERGENCY_ANTIMATTER_RESERVE_KG &&
state.quantum_fluid_level >
RAPSConfig::EMERGENCY_QUANTUM_FLUID_LITERS &&
state.field_coupling_stress <
RAPSConfig::CRITICAL_FIELD_COUPLING_THRESHOLD &&
state.spacetime_stability_index > 0.6f;
}
void AdvancedPropulsionControlUnit::enter_emergency_mode() {
emergency_mode_active_ = true;
current_propulsion_state_.emergency_mode_active = true;
// Reset PID integrals to prevent windup in emergency
warp_error_integral_ = 0.0f;
warp_error_previous_ = 0.0f;
flux_error_integral_ = 0.0f;
flux_error_previous_ = 0.0f;
dilation_error_integral_ = 0.0f;
dilation_error_previous_ = 0.0f;
gravity_error_integral_ = 0.0f;
gravity_error_previous_ = 0.0f;
fluid_error_integral_ = 0.0f;
fluid_error_previous_ = 0.0f;
PlatformHAL::metric_emit("apcu.emergency_mode_activated", 1.0f);
}
void AdvancedPropulsionControlUnit::apply_emergency_limits(
SpacetimeModulationCommand& command) {
// Reduce all targets to conservative values
command.target_warp_field_strength *= 0.5f;
command.target_gravito_flux_bias *= 0.3f;
command.target_time_dilation_factor = 1.0f +
(command.target_time_dilation_factor - 1.0f) * 0.3f;
command.target_artificial_gravity_g *= 0.5f;
command.target_power_budget_GW = std::min(command.target_power_budget_GW,
MAX_SYSTEM_POWER_DRAW_GW * 0.6f);
// Force conservative control modes
command.enable_emergency_damping = true;
command.enable_resonance_suppression = true;
PlatformHAL::metric_emit("apcu.emergency_limits_applied", 1.0f);
}
bool AdvancedPropulsionControlUnit::initiate_emergency_spacetime_collapse() {
PlatformHAL::metric_emit("apcu.emergency_collapse_initiated", 1.0f);
// Create emergency shutdown command
SpacetimeModulationCommand emergency_command = {
0.0f, // Collapse warp field
0.0f, // Neutralize flux
1.0f, // Return to normal time
0.0f, // Remove artificial gravity
0.0f, // Stop fluid flow
MIN_POWER_DRAW_GW,
true, true, false // Force damping, suppression, disable active dilation
};
// Override current command
active_spacetime_command_ = emergency_command;
std::strncpy(active_directive_id_, "EMERGENCY_COLLAPSE", sizeof(active_directive_id_)
- 1);
active_directive_id_[sizeof(active_directive_id_) - 1] = '\0';
enter_emergency_mode();
return true;
}
bool AdvancedPropulsionControlUnit::execute_controlled_shutdown() {
PlatformHAL::metric_emit("apcu.controlled_shutdown_initiated", 1.0f);
// Similar to emergency collapse but more gradual
SpacetimeModulationCommand shutdown_command = {
0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
MIN_POWER_DRAW_GW,
false, false, false // Allow smooth transition
};
return receive_and_execute_spacetime_command(shutdown_command,
"CONTROLLED_SHUTDOWN");
}
bool AdvancedPropulsionControlUnit::restore_from_safe_state(
const SpacetimeModulationState& safe_state) {
// Validate that the provided state is actually safe
if (!is_state_safe_to_save(safe_state)) {
PlatformHAL::metric_emit("apcu.restore_rejected_unsafe_state", 1.0f);
return false;
}
||
// Check resource availability for restoration
if (safe_state.remaining_antimatter_kg > current_propulsion_state_.remaining_antimatter_kg
safe_state.quantum_fluid_level > current_propulsion_state_.quantum_fluid_level) {
PlatformHAL::metric_emit("apcu.restore_rejected_insufficient_resources", 1.0f);
return false;
}
// Restore state (physics properties only, not resources)
current_propulsion_state_.warp_field_strength = safe_state.warp_field_strength;
current_propulsion_state_.gravito_flux_bias = safe_state.gravito_flux_bias;
current_propulsion_state_.spacetime_curvature_magnitude =
safe_state.spacetime_curvature_magnitude;
current_propulsion_state_.time_dilation_factor = safe_state.time_dilation_factor;
current_propulsion_state_.induced_gravity_g = safe_state.induced_gravity_g;
// Reset PID controllers to prevent instability
warp_error_integral_ = 0.0f;
warp_error_previous_ = 0.0f;
flux_error_integral_ = 0.0f;
flux_error_previous_ = 0.0f;
dilation_error_integral_ = 0.0f;
dilation_error_previous_ = 0.0f;
gravity_error_integral_ = 0.0f;
gravity_error_previous_ = 0.0f;
// Exit emergency mode if we successfully restored to safe state
if (emergency_mode_active_) {
emergency_mode_active_ = false;
current_propulsion_state_.emergency_mode_active = false;
PlatformHAL::metric_emit("apcu.emergency_mode_deactivated", 1.0f);
}
PlatformHAL::metric_emit("apcu.state_restored", 1.0f);
return true;
SpacetimeModulationState AdvancedPropulsionControlUnit::get_current_state() const {
return current_propulsion_state_;
}
}
bool AdvancedPropulsionControlUnit::is_operational_state_safe() const {
bool safe = true;
// Critical resource checks
if (current_propulsion_state_.remaining_antimatter_kg <
RAPSConfig::CRITICAL_ANTIMATTER_KG) {
PlatformHAL::metric_emit("apcu.safety_fuel_critical", 1.0f);
safe = false;
}
if (current_propulsion_state_.quantum_fluid_level <
RAPSConfig::CRITICAL_QUANTUM_FLUID_LITERS) {
PlatformHAL::metric_emit("apcu.safety_quantum_fluid_critical", 1.0f);
safe = false;
}
// Power system checks
if (current_propulsion_state_.power_draw_GW > MAX_SYSTEM_POWER_DRAW_GW *
0.98f) {
PlatformHAL::metric_emit("apcu.safety_power_critical", 1.0f);
safe = false;
}
// Spacetime distortion limits
if (current_propulsion_state_.spacetime_curvature_magnitude >
MAX_SPACETIME_CURVATURE_MAGNITUDE * 0.98f) {
PlatformHAL::metric_emit("apcu.safety_curvature_critical", 1.0f);
safe = false;
}
if (current_propulsion_state_.time_dilation_factor > MAX_TIME_DILATION_FACTOR *
0.98f) {
safe = false;
PlatformHAL::metric_emit("apcu.safety_time_dilation_critical", 1.0f);
}
if (std::fabs(current_propulsion_state_.induced_gravity_g) > MAX_INDUCED_GRAVITY_G
* 0.98f) {
safe = false;
PlatformHAL::metric_emit("apcu.safety_gravity_critical", 1.0f);
}
// Field integrity checks (critical - prevents singularities)
if (current_propulsion_state_.warp_field_strength > MAX_WARP_FIELD_STRENGTH *
1.01f ||
std::fabs(current_propulsion_state_.gravito_flux_bias) > MAX_GRAVITO_FLUX_BIAS *
1.01f) {
safe = false;
PlatformHAL::metric_emit("apcu.safety_field_oob_critical", 1.0f);
}
// Resonance/coupling stress
if (current_propulsion_state_.field_coupling_stress >
RAPSConfig::CRITICAL_FIELD_COUPLING_THRESHOLD) {
PlatformHAL::metric_emit("apcu.safety_coupling_stress_critical", 1.0f);
safe = false;
}
// Stability index
if (current_propulsion_state_.spacetime_stability_index < 0.3f) {
PlatformHAL::metric_emit("apcu.safety_stability_critical", 1.0f);
safe = false;
}
// Control authority (if we're losing control)
if (current_propulsion_state_.control_authority_remaining < 0.1f) {
PlatformHAL::metric_emit("apcu.safety_control_authority_low", 1.0f);
safe = false;
}
return safe;
}:
