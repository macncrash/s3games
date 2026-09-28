#pragma once

// Top-down basin. The dock slab in the boot picture uses these edges.
namespace cranebuoy {

constexpr int NBUOY = 3;
constexpr float BUOY_X[NBUOY] = {64.f, 250.f, 160.f};
constexpr float BUOY_Y[NBUOY] = {108.f, 80.f, 128.f};
constexpr float HIT_R = 13.f;
constexpr float RING_IN = 16.f;
constexpr float RING_OUT = 78.f;
constexpr float SWEEP_NEED = 2.05f;  // port rounding, a bit more than a third of a circle
constexpr float DOCK_L = 118.f;
constexpr float DOCK_R = 202.f;
constexpr float DOCK_T = 170.f;
constexpr float DOCK_B = 216.f;
constexpr float START_X = 160.f;
constexpr float START_Y = 194.f;
constexpr float START_H = -1.5707963f;
constexpr float LEG_LIMIT = 120.f;

}  // namespace cranebuoy
