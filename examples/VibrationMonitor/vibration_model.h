// !!! This file is generated using emlearn !!!
//
// This mock model demonstrates the format emlearn produces when converting
// a trained RandomForest classifier to C code. It classifies 6-axis vibration
// data into two classes:
//   0 = Normal operation
//   1 = Anomaly detected
//
// In a real project, generate this file using:
//   emlearn convert vibration_model.pkl --trees --optimize --output vibration_model.h
//
// Features (6 total, quantized to int16_t by emlearn):
//   0: X-axis RMS acceleration (g * 1000)
//   1: Y-axis RMS acceleration (g * 1000)
//   2: Z-axis RMS acceleration (g * 1000)
//   3: X-axis peak-to-peak (g * 1000)
//   4: Y-axis peak-to-peak (g * 1000)
//   5: Z-axis peak-to-peak (g * 1000)
//
// The model uses 3 decision trees with majority voting.
// Each tree checks quantized feature values against thresholds.

#include <stdint.h>

// Tree 0: Primary Z-axis RMS check
static inline int32_t vibration_tree_0(const int16_t *features, int32_t features_length) {
    (void)features_length;
    if (features[2] < 2000) {
        // Z-axis RMS < 2.0g — normal
        return 0;
    } else {
        // Z-axis RMS >= 2.0g — anomaly
        return 1;
    }
}

// Tree 1: Secondary check combining Z-axis RMS and peak-to-peak
static inline int32_t vibration_tree_1(const int16_t *features, int32_t features_length) {
    (void)features_length;
    if (features[5] < 4000) {
        // Z-axis peak-to-peak < 4.0g — normal
        return 0;
    } else {
        // Z-axis peak-to-peak >= 4.0g — anomaly
        return 1;
    }
}

// Tree 2: Tertiary check on X-axis RMS
static inline int32_t vibration_tree_2(const int16_t *features, int32_t features_length) {
    (void)features_length;
    if (features[0] < 1500) {
        // X-axis RMS < 1.5g — normal
        return 0;
    } else {
        // X-axis RMS >= 1.5g — anomaly
        return 1;
    }
}

// Majority voting across all trees
int32_t vibration_predict(const int16_t *features, int32_t features_length) {
    int32_t votes[2] = {0,};
    int32_t _class = -1;

    _class = vibration_tree_0(features, features_length); votes[_class] += 1;
    _class = vibration_tree_1(features, features_length); votes[_class] += 1;
    _class = vibration_tree_2(features, features_length); votes[_class] += 1;

    int32_t most_voted_class = -1;
    int32_t most_voted_votes = 0;
    for (int32_t i = 0; i < 2; i++) {
        if (votes[i] > most_voted_votes) {
            most_voted_class = i;
            most_voted_votes = votes[i];
        }
    }
    return most_voted_class;
}

// Probability output (mean vote fraction per class)
int vibration_predict_proba(const int16_t *features, int32_t features_length, float *out, int out_length) {
    (void)features_length;
    int32_t _class = -1;

    for (int i = 0; i < out_length; i++) {
        out[i] = 0.0f;
    }

    _class = vibration_tree_0(features, features_length); out[_class] += 1.0f;
    _class = vibration_tree_1(features, features_length); out[_class] += 1.0f;
    _class = vibration_tree_2(features, features_length); out[_class] += 1.0f;

    // Compute mean probability
    for (int i = 0; i < out_length; i++) {
        out[i] = out[i] / 3;
    }
    return 0;
}
