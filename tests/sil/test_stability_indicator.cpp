#include "raps/safety/stability_indicator.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

void test_compute_Su() {
    // 1. Boundary behavior: phi=0, chi=0 -> S_u = 1.0
    double su_max = StabilityIndicator::compute_Su(0.0, 0.0);
    assert(std::abs(su_max - 1.0) < 1e-9);

    // 2. Numerical output
    // S_u = 1 / (1 + 0.5^2 + 0.5^3) = 1 / (1 + 0.25 + 0.125) = 1 / 1.375 = 0.727272...
    double su_val = StabilityIndicator::compute_Su(0.5, 0.5);
    assert(std::abs(su_val - 1.0/1.375) < 1e-9);

    // 3. Monotonicity: as phi or chi increase, S_u decreases
    double su_larger_phi = StabilityIndicator::compute_Su(0.6, 0.5);
    assert(su_larger_phi < su_val);

    double su_larger_chi = StabilityIndicator::compute_Su(0.5, 0.6);
    assert(su_larger_chi < su_val);
}

void test_maneuver_aware_thresholds() {
    auto cruise = StabilityConfig::get_thresholds(StabilityConfig::ManeuverClass::CRUISE);
    assert(std::abs(cruise.S_u_min - 0.82) < 1e-9);
    assert(std::abs(cruise.S_u_warn - 0.86) < 1e-9);

    auto mild = StabilityConfig::get_thresholds(StabilityConfig::ManeuverClass::MILD_MANEUVER);
    assert(std::abs(mild.S_u_min - 0.78) < 1e-9);
    assert(std::abs(mild.S_u_warn - 0.83) < 1e-9);

    auto aggressive = StabilityConfig::get_thresholds(StabilityConfig::ManeuverClass::AGGRESSIVE_MANEUVER);
    assert(std::abs(aggressive.S_u_min - 0.72) < 1e-9);
    assert(std::abs(aggressive.S_u_warn - 0.78) < 1e-9);
}

void test_rate_of_change() {
    StabilityIndicator ind;
    // initial state: S_u is ~1.0
    // We send a valid reading at t=1000
    ind.update_stability_state(1000, StabilityConfig::ManeuverClass::CRUISE, 0.0, 0.0); // S_u = 1.0

    // Small change at t=1001, rate should be compliant
    // phi=0.1, chi=0 => S_u = 1/(1+0.01) = 0.990099
    // dS_u/dt = (0.990099 - 1.0) / 1 = -0.0099
    // cruise rate limit is 0.015, so |dS_u/dt| is compliant.
    auto ev = ind.update_stability_state(1001, StabilityConfig::ManeuverClass::CRUISE, 0.1, 0.0);
    assert(ev.flag_type == DSMFlagType::NONE);

    // Large change at t=1002, rate violation
    // phi=0.5, chi=0 => S_u = 1/(1+0.25) = 0.8
    // dS_u/dt = (0.8 - 0.990099) / 1 = -0.19
    // |dS_u/dt| > 0.015 -> violation
    auto ev_viol = ind.update_stability_state(1002, StabilityConfig::ManeuverClass::CRUISE, 0.5, 0.0);
    assert(ev_viol.flag_type == DSMFlagType::SU_RATE_VIOLATION);
}

void test_hysteresis() {
    StabilityIndicator ind;
    ind.update_stability_state(1000, StabilityConfig::ManeuverClass::CRUISE, 0.0, 0.0); // S_u = 1.0
    assert(ind.is_safe() == true);

    // Drop to just above S_u_min (0.82) -> e.g. 0.83
    // S_u = 1 / (1 + phi^2) => phi = sqrt(1/S_u - 1)
    // For S_u = 0.83 => phi = sqrt(1/0.83 - 1) ~ 0.4525
    auto ev1 = ind.update_stability_state(2000, StabilityConfig::ManeuverClass::CRUISE, 0.4525, 0.0);
    assert(ind.is_safe() == true); // still safe
    assert(ev1.flag_type == DSMFlagType::NONE);

    // Drop below S_u_min (0.82) -> e.g. 0.80
    // S_u = 0.80 => phi = sqrt(1/0.80 - 1) = sqrt(0.25) = 0.5
    auto ev2 = ind.update_stability_state(3000, StabilityConfig::ManeuverClass::CRUISE, 0.5, 0.0);
    assert(ind.is_safe() == false); // exited safe mode
    assert(ev2.flag_type == DSMFlagType::SU_HYSTERESIS_TRANSITION);

    // Hover near boundary: S_u goes to 0.84 (below warn 0.86, above min 0.82)
    // S_u = 0.84 => phi = sqrt(1/0.84 - 1) ~ 0.4364
    auto ev3 = ind.update_stability_state(4000, StabilityConfig::ManeuverClass::CRUISE, 0.4364, 0.0);
    assert(ind.is_safe() == false); // no oscillation, still not safe
    // Since it's still unsafe but above S_u_min, and no transition, flag is NONE.
    assert(ev3.flag_type == DSMFlagType::NONE);

    // Enter safe mode: S_u goes above S_u_warn (0.86) -> e.g. 0.90
    // S_u = 0.90 => phi = sqrt(1/0.90 - 1) ~ 0.3333
    auto ev4 = ind.update_stability_state(5000, StabilityConfig::ManeuverClass::CRUISE, 0.3333, 0.0);
    assert(ind.is_safe() == true); // entered safe mode
    assert(ev4.flag_type == DSMFlagType::SU_HYSTERESIS_TRANSITION);
}

void test_dsm_integration() {
    StabilityIndicator ind;
    // S_u = 1.0 -> no flag
    auto ev = ind.update_stability_state(1000, StabilityConfig::ManeuverClass::AGGRESSIVE_MANEUVER, 0.0, 0.0);
    assert(ev.timestamp == 1000);
    assert(ev.maneuver_class == StabilityConfig::ManeuverClass::AGGRESSIVE_MANEUVER);
    assert(std::abs(ev.S_u - 1.0) < 1e-9);
    assert(ev.dS_u_dt == 0.0);
    assert(ev.flag_type == DSMFlagType::NONE);

    // S_u drops to 0.6 (aggressive min is 0.72)
    // S_u = 0.6 => phi = sqrt(1/0.6 - 1) = sqrt(0.666) ~ 0.8165
    // large drop -> rate violation triggers first
    auto ev2 = ind.update_stability_state(1001, StabilityConfig::ManeuverClass::AGGRESSIVE_MANEUVER, 0.8165, 0.0);
    assert(ev2.timestamp == 1001);
    assert(ev2.flag_type == DSMFlagType::SU_RATE_VIOLATION); // Rate limit takes precedence as coded

    // Keep it at 0.6 for long time -> no rate violation, but it's low
    auto ev3 = ind.update_stability_state(2001, StabilityConfig::ManeuverClass::AGGRESSIVE_MANEUVER, 0.8165, 0.0);
    assert(ev3.timestamp == 2001);
    // Since is_safe_mode was false from previous transition/drop (actually previous trigger was rate violation,
    // but the hysteresis state was updated to unsafe), and it is still below S_u_min, it emits SU_LOW.
    assert(ev3.flag_type == DSMFlagType::SU_LOW);
}

int main() {
    test_compute_Su();
    test_maneuver_aware_thresholds();
    test_rate_of_change();
    test_hysteresis();
    test_dsm_integration();
    std::cout << "Stability Indicator tests passed." << std::endl;
    return 0;
}
