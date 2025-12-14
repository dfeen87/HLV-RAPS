#pragma once

#include "RAPSController.hpp"
#include "RAPSDefinitions.hpp"

class RedundantSupervisor {
public:
    enum class FailureMode {
        CRITICAL_ROLLBACK_FAIL,
        CRITICAL_NO_ROLLBACK,
        PRIMARY_CHANNEL_LOCKUP,
        MISMATED_PREDICTION
    };

    void init();
    void run_cycle(const PhysicsState& current_state);
    void notify_failure(FailureMode mode);

    void synchronize_inactive_controller(
        const PhysicsState& current_state);

    bool check_a_b_prediction_mismatch(
        const PredictionResult& result_a,
        const PredictionResult& result_b) const;

private:
    // Controllers
    RAPSController controller_a_;
    RAPSController controller_b_;

    // Redundancy state
    bool is_channel_a_active_{true};
    uint32_t last_sync_timestamp_{0};

    // Prediction tracking
    PredictionResult last_active_prediction_;

    // Internal actions
    void switch_to_channel_b();
    void switch_to_channel_a();
    void log_supervisor_exception(FailureMode mode);
};
