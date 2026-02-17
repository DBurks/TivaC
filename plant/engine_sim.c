#include "engine_sim.h"

#define IDLE_RPM 800.0f
#define ALPHA    0.05f 

static float current_rpm = IDLE_RPM;

void Engine_Init(void) {
    current_rpm = IDLE_RPM;
}

void Engine_Update(uint8_t throttle) {
    // Realistic Polynomial: 0.6*T^2 + 10*T + 800
    float target = (0.6f * throttle * throttle) + (10.0f * throttle) + IDLE_RPM;
    // Low pass filter (Inertia)
    current_rpm += ALPHA * (target - current_rpm);
}

uint16_t Engine_GetRPM(void) {
    return (uint16_t)current_rpm;
}
