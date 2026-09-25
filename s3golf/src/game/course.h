// Three side-view holes. The cup is a well cut through the green.
#pragma once

#include <algorithm>
#include <cmath>

namespace golf {

constexpr int kHoles = 3;
constexpr int kViewH = 120;
constexpr int kBaseRow = 102;  // bitmap row where world y is 0
constexpr int kGround0 = 206;  // screen y where world y is 0
constexpr float kWaterY = 12.f;

enum Kind { KIND_FAIR = 1, KIND_ROUGH, KIND_GREEN, KIND_SAND, KIND_WATER, KIND_CUP };

struct Seg {
    float x0 = 0, x1 = 0, y0 = 0, y1 = 0;
    int kind = KIND_FAIR;
};

struct Hole {
    const char* name = "";
    const char* hint = "";
    int par = 3;
    float length = 0;
    float tee = 0;
    float cup = 0;
    float cupHalf = 9;
    float cupDepth = 18;
    float lip = 0;
    Seg seg[8]{};
    int n = 0;
};

struct Surf {
    float y = 0;
    int kind = KIND_FAIR;
    bool well = false;
};

inline float lerpY(const Seg& s, float x) {
    float span = s.x1 - s.x0;
    if (span < 1e-4f) return s.y0;
    float t = std::clamp((x - s.x0) / span, 0.f, 1.f);
    return s.y0 + (s.y1 - s.y0) * t;
}

inline const Seg* segAt(const Hole& h, float x) {
    if (h.n <= 0) return nullptr;
    if (x < 0.f) x = 0.f;
    if (x >= h.length) x = h.length - 0.01f;
    for (int i = 0; i < h.n; i++) {
        if (x >= h.seg[i].x0 && x < h.seg[i].x1) return &h.seg[i];
    }
    return &h.seg[h.n - 1];
}

inline Surf groundAt(const Hole& h, float x, bool openWell) {
    Surf o;
    const Seg* s = segAt(h, x);
    if (!s) return o;
    float sx = std::clamp(x, s->x0, s->x1 - 0.001f);
    o.y = lerpY(*s, sx);
    o.kind = s->kind;
    if (openWell && std::fabs(x - h.cup) <= h.cupHalf) {
        o.y = h.lip - h.cupDepth;
        o.kind = KIND_CUP;
        o.well = true;
    }
    return o;
}

inline bool waterSpan(const Hole& h, float x) {
    const Seg* s = segAt(h, x);
    return s && s->kind == KIND_WATER;
}

// Flat lip between the ball and the cup, mouth excluded. A putt can finish.
inline bool puttLine(const Hole& h, float x) {
    if (std::fabs(x - h.cup) <= h.cupHalf + 0.8f) return false;
    float a = std::min(x, h.cup);
    float b = std::max(x, h.cup);
    for (float s = a; s <= b + 0.01f; s += 2.f) {
        if (std::fabs(s - h.cup) <= h.cupHalf) continue;
        Surf g = groundAt(h, s, false);
        if (g.kind == KIND_WATER || g.kind == KIND_SAND) return false;
        if (std::fabs(g.y - h.lip) > 1.6f) return false;
    }
    return true;
}

inline Hole finishHole(Hole h) {
    const Seg* s = segAt(h, h.cup);
    h.lip = s ? lerpY(*s, h.cup) : 0.f;
    return h;
}

inline Hole hole0() {
    Hole h;
    h.name = "LINKS";
    h.hint = "THE GREEN IS NOT THE CUP";
    h.length = 460;
    h.tee = 42;
    h.cup = 300;
    h.cupHalf = 9;
    h.n = 4;
    h.seg[0] = {0, 220, 24, 24, KIND_FAIR};
    h.seg[1] = {220, 260, 24, 32, KIND_FAIR};
    h.seg[2] = {260, 400, 32, 32, KIND_GREEN};
    h.seg[3] = {400, 460, 32, 20, KIND_ROUGH};
    return finishHole(h);
}

inline Hole hole1() {
    Hole h;
    h.name = "CARRY";
    h.hint = "CARRY THE WATER";
    h.length = 520;
    h.tee = 40;
    h.cup = 360;
    h.cupHalf = 9;
    h.n = 6;
    h.seg[0] = {0, 130, 22, 22, KIND_FAIR};
    h.seg[1] = {130, 250, -30, -30, KIND_WATER};
    h.seg[2] = {250, 286, 16, 34, KIND_SAND};
    h.seg[3] = {286, 450, 34, 34, KIND_GREEN};
    h.seg[4] = {450, 490, 34, 18, KIND_ROUGH};
    h.seg[5] = {490, 520, -30, -30, KIND_WATER};
    return finishHole(h);
}

inline Hole hole2() {
    Hole h;
    h.name = "CROWN";
    h.hint = "ONLY THE HOLE WILL HOLD";
    h.length = 540;
    h.tee = 36;
    h.cup = 340;
    h.cupHalf = 12;
    h.n = 5;
    h.seg[0] = {0, 180, 20, 20, KIND_FAIR};
    h.seg[1] = {180, 280, 20, 50, KIND_GREEN};  // front slope rolls off
    h.seg[2] = {280, 420, 50, 50, KIND_GREEN};  // collar around the well
    h.seg[3] = {420, 500, 50, 18, KIND_GREEN};  // back slope rolls off
    h.seg[4] = {500, 540, 18, 18, KIND_ROUGH};
    return finishHole(h);
}

inline const Hole& holeAt(int i) {
    static const Hole holes[kHoles] = {hole0(), hole1(), hole2()};
    if (i < 0) i = 0;
    if (i >= kHoles) i = kHoles - 1;
    return holes[i];
}

}  // namespace golf
