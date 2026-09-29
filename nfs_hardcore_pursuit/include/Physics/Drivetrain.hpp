#pragma once
#include "Math.hpp"
#include <vector>
#include <cmath>

namespace Pursuit {
namespace Physics {

enum class GearboxMode {
    Manual,
    Automatic
};

class Drivetrain {
public:
    // Engine parameters
    float idleRPM{950.0f};
    float maxRPM{8600.0f};
    float currentRPM{950.0f};
    float peakTorque{780.0f};        // Nm (High-output twin-turbo V8)
    float engineInertia{0.22f};      // kg*m^2
    bool revLimiterActive{false};
    float revLimiterTimer{0.0f};

    // Gearbox
    int currentGear{1};              // -1: Reverse, 0: Neutral, 1-6: Forward
    GearboxMode mode{GearboxMode::Automatic};
    std::vector<float> gearRatios{3.82f, 2.36f, 1.68f, 1.31f, 1.00f, 0.79f}; // 1st to 6th
    float reverseRatio{-3.55f};
    float finalDriveRatio{3.73f};
    float shiftDelayTimer{0.0f};
    bool isShifting{false};

    // Limited Slip Differential (LSD)
    float lsdLockRatio{0.75f};       // 75% Salisbury 2-way lockup for aggressive drifting
    float driveEfficiency{0.88f};    // Drivetrain mechanical efficiency

    // EMP impact state
    bool empDisabled{false};
    float empTimer{0.0f};

    // Get current gear ratio
    float GetCurrentRatio() const {
        if (currentGear == 0 || isShifting) return 0.0f;
        if (currentGear == -1) return reverseRatio * finalDriveRatio;
        if (currentGear >= 1 && currentGear <= (int)gearRatios.size()) {
            return gearRatios[currentGear - 1] * finalDriveRatio;
        }
        return 0.0f;
    }

    // Realistic engine torque curve (peaking at mid-high RPM)
    float SampleEngineTorque(float rpm) const {
        float normRPM = Math::Clamp(rpm, idleRPM, maxRPM);
        // Normalized RPM from 0 to 1
        float x = (normRPM - idleRPM) / (maxRPM - idleRPM);
        
        // Quadratic polynomial curve peak around x=0.55 (~5000 RPM)
        float shape = -3.8f * (x - 0.55f) * (x - 0.55f) + 1.0f;
        shape = Math::Clamp(shape, 0.40f, 1.0f);
        return peakTorque * shape;
    }

    void Update(float throttle, float dt, float wheelOmegaLeft, float wheelOmegaRight) {
        if (empDisabled) {
            empTimer -= dt;
            if (empTimer <= 0.0f) {
                empDisabled = false;
            } else {
                throttle = 0.0f; // EMP completely cuts ignition / fuel injection!
            }
        }

        if (shiftDelayTimer > 0.0f) {
            shiftDelayTimer -= dt;
            if (shiftDelayTimer <= 0.0f) {
                isShifting = false;
            }
        }

        // Average driven wheel angular velocity
        float avgWheelOmega = 0.5f * (wheelOmegaLeft + wheelOmegaRight);
        float gearRatio = GetCurrentRatio();

        if (currentGear != 0 && !isShifting) {
            // Engine coupled to wheels
            float targetRPM = std::abs(avgWheelOmega * gearRatio) * (60.0f / (2.0f * Math::PI));
            currentRPM = Math::Lerp(currentRPM, std::max(idleRPM, targetRPM), 0.25f);
        } else {
            // Free revving in neutral or clutch-in
            float revTorque = throttle * peakTorque * 0.8f - (currentRPM * 0.04f);
            float rpmAccel = (revTorque / engineInertia) * (60.0f / (2.0f * Math::PI));
            currentRPM += rpmAccel * dt;
        }

        // Hardcut Rev Limiter
        if (currentRPM >= maxRPM) {
            currentRPM = maxRPM;
            revLimiterActive = true;
            revLimiterTimer = 0.08f; // pop & flame cut
        }

        if (revLimiterActive) {
            revLimiterTimer -= dt;
            if (revLimiterTimer <= 0.0f) {
                revLimiterActive = false;
            }
        }

        currentRPM = Math::Clamp(currentRPM, idleRPM, maxRPM + 150.0f);

        // Automatic Transmission Logic
        if (mode == GearboxMode::Automatic && !isShifting) {
            if (currentGear > 0) {
                if (currentRPM > 7200.0f && currentGear < (int)gearRatios.size()) {
                    ShiftUp();
                } else if (currentRPM < 2800.0f && currentGear > 1) {
                    ShiftDown();
                }
            }
        }
    }

    void ShiftUp() {
        if (currentGear < (int)gearRatios.size() && !isShifting) {
            currentGear++;
            isShifting = true;
            shiftDelayTimer = 0.12f; // Fast dual-clutch shift
        }
    }

    void ShiftDown() {
        if (currentGear > 1 && !isShifting) {
            currentGear--;
            isShifting = true;
            shiftDelayTimer = 0.15f;
        }
    }

    // Split driveshaft torque through Limited Slip Differential (LSD)
    void DistributeTorque(float throttle, float wheelOmegaLeft, float wheelOmegaRight,
                          float& outTorqueLeft, float& outTorqueRight) {
        if (currentGear == 0 || isShifting || revLimiterActive || empDisabled) {
            outTorqueLeft = 0.0f;
            outTorqueRight = 0.0f;
            return;
        }

        float engineTorque = SampleEngineTorque(currentRPM) * throttle;
        float gearRatio = GetCurrentRatio();
        float driveshaftTorque = engineTorque * gearRatio * driveEfficiency;

        // Differential speed difference
        float omegaDiff = wheelOmegaRight - wheelOmegaLeft;
        float bias = lsdLockRatio * Math::Clamp(omegaDiff * 0.15f, -0.4f, 0.4f);

        outTorqueLeft  = driveshaftTorque * (0.5f + bias);
        outTorqueRight = driveshaftTorque * (0.5f - bias);
    }
};

} // namespace Physics
} // namespace Pursuit
