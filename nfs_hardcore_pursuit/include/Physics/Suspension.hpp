#pragma once
#include "Math.hpp"

namespace Pursuit {
namespace Physics {

struct SuspensionCorner {
    float springRate{55000.0f};      // N/m (Stiff sports suspension)
    float bumpDamping{4200.0f};      // N*s/m (Compression damping)
    float reboundDamping{6500.0f};   // N*s/m (Extension damping)
    float restLength{0.35f};         // meters
    float minLength{0.18f};          // bump-stop limit
    float maxLength{0.45f};          // droop limit
    float currentLength{0.35f};
    float compressionVelocity{0.0f};

    float CalculateForce(float dt) {
        float displacement = restLength - currentLength; // positive when compressed
        float springForce = displacement * springRate;

        // Asymmetric damping (rebound is typically higher to control body oscillation)
        float dampingCoeff = (compressionVelocity >= 0.0f) ? bumpDamping : reboundDamping;
        float damperForce = compressionVelocity * dampingCoeff;

        float totalForce = springForce + damperForce;
        return std::max(0.0f, totalForce); // Suspension can only push up, cannot pull down
    }
};

class SuspensionSystem {
public:
    SuspensionCorner frontLeft;
    SuspensionCorner frontRight;
    SuspensionCorner rearLeft;
    SuspensionCorner rearRight;

    float frontAntiRollBarRate{9000.0f};  // N/m anti-roll stiffness
    float rearAntiRollBarRate{7500.0f};

    // Calculate normal loads for all 4 wheels including anti-roll bar load transfer
    void ComputeLoads(float vehicleMass, float cgToFront, float cgToRear, float trackWidth,
                      float longAccel, float latAccel, float cgHeight,
                      float& outFzFL, float& outFzFR, float& outFzRL, float& outFzRR) {
        constexpr float g = 9.81f;
        float totalWeight = vehicleMass * g;
        float wheelBase = cgToFront + cgToRear;

        // Static load distribution
        float staticFront = totalWeight * (cgToRear / wheelBase);
        float staticRear  = totalWeight * (cgToFront / wheelBase);

        // Dynamic longitudinal weight transfer (acceleration squats rear, braking dives front)
        float longLoadTransfer = (vehicleMass * longAccel * cgHeight) / wheelBase;

        float frontAxleLoad = std::max(100.0f, staticFront - longLoadTransfer);
        float rearAxleLoad  = std::max(100.0f, staticRear + longLoadTransfer);

        // Dynamic lateral weight transfer (cornering rolls body onto outside wheels)
        float totalLatTransfer = (vehicleMass * latAccel * cgHeight) / trackWidth;
        
        // Distribution of roll stiffness (front vs rear ARB balance controls understeer/oversteer)
        float rollStiffnessFront = 0.55f;
        float frontLatTransfer = totalLatTransfer * rollStiffnessFront;
        float rearLatTransfer  = totalLatTransfer * (1.0f - rollStiffnessFront);

        // 4 wheel vertical loads
        outFzFL = std::max(200.0f, (frontAxleLoad * 0.5f) - frontLatTransfer);
        outFzFR = std::max(200.0f, (frontAxleLoad * 0.5f) + frontLatTransfer);
        outFzRL = std::max(200.0f, (rearAxleLoad * 0.5f) - rearLatTransfer);
        outFzRR = std::max(200.0f, (rearAxleLoad * 0.5f) + rearLatTransfer);
    }
};

} // namespace Physics
} // namespace Pursuit
