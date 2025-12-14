#pragma once

#include <cmath>

// Determines whether A/B predictions diverge beyond acceptable bounds
inline bool check_prediction_mismatch(
    const PredictionResult& result_a,
    const PredictionResult& result_b) {

    float pos_a =
        result_a.predicted_end_state.position_m[0] +
        result_a.predicted_end_state.position_m[1] +
        result_a.predicted_end_state.position_m[2];

    float pos_b =
        result_b.predicted_end_state.position_m[0] +
        result_b.predicted_end_state.position_m[1] +
        result_b.predicted_end_state.position_m[2];

    return std::fabs(pos_a - pos_b) >
           RAPSConfig::ACCEPT_POSITION_DEV_M;
}
