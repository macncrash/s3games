#pragma once

// Side-view yard. The painted box and the crane chassis share these numbers.
namespace cranebox {

constexpr float DT = 1.f / 60.f;
constexpr float GROUND = 176.f;
constexpr float REAR = 28.f;   // centre to the back of the chassis
constexpr float NOSE = 36.f;   // centre to the front bumper
constexpr float BOX_L = 430.f;
constexpr float BOX_R = 568.f;
constexpr float START_X = 86.f;
constexpr float STOP_V = 7.f;
constexpr float HOLD_NEED = 0.55f;
constexpr float LEG_LIMIT = 26.f;
constexpr float GAS_A = 78.f;
constexpr float BRAKE_A = 150.f;
constexpr float DRAG = 0.55f;
constexpr float V_MAX = 128.f;

}  // namespace cranebox
