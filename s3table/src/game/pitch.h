#pragma once

namespace table {

// Ice is y-down. The mouth is a slot in each end rail. A goal counts only when
// the whole puck has gone past that end line and is still inside the slot.
constexpr float kLeft = 44.f;
constexpr float kRight = 276.f;
constexpr float kTop = 56.f;
constexpr float kBot = 176.f;
constexpr float kMid = (kTop + kBot) * 0.5f;
constexpr float kMouthL = 108.f;
constexpr float kMouthR = 212.f;
constexpr float kPocket = 16.f;
constexpr float kPuckR = 6.f;
constexpr float kMalletR = 12.f;
constexpr float kCenter = 160.f;
constexpr int kSeven = 7;

}  // namespace table
