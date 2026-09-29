// Cliff shelf in plane pixels. The drop is everything outside these slabs.
// The boom is the gate at the east end of the last straight.
#pragma once

namespace cliffboom {

struct Slab {
    float x0, y0, x1, y1;
};

// Overlapping slabs so a tight turn still has rock under the tyres.
constexpr Slab kShelf[] = {
    {16.f, 148.f, 230.f, 222.f},
    {148.f, 46.f, 258.f, 206.f},
    {176.f, 36.f, 400.f, 124.f},
    {292.f, 56.f, 418.f, 224.f},
    {328.f, 142.f, 430.f, 222.f},
};
constexpr int kShelfN = int(sizeof(kShelf) / sizeof(kShelf[0]));

constexpr float BOOM_X = 416.f;
constexpr float BOOM_Y0 = 150.f;
constexpr float BOOM_Y1 = 216.f;
constexpr float POCKET_X0 = 376.f;
constexpr float POCKET_X1 = 408.f;
constexpr float POCKET_Y0 = 162.f;
constexpr float POCKET_Y1 = 208.f;

constexpr float START_X = 72.f;
constexpr float START_Y = 184.f;
constexpr float LEG_LIMIT = 80.f;

inline bool onShelf(float x, float y) {
    for (int i = 0; i < kShelfN; i++) {
        const Slab& s = kShelf[i];
        if (x >= s.x0 && x <= s.x1 && y >= s.y0 && y <= s.y1) return true;
    }
    return false;
}

}  // namespace cliffboom
