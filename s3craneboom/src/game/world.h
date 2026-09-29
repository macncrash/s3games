#pragma once

// Side-view yard. The hanging drive and the loading boom share these numbers.
namespace craneboom {

constexpr float DT = 1.f / 60.f;
constexpr float GROUND = 184.f;
constexpr float TIP_Y = 92.f;     // jib tip, world y
constexpr float JIB = 96.f;       // chassis centre to the hook
constexpr float DRIVE_H = 20.f;
constexpr float BOOM_TOP = 132.f;
constexpr float BOOM_L = 456.f;
constexpr float END_L = 560.f;
constexpr float END_R = 652.f;
constexpr float END_IN = 8.f;
constexpr float START_X = 78.f;
constexpr float CABLE_MIN = 8.f;
constexpr float CABLE_MAX = 78.f;
constexpr float HOIST = 34.f;
constexpr float STOP_V = 8.f;
constexpr float HOLD_NEED = 0.55f;
constexpr float LEG_LIMIT = 26.f;
constexpr float GAS_A = 72.f;
constexpr float BRAKE_A = 160.f;
constexpr float DRAG = 0.62f;
constexpr float V_MAX = 108.f;
constexpr float PAST = END_R + 12.f;

inline float restCable() { return BOOM_TOP - TIP_Y - DRIVE_H * 0.5f; }
inline float endMid() { return (END_L + END_IN + END_R - END_IN) * 0.5f; }

}  // namespace craneboom
