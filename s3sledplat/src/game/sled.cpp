#include "sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace sledplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kG = 9.6f;
constexpr float kH0 = 14.2f;
constexpr float kDeck = 5.20f;
constexpr float kLip = 0.24f;
constexpr float kGradeEnd = 62.f;
constexpr float kNear = 78.f;
constexpr float kFar = 112.f;
constexpr float kMark = 96.f;
constexpr float kNose = 2.30f;
constexpr float kTail = 2.40f;
constexpr float kLevelTol = 1.15f;
constexpr float kAttMax = 0.08f;
constexpr float kStop = 0.22f;
constexpr float kHoldNeed = 0.50f;
constexpr float kStillNeed = 1.10f;
constexpr float kLegLimit = 46.f;
constexpr float kSill = 0.36f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

// Running surface. The timber deck stands a lip above the snow, then falls away.
float road(float x) {
    const float lip = kDeck - kLip;
    const float knee = 0.86f;
    if (x < kGradeEnd) {
        float t = clampf(x / kGradeEnd, 0.f, 1.f);
        float hKnee = kH0 + (lip - kH0) * knee;
        if (t <= knee) return kH0 + (lip - kH0) * t;
        float u = (t - knee) / (1.f - knee);
        float s = u * u * (3.f - 2.f * u);
        return hKnee + (lip - hKnee) * s;
    }
    if (x < kNear) return lip;
    if (x <= kFar) return kDeck;
    float u = clampf((x - kFar) / 2.4f, 0.f, 1.f);
    return kDeck - u * u * 8.f;
}

float roadSlope(float x) {
    const float e = 0.2f;
    return (road(x + e) - road(x - e)) / (2.f * e);
}

int zoneAt(float x) {
    if (x >= kNear && x <= kFar) return 2;
    if (x >= kGradeEnd) return 1;
    return 0;
}

float dragOf(float v, int zone) {
    float mu = zone == 2 ? 0.10f : zone == 1 ? 0.045f : 0.040f;
    return mu * kG + 0.015f * v + 0.0032f * v * v;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.012f, 0.16f, 0.55f, 0.22f};
    p.op[1] = {2.01f, 0.28f, 0.02f, 0.20f, 0.3f, 0.16f};
    p.op[2] = {2.99f, 0.08f, 0.02f, 0.22f, 0.2f, 0.18f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.18f, 0.2f, 0.2f};
    p.vol = 0.2f;
    p.tone = 1800.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (x_ >= kNear) return 2;
    return 1;
}

int Game::pose() const {
    int best = 0;
    float bd = 100.f;
    for (int i = 0; i < kPoses; i++) {
        float d = std::fabs(att_ - kPoseAtt[i]);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    timber_ = true;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    snap_ = true;
    x_ = kMark;
    h_ = kDeck;
    v_ = 0.f;
    att_ = 0.f;
    bed_ = 0.f;
    weight_ = 0.f;
    brake_ = 0.f;
    hold_ = 0.f;
    still_ = 0.f;
    legT_ = 0.f;
    shake_ = 0.f;
    camX_ = kMark + 0.2f;
    camH_ = kDeck + 1.45f;
    camS_ = 18.f;
}

void Game::startRun() {
    x_ = 4.f;
    v_ = 9.2f;
    h_ = road(x_);
    att_ = std::atan(roadSlope(x_));
    bed_ = h_ - kDeck;
    weight_ = 0.f;
    brake_ = 0.f;
    hold_ = 0.f;
    still_ = 0.f;
    legT_ = 0.f;
    won_ = false;
    over_ = false;
    timber_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    puffN_ = 0;
    puffT_ = 0.f;
    shake_ = 0.f;
    for (Puff& p : puffs_) p = {};
    mode_ = Mode::Run;
    snap_ = true;
    camX_ = x_ + 4.f;
    camH_ = h_;
    camS_ = 12.f;
    blip(520.f);
}

void Game::human(float& weight, float& brake, bool& push) {
    const gs::Pad& p = sys_->pad;
    weight = 0.f;
    if (p.down(gs::BTN_UP)) weight += 1.f;
    if (p.down(gs::BTN_DOWN)) weight -= 1.f;
    if (std::fabs(p.axisY) > 0.22f) weight = clampf(p.axisY, -1.f, 1.f);
    brake = 0.f;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Z) ||
        p.down(gs::BTN_TURBO))
        brake = 1.f;
    if (p.brake > 0.05f) brake = std::max(brake, p.brake);
    // Down leans the nose and shoves the sled along the flat. The trigger creeps without tipping.
    push = (p.down(gs::BTN_DOWN) || p.accel > 0.25f) && brake < 0.2f;
}

void Game::pilot(float& weight, float& brake, bool& push) const {
    weight = 0.f;
    brake = 0.f;
    push = false;
    const float err = kMark - x_;
    if (x_ > kFar - 3.5f) {
        brake = 1.f;
        return;
    }
    if (x_ < kGradeEnd - 0.4f) return;

    float vWant = std::sqrt(std::max(0.f, 2.8f * std::max(err, 0.f)));
    if (err < 3.0f) vWant = std::min(vWant, 0.42f + std::max(err, 0.f) * 0.85f);
    if (err <= 0.06f) vWant = 0.f;
    if (v_ > vWant + 0.28f) brake = 1.f;
    else if (v_ > vWant + 0.06f) brake = 0.5f;
    if (err > 1.8f && v_ + 0.35f < vWant && v_ < 2.4f) push = true;
    if (std::fabs(err) <= kLevelTol && std::fabs(att_) <= kAttMax && v_ < 0.85f) brake = 1.f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    banner_ = "LEVEL";
    v_ = 0.f;
    att_ = 0.f;
    h_ = kDeck;
    bed_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.25f, 0.08f, 150);
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    shake_ = 1.f;
    if (std::strcmp(why, "past the platform") == 0) att_ = -0.42f;
    sys_->rumble(0.5f, 0.25f, 180);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.4f, 380.f, 0.28f);
}

void Game::physics(float weight, float brake, bool push) {
    if (mode_ != Mode::Run) return;
    legT_ += kDt;
    weight_ = clampf(weight, -1.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);

    const int zone = zoneAt(x_);
    const bool flat = x_ >= kGradeEnd;
    const float slope = roadSlope(x_);
    float along = -kG * slope / std::sqrt(1.f + slope * slope);
    if (v_ < 0.03f && along < 0.25f) along = 0.f;
    float decel = dragOf(v_, zone);
    decel += brake_ * (7.6f + std::min(v_, 10.f) * 0.16f);
    if (weight_ > 0.f) decel += weight_ * 1.15f;
    if (weight_ < 0.f) decel *= (1.f + 0.4f * weight_);
    if (push && brake_ < 0.2f && flat && v_ < 3.2f) along += 3.4f;

    v_ = std::max(0.f, v_ + (along - decel) * kDt);
    if (v_ > 16.f) v_ = 16.f;
    x_ += v_ * kDt;
    h_ = road(x_);
    bed_ = (h_ + kSill * std::cos(att_)) - (kDeck + kSill);

    float want = std::atan(slope) + weight_ * 0.22f;
    att_ += (want - att_) * (1.f - std::exp(-8.5f * kDt));

    if ((v_ > 2.4f && x_ < kNear) || (brake_ > 0.45f && v_ > 0.8f)) {
        puffT_ += kDt;
        if (puffT_ > 0.06f) {
            puffT_ = 0.f;
            Puff& p = puffs_[puffN_++ & 7];
            p.x = x_ - kTail * 0.75f;
            p.y = h_ + 0.08f;
            p.life = 0.42f;
        }
    }

    if (!timber_ && x_ >= kNear) {
        timber_ = true;
        sys_->rumble(0.16f, 0.05f, 60);
        sys_->apu.noiseBurst(0.16f, 240.f, 0.1f);
    }

    if (!std::isfinite(x_) || !std::isfinite(v_) || !std::isfinite(att_)) {
        fail("lost the sled", "LOST");
        return;
    }
    if (x_ > kFar - 0.02f || h_ < kDeck - 0.45f) {
        fail("past the platform", "PAST");
        return;
    }
    if (legT_ > kLegLimit) {
        fail("too late", "TOO LATE");
        return;
    }

    const bool aboard = (x_ - kTail) >= kNear - 0.02f && (x_ + kNose) <= kFar - 0.02f;
    const bool atMark = std::fabs(x_ - kMark) <= kLevelTol;
    const bool level = std::fabs(att_) <= kAttMax && std::fabs(h_ - kDeck) <= 0.06f;
    const bool stopped = v_ <= kStop;
    if (aboard && atMark && level && stopped) {
        hold_ += kDt;
        still_ = 0.f;
        if (hold_ >= kHoldNeed) win();
        return;
    }
    hold_ = 0.f;
    if (stopped && flat) {
        still_ += kDt;
        if (still_ >= kStillNeed) {
            if (x_ < kNear || !aboard) fail("short of the platform", "SHORT");
            else fail("not level", "NOT LEVEL");
        }
    } else {
        still_ = 0.f;
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::audio() {
    if (mode_ != Mode::Run) {
        sys_->apu.noise(0.f, 700.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
        return;
    }
    float scrape = clampf(v_ / 14.f, 0.f, 1.f) * (brake_ > 0.4f ? 0.08f : 0.035f);
    if (x_ >= kNear) scrape *= 0.55f;
    sys_->apu.noise(scrape, 420.f + v_ * 38.f, false);
    if (brake_ > 0.5f && v_ > 0.5f) sys_->apu.tone(2, 86.f, 0.035f);
    else sys_->apu.tone(2, 0.f, 0.f);
    if (hold_ > 0.02f) sys_->setLight(40, 170, 80);
    else if (brake_ > 0.6f) sys_->setLight(170, 120, 40);
    else sys_->setLight(50, 90, 150);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 5, 10);
    uint16_t mid = gs::rgb4(6, 9, 13);
    uint16_t hor = gs::rgb4(12, 12, 13);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 6, 5), 0.35f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(12, 14, 10), 0.3f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.62f ? lerpC(zen, mid, t / 0.62f) : lerpC(mid, hor, (t - 0.62f) / 0.38f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(8, 10, 13));
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog, bool shadow) {
    if (ht < 1.1f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -4000L, 4000L));
    if (s.x > gs::SCREEN_W + 4 || s.y > gs::SCREEN_H + 4) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal) {
    if (destH < 1.5f || m.h < 1) return;
    float sc = destH / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(float(m.w) * sc)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(sx - ax * sc)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(sy - ay * sc)), -4000L, 4000L));
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal, int fog) {
    if (w < 1.1f || h < 1.1f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -4000L, 4000L));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    const float adv = 17.f * scale;
    x -= float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, std::max(8.f, g.h * scale), pal, false);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    const bool framing = mode_ == Mode::Win || mode_ == Mode::Fail || hold_ > 0.02f || mode_ == Mode::Title;
    float dist = std::max(0.f, kMark - x_);
    float span = framing ? 13.5f : clampf(13.f + dist * 0.16f, 13.f, 30.f);
    float wantS = 300.f / span;
    float lead = (mode_ == Mode::Run && !framing) ? clampf(v_ * 0.28f, 1.5f, 7.f) : 0.4f;
    float wantX = framing ? kMark + 0.6f : x_ + lead;
    // camH below the deck puts the bay up in the picture, clear of the bottom copy.
    float wantH = framing ? kDeck - 0.55f : h_ * 0.62f + kDeck * 0.38f + 0.4f;
    if (mode_ == Mode::Title) {
        wantX = kMark + 1.8f;
        wantH = kDeck - 1.55f;
        wantS = 15.5f;
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else if (snap_) {
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
        snap_ = false;
    } else {
        camX_ += (wantX - camX_) * 0.14f;
        camH_ += (wantH - camH_) * 0.14f;
        camS_ += (wantS - camS_) * 0.12f;
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(legT_ * 86.f) * shake_ * 4.f;
        shy_ = std::cos(legT_ * 64.f) * shake_ * 2.5f;
        shake_ *= 0.88f;
        if (shake_ < 0.04f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }

    const float scale = camS_;
    const float ax = 168.f + shx_;
    const float ay = 142.f + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX_) * scale;
        sy = ay - (wy - camH_) * scale;
    };
    float left = camX_ - (ax + 30.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 30.f) / scale;

    if (mode_ == Mode::Title) {
        text("SLED PLAT", 160, 20, 1.05f, PAL_HUD);
        text("STOP LEVEL", 160, 44, 0.62f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 24, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 22, 1.05f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("LEVEL", 160, 20, 1.15f, PAL_GOOD);
    }

    for (const Flake& f : flakes_) spr(art_.flake, f.x, f.y, f.s + 1.5f, PAL_SNOW, false, 0);

    const SledImg& sled = art_.sled[pose()];
    float ssx, ssy;
    project(x_, h_, ssx, ssy);
    float dest = float(sled.img.h) / sled.ppm * scale;
    sprAnchor(sled.img, sled.ax, sled.ay, ssx, ssy, std::max(12.f, dest), PAL_SLED);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float px, py;
        project(p.x, p.y, px, py);
        spr(art_.puff, px, py, 6.f + (1.f - p.life) * 10.f, PAL_DUST, false, int((1.f - p.life) * 8));
    }

    float shx, shy;
    project(x_, h_, shx, shy);
    spr(art_.shade, shx, shy + 2.f, std::max(4.f, 0.55f * scale), PAL_DUST, false, 0);

    // Bay posts, the LEVEL board, and the gold fascia that is the only legal stop.
    for (float side : {-1.f, 1.f}) {
        float px, py;
        project(kMark + side * kLevelTol, kDeck, px, py);
        float ht = std::clamp(2.7f * scale, 16.f, 90.f);
        spr(art_.post, px, py - ht * 0.46f, ht, PAL_LAMP, side < 0.f, 0);
    }
    float bx, by;
    project(kMark, kDeck + 2.55f, bx, by);
    spr(art_.board, bx, by, std::clamp(0.55f * scale, 10.f, 28.f), PAL_SIGN, false, 0);
    spr(art_.word, bx, by, std::clamp(0.38f * scale, 8.f, 20.f), PAL_AMBER, false, 0);
    project(kFar - 0.4f, kDeck, bx, by);
    spr(art_.lamp, bx, by - std::clamp(1.3f * scale, 10.f, 36.f) * 0.4f, std::clamp(1.3f * scale, 10.f, 36.f), PAL_LAMP,
         false, 0);

    float f0, f1, fy;
    project(kMark - kLevelTol, kDeck, f0, fy);
    project(kMark + kLevelTol, kDeck, f1, fy);
    float fasciaH = std::max(4.f, 0.42f * scale);
    sprBox(art_.fascia, (f0 + f1) * 0.5f, fy - fasciaH * 0.15f, std::max(6.f, f1 - f0), fasciaH, PAL_MARK);

    project(kMark + 4.6f, kDeck, bx, by);
    float houseH = std::clamp(3.15f * scale, 18.f, 110.f);
    spr(art_.house, bx, by - houseH * 0.48f, houseH, PAL_HOUSE, false, 0);

    float deckH = std::max(4.f, 0.28f * scale);
    float step = 3.6f;
    float p0 = std::max(kNear, std::floor(left / step) * step);
    for (float wx = p0; wx < kFar && wx < right + step; wx += step) {
        float x0 = std::max(wx, kNear);
        float x1 = std::min(wx + step, kFar);
        float s0, y0, s1, y1;
        project(x0, kDeck, s0, y0);
        project(x1, kDeck, s1, y1);
        sprBox(art_.plank, (s0 + s1) * 0.5f, y0, std::max(2.f, s1 - s0 + 1.f), deckH, PAL_TIMBER);
    }
    {
        float s0, top, s1, bot;
        project(kNear - 0.15f, kDeck, s0, top);
        project(kNear + 0.35f, kDeck - kLip, s1, bot);
        sprBox(art_.riser, (s0 + s1) * 0.5f, top, std::max(3.f, s1 - s0), std::max(3.f, bot - top), PAL_TIMBER);
    }
    for (float wx = kNear + 1.6f; wx < kFar - 0.6f; wx += 4.2f) {
        float gx, gy, dx, dy;
        project(wx, 0.15f, gx, gy);
        project(wx, kDeck, dx, dy);
        float ht = gy - dy;
        if (ht < 6.f) continue;
        spr(art_.trestle, dx, dy + ht * 0.5f, ht, PAL_TIMBER, false, 1);
    }

    for (float wx = kGradeEnd + 2.f; wx < kNear - 1.5f; wx += 3.4f) {
        float px, py;
        project(wx, road(wx) + 0.05f, px, py);
        spr(art_.chev, px, py - 2.f, std::max(5.f, 0.42f * scale), PAL_MARK, false, 0);
    }

    // Snow pack. Under the deck the fill stops at the cavity so the trestles read.
    float col = std::clamp(22.f / scale, 0.9f, 1.8f);
    float yCut = camH_ - (gs::SCREEN_H + 12.f - ay) / scale;
    for (float wx = std::floor(left / col) * col; wx < right; wx += col) {
        float mid = wx + col * 0.5f;
        float top = road(mid);
        bool deck = mid >= kNear && mid <= kFar;
        if (deck) top = 0.15f;
        else if (mid > kFar) top = std::min(top, 0.2f);
        float base = std::min(top - 0.5f, yCut);
        float s0, syTop, s1, syBase;
        project(wx, top, s0, syTop);
        project(wx + col, base, s1, syBase);
        if (syBase - syTop < 2.f) continue;
        float tile = 26.f;
        int rows = 0;
        for (float y = syTop; y < syBase && y < gs::SCREEN_H + 8.f && rows < 6; y += tile - 1.f, rows++) {
            float th = std::min(tile, syBase - y);
            sprBox(art_.snow, (s0 + s1) * 0.5f, y, std::max(2.f, s1 - s0 + 1.f), th, PAL_SNOW, mid > kFar ? 4 : 0);
        }
    }
    {
        float s0, top, s1, bot;
        project(std::max(kNear, left), kDeck - 0.05f, s0, top);
        project(std::min(kFar, right), 0.15f, s1, bot);
        if (s1 > s0 + 2.f && bot > top + 2.f) sprBox(art_.rock, (s0 + s1) * 0.5f, top, s1 - s0, bot - top, PAL_ROCK, 6);
    }

    const float trees[] = {10.f, 22.f, 34.f, 47.f, 58.f, 70.f, 116.f, 126.f};
    for (float tx : trees) {
        if (tx < left - 4.f || tx > right + 4.f) continue;
        float ground = (tx > kFar || (tx >= kNear && tx <= kFar)) ? 0.2f : road(tx);
        float px, py;
        project(tx, ground, px, py);
        float ht = std::clamp((3.6f + std::fmod(tx, 3.f)) * scale * 0.55f, 16.f, 92.f);
        int fog = tx > kFar ? 7 : (tx < 30.f ? 3 : 1);
        spr(art_.pine, px, py - ht * 0.46f, ht, PAL_PINE, tx > kMark, fog);
    }
    if (kGradeEnd - 8.f > left && kGradeEnd - 8.f < right) {
        float px, py;
        project(kGradeEnd - 8.f, road(kGradeEnd - 8.f), px, py);
        float ht = std::clamp(2.2f * scale, 14.f, 60.f);
        spr(art_.shed, px, py - ht * 0.45f, ht, PAL_HOUSE, false, 3);
    }
    if (kFar + 3.f > left && kFar + 3.f < right) {
        float px, py;
        project(kFar + 2.4f, 0.15f, px, py);
        spr(art_.rock, px, py - std::max(6.f, 0.7f * scale) * 0.4f, std::max(6.f, 0.7f * scale), PAL_ROCK, false, 2);
    }

    for (int i = 0; i < 3; i++) {
        float sxr = std::fmod(30.f + float(i) * 150.f - camX_ * scale * 0.12f, 520.f);
        if (sxr < -80.f) sxr += 520.f;
        spr(art_.ridge, sxr, 118.f, 26.f + float(i % 2) * 8.f, PAL_FAR, false, 8);
    }
    for (int i = 0; i < 4; i++) {
        float sxc = std::fmod(20.f + float(i) * 120.f - camX_ * scale * 0.04f + anim_ * 4.f, 460.f);
        if (sxc < -40.f) sxc += 460.f;
        spr(art_.cloud, sxc, 26.f + float(i % 3) * 14.f, 12.f + float(i % 2) * 4.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 286.f, 34.f, 18.f, PAL_SKY, false, 0);

    if (mode_ == Mode::Title) {
        hudC(22, "UP SIT BACK    DOWN PUSH    Z BRAKE", PAL_HUD);
        hudC(23, "STOP THE DOOR LEVEL WITH THE BAY", PAL_GOOD);
        hudC(24, "CLOSE IS NOT LEVEL", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(26, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START RUNS   ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(24, why_, PAL_BAD);
        hudC(26, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(23, "LEVEL WITH THE PLATFORM", PAL_GOOD);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%.1f S", legT_);
        hudC(25, buf, PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(1, 1, buf, v_ < 1.f ? PAL_GOOD : PAL_HUD);
        // On the grade the nose should follow the snow. On the timber it has to sit flat.
        float rel = x_ >= kNear ? att_ : att_ - std::atan(roadSlope(x_));
        const char* nose = "NOSE OK";
        int np = PAL_GOOD;
        if (rel > kAttMax) {
            nose = "NOSE UP";
            np = PAL_AMBER;
        } else if (rel < -kAttMax) {
            nose = "NOSE DN";
            np = PAL_AMBER;
        }
        hud(28, 1, nose, np);

        float err = kMark - x_;
        const char* line = "TO THE PLATFORM";
        int pal = PAL_HUD;
        if (std::fabs(err) <= kLevelTol && std::fabs(att_) <= kAttMax && v_ <= kStop) {
            line = "LEVEL";
            pal = PAL_GOOD;
        } else if (std::fabs(err) <= kLevelTol && std::fabs(att_) <= kAttMax) {
            line = "IN THE BAY";
            pal = PAL_GOOD;
        } else if (std::fabs(err) <= kLevelTol) {
            line = "SIT LEVEL";
            pal = PAL_AMBER;
        } else if (err < 0.f) {
            line = "PAST THE BAY";
            pal = PAL_BAD;
        } else if (x_ >= kNear) {
            std::snprintf(buf, sizeof buf, "BAY %4.1f M", err);
            line = buf;
            pal = PAL_AMBER;
        } else {
            std::snprintf(buf, sizeof buf, "PLAT %3.0f M", std::max(0.f, kNear - x_));
            line = buf;
        }
        hudC(2, line, pal);
        if (hold_ > 0.02f) {
            int n = std::clamp(int(hold_ / kHoldNeed * 5.f + 0.001f), 0, 5);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
            hudC(3, buf, PAL_GOOD);
        } else if (brake_ > 0.45f) {
            hudC(3, "BRAKE", PAL_AMBER);
        }
        hudC(26, "UP DOWN WEIGHT    Z BRAKE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 10, 13));
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.16f, 0.24f, 0.12f);
    sys.apu.setPatch(0, bellPatch());
    uint32_t r = 0x51ed1u;
    for (Flake& f : flakes_) {
        r = r * 1664525u + 1013904223u;
        f.x = float(r % 340);
        r = r * 1664525u + 1013904223u;
        f.y = float(r % 230);
        f.s = 1.4f + float(r % 3);
        f.v = 16.f + float((r >> 4) % 22);
    }
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) anim_ += kDt;
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    for (Puff& p : puffs_)
        if (p.life > 0.f) p.life = std::max(0.f, p.life - kDt);
    if (mode_ != Mode::Pause) {
        for (Flake& f : flakes_) {
            f.y += f.v * kDt;
            f.x += 10.f * kDt;
            if (f.y > 232.f) {
                f.y = -6.f;
                f.x = std::fmod(f.x + 37.f, 320.f);
            }
            if (f.x > 330.f) f.x -= 340.f;
        }
    }

    if (chime_ >= 0) {
        static const float notes[] = {494.f, 659.f, 784.f, 988.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.13f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.18f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        att_ = std::sin(anim_ * 0.7f) * 0.015f;
        h_ = kDeck;
        x_ = kMark;
        v_ = 0.f;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(640.f);
            startRun();
        } else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) {
            blip(500.f);
            mode_ = Mode::Run;
        } else if (pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        audio();
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            blip(480.f);
            startRun();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        blip(440.f);
        mode_ = Mode::Pause;
        sys.apu.noise(0.f, 700.f, false);
        sys.apu.tone(2, 0.f, 0.f);
        draw();
        return;
    }

    float weight = 0.f, brake = 0.f;
    bool push = false;
    if (bot_) pilot(weight, brake, push);
    else human(weight, brake, push);
    physics(weight, brake, push);
    audio();
    draw();
}

}  // namespace sledplat
