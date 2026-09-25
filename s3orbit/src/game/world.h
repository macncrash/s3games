#pragma once

// Screen-space docking geometry. The clamp seat is the origin the probe must
// meet; the mouth of the arm opens to the left of that seat.
namespace orbit {

constexpr float kShoulderX = 248.f;
constexpr float kShoulderY = 114.f;
constexpr float kElbowX = 208.f;
constexpr float kNose = 21.f;       // probe tip, right of the ship centre
constexpr float kShipR = 7.f;
constexpr float kStartX = 28.f;
constexpr float kStartY = 40.f;
constexpr float kShowX = 72.f;
constexpr float kShowY = 176.f;
constexpr float kStationL = 236.f;
constexpr float kStationT = 28.f;
constexpr float kStationW = 84.f;
constexpr float kStationH = 176.f;
constexpr float kCollarBack = 11.f; // forearm meets the clamp this far right of the seat
constexpr float kSeatX = 3.6f;
constexpr float kSeatY = 4.6f;
constexpr float kJawIn = 9.f;
constexpr float kJawOut = 18.f;
constexpr float kJawL = -16.f;
constexpr float kJawR = 10.f;
constexpr float kSoftRvx = 22.f;
constexpr float kHardRvx = 44.f;
constexpr float kHardMetal = 36.f;
constexpr float kDwellNeed = 0.34f;
constexpr int kTries = 3;
constexpr float kWindow = 90.f;

}  // namespace orbit
