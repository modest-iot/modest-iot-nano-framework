// Copyright (C) 2026 Modest IoT
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

/**
 * Result of an ML inference operation.
 * @ingroup core_module
 */
enum class InferenceResult : uint8_t {
    Success,          ///< Inference completed successfully.
    Error,            ///< Generic error during inference.
    ModelNotLoaded,   ///< No model has been loaded.
    InvalidFeatures   ///< Features pointer is null or length is zero.
};

/**
 * Abstract base class for ML inference on microcontrollers.
 *
 * This interface provides a uniform API for running classification and
 * regression inference on embedded devices. Concrete implementations wrap
 * specific ML runtimes (e.g., emlearn, TensorFlow Lite Micro).
 *
 * The controller is a domain service — not a Sensor or Actuator. It is a
 * pure computation service: given features, return a prediction. Devices
 * call it in their on(Event) handler after reading sensor values.
 * @ingroup core_module
 */
class AiMicroModelController {
public:
    virtual ~AiMicroModelController() = default;

    /// Run classification inference on the given features.
    /// @param features Array of feature values.
    /// @param numberOfFeatures Number of features in the array.
    /// @return Predicted class index (>= 0), or -1 on error.
    virtual int32_t predict(const float* features, int32_t numberOfFeatures) = 0;

    /// Run regression inference on the given features.
    /// @param features Array of feature values.
    /// @param numberOfFeatures Number of features in the array.
    /// @return Predicted continuous value, or NAN on error.
    virtual float regress(const float* features, int32_t numberOfFeatures) = 0;

    /// Check if a model is loaded and ready for inference.
    /// @return True if a model is loaded and ready.
    virtual bool isModelLoaded() const = 0;

    /// Get the result of the last inference operation.
    /// @return InferenceResult status of the last predict() or regress() call.
    virtual InferenceResult lastResult() const = 0;
};
