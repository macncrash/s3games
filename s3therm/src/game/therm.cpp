#include "game/therm.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace therm {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHover = 0.46f;
constexpr float kBuoy = 24.f;
constexpr float kVDrag = 1.05f;
constexpr float kWindResp = 1.2f;
constexpr float kBurn = 0.104f;
constexpr float kVent = 0.35f;
constexpr float kLeak = 0.12f;
constexpr float kAmbient = 0.18f;
constexpr float kOver = 0.98f;
constexpr float kSoft = 4.2f;
constexpr float kMarkX = 760.f;
constexpr float kMarkR = 96.f;
constexpr float kXMin = -100.f;
constexpr float kXMax = 1200.f;
constexpr float kClock = 140.f;
constexpr float kBagHalf = 15.f;
constexpr float kBagWorldH = 46.f;
constexpr float kLowTop = 32.f;
constexpr float kHeadTop = 78.f;
constexpr float kTailTop = 170.f;
constexpr float kClearX = 560.f;

struct Tree {
    float x, h;
};

// A wall of canopy, then open ground, then the mark, then trees again.
constexpr Tree kTrees[] = {
    {168.f, 62.f}, {206.f, 78.f}, {244.f, 58.f}, {284.f, 86.f}, {324.f, 70.f}, {364.f, 80.f},
    {404.f, 64.f}, {444.f, 88.f}, {484.f, 74.f}, {524.f, 68.f}, {980.f, 82.f}, {1030.f, 94.f}, {1084.f, 72.f},
};
constexpr int kTreeN = int(sizeof kTrees / sizeof kTrees[0]);

float treeHalf(float h) { return h * (float(kTreeBmpW) * 0.5f / float(kTreeBmpH)); }

float windAt(float y) {
    if (y < kLowTop) return 4.6f;
    if (y < kHeadTop) return -7.4f;
    if (y < kTailTop) return 12.6f;
    return 1.4f;
}

uint16_t lerp4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto c = [&](int u, int v) { return int(std::lround(u + (v - u) * t)); };
    return gs::rgb4(c(ar, br), c(ag, bg), c(ab, bb));
}

}  // namespace

void Game::begin() {
    x_ = 120.f;
    y_ = 24.f;
    vx_ = 1.5f;
    vy_ = 1.1f;
    temp_ = 0.58f;
    time_ = 0.f;
    left_ = kClock;
    burn_ = false;
    vent_ = false;
    drop_ = false;
    shake_ = 0.f;
    fanT_ = -1.f;
    fanStep_ = -1;
    burst_ = 0.f;
    over_ = false;
    won_ = false;
    why_ = "";
    report_[0] = 0;
    camX_ = x_ + 28.f;
    camY_ = std::clamp(y_ * 0.22f + 28.f, 26.f, 52.f);
    zoom_ = 1.28f;
    mode_ = Mode::Play;
    if (sys_) sys_->apu.noiseBurst(0.06f, 800.f, 5.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(11, 12, 14));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.10f, 0.24f, 0.12f);
    begin();
    if (!bot_) mode_ = Mode::Title;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (std::fabs(x_ - kMarkX) < kMarkR + 40.f) return 3;
    if (y_ > 80.f) return 2;
    return 1;
}

const char* Game::band() const {
    if (y_ < kLowTop) return "LOW >";
    if (y_ < kHeadTop) return "HEAD <";
    if (y_ < kTailTop) return "TAIL >";
    return "THIN";
}

const char* Game::hint() const {
    bool flaring = std::fabs(x_ - kMarkX) <= kMarkR && y_ < 24.f;
    if (temp_ > 0.90f && !flaring) return "LET GO, THE BAG IS HOT";
    if (y_ >= kLowTop && y_ < kHeadTop && x_ < kClearX) return "CLIMB THROUGH THE HEAD WIND";
    if (x_ < kClearX) return "GET OVER THE TREES";
    if (x_ > kMarkX + kMarkR) return "PAST IT. THE TREES ARE NEXT";
    if (flaring) return "SET THE BASKET DOWN SOFT";
    return "RIDE THE TAIL TO THE MARK";
}

bool Game::hitTree() const {
    for (const Tree& t : kTrees) {
        if (std::fabs(x_ - t.x) > treeHalf(t.h) + kBagHalf) continue;
        if (y_ < t.h) return true;
    }
    return false;
}

void Game::pilot(bool& burn, bool& vent) {
    burn = false;
    vent = false;
    // One committed drop. Climbing back in the headwind just parks the bag.
    if (!drop_ && x_ >= 705.f && y_ > 90.f) drop_ = true;

    float aim = drop_ ? 3.f : 112.f;
    float x0 = std::min(x_, x_ + vx_ * 1.5f) - 6.f;
    float x1 = std::max(x_, x_ + vx_ * 1.5f) + 28.f;
    float floorY = 0.f;
    for (const Tree& t : kTrees) {
        float half = treeHalf(t.h);
        if (t.x + half < x0 || t.x - half > x1) continue;
        floorY = std::max(floorY, t.h + 6.f);
    }
    if (aim < floorY) aim = floorY;

    bool onMark = std::fabs(x_ - kMarkX) <= kMarkR;
    float wantVy;
    if (drop_ && onMark && y_ < 22.f) wantVy = std::clamp((0.2f - y_) * 0.75f, -2.8f, 1.2f);
    else wantVy = std::clamp((aim - y_) * 0.85f, drop_ ? -9.f : -3.5f, 10.f);

    float ay = std::clamp((wantVy - vy_) * 3.1f, -16.f, 18.f);
    float need = kHover + (ay + vy_ * kVDrag) / kBuoy;
    need = std::clamp(need, 0.12f, 0.84f);
    if (temp_ > 0.90f) need = std::min(need, 0.64f);
    if (temp_ < need - 0.02f) burn = true;
    else if (temp_ > need + 0.028f) vent = true;
}

void Game::human(bool& burn, bool& vent) {
    const gs::Pad& p = sys_->pad;
    burn = p.down(gs::BTN_C) || p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO) || p.accel > 0.2f;
    vent = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.2f;
    if (burn && vent) burn = false;
}

void Game::succeed() {
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "MARK";
    burn_ = false;
    vent_ = false;
    fanT_ = 0.f;
    fanStep_ = -1;
    y_ = 0.f;
    vy_ = 0.f;
    std::snprintf(report_, sizeof report_, "S3 THERM  THE MARK  NOT THE TREES  %.1fs", time_);
    if (sys_) sys_->rumble(0.25f, 0.55f, 200);
}

void Game::fail(const char* why) {
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    burn_ = false;
    vent_ = false;
    shake_ = 6.f;
    burst_ = 0.4f;
    std::snprintf(report_, sizeof report_, "S3 THERM  FAIL  %s  x %.0f y %.0f vy %.1f temp %.2f  %.1fs", why, x_, y_,
                  vy_, temp_, time_);
    if (sys_) {
        sys_->apu.noiseBurst(0.5f, 980.f, 7.f);
        sys_->rumble(0.9f, 0.45f, 220);
    }
}

void Game::physics(bool burn, bool vent) {
    burn_ = burn;
    vent_ = vent;
    time_ += kDt;
    left_ -= kDt;

    float dT = -(temp_ - kAmbient) * kLeak;
    if (burn) dT += kBurn;
    if (vent) dT -= kVent;
    temp_ = std::clamp(temp_ + dT * kDt, 0.f, 1.f);

    float ay = (temp_ - kHover) * kBuoy - vy_ * kVDrag;
    vy_ = std::clamp(vy_ + ay * kDt, -14.f, 16.f);
    y_ += vy_ * kDt;
    float w = windAt(std::max(y_, 0.f));
    vx_ = std::clamp(vx_ + (w - vx_) * kWindResp * kDt, -22.f, 26.f);
    x_ += vx_ * kDt;

    if (hitTree()) {
        fail("TREES");
        return;
    }
    if (y_ <= 0.f) {
        float impact = vy_;
        y_ = 0.f;
        vy_ = 0.f;
        vx_ = 0.f;
        if (std::fabs(x_ - kMarkX) <= kMarkR && impact >= -kSoft) succeed();
        else if (std::fabs(x_ - kMarkX) <= kMarkR) fail("HARD");
        else fail("MISS");
        return;
    }
    if (temp_ >= kOver) {
        fail("HOT");
        return;
    }
    if (x_ < kXMin || x_ > kXMax) {
        fail("DRIFT");
        return;
    }
    if (left_ <= 0.f) {
        left_ = 0.f;
        fail("DAY");
    }
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (burst_ > 0.f) burst_ -= kDt;

    if (fanT_ >= 0.f) {
        fanT_ += kDt;
        static const float notes[] = {392.f, 523.25f, 659.25f, 783.99f};
        int step = int(fanT_ / 0.13f);
        if (step != fanStep_ && step >= 0 && step < 4) {
            fanStep_ = step;
            a.tone(2, notes[step], 0.085f);
        }
        if (fanT_ > 0.85f) {
            fanT_ = -1.f;
            a.tone(2, 0.f, 0.f);
        }
    } else if (mode_ != Mode::Play) {
        a.tone(2, 0.f, 0.f);
    }

    if (mode_ == Mode::Fail) {
        a.noise(0.f, 400.f, false);
        a.tone(0, 0.f, 0.f);
        a.tone(1, 0.f, 0.f);
        return;
    }

    bool titleFlame = mode_ == Mode::Title && std::sin(t_ * 7.f) > 0.15f;
    if ((mode_ == Mode::Play && burn_) || titleFlame) {
        a.noise(titleFlame ? 0.035f : 0.07f, 1500.f, false);
        a.tone(0, 68.f + temp_ * 46.f, titleFlame ? 0.02f : 0.034f);
    } else if (mode_ == Mode::Play && vent_) {
        a.noise(0.04f, 480.f, false);
        a.tone(0, 0.f, 0.f);
    } else if (mode_ == Mode::Play || mode_ == Mode::Title) {
        float wind = 0.012f + std::min(std::fabs(vx_), 16.f) * 0.001f;
        a.noise(wind, 260.f + std::fabs(vx_) * 18.f, true);
        a.tone(0, 0.f, 0.f);
    } else {
        a.noise(0.f, 0.f, false);
        a.tone(0, 0.f, 0.f);
    }

    if (mode_ == Mode::Play && vy_ > 1.6f) a.tone(1, 460.f + vy_ * 7.f, 0.02f);
    else a.tone(1, 0.f, 0.f);
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 210.f;
        camY_ = 62.f;
        zoom_ = 0.92f;
        return;
    }
    float tx = x_ + 26.f;
    float ty = std::clamp(y_ * 0.22f + 28.f, 26.f, 52.f);
    float k = 1.f - std::exp(-kDt * 3.4f);
    if (std::fabs(tx - camX_) > 420.f) camX_ = tx;
    else camX_ += (tx - camX_) * k;
    if (std::fabs(ty - camY_) > 220.f) camY_ = ty;
    else camY_ += (ty - camY_) * k;
    zoom_ = 1.28f;
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 49.f) * shake_;
        camY_ += std::cos(t_ * 43.f) * shake_ * 0.65f;
        shake_ = std::max(0.f, shake_ - kDt * 7.f);
    }
}

void Game::toScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 164.f + (wx - camX_) * zoom_;
    sy = 150.f - (wy - camY_) * zoom_;
}

void Game::stamp(const gs::Mipped& m, float wx, float wy, float worldW, float worldH, int pal, float ax, float ay,
                 int fog, bool shadow) {
    if (worldW < 0.4f || worldH < 0.4f || m.w < 1 || m.h < 1 || zoom_ < 0.05f) return;
    float sw = worldW * zoom_;
    float sh = worldH * zoom_;
    if (sw < 0.8f || sh < 0.8f) return;
    float sx, sy;
    toScreen(wx, wy, sx, sy);
    float left = sx - ax * sw;
    float top = sy - ay * sh;
    if (left > gs::SCREEN_W + 48 || top > gs::SCREEN_H + 48 || left + sw < -48 || top + sh < -48) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(sw), 1L, 1800L));
    s.h = int16_t(std::clamp(std::lround(sh), 1L, 1800L));
    s.x = int16_t(std::clamp(std::lround(left), -1800L, 1800L));
    s.y = int16_t(std::clamp(std::lround(top), -1800L, 1800L));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 12));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -1800L, 1800L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -1800L, 1800L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    camera();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;

    float day = std::clamp(left_ / kClock, 0.f, 1.f);
    float dusk = (1.f - day) * 0.5f;
    for (int sy = 0; sy < gs::SCREEN_H; sy++) {
        float wy = camY_ + (150.f - float(sy)) / zoom_;
        uint16_t c;
        if (wy < -10.f) c = gs::rgb4(1, 3, 1);
        else if (wy < 0.f) c = gs::rgb4(2, 6, 2);
        else {
            float t = std::clamp(wy / 210.f, 0.f, 1.f);
            uint16_t hor = gs::rgb4(14, 10, 7);
            uint16_t mid = gs::rgb4(8, 11, 14);
            uint16_t zen = gs::rgb4(3, 5, 11);
            c = t < 0.42f ? lerp4(hor, mid, t / 0.42f) : lerp4(mid, zen, (t - 0.42f) / 0.58f);
            if (wy >= kLowTop && wy < kHeadTop) c = lerp4(c, gs::rgb4(5, 7, 9), 0.30f);
            else if (wy >= kHeadTop && wy < kTailTop) c = lerp4(c, gs::rgb4(12, 14, 15), 0.16f);
        }
        if (dusk > 0.f) c = lerp4(c, gs::rgb4(6, 3, 5), dusk);
        v.lineBackdrop[sy] = c;
        v.lineFog[sy] = 0;
    }

    float drawX = x_;
    float drawY = y_;
    bool showFlame = burn_;
    if (mode_ == Mode::Title) {
        drawX = 120.f;
        drawY = 28.f + std::sin(t_ * 1.6f) * 3.2f;
        showFlame = std::sin(t_ * 7.f) > 0.15f;
    }
    float lean = std::clamp(vx_, -12.f, 12.f) * 0.16f;

    if (mode_ == Mode::Title) {
        spr(art_.therm, 160.f, 26.f, float(art_.therm.h), PAL_BANNER);
        spr(art_.oneEnv, 160.f, 52.f, float(art_.oneEnv.h), PAL_HUD);
    } else if (mode_ == Mode::Win) {
        spr(art_.theMark, 160.f, 36.f, float(art_.theMark.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped* word = &art_.missed;
        if (why_ && !std::strcmp(why_, "TREES")) word = &art_.theTrees;
        else if (why_ && !std::strcmp(why_, "HOT")) word = &art_.tooHot;
        else if (why_ && !std::strcmp(why_, "HARD")) word = &art_.tooHard;
        else if (why_ && !std::strcmp(why_, "DRIFT")) word = &art_.drifted;
        else if (why_ && !std::strcmp(why_, "DAY")) word = &art_.theDay;
        spr(*word, 160.f, 36.f, float(word->h), PAL_ALERT);
    }

    if (showFlame) {
        int fr = int(t_ * 16.f) % 3;
        if (fr < 0) fr = 0;
        float fh = 12.f + float(fr) * 2.4f;
        stamp(art_.flame[fr], drawX, drawY + 11.f, 8.f, fh, PAL_FLAME, 0.5f, 1.f);
    }
    float bagW = kBagWorldH * float(kBagBmpW) / float(kBagBmpH);
    float basW = 13.f * float(kBasketBmpW) / float(kBasketBmpH);
    stamp(art_.basket, drawX, drawY, basW, 13.f, PAL_BASKET, 0.5f, 1.f);
    stamp(art_.bag, drawX + lean, drawY + 10.f, bagW, kBagWorldH, PAL_BAG, 0.5f, 1.f);

    for (int i = 0; i < 3; i++) {
        float bx = std::fmod(40.f + i * 380.f + t_ * (16.f + i * 5.f), 1280.f) - 60.f;
        float by = 150.f + i * 16.f + std::sin(t_ * 1.4f + i) * 5.f;
        int fr = (int(t_ * 7.f) + i) & 1;
        stamp(art_.bird[fr], bx, by, 16.f, 9.f, PAL_BIRD, 0.5f, 0.5f, 1);
    }

    int flagFr = std::sin(t_ * 3.4f) > 0.f ? 0 : 1;
    float flagH = 88.f;
    float flagW = flagH * float(kFlagBmpW) / float(kFlagBmpH);
    stamp(art_.flag[flagFr], kMarkX, 0.f, flagW, flagH, PAL_FLAG, 0.22f, 1.f);

    int order[kTreeN];
    for (int i = 0; i < kTreeN; i++) order[i] = i;
    for (int i = 1; i < kTreeN; i++) {
        int key = order[i];
        int j = i;
        while (j > 0 && std::fabs(kTrees[order[j - 1]].x - camX_) < std::fabs(kTrees[key].x - camX_)) {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = key;
    }
    for (int n = 0; n < kTreeN; n++) {
        const Tree& t = kTrees[order[n]];
        float tw = t.h * float(kTreeBmpW) / float(kTreeBmpH);
        float d = std::fabs(t.x - drawX);
        int fog = d > 240.f ? std::min(8, int((d - 240.f) / 55.f)) : 0;
        int pal = (order[n] & 1) ? PAL_TREE2 : PAL_TREE;
        stamp(art_.tree, t.x, 0.f, tw, t.h, pal, 0.5f, 1.f, fog);
    }

    stamp(art_.mark, kMarkX, 0.f, kMarkR * 2.f, 16.f, PAL_MARK, 0.5f, 1.f);
    float shade = std::clamp(20.f - drawY * 0.06f, 9.f, 20.f);
    stamp(art_.shadow, drawX, 0.f, shade * 1.6f, 7.f, PAL_TREE, 0.5f, 0.5f, 0, true);

    for (int i = 0; i < 4; i++) {
        float cx = std::fmod(80.f + i * 300.f + t_ * (4.f + i), 1400.f) - 80.f;
        float cy = 168.f + (i % 2) * 22.f;
        stamp(art_.cloud, cx, cy, 70.f, 26.f, PAL_CLOUD, 0.5f, 0.5f, 3);
    }
    stamp(art_.sun, 80.f, 210.f, 54.f, 54.f, PAL_SUN, 0.5f, 0.5f, 2);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(22, "ONE ENVELOPE", PAL_BANNER);
        hudC(23, "THE MARK, NOT THE TREES", PAL_HUD);
        hudC(24, "C SPACE BURN   DOWN X VENT", PAL_HUD);
        hudC(25, "LET GO BEFORE IT COOKS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        return;
    }
    if (mode_ == Mode::Pause) {
        hud(1, 0, "S3 THERM", PAL_BANNER);
        hudC(13, "PAUSED", PAL_BANNER);
        hudC(15, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "%.0f SECONDS", time_);
        hudC(24, buf, PAL_WIN);
        hudC(25, "ONE ENVELOPE HELD", PAL_BANNER);
        hudC(26, "NOT THE TREES", PAL_WIN);
        if (!bot_) hudC(27, "START FLIES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        const char* sentence = "THAT WAS NOT THE MARK";
        if (why_ && !std::strcmp(why_, "TREES")) sentence = "THE MARK, NOT THE TREES";
        else if (why_ && !std::strcmp(why_, "HOT")) sentence = "THE ENVELOPE IS GONE";
        else if (why_ && !std::strcmp(why_, "HARD")) sentence = "SET THE BASKET DOWN SOFT";
        else if (why_ && !std::strcmp(why_, "DRIFT")) sentence = "THE WIND TOOK THE BAG";
        else if (why_ && !std::strcmp(why_, "DAY")) sentence = "THE LIGHT RAN OUT";
        hudC(25, sentence, PAL_ALERT);
        if (!bot_) hudC(27, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 THERM", PAL_BANNER);
    int bars = std::clamp(int(std::lround(temp_ * 12.f)), 0, 12);
    char bar[16];
    for (int i = 0; i < 12; i++) bar[i] = i < bars ? '#' : '-';
    bar[12] = 0;
    std::snprintf(buf, sizeof buf, "TEMP %s", bar);
    int tPal = temp_ > 0.86f ? PAL_ALERT : PAL_HUD;
    hud(1, 1, buf, tPal);

    std::snprintf(buf, sizeof buf, "ALT %d  VY %d  %s", int(std::lround(y_)), int(std::lround(vy_)), band());
    int bPal = (y_ >= kLowTop && y_ < kHeadTop) ? PAL_ALERT : PAL_HUD;
    hud(1, 24, buf, bPal);

    if (std::fabs(x_ - kMarkX) <= kMarkR && y_ < 48.f) std::snprintf(buf, sizeof buf, "OVER THE MARK");
    else {
        int d = int(std::lround(kMarkX - x_));
        if (d >= 0) std::snprintf(buf, sizeof buf, "MARK %d", d);
        else std::snprintf(buf, sizeof buf, "PAST THE MARK %d", -d);
    }
    hud(1, 25, buf, PAL_BANNER);
    hud(1, 26, hint(), temp_ > 0.90f ? PAL_ALERT : PAL_HUD);

    int sec = int(std::ceil(std::max(0.f, left_) - 1e-4f));
    std::snprintf(buf, sizeof buf, "TIME %d", sec);
    hud(1, 27, buf, sec <= 15 ? PAL_ALERT : PAL_HUD);

    int hot = int(std::clamp(temp_, 0.f, 1.f) * 255.f);
    sys_->setLight(hot, 48, 16);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;

    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || bot_) begin();
        else if (sys.pad.pressed(gs::BTN_MODE)) sys.eject();
        if (mode_ == Mode::Title) {
            audio();
            draw();
            return;
        }
    }
    if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        audio();
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (sys.pad.pressed(gs::BTN_START) && !bot_) begin();
        audio();
        draw();
        return;
    }
    if (sys.pad.pressed(gs::BTN_START) && !bot_) {
        mode_ = Mode::Pause;
        audio();
        draw();
        return;
    }

    bool burn = false, vent = false;
    if (bot_) pilot(burn, vent);
    else human(burn, vent);
    physics(burn, vent);
    audio();
    draw();
}

}  // namespace therm
