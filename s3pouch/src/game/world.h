#pragma once

namespace pouch {

// 320x224, top to bottom. A hitbox is 2*HH tall, so a refuge only counts
// where that box does not reach into a street.
constexpr float HH = 8.f;
constexpr float HW = 8.f;
constexpr float SPEED = 1.55f;
constexpr float SLIDE = 2.05f;
constexpr float HOME_X = 160.f;

constexpr int Y_SHOP1 = 8;
constexpr int Y_GOAL0 = 8, Y_GOAL1 = 32;
constexpr int Y_CURB0 = 32, Y_CURB1 = 36;
constexpr int Y_ST3_0 = 36, Y_ST3_1 = 72;
constexpr int Y_MED2_0 = 72, Y_MED2_1 = 96;
constexpr int Y_ST2_0 = 96, Y_ST2_1 = 132;
constexpr int Y_MED1_0 = 132, Y_MED1_1 = 156;
constexpr int Y_ST1_0 = 156, Y_ST1_1 = 192;
constexpr int Y_CURB2_0 = 192, Y_CURB2_1 = 196;
constexpr int Y_START0 = 196, Y_START1 = 224;

constexpr float S3_Y0 = 36.f, S3_Y1 = 72.f;
constexpr float S2_Y0 = 96.f, S2_Y1 = 132.f;
constexpr float S1_Y0 = 156.f, S1_Y1 = 192.f;

constexpr float START_CY = 200.f;
constexpr float MED1_LO = 140.f, MED1_HI = 148.f;
constexpr float MED2_LO = 80.f, MED2_HI = 88.f;
constexpr float GOAL_CY = 28.f;

constexpr float POS_START = 208.f;
constexpr float POS_MED1 = 144.f;
constexpr float POS_MED2 = 84.f;
constexpr float POS_GOAL = 22.f;

constexpr int SEDAN_W = 40, SEDAN_H = 18;
constexpr int VAN_W = 52, VAN_H = 20;
constexpr int BUS_W = 76, BUS_H = 24;

inline bool overlapsStreet(float cy, float y0, float y1) {
    return (cy + HH) > y0 && (cy - HH) < y1;
}

// 0 start curb, 1 after street 1, 2 after street 2, 3 far side, -1 in a street.
inline int bandAt(float cy) {
    if (overlapsStreet(cy, S1_Y0, S1_Y1) || overlapsStreet(cy, S2_Y0, S2_Y1) || overlapsStreet(cy, S3_Y0, S3_Y1))
        return -1;
    if (cy >= START_CY) return 0;
    if (cy >= MED1_LO && cy <= MED1_HI) return 1;
    if (cy >= MED2_LO && cy <= MED2_HI) return 2;
    if (cy <= GOAL_CY) return 3;
    return -1;
}

inline float hopUp(int band) {
    if (band <= 0) return POS_MED1;
    if (band == 1) return POS_MED2;
    return POS_GOAL;
}

inline float hopDown(int band) {
    if (band >= 2) return POS_MED1;
    return POS_START;
}

}  // namespace pouch
