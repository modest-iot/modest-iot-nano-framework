// Copyright (C) 2026 Modest IoT
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstddef>

// Fake emlearn C API for host tests.
// Mirrors the emlearn library's C structs (eml_trees.h, eml_net.h) so that
// EmlearnController can be tested without the actual emlearn library.
//
// Struct definitions match emlearn's real types from:
//   https://github.com/emlearn/emlearn/blob/master/emlearn/eml_trees.h

// ---------------------------------------------------------------------------
// EmlTrees types (matches eml_trees.h)
// ---------------------------------------------------------------------------

struct EmlTreesNode {
    int8_t  feature;
    int16_t value;
    int16_t left;
    int16_t right;
};

struct EmlTrees {
    int32_t  n_nodes;
    EmlTreesNode *nodes;

    int32_t  n_trees;
    int32_t *tree_roots;

    int32_t  n_leaves;
    uint8_t *leaves;
    int8_t   leaf_bits;

    int8_t n_features;
    int8_t n_classes;
};

// ---------------------------------------------------------------------------
// EmlNet types (fake — emlearn's net struct is complex; simplified for tests)
// ---------------------------------------------------------------------------

struct EmlNetLayer {
    int32_t nInputs;
    int32_t nOutputs;
    const float* weights;
    const float* biases;
};

struct EmlNet {
    const EmlNetLayer* layers;
    int32_t nLayers;
    int32_t nInputs;
    int32_t nOutputs;
};

// ---------------------------------------------------------------------------
// Mock predict functions (matches eml_trees.h / eml_net.h signatures)
// ---------------------------------------------------------------------------

namespace EmlearnFake {
    inline int gTreesPredictResult = 0;
    inline int gNetPredictResult = 0;
    inline float gTreesRegressResult = 0.0f;
}

// emlearn's real signature: int eml_trees_predict(const EmlTrees*, const int16_t*, int8_t)
static inline int eml_trees_predict(const EmlTrees* model, const int16_t* features, int8_t nFeatures) {
    (void)model;
    (void)features;
    (void)nFeatures;
    return EmlearnFake::gTreesPredictResult;
}

// emlearn's real signature: float eml_trees_regress(const EmlTrees*, const int16_t*, int8_t)
static inline float eml_trees_regress(const EmlTrees* model, const int16_t* features, int8_t nFeatures) {
    (void)model;
    (void)features;
    (void)nFeatures;
    return EmlearnFake::gTreesRegressResult;
}

// emlearn's real signature: int eml_net_predict(const EmlNet*, const float*, int32_t)
static inline int eml_net_predict(const EmlNet* model, const float* features, int32_t nFeatures) {
    (void)model;
    (void)features;
    (void)nFeatures;
    return EmlearnFake::gNetPredictResult;
}

static inline int eml_net_predict_proba(const EmlNet* model, const float* features, int32_t nFeatures, float* probs) {
    (void)model;
    (void)features;
    (void)nFeatures;
    (void)probs;
    return EmlearnFake::gNetPredictResult;
}
