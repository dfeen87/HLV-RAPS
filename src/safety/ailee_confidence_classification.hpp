#pragma once

inline AileeStatus classify_ailee_confidence(
    float confidence) {

    if (confidence >= RAPSConfig::AILEE_CONFIDENCE_ACCEPTED) {
        PlatformHAL::metric_emit(
            "ailee.status", 1.0f, "status", "ACCEPTED");
        return AileeStatus::ACCEPTED;
    }

    if (confidence >= RAPSConfig::AILEE_CONFIDENCE_BORDERLINE) {
        PlatformHAL::metric_emit(
            "ailee.status", 2.0f, "status", "BORDERLINE");
        return AileeStatus::BORDERLINE;
    }

    PlatformHAL::metric_emit(
        "ailee.status", 3.0f, "status", "OUTRIGHT_REJECTED");
    return AileeStatus::OUTRIGHT_REJECTED;
}
