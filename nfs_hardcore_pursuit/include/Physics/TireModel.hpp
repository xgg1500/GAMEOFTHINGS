#pragma once
#include "Math.hpp"
#include <cmath>

namespace Pursuit {
namespace Physics {

// Advanced Pacejka '96 Magic Formula parameters
struct PacejkaCoefficients {
    float B{10.0f};  // Stiffness factor
    float C{1.35f};  // Shape factor
    float D{1.15f};  // Peak friction coefficient (dry tarmac)
    float E{-0.85f}; // Curvature factor
};

class TireModel {
public:
    PacejkaCoefficients lateralCoeffs;
    PacejkaCoefficients longCoeffs;

    // Tire physical properties
    float radius{0.33f};           // meters (~18-19" racing wheel)
    float mass{18.0f};             // kg
    float inertia{0.5f * 18.0f * 0.33f * 0.33f}; // 0.5 * m * r^2
    float angularVelocity{0.0f};   // rad/s
    float slipAngle{0.0f};         // radians
    float slipRatio{0.0f};         // dimensionless (-1.0 to +inf)
    float normalLoad{4000.0f};     // Newtons (dynamic vertical force Fz)
    float frictionMultiplier{1.0f};// Surface condition (1.0 dry, 0.65 wet, 0.1 oil/thermite)
    bool isBlown{false};           // Spike strip puncture status

    TireModel() {
        // High-performance semi-slick compound defaults
        lateralCoeffs = { 9.5f, 1.40f, 1.25f, -0.60f };
        longCoeffs    = { 11.0f, 1.55f, 1.30f, -0.75f };
    }

    // Evaluate Pacejka Magic Formula curve
    static float EvaluatePacejka(float slip, float Fz, const PacejkaCoefficients& c, float muScale) {
        if (Fz <= 10.0f) return 0.0f;

        // Tire load sensitivity: friction coefficient drops slightly as normal load increases
        float loadFactor = 1.0f - 0.000035f * std::max(0.0f, Fz - 4000.0f);
        float peakFriction = c.D * muScale * std::max(0.45f, loadFactor);
        float D = peakFriction * Fz;

        float Bx = c.B * slip;
        float force = D * std::sin(c.C * std::atan(Bx - c.E * (Bx - std::atan(Bx))));
        return force;
    }

    // Calculate forces with friction ellipse (combined slip)
    void CalculateForces(float forwardSpeed, float lateralSpeed, float driveTorque, float brakeTorque,
                         float dt, float& outFx, float& outFy) {
        if (isBlown) {
            // Blown tire from spike strip: severe friction loss and violent resistance
            frictionMultiplier = 0.22f;
        }

        // 1. Calculate Longitudinal Slip Ratio (kappa)
        // kappa = (omega * r - V_x) / max(|V_x|, epsilon)
        float wheelLinearSpeed = angularVelocity * radius;
        float absForwardSpeed = std::abs(forwardSpeed);
        float denomSpeed = std::max(absForwardSpeed, 1.0f);

        slipRatio = (wheelLinearSpeed - forwardSpeed) / denomSpeed;
        slipRatio = Math::Clamp(slipRatio, -2.5f, 2.5f);

        // 2. Calculate Lateral Slip Angle (alpha)
        // alpha = -arctan(V_y / max(|V_x|, epsilon))
        if (absForwardSpeed > 0.3f) {
            slipAngle = -std::atan2(lateralSpeed, absForwardSpeed);
        } else {
            slipAngle = 0.0f;
        }

        // 3. Raw pure forces
        float pureFx = EvaluatePacejka(slipRatio, normalLoad, longCoeffs, frictionMultiplier);
        float pureFy = EvaluatePacejka(slipAngle, normalLoad, lateralCoeffs, frictionMultiplier);

        // 4. Combined slip via Friction Ellipse (Kamm Circle)
        // Eliminates non-physical super-grip during simultaneous hard braking + cornering
        float maxGrip = frictionMultiplier * normalLoad * 1.25f;
        float totalForceMag = std::sqrt(pureFx * pureFx + pureFy * pureFy);

        if (totalForceMag > maxGrip && totalForceMag > 1e-4f) {
            float scale = maxGrip / totalForceMag;
            outFx = pureFx * scale;
            outFy = pureFy * scale;
        } else {
            outFx = pureFx;
            outFy = pureFy;
        }

        // 5. Update Wheel Rotational Dynamics
        // I * d(omega)/dt = DriveTorque - BrakeTorque*sign(omega) - Fx * r
        float brakingTorqueApplied = 0.0f;
        if (std::abs(angularVelocity) > 0.05f) {
            brakingTorqueApplied = (angularVelocity > 0.0f ? 1.0f : -1.0f) * brakeTorque;
        } else if (brakeTorque > 0.0f) {
            // Lock wheel when stationary
            angularVelocity = 0.0f;
        }

        float netTorque = driveTorque - brakingTorqueApplied - (outFx * radius);
        float angularAccel = netTorque / inertia;
        angularVelocity += angularAccel * dt;

        // Prevent numerical oscillation near 0 speed
        if (brakeTorque > 0.0f && std::abs(angularVelocity) < 0.2f && std::abs(driveTorque) < 1.0f) {
            angularVelocity = 0.0f;
        }
    }
};

} // namespace Physics
} // namespace Pursuit
