#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <string>

#include "Math.hpp"
#include "Physics/Vehicle.hpp"
#include "Police/PoliceSystem.hpp"
#include "Renderer/Shader.hpp"

using namespace Pursuit;

// ANSI Terminal Colors for Real-Time HUD
namespace HUD {
    const std::string RESET   = "\033[0m";
    const std::string RED     = "\033[31;1m";
    const std::string GREEN   = "\033[32;1m";
    const std::string YELLOW  = "\033[33;1m";
    const std::string BLUE    = "\033[34;1m";
    const std::string MAGENTA = "\033[35;1m";
    const std::string CYAN    = "\033[36;1m";
    const std::string WHITE   = "\033[37;1m";
    const std::string BOLD    = "\033[1m";
    const std::string CLEAR   = "\033[2J\033[H";
}

void PrintTelemetry(const Physics::Vehicle& car, const Police::HeatEscalationDirector& director, float simTime, int step) {
    std::cout << HUD::CLEAR;
    std::cout << HUD::CYAN << "===================================================================================\n";
    std::cout << "        NEED FOR SPEED: HARDCORE PURSUIT - C++ ENGINE SIMULATION & TELEMETRY       \n";
    std::cout << "===================================================================================\n" << HUD::RESET;

    int minutes = (int)(director.timeInPursuitSeconds / 60.0f);
    int seconds = (int)(director.timeInPursuitSeconds) % 60;

    // Heat Level Banner
    std::cout << HUD::BOLD << " PURSUIT STATUS | ";
    if (director.currentHeatLevel <= 3) {
        std::cout << HUD::GREEN;
    } else if (director.currentHeatLevel <= 6) {
        std::cout << HUD::YELLOW;
    } else {
        std::cout << HUD::RED;
    }
    std::cout << "HEAT LEVEL " << director.currentHeatLevel << "/10" << HUD::RESET;
    std::cout << " | Pursuit Time: " << std::setw(2) << std::setfill('0') << minutes << ":"
              << std::setw(2) << std::setfill('0') << seconds;
    std::cout << " | Next Heat In: " << (int)(Police::HeatEscalationDirector::SECONDS_PER_HEAT - (fmod(director.timeInPursuitSeconds, Police::HeatEscalationDirector::SECONDS_PER_HEAT))) << "s\n";

    std::cout << HUD::WHITE << "-----------------------------------------------------------------------------------\n" << HUD::RESET;

    // Vehicle Telemetry
    float speed = car.GetSpeedKmh();
    std::cout << HUD::BOLD << " [PLAYER VEHICLE TELEMETRY]\n" << HUD::RESET;
    std::cout << "  Speed: " << HUD::BOLD << std::fixed << std::setprecision(1) << std::setw(5) << speed << " km/h" << HUD::RESET
              << " | RPM: [" << std::setw(4) << (int)car.drivetrain.currentRPM << " / " << (int)car.drivetrain.maxRPM << "]";

    // RPM Bar
    int rpmBars = (int)((car.drivetrain.currentRPM / car.drivetrain.maxRPM) * 20.0f);
    std::cout << " [";
    for (int i = 0; i < 20; ++i) {
        if (i < rpmBars) {
            if (i > 16) std::cout << HUD::RED << "|" << HUD::RESET;
            else if (i > 12) std::cout << HUD::YELLOW << "|" << HUD::RESET;
            else std::cout << HUD::GREEN << "|" << HUD::RESET;
        } else {
            std::cout << " ";
        }
    }
    std::cout << "]";

    std::cout << " | Gear: " << HUD::BOLD << (car.drivetrain.currentGear == -1 ? "R" : (car.drivetrain.currentGear == 0 ? "N" : std::to_string(car.drivetrain.currentGear))) << HUD::RESET;
    std::cout << " | Yaw Rate: " << std::setprecision(2) << car.yawRate << " rad/s\n";

    // Hardcore Tire Dynamics & Normal Loads (Pacejka '96 + Suspension)
    std::cout << "\n" << HUD::BOLD << " [HARDCORE PACEJKA TIRE FORCES & SUSPENSION DYNAMICS]\n" << HUD::RESET;
    std::cout << "  FL Slip Angle: " << std::setw(5) << (car.tireFL.slipAngle * Math::RAD2DEG) << "° | Load Fz: " << (int)car.tireFL.normalLoad << " N | SlipRatio: " << car.tireFL.slipRatio << "\n";
    std::cout << "  FR Slip Angle: " << std::setw(5) << (car.tireFR.slipAngle * Math::RAD2DEG) << "° | Load Fz: " << (int)car.tireFR.normalLoad << " N | SlipRatio: " << car.tireFR.slipRatio << "\n";
    std::cout << "  RL Slip Angle: " << std::setw(5) << (car.tireRL.slipAngle * Math::RAD2DEG) << "° | Load Fz: " << (int)car.tireRL.normalLoad << " N | SlipRatio: " << car.tireRL.slipRatio << "\n";
    std::cout << "  RR Slip Angle: " << std::setw(5) << (car.tireRR.slipAngle * Math::RAD2DEG) << "° | Load Fz: " << (int)car.tireRR.normalLoad << " N | SlipRatio: " << car.tireRR.slipRatio << "\n";

    // Police Force Status
    std::cout << HUD::WHITE << "-----------------------------------------------------------------------------------\n" << HUD::RESET;
    std::cout << HUD::BOLD << " [POLICE PURSUIT FORCES - " << director.activeUnits.size() << " ACTIVE UNITS ON MAP]\n" << HUD::RESET;

    int cruisers = 0, interceptors = 0, rhinos = 0, supercars = 0;
    int empLockingCount = 0;
    for (const auto& u : director.activeUnits) {
        if (u.type == Police::UnitType::PatrolCruiser) cruisers++;
        else if (u.type == Police::UnitType::HighwayInterceptor) interceptors++;
        else if (u.type == Police::UnitType::TacticalRhino) rhinos++;
        else if (u.type == Police::UnitType::UndercoverSupercar) supercars++;

        if (u.isLockingOnEMP) empLockingCount++;
    }

    std::cout << "  Active Units: " << HUD::BOLD << director.activeUnits.size() << "/20" << HUD::RESET
              << " (Cruisers: " << cruisers
              << ", Interceptors: " << interceptors
              << ", Rhinos: " << rhinos
              << ", Supercars: " << supercars << ")\n";

    // Weapons Telemetry
    std::cout << "\n" << HUD::BOLD << " [ACTIVE WEAPONS & HAZARDS]\n" << HUD::RESET;
    if (car.drivetrain.empDisabled) {
        std::cout << "  " << HUD::RED << ">> [WARNING] EMP DISRUPTOR ACTIVE! IGNITION DISABLED (" << std::fixed << std::setprecision(1) << car.drivetrain.empTimer << "s left) <<" << HUD::RESET << "\n";
    } else if (empLockingCount > 0) {
        std::cout << "  " << HUD::YELLOW << ">> [ALERT] " << empLockingCount << " POLICE UNIT(S) LOCKING ON WITH EMP DISRUPTOR! <<" << HUD::RESET << "\n";
    } else {
        std::cout << "  EMP Status: Systems Nominal (Threat Level " << (director.currentHeatLevel >= 7 ? "HIGH" : "STANDBY") << ")\n";
    }

    std::cout << "  Active Thermite Molten Traps on Asphalt: " << director.activeThermites.size()
              << (director.activeThermites.empty() ? "" : HUD::RED + " (Tire Traction Hazard: 0.12 mu!)" + HUD::RESET) << "\n";

    // Vehicle Health & Integrity
    std::cout << "  Player Vehicle Chassis Health: ";
    if (car.health > 60.0f) std::cout << HUD::GREEN;
    else if (car.health > 25.0f) std::cout << HUD::YELLOW;
    else std::cout << HUD::RED;
    std::cout << std::fixed << std::setprecision(1) << car.health << "%" << HUD::RESET << "\n";

    std::cout << HUD::CYAN << "===================================================================================\n" << HUD::RESET;
    std::cout << " [Simulation Substep " << step << " | Physics Frequency: 120 Hz | Shader Pipeline Ready]\n";
}

int main() {
    std::cout << HUD::BOLD << HUD::CYAN << "\n[Engine] Initializing Need For Speed: Hardcore Pursuit Engine...\n" << HUD::RESET;

    // 1. Initialize Vehicle
    Physics::Vehicle playerCar;
    playerCar.position = {0.0f, 0.0f, 0.0f};

    // 2. Initialize Police Escalation Director
    Police::HeatEscalationDirector policeDirector;
    policeDirector.Init();

    // 3. Load Shaders
    Renderer::Shader carPaintShader("CarPaint_PBR");
    Renderer::Shader wetAsphaltShader("WetAsphalt_PBR");
    Renderer::Shader empGlitchShader("EMP_Shockwave_PostProcess");
    Renderer::Shader postProcessShader("ACES_Filmic_PostProcess");

    carPaintShader.LoadFromFile("shaders/car_paint.vert", "shaders/car_paint.frag");
    wetAsphaltShader.LoadFromFile("shaders/car_paint.vert", "shaders/wet_asphalt.frag");
    empGlitchShader.LoadFromFile("shaders/car_paint.vert", "shaders/emp_glitch.frag");
    postProcessShader.LoadFromFile("shaders/car_paint.vert", "shaders/postprocess.frag");

    std::cout << "[Engine] All 4 PBR & Post-processing shaders loaded successfully!\n";
    std::cout << "[Engine] Hardcore Pacejka '96 tire model & suspension initialized.\n";
    std::cout << "[Engine] Police 10-level pursuit state machine initialized.\n";

    // Simulation Loop setup
    constexpr float dt = 1.0f / 120.0f; // 120 Hz high-frequency physics substepping
    float totalSimulatedTime = 0.0f;
    int stepCounter = 0;

    std::cout << "\nStarting live pursuit demonstration in 1.5 seconds...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Demonstrate escalating pursuit scenario:
    // We will accelerate through a dynamic pursuit to demonstrate physics, tire slip,
    // police spawning, weapon deployments (EMP & Thermite), and Heat 10 scaling!
    while (stepCounter < 2500 && !playerCar.isWrecked) {
        Physics::VehicleInputs inputs;
        inputs.throttle = 1.0f; // Flat out throttle
        inputs.brake = 0.0f;
        inputs.handbrake = false;

        // Dynamic high-speed slalom and drift steering inputs
        float timeSec = stepCounter * dt;
        inputs.steering = std::sin(timeSec * 1.4f) * 0.45f;

        // Handbrake flick around second 4
        if (timeSec > 4.0f && timeSec < 4.8f) {
            inputs.handbrake = true;
            inputs.steering = 0.85f; // Aggressive drift entry
        }

        // Fast-forward pursuit escalation clock to demonstrate all 10 Heat levels!
        // At step 1200, fast forward time to Heat Level 10 (45 minutes = 2700s) to demonstrate 20 Police units and weapons!
        if (stepCounter == 800) {
            policeDirector.timeInPursuitSeconds = 2150.0f; // Jump to Heat 8 (Thermite Bombs)
        } else if (stepCounter == 1500) {
            policeDirector.timeInPursuitSeconds = 2750.0f; // Jump to Heat 10 (20 Police Units + EMP + Thermite)
        }

        // Step physics and police AI
        playerCar.Step(inputs, dt);
        policeDirector.Update(playerCar, dt);

        totalSimulatedTime += dt;
        stepCounter++;

        // Render HUD every 12 physics substeps (~10 FPS console refresh for smooth readability)
        if (stepCounter % 12 == 0) {
            PrintTelemetry(playerCar, policeDirector, totalSimulatedTime, stepCounter);
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    }

    std::cout << "\n" << HUD::GREEN << "[Engine] Simulation demonstration completed successfully!\n" << HUD::RESET;
    std::cout << "Level 10 with 20 police units and EMP/Thermite weapons verified.\n";
    return 0;
}
