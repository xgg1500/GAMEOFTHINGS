#pragma once
#include "../Math.hpp"
#include "../Physics/Vehicle.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

namespace Pursuit {
namespace Police {

enum class UnitType {
    PatrolCruiser,       // Heat 1-2
    HighwayInterceptor,  // Heat 3-5
    TacticalRhino,       // Heat 4-10 (Heavy ramming armored SUV)
    UndercoverSupercar,  // Heat 6-10 (Ultra high-speed)
    HelicopterSupport    // Air visual & weapon support
};

enum class WeaponType {
    None,
    EMPDisruptor,
    ThermiteBomb,
    SpikeStrip
};

struct ThermiteTrap {
    Math::Vec3 position;
    float radius{6.5f};      // meters
    float lifetime{15.0f};   // burns for 15 seconds
    bool active{true};
};

struct SpikeTrap {
    Math::Vec3 position;
    float width{5.0f};
    bool active{true};
};

class PoliceUnit {
public:
    int id{0};
    UnitType type{UnitType::PatrolCruiser};
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    float yawAngle{0.0f};
    float topSpeedKmh{220.0f};
    float acceleration{14.0f}; // m/s^2
    float mass{1750.0f};       // kg
    float health{100.0f};
    bool isDestroyed{false};

    // Weapon capabilities
    bool hasEMP{false};
    float empLockonTimer{0.0f};
    float empCooldown{0.0f};
    bool isLockingOnEMP{false};

    bool hasThermite{false};
    float thermiteCooldown{0.0f};

    void ConfigureForHeat(int heatLevel, int unitIndex) {
        empCooldown = 5.0f + (unitIndex % 4) * 2.0f;
        thermiteCooldown = 8.0f + (unitIndex % 3) * 3.0f;

        if (heatLevel >= 7) {
            hasEMP = true;
        }
        if (heatLevel >= 8) {
            hasThermite = true;
        }

        if (heatLevel <= 2) {
            type = UnitType::PatrolCruiser;
            topSpeedKmh = 230.0f;
            mass = 1800.0f;
            health = 100.0f;
        } else if (heatLevel <= 4) {
            if (unitIndex % 3 == 0) {
                type = UnitType::TacticalRhino;
                topSpeedKmh = 210.0f;
                mass = 3800.0f; // Heavy armored ramming ram
                health = 250.0f;
            } else {
                type = UnitType::HighwayInterceptor;
                topSpeedKmh = 280.0f;
                mass = 1650.0f;
                health = 140.0f;
            }
        } else if (heatLevel <= 7) {
            if (unitIndex % 4 == 0) {
                type = UnitType::TacticalRhino;
                topSpeedKmh = 240.0f;
                mass = 4200.0f;
                health = 320.0f;
            } else {
                type = UnitType::UndercoverSupercar;
                topSpeedKmh = 320.0f;
                mass = 1550.0f;
                health = 180.0f;
            }
        } else {
            // Heat 8 to 10
            if (unitIndex % 5 == 0) {
                type = UnitType::TacticalRhino;
                topSpeedKmh = 260.0f;
                mass = 4800.0f; // Heavy assault tank-like Rhino
                health = 450.0f;
            } else {
                type = UnitType::UndercoverSupercar;
                topSpeedKmh = 350.0f; // Extreme pursuit interceptors
                mass = 1600.0f;
                health = 220.0f;
            }
        }
    }

    std::string GetTypeName() const {
        switch (type) {
            case UnitType::PatrolCruiser: return "Patrol Cruiser (Crown Vic)";
            case UnitType::HighwayInterceptor: return "Highway Interceptor (Corvette C8)";
            case UnitType::TacticalRhino: return "Tactical Rhino (Armored BearCat)";
            case UnitType::UndercoverSupercar: return "Undercover Interceptor (Ford GT)";
            case UnitType::HelicopterSupport: return "Air Support Helicopter";
        }
        return "Unknown";
    }

    void UpdateAI(Physics::Vehicle& player, float dt,
                  std::vector<ThermiteTrap>& worldThermites,
                  bool& outEmpFired) {
        if (isDestroyed) return;

        outEmpFired = false;
        if (empCooldown > 0.0f) empCooldown -= dt;
        if (thermiteCooldown > 0.0f) thermiteCooldown -= dt;

        Math::Vec3 toPlayer = player.position - position;
        float distToPlayer = toPlayer.Length();

        // 1. Driving pursuit behavior
        float desiredSpeedMs = (topSpeedKmh / 3.6f);
        Math::Vec3 targetDir = toPlayer.Normalized();

        if (type == UnitType::TacticalRhino) {
            // Rhino: Tries to set up head-on ramming trajectories
            Math::Vec3 playerForward = player.GetForwardVector();
            Math::Vec3 interceptPoint = player.position + playerForward * (player.velocity.z * 1.5f);
            targetDir = (interceptPoint - position).Normalized();
        }

        // Steer towards target
        yawAngle = std::atan2(targetDir.x, targetDir.z);
        Math::Vec3 desiredVel = targetDir * desiredSpeedMs;
        velocity.x = Math::Lerp(velocity.x, desiredVel.x, Math::Clamp(dt * 3.5f, 0.0f, 1.0f));
        velocity.z = Math::Lerp(velocity.z, desiredVel.z, Math::Clamp(dt * 3.5f, 0.0f, 1.0f));
        position += velocity * dt;

        // 2. Collision / Ramming with Player
        if (distToPlayer < 4.2f) {
            Math::Vec3 relVel = velocity - (player.GetForwardVector() * player.velocity.z);
            float impactForce = relVel.Length() * mass * 0.4f;

            Math::Vec3 impulse = targetDir * impactForce;
            player.ApplyImpulse(impulse, Math::Vec3(0, 0, 1.2f));
            health -= impactForce * 0.002f;
            if (health <= 0.0f) isDestroyed = true;
        }

        // 3. EMP Weapon Logic (Heat 7-10)
        if (hasEMP && empCooldown <= 0.0f && distToPlayer < 45.0f && distToPlayer > 8.0f) {
            if (!isLockingOnEMP) {
                isLockingOnEMP = true;
                empLockonTimer = 3.2f; // 3.2 seconds lock-on warning
            } else {
                empLockonTimer -= dt;
                if (empLockonTimer <= 0.0f) {
                    // FIRE EMP!
                    outEmpFired = true;
                    player.drivetrain.empDisabled = true;
                    player.drivetrain.empTimer = 4.0f; // 4 seconds engine ignition cut
                    isLockingOnEMP = false;
                    empCooldown = 22.0f; // Recharge cooldown
                }
            }
        } else if (distToPlayer >= 50.0f) {
            isLockingOnEMP = false;
        }

        // 4. Thermite Bomb Weapon Logic (Heat 8-10)
        if (hasThermite && thermiteCooldown <= 0.0f && distToPlayer < 30.0f) {
            // Drop thermite trap ahead of player or behind chasing cop
            ThermiteTrap trap;
            trap.position = position + (targetDir * 12.0f);
            trap.lifetime = 15.0f;
            trap.active = true;
            worldThermites.push_back(trap);

            thermiteCooldown = 28.0f; // Cooldown between drops
        }
    }
};

class HeatEscalationDirector {
public:
    int currentHeatLevel{1};
    float timeInPursuitSeconds{0.0f};
    constexpr static float SECONDS_PER_HEAT = 300.0f; // Exactly 5 minutes (300 seconds) per heat level
    constexpr static int MAX_HEAT_LEVEL = 10;
    constexpr static int HEAT_10_POLICE_COUNT = 20;   // Exactly 20 police units at Heat 10

    std::vector<PoliceUnit> activeUnits;
    std::vector<ThermiteTrap> activeThermites;
    std::vector<SpikeTrap> activeSpikes;

    bool lastEmpShockwaveTriggered{false};
    float empEffectTimer{0.0f};

    // Calculate required police count based on heat level
    int GetTargetPoliceCount(int heat) const {
        switch (heat) {
            case 1: return 2;
            case 2: return 4;
            case 3: return 6;
            case 4: return 8;
            case 5: return 10;
            case 6: return 12;
            case 7: return 14;
            case 8: return 16;
            case 9: return 18;
            case 10: return HEAT_10_POLICE_COUNT; // 20 units
            default: return 20;
        }
    }

    void Init() {
        currentHeatLevel = 1;
        timeInPursuitSeconds = 0.0f;
        SpawnUnitsForLevel(currentHeatLevel);
    }

    void SpawnUnitsForLevel(int heat) {
        int targetCount = GetTargetPoliceCount(heat);
        activeUnits.clear();

        for (int i = 0; i < targetCount; ++i) {
            PoliceUnit u;
            u.id = i + 1;
            u.ConfigureForHeat(heat, i);

            // Spawn around player perimeter (behind, ahead, or flanks)
            float angle = (float)i * (2.0f * Math::PI / (float)targetCount);
            float spawnDist = 45.0f + (float)(i % 5) * 8.0f;
            u.position = { std::sin(angle) * spawnDist, 0.0f, std::cos(angle) * spawnDist };
            activeUnits.push_back(u);
        }
    }

    void Update(Physics::Vehicle& player, float dt) {
        timeInPursuitSeconds += dt;

        // Level increases every 5 minutes (300 seconds)
        int calculatedHeat = 1 + (int)(timeInPursuitSeconds / SECONDS_PER_HEAT);
        if (calculatedHeat > MAX_HEAT_LEVEL) calculatedHeat = MAX_HEAT_LEVEL;

        if (calculatedHeat != currentHeatLevel) {
            currentHeatLevel = calculatedHeat;
            SpawnUnitsForLevel(currentHeatLevel);
        }

        // Maintain active police count (respawn destroyed units after delay)
        int targetCount = GetTargetPoliceCount(currentHeatLevel);
        while ((int)activeUnits.size() < targetCount) {
            PoliceUnit u;
            u.id = (int)activeUnits.size() + 1;
            u.ConfigureForHeat(currentHeatLevel, u.id);
            u.position = player.position - player.GetForwardVector() * 85.0f; // Reinforcements spawn from rear
            activeUnits.push_back(u);
        }

        // Update all police units & check weapon triggers
        lastEmpShockwaveTriggered = false;
        for (auto& unit : activeUnits) {
            bool empFired = false;
            unit.UpdateAI(player, dt, activeThermites, empFired);
            if (empFired) {
                lastEmpShockwaveTriggered = true;
                empEffectTimer = 1.5f; // Screen glitch duration
            }
        }

        if (empEffectTimer > 0.0f) {
            empEffectTimer -= dt;
        }

        // Update Thermite Traps on asphalt
        for (auto it = activeThermites.begin(); it != activeThermites.end();) {
            it->lifetime -= dt;
            if (it->lifetime <= 0.0f) {
                it = activeThermites.erase(it);
                continue;
            }

            // Check if player is on the thermite zone
            float distSq = (player.position - it->position).LengthSq();
            if (distSq < (it->radius * it->radius)) {
                // Severe thermal damage and loss of tire traction
                player.health -= 22.0f * dt;
                player.tireRL.frictionMultiplier = 0.12f;
                player.tireRR.frictionMultiplier = 0.12f;
                player.tireFL.frictionMultiplier = 0.12f;
                player.tireFR.frictionMultiplier = 0.12f;
                if (player.health <= 0.0f) {
                    player.health = 0.0f;
                    player.isWrecked = true;
                }
            }
            ++it;
        }
    }
};

} // namespace Police
} // namespace Pursuit
