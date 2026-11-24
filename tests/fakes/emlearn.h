// Copyright (C) 2026 Modest IoT
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstddef>

// Suppress weak declarations in EmlearnController.h — this header provides
// the actual definitions used by host tests instead of the weak symbols.
#define EML_TREES_PREDICT_DEFINED
#define EML_TREES_REGRESS_DEFINED
#define EML_NET_PREDICT_DEFINED

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

// Override the weak emlearn C library declarations from EmlearnController.h.
// These inline definitions replace the weak symbols so host tests can control
// the return values via EmlearnFake::gTreesPredictResult, etc.
// Must be inside extern "C" to match the linkage of weak declarations in EmlearnController.h.
// Using 'inline' (not 'static inline') so the definition can override the weak declaration.
extern "C" {

int eml_trees_predict(const EmlTrees* model, const int16_t* features, int8_t nFeatures) {
    (void)model;
    (void)features;
    (void)nFeatures;
    return EmlearnFake::gTreesPredictResult;
}

float eml_trees_regress(const EmlTrees* model, const int16_t* features, int8_t nFeatures) {
    (void)model;
    (void)features;
    (void)nFeatures;
    return EmlearnFake::gTreesRegressResult;
}

int eml_net_predict(const EmlNet* model, const float* features, int32_t nFeatures) {
    (void)model;
    (void)features;
    (void)nFeatures;
    return EmlearnFake::gNetPredictResult;
}

int eml_net_predict_proba(const EmlNet* model, const float* features, int32_t nFeatures, float* probs) {
    (void)model;
    (void)features;
    (void)nFeatures;
    (void)probs;
    return EmlearnFake::gNetPredictResult;
}

} // extern "C"
