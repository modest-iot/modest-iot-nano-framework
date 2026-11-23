// Copyright (C) 2026 Modest IoT
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cmath>
#include <cassert>
#include <cstring>

#include "emlearn.h"
#include "AiMicroModelController.h"
#include "EmlearnController.h"

// Test helper: concrete implementation of AiMicroModelController for testing abstract interface
class MockController : public AiMicroModelController {
public:
    int32_t predict(const float* features, int32_t nFeatures) override {
        (void)features;
        (void)nFeatures;
        return mockPredictResult;
    }

    float regress(const float* features, int32_t nFeatures) override {
        (void)features;
        (void)nFeatures;
        return mockRegressResult;
    }

    bool isModelLoaded() const override { return mockLoaded; }
    InferenceResult lastResult() const override { return mockLastResult; }

    int32_t mockPredictResult = 0;
    float mockRegressResult = 0.0f;
    bool mockLoaded = true;
    InferenceResult mockLastResult = InferenceResult::Success;
};

// Test functions
void test_predict_tree_model() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    EmlearnFake::gTreesPredictResult = 1;
    float features[3] = {1.0f, 2.0f, 3.0f};
    int32_t result = controller.predict(features, 3);

    assert(result == 1);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_predict_net_model() {
    EmlearnController controller;
    EmlNet model = {};
    controller.loadNetModel(&model);

    EmlearnFake::gNetPredictResult = 2;
    float features[3] = {1.0f, 2.0f, 3.0f};
    int32_t result = controller.predict(features, 3);

    assert(result == 2);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_regress_tree_model() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    EmlearnFake::gTreesRegressResult = 42.5f;
    float features[3] = {1.0f, 2.0f, 3.0f};
    float result = controller.regress(features, 3);

    assert(result == 42.5f);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_no_model_loaded_predict() {
    EmlearnController controller;
    float features[3] = {1.0f, 2.0f, 3.0f};
    int32_t result = controller.predict(features, 3);

    assert(result == -1);
    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);
    assert(controller.isModelLoaded() == false);
}

void test_no_model_loaded_regress() {
    EmlearnController controller;
    float features[3] = {1.0f, 2.0f, 3.0f};
    float result = controller.regress(features, 3);

    assert(std::isnan(result));
    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);
}

void test_null_features_predict() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    int32_t result = controller.predict(nullptr, 3);
    assert(result == -1);
    assert(controller.lastResult() == InferenceResult::InvalidFeatures);
}

void test_null_features_regress() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    float result = controller.regress(nullptr, 3);
    assert(std::isnan(result));
    assert(controller.lastResult() == InferenceResult::InvalidFeatures);
}

void test_zero_features_predict() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    float features[1] = {0.0f};
    int32_t result = controller.predict(features, 0);
    assert(result == -1);
    assert(controller.lastResult() == InferenceResult::InvalidFeatures);
}

void test_zero_features_regress() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    float features[1] = {0.0f};
    float result = controller.regress(features, 0);
    assert(std::isnan(result));
    assert(controller.lastResult() == InferenceResult::InvalidFeatures);
}

void test_negative_features_predict() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    float features[1] = {0.0f};
    int32_t result = controller.predict(features, -1);
    assert(result == -1);
    assert(controller.lastResult() == InferenceResult::InvalidFeatures);
}

void test_load_null_tree_model() {
    EmlearnController controller;
    controller.loadTreeModel(nullptr);
    assert(controller.isModelLoaded() == false);
    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);
}

void test_load_null_net_model() {
    EmlearnController controller;
    controller.loadNetModel(nullptr);
    assert(controller.isModelLoaded() == false);
    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);
}

void test_switch_from_tree_to_net() {
    EmlearnController controller;
    EmlTrees treeModel = {};
    EmlNet netModel = {};

    controller.loadTreeModel(&treeModel);
    assert(controller.isModelLoaded() == true);

    EmlearnFake::gTreesPredictResult = 1;
    float features[3] = {1.0f, 2.0f, 3.0f};
    assert(controller.predict(features, 3) == 1);

    controller.loadNetModel(&netModel);
    EmlearnFake::gNetPredictResult = 2;
    assert(controller.predict(features, 3) == 2);
}

void test_switch_from_net_to_tree() {
    EmlearnController controller;
    EmlTrees treeModel = {};
    EmlNet netModel = {};

    controller.loadNetModel(&netModel);
    assert(controller.isModelLoaded() == true);

    EmlearnFake::gNetPredictResult = 2;
    float features[3] = {1.0f, 2.0f, 3.0f};
    assert(controller.predict(features, 3) == 2);

    controller.loadTreeModel(&treeModel);
    EmlearnFake::gTreesPredictResult = 1;
    assert(controller.predict(features, 3) == 1);
}

void test_multiple_inferences() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    float features[3] = {1.0f, 2.0f, 3.0f};

    EmlearnFake::gTreesPredictResult = 0;
    assert(controller.predict(features, 3) == 0);

    EmlearnFake::gTreesPredictResult = 1;
    assert(controller.predict(features, 3) == 1);

    EmlearnFake::gTreesPredictResult = 2;
    assert(controller.predict(features, 3) == 2);

    assert(controller.lastResult() == InferenceResult::Success);
}

void test_last_result_tracking() {
    EmlearnController controller;

    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);

    EmlTrees model = {};
    controller.loadTreeModel(&model);
    assert(controller.lastResult() == InferenceResult::Success);

    controller.predict(nullptr, 0);
    assert(controller.lastResult() == InferenceResult::InvalidFeatures);

    EmlearnFake::gTreesPredictResult = 1;
    float features[3] = {1.0f, 2.0f, 3.0f};
    controller.predict(features, 3);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_is_model_loaded_initial() {
    EmlearnController controller;
    assert(controller.isModelLoaded() == false);
}

void test_is_model_loaded_after_tree() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);
    assert(controller.isModelLoaded() == true);
}

void test_is_model_loaded_after_net() {
    EmlearnController controller;
    EmlNet model = {};
    controller.loadNetModel(&model);
    assert(controller.isModelLoaded() == true);
}

void test_abstract_interface() {
    MockController mock;
    mock.mockPredictResult = 5;
    mock.mockRegressResult = 3.14f;
    mock.mockLoaded = true;
    mock.mockLastResult = InferenceResult::Success;

    AiMicroModelController* base = &mock;

    float features[3] = {1.0f, 2.0f, 3.0f};
    assert(base->predict(features, 3) == 5);
    assert(base->regress(features, 3) == 3.14f);
    assert(base->isModelLoaded() == true);
    assert(base->lastResult() == InferenceResult::Success);
}

void test_nan_features() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    EmlearnFake::gTreesPredictResult = 0;
    float features[3] = {NAN, 2.0f, 3.0f};
    int32_t result = controller.predict(features, 3);

    assert(result == 0);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_inf_features() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    EmlearnFake::gTreesPredictResult = 1;
    float features[3] = {INFINITY, 2.0f, 3.0f};
    int32_t result = controller.predict(features, 3);

    assert(result == 1);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_negative_features_value() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    EmlearnFake::gTreesPredictResult = 0;
    float features[3] = {-40.0f, -10.5f, 0.0f};
    int32_t result = controller.predict(features, 3);

    assert(result == 0);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_large_feature_array() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    EmlearnFake::gTreesPredictResult = 2;
    float features[100];
    for (int i = 0; i < 100; i++) {
        features[i] = static_cast<float>(i);
    }
    int32_t result = controller.predict(features, 100);

    assert(result == 2);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_regress_net_model() {
    EmlearnController controller;
    EmlNet model = {};
    controller.loadNetModel(&model);

    EmlearnFake::gNetPredictResult = 5;
    float features[3] = {1.0f, 2.0f, 3.0f};
    float result = controller.regress(features, 3);

    assert(result == 5.0f);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_predict_after_null_model_switch() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    controller.loadTreeModel(nullptr);
    assert(controller.isModelLoaded() == false);

    float features[3] = {1.0f, 2.0f, 3.0f};
    int32_t result = controller.predict(features, 3);
    assert(result == -1);
    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);
}

void test_regress_after_null_model_switch() {
    EmlearnController controller;
    EmlTrees model = {};
    controller.loadTreeModel(&model);

    controller.loadNetModel(nullptr);
    assert(controller.isModelLoaded() == false);

    float features[3] = {1.0f, 2.0f, 3.0f};
    float result = controller.regress(features, 3);
    assert(std::isnan(result));
    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);
}

void test_consecutive_tree_and_net() {
    EmlearnController controller;
    EmlTrees treeModel = {};
    EmlNet netModel = {};

    float features[3] = {1.0f, 2.0f, 3.0f};

    controller.loadTreeModel(&treeModel);
    EmlearnFake::gTreesPredictResult = 1;
    assert(controller.predict(features, 3) == 1);

    controller.loadNetModel(&netModel);
    EmlearnFake::gNetPredictResult = 2;
    assert(controller.predict(features, 3) == 2);

    controller.loadTreeModel(&treeModel);
    EmlearnFake::gTreesPredictResult = 3;
    assert(controller.predict(features, 3) == 3);
}

void test_emlearn_controller_always_available() {
    // EmlearnController is always available via ModestIoT.h — no build guard.
    // This test verifies the class is instantiable without any preprocessor flags.
    EmlearnController controller;
    assert(controller.isModelLoaded() == false);
    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);
}

// --- Generated model (function pointer) tests ---
// Note: Must use static functions, not lambdas, because loadGeneratedModel
// accepts a plain C function pointer (int32_t (*)(const int16_t*, int32_t)).
// Capturing lambdas cannot convert to function pointers.

static int32_t gMockGeneratedResult = 0;
static int32_t gMockGeneratedCallCount = 0;

static int32_t mockGeneratedPredict(const int16_t* features, int32_t nFeatures) {
    (void)features;
    (void)nFeatures;
    gMockGeneratedCallCount++;
    return gMockGeneratedResult;
}

static int32_t mockGeneratedPredictCheckQuantization(const int16_t* features, int32_t nFeatures) {
    // Float 0.5f quantizes to int16_t 0, float 2.5f quantizes to 2
    if (nFeatures >= 2 && features[0] == 0 && features[1] == 2) {
        return 0;
    }
    return 1;
}

static int32_t mockGeneratedPredictCheckNaN(const int16_t* features, int32_t nFeatures) {
    // NaN should be clamped to 0
    if (nFeatures >= 1 && features[0] == 0) {
        return 2;
    }
    return -1;
}

static int32_t mockGeneratedPredictCheckInf(const int16_t* features, int32_t nFeatures) {
    // Inf should be clamped to 0
    if (nFeatures >= 1 && features[0] == 0) {
        return 3;
    }
    return -1;
}

void test_predict_generated_model() {
    gMockGeneratedResult = 1;
    gMockGeneratedCallCount = 0;
    EmlearnController controller;
    controller.loadGeneratedModel(&mockGeneratedPredict);

    assert(controller.isModelLoaded() == true);
    assert(controller.lastResult() == InferenceResult::Success);

    float features[3] = {1.0f, 2.0f, 3.0f};
    int32_t result = controller.predict(features, 3);
    assert(result == 1);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_predict_generated_model_quantization() {
    EmlearnController controller;
    controller.loadGeneratedModel(&mockGeneratedPredictCheckQuantization);

    float features[3] = {0.5f, 2.5f, 3.7f};
    int32_t result = controller.predict(features, 3);
    assert(result == 0);
}

void test_predict_generated_model_nan_clamping() {
    EmlearnController controller;
    controller.loadGeneratedModel(&mockGeneratedPredictCheckNaN);

    float features[2] = {NAN, 1.0f};
    int32_t result = controller.predict(features, 2);
    assert(result == 2);
}

void test_predict_generated_model_inf_clamping() {
    EmlearnController controller;
    controller.loadGeneratedModel(&mockGeneratedPredictCheckInf);

    float features[2] = {INFINITY, 1.0f};
    int32_t result = controller.predict(features, 2);
    assert(result == 3);
}

void test_regress_generated_model() {
    gMockGeneratedResult = 5;
    EmlearnController controller;
    controller.loadGeneratedModel(&mockGeneratedPredict);

    float features[3] = {1.0f, 2.0f, 3.0f};
    float result = controller.regress(features, 3);
    assert(result == 5.0f);
    assert(controller.lastResult() == InferenceResult::Success);
}

void test_load_null_generated_model() {
    EmlearnController controller;
    controller.loadGeneratedModel(nullptr);
    assert(controller.isModelLoaded() == false);
    assert(controller.lastResult() == InferenceResult::ModelNotLoaded);
}

void test_generated_model_null_features() {
    gMockGeneratedResult = 0;
    EmlearnController controller;
    controller.loadGeneratedModel(&mockGeneratedPredict);

    int32_t result = controller.predict(nullptr, 3);
    assert(result == -1);
    assert(controller.lastResult() == InferenceResult::InvalidFeatures);
}

void test_generated_model_zero_features() {
    gMockGeneratedResult = 0;
    EmlearnController controller;
    controller.loadGeneratedModel(&mockGeneratedPredict);

    float features[1] = {0.0f};
    int32_t result = controller.predict(features, 0);
    assert(result == -1);
    assert(controller.lastResult() == InferenceResult::InvalidFeatures);
}

void test_switch_from_generated_to_tree() {
    gMockGeneratedResult = 1;
    EmlearnController controller;
    EmlTrees treeModel = {};

    controller.loadGeneratedModel(&mockGeneratedPredict);
    float features[3] = {1.0f, 2.0f, 3.0f};
    assert(controller.predict(features, 3) == 1);

    controller.loadTreeModel(&treeModel);
    EmlearnFake::gTreesPredictResult = 2;
    assert(controller.predict(features, 3) == 2);
}

void test_switch_from_tree_to_generated() {
    gMockGeneratedResult = 3;
    EmlearnController controller;
    EmlTrees treeModel = {};

    controller.loadTreeModel(&treeModel);
    EmlearnFake::gTreesPredictResult = 2;
    float features[3] = {1.0f, 2.0f, 3.0f};
    assert(controller.predict(features, 3) == 2);

    controller.loadGeneratedModel(&mockGeneratedPredict);
    assert(controller.predict(features, 3) == 3);
}

void test_generated_model_multiple_inferences() {
    gMockGeneratedCallCount = 0;
    gMockGeneratedResult = 0;
    EmlearnController controller;
    controller.loadGeneratedModel(&mockGeneratedPredict);

    float features[3] = {1.0f, 2.0f, 3.0f};
    gMockGeneratedResult = 1;
    assert(controller.predict(features, 3) == 1);
    gMockGeneratedResult = 2;
    assert(controller.predict(features, 3) == 2);
    gMockGeneratedResult = 3;
    assert(controller.predict(features, 3) == 3);
    assert(gMockGeneratedCallCount == 3);
    assert(controller.lastResult() == InferenceResult::Success);
}

// Main test runner
int main() {
    test_predict_tree_model();
    test_predict_net_model();
    test_regress_tree_model();
    test_no_model_loaded_predict();
    test_no_model_loaded_regress();
    test_null_features_predict();
    test_null_features_regress();
    test_zero_features_predict();
    test_zero_features_regress();
    test_negative_features_predict();
    test_load_null_tree_model();
    test_load_null_net_model();
    test_switch_from_tree_to_net();
    test_switch_from_net_to_tree();
    test_multiple_inferences();
    test_last_result_tracking();
    test_is_model_loaded_initial();
    test_is_model_loaded_after_tree();
    test_is_model_loaded_after_net();
    test_abstract_interface();
    test_nan_features();
    test_inf_features();
    test_negative_features_value();
    test_large_feature_array();
    test_regress_net_model();
    test_predict_after_null_model_switch();
    test_regress_after_null_model_switch();
    test_consecutive_tree_and_net();
    test_emlearn_controller_always_available();
    test_predict_generated_model();
    test_predict_generated_model_quantization();
    test_predict_generated_model_nan_clamping();
    test_predict_generated_model_inf_clamping();
    test_regress_generated_model();
    test_load_null_generated_model();
    test_generated_model_null_features();
    test_generated_model_zero_features();
    test_switch_from_generated_to_tree();
    test_switch_from_tree_to_generated();
    test_generated_model_multiple_inferences();

    return 0;
}
