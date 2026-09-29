#pragma once
#include "Math.hpp"
#include "TireModel.hpp"
#include "Suspension.hpp"
#include "Drivetrain.hpp"
#include <cmath>

namespace Pursuit {
namespace Physics {

struct VehicleInputs {
    float throttle{0.0f};  // 0.0 to 1.0
    float brake{0.0f};     // 0.0 to 1.0
    float steering{0.0f};  // -1.0 (full left) to +1.0 (full right)
    bool handbrake{false};
    bool nitro{false};
};

class Vehicle {
public:
    // Dimensions and Inertial Properties
    float mass{1480.0f};              // kg (High-performance sports coupe)
    float momentOfInertiaZ{2150.0f};  // kg*m^2 (Yaw inertia)
    float wheelbase{2.72f};           // meters
    float trackWidth{1.62f};          // meters
    float cgToFront{1.28f};           // distance from CG to front axle (m)
    float cgToRear{1.44f};            // distance from CG to rear axle (m)
    float cgHeight{0.48f};            // Center of gravity height above ground (m)

    // Aerodynamics
    float frontalArea{2.15f};         // m^2
    float dragCoeff{0.32f};           // Cd
    float downforceCoeff{0.45f};      // Cl (GT-wing downforce)
    constexpr static float airDensity{1.225f}; // kg/m^3

    // State Variables (2D Rigid Body with 3D orientation & suspension)
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f}; // Local car coordinates: x=lateral, z=forward
    float yawAngle{0.0f};                  // World yaw angle in radians
    float yawRate{0.0f};                   // Angular velocity (rad/s)
    float steeringAngle{0.0f};             // Front wheel steer angle in radians
    float maxSteerAngle{38.0f * Math::DEG2RAD};

    // Subsystems
    TireModel tireFL, tireFR, tireRL, tireRR;
    SuspensionSystem suspension;
    Drivetrain drivetrain;

    // Braking system
    float maxBrakeTorque{3800.0f}; // Nm total
    float brakeBiasFront{0.68f};   // 68% front, 32% rear
    float handbrakeTorque{4500.0f};

    // Damage & Health
    float health{100.0f};
    bool isWrecked{false};

    Vehicle() {
        // Rear tire width/grip upgrade for high-powered RWD
        tireRL.lateralCoeffs.D = 1.30f;
        tireRR.lateralCoeffs.D = 1.30f;
    }

    float GetSpeedKmh() const {
        return velocity.z * 3.6f;
    }

    Math::Vec3 GetForwardVector() const {
        return { std::sin(yawAngle), 0.0f, std::cos(yawAngle) };
    }

    Math::Vec3 GetRightVector() const {
        return { std::cos(yawAngle), 0.0f, -std::sin(yawAngle) };
    }

    // Step physics at high frequency (e.g. 120-240Hz sub-stepping)
    void Step(const VehicleInputs& inputs, float dt) {
        if (isWrecked) return;

        // 1. Dynamic Speed-Sensitive Steering
        // At high speeds, steer angle is progressively limited to prevent instant spins
        float currentSpeed = std::abs(velocity.z);
        float speedRatio = Math::Clamp(currentSpeed / 60.0f, 0.0f, 1.0f);
        float dynamicMaxSteer = Math::Lerp(maxSteerAngle, 14.0f * Math::DEG2RAD, speedRatio);
        
        float targetSteer = inputs.steering * dynamicMaxSteer;
        steeringAngle = Math::Lerp(steeringAngle, targetSteer, Math::Clamp(dt * 15.0f, 0.0f, 1.0f));

        // 2. Aerodynamic Forces
        float speedSq = velocity.z * velocity.z;
        float aeroDragForce = 0.5f * airDensity * dragCoeff * frontalArea * speedSq * (velocity.z > 0 ? 1.0f : -1.0f);
        float aeroDownforce = 0.5f * airDensity * downforceCoeff * frontalArea * speedSq;

        // 3. Dynamic Weight Transfer and Suspension Loads
        float longAccel = (velocity.z > 0 ? 1.0f : -1.0f) * (velocity.z * 0.1f); // Estimated acceleration
        float latAccel = velocity.z * yawRate;

        float fzFL, fzFR, fzRL, fzRR;
        suspension.ComputeLoads(mass, cgToFront, cgToRear, trackWidth,
                                longAccel, latAccel, cgHeight,
                                fzFL, fzFR, fzRL, fzRR);

        // Add aero downforce split 40% front / 60% rear
        fzFL += aeroDownforce * 0.20f;
        fzFR += aeroDownforce * 0.20f;
        fzRL += aeroDownforce * 0.30f;
        fzRR += aeroDownforce * 0.30f;

        tireFL.normalLoad = fzFL;
        tireFR.normalLoad = fzFR;
        tireRL.normalLoad = fzRL;
        tireRR.normalLoad = fzRR;

        // 4. Drivetrain update & Torque distribution
        drivetrain.Update(inputs.throttle, dt, tireRL.angularVelocity, tireRR.angularVelocity);

        float driveTorqueL = 0.0f, driveTorqueR = 0.0f;
        drivetrain.DistributeTorque(inputs.throttle, tireRL.angularVelocity, tireRR.angularVelocity,
                                   driveTorqueL, driveTorqueR);

        // 5. Brake Torques
        float frontBrakeTorque = inputs.brake * maxBrakeTorque * brakeBiasFront * 0.5f;
        float rearBrakeTorque  = inputs.brake * maxBrakeTorque * (1.0f - brakeBiasFront) * 0.5f;

        if (inputs.handbrake) {
            rearBrakeTorque += handbrakeTorque;
        }

        // 6. Wheel Local Velocities
        // Wheel positions relative to CG:
        // FL: (+track/2, +cgToFront)
        // FR: (-track/2, +cgToFront)
        // RL: (+track/2, -cgToRear)
        // RR: (-track/2, -cgToRear)
        float halfTrack = trackWidth * 0.5f;

        float vLongFL = velocity.z - (yawRate * halfTrack);
        float vLatFL  = velocity.x + (yawRate * cgToFront);

        float vLongFR = velocity.z + (yawRate * halfTrack);
        float vLatFR  = velocity.x + (yawRate * cgToFront);

        float vLongRL = velocity.z - (yawRate * halfTrack);
        float vLatRL  = velocity.x - (yawRate * cgToRear);

        float vLongRR = velocity.z + (yawRate * halfTrack);
        float vLatRR  = velocity.x - (yawRate * cgToRear);

        // Transform front wheels into steered frame:
        float cosSteer = std::cos(steeringAngle);
        float sinSteer = std::sin(steeringAngle);

        float steeredLongFL =  vLongFL * cosSteer + vLatFL * sinSteer;
        float steeredLatFL  = -vLongFL * sinSteer + vLatFL * cosSteer;

        float steeredLongFR =  vLongFR * cosSteer + vLatFR * sinSteer;
        float steeredLatFR  = -vLongFR * sinSteer + vLatFR * cosSteer;

        // 7. Calculate Tire Forces using Pacejka Magic Formula
        float fxFL, fyFL, fxFR, fyFR, fxRL, fyRL, fxRR, fyRR;
        tireFL.CalculateForces(steeredLongFL, steeredLatFL, 0.0f, frontBrakeTorque, dt, fxFL, fyFL);
        tireFR.CalculateForces(steeredLongFR, steeredLatFR, 0.0f, frontBrakeTorque, dt, fxFR, fyFR);
        tireRL.CalculateForces(vLongRL, vLatRL, driveTorqueL, rearBrakeTorque, dt, fxRL, fyRL);
        tireRR.CalculateForces(vLongRR, vLatRR, driveTorqueR, rearBrakeTorque, dt, fxRR, fyRR);

        // Rotate front tire forces back to vehicle chassis frame:
        float chassisFxFL = fxFL * cosSteer - fyFL * sinSteer;
        float chassisFyFL = fxFL * sinSteer + fyFL * cosSteer;

        float chassisFxFR = fxFR * cosSteer - fyFR * sinSteer;
        float chassisFyFR = fxFR * sinSteer + fyFR * cosSteer;

        // 8. Sum Forces and Moments on Chassis
        float totalFx = chassisFxFL + chassisFxFR + fxRL + fxRR - aeroDragForce;
        float totalFy = chassisFyFL + chassisFyFR + fyRL + fyRR;

        // Yaw Moment (Torque around vertical axis):
        float yawMoment =
            (chassisFyFL + chassisFyFR) * cgToFront -
            (fyRL + fyRR) * cgToRear +
            (chassisFxFR - chassisFxFL) * halfTrack +
            (fxRR - fxRL) * halfTrack;

        // 9. Rigid Body Newton-Euler Integration
        // a_z = F_z / m + v_x * r (Coriolis term)
        // a_x = F_x / m - v_z * r
        float accelZ = (totalFx / mass) + (velocity.x * yawRate);
        float accelX = (totalFy / mass) - (velocity.z * yawRate);
        float angularAccelYaw = yawMoment / momentOfInertiaZ;

        velocity.z += accelZ * dt;
        velocity.x += accelX * dt;
        yawRate += angularAccelYaw * dt;

        // High-damping when almost stopped to prevent numeric creep
        if (std::abs(velocity.z) < 0.1f && std::abs(inputs.throttle) < 0.05f && inputs.brake > 0.1f) {
            velocity.z = 0.0f;
            velocity.x = 0.0f;
            yawRate = 0.0f;
        }

        // 10. Integrate World Coordinates
        yawAngle += yawRate * dt;
        
        // Wrap yaw angle to [-pi, pi]
        if (yawAngle > Math::PI) yawAngle -= 2.0f * Math::PI;
        if (yawAngle < -Math::PI) yawAngle += 2.0f * Math::PI;

        Math::Vec3 forward = GetForwardVector();
        Math::Vec3 right = GetRightVector();

        Math::Vec3 worldVelocity = forward * velocity.z + right * velocity.x;
        position += worldVelocity * dt;
    }

    void ApplyImpulse(const Math::Vec3& impulse, const Math::Vec3& contactPointRel) {
        velocity.x += impulse.x / mass;
        velocity.z += impulse.z / mass;
        yawRate += (contactPointRel.x * impulse.z - contactPointRel.z * impulse.x) / momentOfInertiaZ;
        health -= impulse.Length() * 0.005f;
        if (health <= 0.0f) {
            health = 0.0f;
            isWrecked = true;
        }
    }
};

} // namespace Physics
} // namespace Pursuit
