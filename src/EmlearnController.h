// Copyright (C) 2026 Modest IoT
// SPDX-License-Identifier: MIT

#pragma once

#include <cmath>
#include <cstdint>
#include "AiMicroModelController.h"

// EmlearnController is always available via ModestIoT.h — no build guard needed.
// The emlearn types (EmlTrees, EmlNet) are forward-declared so this header
// compiles without the emlearn library installed. Only loadTreeModel() and
// loadNetModel() require the actual emlearn headers (<eml_trees.h>, <eml_net.h>);
// the most common path, loadGeneratedModel(), needs no emlearn types at all.
//
// For host tests, fake types are defined in tests/fakes/emlearn.h.

// Forward declarations of emlearn C types.
// The actual definitions come from <eml_trees.h> / <eml_net.h> on real builds,
// or from tests/fakes/emlearn.h in host tests.
struct EmlTrees;
struct EmlNet;

// emlearn C library function signatures (declared here so EmlearnController
// compiles without the emlearn library; linked at build time when present).
extern "C" {
    int eml_trees_predict(const EmlTrees* model, const int16_t* features, int8_t nFeatures);
    float eml_trees_regress(const EmlTrees* model, const int16_t* features, int8_t nFeatures);
    int eml_net_predict(const EmlNet* model, const float* features, int32_t nFeatures);
}

/**
 * emlearn-specific implementation of AiMicroModelController.
 *
 * Wraps emlearn's C library API (eml_trees_predict, eml_net_predict) behind
 * the uniform AiMicroModelController interface. Supports tree-based models
 * (RandomForest, DecisionTree), neural network models, and emlearn-generated
 * inline model functions.
 *
 * No build guard is required — simply include <ModestIoT.h> and use.
 * The emlearn library itself is only needed at link time when loadTreeModel()
 * or loadNetModel() is called. The loadGeneratedModel() path (most common)
 * works with any emlearn-generated model header.
 *
 * Example using a generated emlearn model (most common):
 * @code
 * #include <ModestIoT.h>
 * #include "vibration_model.h"  // emlearn-generated inline code
 *
 * EmlearnController controller;
 * controller.loadGeneratedModel(&vibration_predict);
 *
 * float features[6] = { 0.5f, 0.6f, 0.8f, 1.2f, 1.5f, 2.5f };
 * int32_t prediction = controller.predict(features, 6);
 * @endcode
 *
 * Example using the EmlTrees library API:
 * @code
 * #include <ModestIoT.h>
 * #include <eml_trees.h>
 *
 * EmlearnController controller;
 * controller.loadTreeModel(&model);  // EmlTrees struct from emlearn C library
 *
 * float features[6] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
 * int32_t prediction = controller.predict(features, 6);
 * @endcode
 */
class EmlearnController : public AiMicroModelController {
public:
    EmlearnController()
        : modelType(ModelType::None)
        , treeModel(nullptr)
        , netModel(nullptr)
        , generatedPredictFunction(nullptr)
        , lastResultValue(InferenceResult::ModelNotLoaded)
    {
    }

    /// Initialize with a tree-based model (RandomForest, DecisionTree, etc.).
    /// @param model Pointer to the emlearn tree model in EmlTrees format.
    void loadTreeModel(const EmlTrees* model) {
        if (model == nullptr) {
            modelType = ModelType::None;
            treeModel = nullptr;
            lastResultValue = InferenceResult::ModelNotLoaded;
            return;
        }
        treeModel = model;
        netModel = nullptr;
        modelType = ModelType::Trees;
        lastResultValue = InferenceResult::Success;
    }

    /// Initialize with a neural network model.
    /// @param model Pointer to the emlearn neural network model.
    void loadNetModel(const EmlNet* model) {
        if (model == nullptr) {
            modelType = ModelType::None;
            netModel = nullptr;
            lastResultValue = InferenceResult::ModelNotLoaded;
            return;
        }
        netModel = model;
        treeModel = nullptr;
        generatedPredictFunction = nullptr;
        modelType = ModelType::Net;
        lastResultValue = InferenceResult::Success;
    }

    /// Initialize with an emlearn-generated model function.
    ///
    /// emlearn generates inline C code with a predict() function that takes
    /// quantized int16_t features. This method wraps that function behind
    /// the AiMicroModelController interface so the Device can interact with
    /// the model through a uniform API.
    ///
    /// @param predictFunction Pointer to the generated predict function.
    ///   Signature: int32_t (*)(const int16_t* features, int32_t nFeatures)
    void loadGeneratedModel(int32_t (*predictFunction)(const int16_t*, int32_t)) {
        if (predictFunction == nullptr) {
            modelType = ModelType::None;
            generatedPredictFunction = nullptr;
            lastResultValue = InferenceResult::ModelNotLoaded;
            return;
        }
        generatedPredictFunction = predictFunction;
        treeModel = nullptr;
        netModel = nullptr;
        modelType = ModelType::Generated;
        lastResultValue = InferenceResult::Success;
    }

    int32_t predict(const float* features, const int32_t numberOfFeatures) override {
        if (features == nullptr || numberOfFeatures <= 0) {
            lastResultValue = InferenceResult::InvalidFeatures;
            return -1;
        }
        if (modelType == ModelType::None) {
            lastResultValue = InferenceResult::ModelNotLoaded;
            return -1;
        }

        int32_t result = -1;

        switch (modelType) {
            case ModelType::Generated:
                if (generatedPredictFunction == nullptr) {
                    lastResultValue = InferenceResult::Error;
                    return -1;
                }
                {
                    int16_t quantizedFeatures[MAX_FEATURES];
                    const int32_t count = quantizeFeatures(features, numberOfFeatures,
                                                     quantizedFeatures, MAX_FEATURES);
                    result = generatedPredictFunction(quantizedFeatures, count);
                }
                break;

            case ModelType::Trees:
                if (treeModel == nullptr) {
                    lastResultValue = InferenceResult::Error;
                    return -1;
                }
                {
                    int16_t quantizedFeatures[MAX_FEATURES];
                    const int32_t count = quantizeFeatures(features, numberOfFeatures,
                                                     quantizedFeatures, MAX_FEATURES);
                    result = eml_trees_predict(treeModel, quantizedFeatures,
                                               static_cast<int8_t>(count));
                }
                break;

            case ModelType::Net:
                if (netModel == nullptr) {
                    lastResultValue = InferenceResult::Error;
                    return -1;
                }
                result = eml_net_predict(netModel, features, numberOfFeatures);
                break;

            case ModelType::None:
                lastResultValue = InferenceResult::ModelNotLoaded;
                return -1;
        }

        lastResultValue = InferenceResult::Success;
        return result;
    }

    float regress(const float* features, const int32_t numberOfFeatures) override {
        if (features == nullptr || numberOfFeatures <= 0) {
            lastResultValue = InferenceResult::InvalidFeatures;
            return NAN;
        }
        if (modelType == ModelType::None) {
            lastResultValue = InferenceResult::ModelNotLoaded;
            return NAN;
        }

        float result = NAN;

        switch (modelType) {
            case ModelType::Generated:
                // Generated models are classification-only; regress returns
                // the predicted class as a float value.
                {
                    const int32_t predictedClass = predict(features, numberOfFeatures);
                    result = (predictedClass < 0) ? NAN : static_cast<float>(predictedClass);
                    if (predictedClass < 0) {
                        lastResultValue = InferenceResult::Error;
                    }
                }
                break;

            case ModelType::Trees:
                if (treeModel == nullptr) {
                    lastResultValue = InferenceResult::Error;
                    return NAN;
                }
                {
                    int16_t quantizedFeatures[MAX_FEATURES];
                    const int32_t count = quantizeFeatures(features, numberOfFeatures,
                                                     quantizedFeatures, MAX_FEATURES);
                    result = eml_trees_regress(treeModel, quantizedFeatures,
                                               static_cast<int8_t>(count));
                }
                break;

            case ModelType::Net:
                if (netModel == nullptr) {
                    lastResultValue = InferenceResult::Error;
                    return NAN;
                }
                {
                    const int32_t predictedClassIndex = eml_net_predict(netModel, features, numberOfFeatures);
                    result = static_cast<float>(predictedClassIndex);
                }
                break;

            case ModelType::None:
                lastResultValue = InferenceResult::ModelNotLoaded;
                return NAN;
        }

        lastResultValue = InferenceResult::Success;
        return result;
    }

    bool isModelLoaded() const override {
        return modelType != ModelType::None;
    }

    InferenceResult lastResult() const override {
        return lastResultValue;
    }

private:
    /// Maximum number of features supported for quantization.
    static constexpr int32_t MAX_FEATURES = 32;

    /// Convert float features to int16_t with NaN/Inf clamping.
    /// @param sourceFeatures Source float array.
    /// @param sourceCount Number of source elements.
    /// @param quantizedFeatures Destination int16_t array.
    /// @param capacity Capacity of destination array.
    /// @return Number of elements written to quantizedFeatures (min of sourceCount and capacity).
    static int32_t quantizeFeatures(const float* sourceFeatures, const int32_t sourceCount,
                                     int16_t* quantizedFeatures, const int32_t capacity) {
        const int32_t count = (sourceCount < capacity) ? sourceCount : capacity;
        for (int32_t i = 0; i < count; i++) {
            if (const float v = sourceFeatures[i]; std::isnan(v) || std::isinf(v)) {
                quantizedFeatures[i] = 0;
            } else {
                quantizedFeatures[i] = static_cast<int16_t>(v);
            }
        }
        return count;
    }

    enum class ModelType : uint8_t {
        None,
        Trees,
        Net,
        Generated
    };

    ModelType modelType;
    const EmlTrees* treeModel;
    const EmlNet* netModel;
    int32_t (*generatedPredictFunction)(const int16_t*, int32_t);
    InferenceResult lastResultValue;
};
