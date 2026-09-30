#include "game/plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace bikeplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kFace = 72.f;
constexpr float kEnd = 112.f;
constexpr float kMark = 88.f;
constexpr float kTail = 1.72f;
constexpr float kStopSpd = 0.16f;
constexpr float kHoldNeed = 0.55f;
constexpr float kXTol = 0.42f;
constexpr float kPTol = 0.055f;
constexpr float kLegLimit = 36.f;
constexpr float kBrake = 7.4f;
constexpr float kDrive = 3.4f;
constexpr float kGradePull = 8.2f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

// Nose-down on the approach, flat on the bay, diving past the end.
float gradeAt(float x) {
    if (x < kFace) {
        float t = clampf(x / kFace, 0.f, 1.f);
        float s = t * t * (3.f - 2.f * t);
        return -0.48f * (1.f - s);
    }
    if (x <= kEnd) return 0.f;
    return clampf((x - kEnd) * 0.55f, 0.f, 1.1f);
}

float deckAt(float x) {
    const float flat = 2.35f;
    if (x < kFace) {
        float t = clampf(x / kFace, 0.f, 1.f);
        float s = t * t * (3.f - 2.f * t);
        return 7.4f + (flat - 7.4f) * s;
    }
    if (x <= kEnd) return flat;
    float u = clampf((x - kEnd) / 3.f, 0.f, 1.f);
    return flat - u * u * 6.f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (aboard_ || x_ > kFace - 3.f) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    aboard_ = false;
    hold_ = 0.f;
    idle_ = 0.f;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    snapCam_ = true;
    x_ = kFace - 28.f;
    v_ = 4.f;
    pitch_ = gradeAt(x_);
    pitchV_ = 0.f;
    camX_ = kMark - 2.f;
    camH_ = deckAt(kMark) + 1.4f;
    camS_ = 8.f;
}

void Game::startRun() {
    x_ = 6.f;
    v_ = 6.6f;
    pitch_ = gradeAt(x_);
    pitchV_ = 0.f;
    pedal_ = 0.f;
    brake_ = 0.f;
    nudge_ = 0.f;
    hold_ = 0.f;
    idle_ = 0.f;
    roll_ = 0.f;
    legT_ = 0.f;
    won_ = false;
    over_ = false;
    aboard_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    shake_ = 0.f;
    for (Puff& p : puffs_) p = {};
    mode_ = Mode::Run;
    snapCam_ = true;
    camX_ = x_ + 6.f;
    camH_ = deckAt(x_) + 1.6f;
    camS_ = 10.f;
    blip(480.f);
}

void Game::pilot(float& pedal, float& brake, float& nudge) const {
    float err = kMark - x_;
    float vStop = std::sqrt(std::max(0.f, 2.f * (kBrake * 0.78f) * std::max(0.f, err)));
    float vWant = err > 24.f ? 9.2f : std::min(8.4f, std::max(0.15f, vStop * 0.9f));
    if (err < 4.f) vWant = std::min(vWant, 1.7f);
    if (err < 1.3f) vWant = std::min(vWant, 0.42f);
    if (std::fabs(pitch_) > kPTol && err < 1.6f && err > -0.1f) vWant = std::max(vWant, 0.28f);
    if (err < kXTol && std::fabs(pitch_) <= kPTol && std::fabs(v_) < 0.4f) vWant = 0.f;

    if (v_ > vWant + 0.08f) {
        brake = 1.f;
        pedal = 0.f;
    } else if (v_ < vWant - 0.12f) {
        pedal = 1.f;
        brake = 0.f;
    } else {
        pedal = 0.12f;
        brake = 0.f;
    }
    if (err < 0.05f && v_ > 0.05f) {
        brake = 1.f;
        pedal = 0.f;
    }
    float g = gradeAt(x_);
    nudge = clampf(-(pitch_ - g) * 5.2f - pitchV_ * 0.55f, -1.f, 1.f);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    banner_ = "LEVEL";
    v_ = 0.f;
    pitch_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.2f, 0.05f, 120);
    sys_->setLight(30, 150, 70);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    shake_ = 1.f;
    sys_->rumble(0.45f, 0.18f, 150);
    sys_->setLight(150, 30, 20);
    sys_->apu.noiseBurst(0.32f, 220.f, 0.24f);
}

void Game::physics(float pedal, float brake, float nudge) {
    pedal_ = clampf(pedal, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    nudge_ = clampf(nudge, -1.f, 1.f);
    legT_ += kDt;

    float g = gradeAt(x_);
    float accel = pedal_ * kDrive - brake_ * kBrake;
    accel += -g * kGradePull;
    accel -= v_ * 0.42f;
    v_ += accel * kDt;
    v_ = clampf(v_, -3.2f, 12.f);
    if (std::fabs(v_) < 0.03f && pedal_ < 0.05f && brake_ > 0.2f) v_ = 0.f;
    x_ += v_ * kDt;
    roll_ += v_ * kDt;

    pitchV_ += ((g - pitch_) * 16.f + nudge_ * 3.2f - pitchV_ * 7.5f) * kDt;
    pitch_ += pitchV_ * kDt;
    pitch_ = clampf(pitch_, -1.2f, 1.2f);

    aboard_ = (x_ - kTail) >= kFace - 0.05f && x_ <= kEnd - 0.15f && x_ >= kFace;

    if (brake_ > 0.45f && std::fabs(v_) > 1.4f) {
        for (Puff& p : puffs_) {
            if (p.life > 0.f) continue;
            p.x = x_ - kTail;
            p.y = deckAt(x_) + 0.15f;
            p.life = 0.35f;
            break;
        }
    }

    if (x_ > kEnd + 0.08f) {
        fail("past the platform", "PAST");
        return;
    }
    if (v_ < -0.35f && x_ < kFace - 12.f && legT_ > 4.f) {
        fail("turned back", "TURNED");
        return;
    }

    bool atMark = std::fabs(x_ - kMark) <= kXTol;
    bool level = std::fabs(pitch_) <= kPTol;
    bool stopped = std::fabs(v_) <= kStopSpd;
    if (aboard_ && atMark && level && stopped) {
        hold_ += kDt;
        idle_ = 0.f;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else {
        hold_ = 0.f;
    }

    bool quiet = stopped && pedal_ <= 0.05f;
    if (quiet && !(aboard_ && atMark && level)) {
        idle_ += kDt;
        if (idle_ > 0.85f) {
            if (x_ < kFace - 0.3f) fail("short of the platform", "SHORT");
            else if (!level) fail("not level", "NOT LEVEL");
            else fail("missed the platform", "MISSED");
            return;
        }
    } else if (!quiet) {
        idle_ = 0.f;
    }
    if (mode_ == Mode::Run && legT_ > kLegLimit) fail("too late", "TOO LATE");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::audio() {
    if (mode_ != Mode::Run) {
        sys_->apu.noise(0.f, 400.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
        return;
    }
    float chain = clampf(std::fabs(v_) / 10.f, 0.f, 1.f) * 0.04f;
    if (brake_ > 0.4f) chain += 0.03f;
    sys_->apu.noise(chain, 280.f + std::fabs(v_) * 30.f, false);
    if (pedal_ > 0.4f) sys_->apu.tone(2, 90.f + v_ * 8.f, 0.03f);
    else sys_->apu.tone(2, 0.f, 0.f);
    bool at = std::fabs(x_ - kMark) <= kXTol && std::fabs(pitch_) <= kPTol;
    if (at) sys_->setLight(30, 140, 70);
    else if (x_ > kFace) sys_->setLight(40, 90, 40);
    else sys_->setLight(20, 50, 30);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 6, 11);
    uint16_t mid = gs::rgb4(6, 10, 13);
    uint16_t hor = gs::rgb4(12, 13, 9);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(11, 4, 3), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(8, 13, 8), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.5f ? lerpC(zen, mid, t / 0.5f) : lerpC(mid, hor, (t - 0.5f) / 0.5f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(5, 8, 7));
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal) {
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H + 4 || s.y + s.h < -4) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
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

    const bool framing = mode_ == Mode::Win || mode_ == Mode::Fail || hold_ > 0.02f;
    float span = framing ? 18.f : 28.f;
    float wantS = 220.f / span;
    float lead = (mode_ == Mode::Run && !framing) ? clampf(v_ * 0.35f, 0.f, 6.f) : 0.f;
    float wantX = framing ? kMark - 0.6f : x_ + 1.2f + lead;
    float wantH = framing ? deckAt(kMark) + 1.1f : deckAt(x_) + 1.5f;
    if (mode_ == Mode::Title) {
        wantX = kMark - 1.f;
        wantH = deckAt(kMark) + 1.6f;
        wantS = 11.f;
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else if (snapCam_) {
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
        snapCam_ = false;
    } else {
        camX_ += (wantX - camX_) * 0.12f;
        camH_ += (wantH - camH_) * 0.12f;
        camS_ += (wantS - camS_) * 0.1f;
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(legT_ * 70.f) * shake_ * 4.f;
        shy_ = std::cos(legT_ * 48.f) * shake_ * 2.2f;
        shake_ *= 0.9f;
        if (shake_ < 0.05f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }

    const float scale = camS_;
    const float ax = 150.f + shx_;
    const float ay = 132.f + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX_) * scale;
        sy = ay - (wy - camH_) * scale;
    };

    if (mode_ == Mode::Title) text("BIKE PLAT", 160, 22, 0.95f, PAL_HUD);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 24, 1.05f, PAL_HUD);
    else if (mode_ == Mode::Fail) text(banner_, 160, 20, 1.0f, PAL_BAD);
    else if (mode_ == Mode::Win) text("LEVEL", 160, 18, 1.1f, PAL_GOOD);

    for (int i = 0; i < 3; i++) {
        float sxc = std::fmod(30.f + float(i) * 140.f - camX_ * scale * 0.04f + anim_ * 4.f, 460.f);
        if (sxc < -40.f) sxc += 460.f;
        spr(art_.cloud, sxc, 28.f + float(i) * 10.f, 16.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 52.f, 24.f, 18.f, PAL_SKY, false, 0);

    float sx, sy;
    for (int i = 0; i < 5; i++) {
        float tx = 8.f + float(i) * 26.f;
        project(tx, deckAt(tx) + 2.4f, sx, sy);
        spr(art_.tree, sx, sy, std::clamp(3.6f * scale, 16.f, 70.f), PAL_TREE, i & 1, 3);
    }

    float band0, band1, bandY;
    project(kMark - kXTol, deckAt(kMark) + 0.22f, band0, bandY);
    project(kMark + kXTol, deckAt(kMark) + 0.22f, band1, bandY);
    sprBox(art_.stripe, (band0 + band1) * 0.5f, bandY, std::max(6.f, band1 - band0), std::max(3.f, 0.18f * scale),
           PAL_MARK);

    project(kMark, deckAt(kMark) + 2.5f, sx, sy);
    spr(art_.sign, sx, sy, std::clamp(1.3f * scale, 12.f, 36.f), PAL_SIGN, false, 0);
    project(kEnd - 2.2f, deckAt(kEnd) + 1.7f, sx, sy);
    spr(art_.lamp, sx, sy, std::clamp(1.8f * scale, 12.f, 40.f), PAL_LAMP, false, 0);

    float step = 2.4f;
    float left = camX_ - (ax + 30.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 30.f) / scale;
    float deckPx = std::max(3.f, 0.28f * scale);
    float plank0 = std::max(kFace, std::floor(left / step) * step);
    for (float wx = plank0; wx < kEnd && wx < right + step; wx += step) {
        float x0 = std::max(wx, kFace);
        float x1 = std::min(wx + step, kEnd);
        float s0, y0, s1, y1;
        project(x0, deckAt(kMark), s0, y0);
        project(x1, deckAt(kMark), s1, y1);
        sprBox(art_.plank, (s0 + s1) * 0.5f, y0, std::max(2.f, s1 - s0 + 1.f), deckPx, PAL_DECK);
    }
    for (float wx = kFace + 1.6f; wx < kEnd - 0.6f; wx += 4.6f) {
        float gx, gy, dx, dy;
        project(wx, deckAt(wx) - 2.2f, gx, gy);
        project(wx, deckAt(wx), dx, dy);
        float ht = gy - dy;
        if (ht < 4.f) continue;
        spr(art_.post, dx, dy + ht * 0.5f, ht, PAL_POST, false, 0);
    }

    int pose = 1;
    if (pitch_ < -0.08f) pose = 0;
    else if (pitch_ > 0.08f) pose = 2;
    float bx, by;
    float mid = x_ - kTail * 0.45f;
    project(mid, deckAt(mid) + 0.95f + pitch_ * 0.15f, bx, by);
    float bodyH = std::clamp(1.55f * scale, 22.f, 86.f);
    spr(art_.body[pose], bx, by, bodyH, PAL_BIKE, false, 0);

    auto wheelAt = [&](float wx) {
        float px, py;
        project(wx, deckAt(wx) + 0.28f, px, py);
        float wh = std::clamp(0.62f * scale, 10.f, 36.f);
        spr(art_.wheel, px, py, wh, PAL_WHEEL, std::sin(roll_ * 3.f) < 0.f, 0);
    };
    wheelAt(x_);
    wheelAt(x_ - kTail);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float px, py;
        project(p.x, p.y, px, py);
        spr(art_.dust, px, py, 6.f + (1.f - p.life) * 8.f, PAL_DUST, false, int((1.f - p.life) * 8));
    }

    float gStep = std::clamp(18.f / scale, 1.6f, 4.f);
    float g0 = std::floor(std::min(left, kFace) / gStep) * gStep;
    for (float wx = g0; wx < std::min(right, kFace); wx += gStep) {
        float px, py;
        float h = deckAt(wx + gStep * 0.5f);
        project(wx + gStep * 0.5f, h, px, py);
        sprBox(art_.path, px, py, gStep * scale + 2.f, std::max(10.f, 0.35f * scale), PAL_PATH);
    }

    if (mode_ == Mode::Title) {
        hudC(16, "Z PEDALS    X BRAKES", PAL_HUD);
        hudC(17, "UP LIFTS THE NOSE", PAL_HUD);
        hudC(18, "DOWN DROPS IT", PAL_AMBER);
        hudC(20, "STOP THE AXLE LEVEL", PAL_HUD);
        hudC(21, "WITH THE PLATFORM", PAL_GOOD);
        hudC(22, "CLOSE IS NOT LEVEL", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START RIDES   ESC TITLE", PAL_AMBER);
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
        std::snprintf(buf, sizeof buf, "PITCH %+5.2f", pitch_);
        hud(1, 1, buf, std::fabs(pitch_) <= kPTol ? PAL_GOOD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(16, 1, buf, PAL_HUD);
        float leftM = kMark - x_;
        std::snprintf(buf, sizeof buf, "MARK %+5.1f", leftM);
        hud(26, 1, buf, std::fabs(leftM) <= kXTol ? PAL_GOOD : PAL_AMBER);

        const char* line = "TO THE BAY";
        int pal = PAL_HUD;
        bool atMark = std::fabs(x_ - kMark) <= kXTol;
        bool level = std::fabs(pitch_) <= kPTol;
        if (atMark && level && std::fabs(v_) <= kStopSpd) {
            line = "LEVEL";
            pal = PAL_GOOD;
        } else if (atMark && level) {
            line = "HOLD STILL";
            pal = PAL_GOOD;
        } else if (x_ > kFace && !level) {
            line = pitch_ > 0.f ? "NOSE UP" : "NOSE DOWN";
            pal = PAL_AMBER;
        } else if (x_ > kMark + kXTol) {
            line = "PAST THE MARK";
            pal = PAL_BAD;
        } else if (x_ > kFace) {
            line = "TO THE MARK";
            pal = PAL_AMBER;
        }
        hudC(2, line, pal);
        if (hold_ > 0.02f) {
            int n = std::clamp(int(hold_ / kHoldNeed * 5.f + 0.001f), 0, 5);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
            hudC(3, buf, PAL_GOOD);
        }
        hudC(26, "UP DOWN NOSE    Z PEDAL  X BRAKE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(5, 8, 7));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.08f, 0.16f, 0.06f);
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
        if (p.life > 0.f) p.life = std::max(0.f, p.life - kDt * 1.4f);

    if (chime_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.16f) {
            if (chime_ < 4) sys.apu.tone(0, notes[chime_], 0.08f);
            else sys.apu.tone(0, 0.f, 0.f);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 6) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        x_ = kFace - 22.f + std::sin(anim_ * 0.55f) * 3.f;
        v_ = 3.f;
        pitch_ = gradeAt(x_);
        aboard_ = false;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(560.f);
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
            blip(400.f);
            mode_ = Mode::Run;
        } else if (pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        audio();
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    float pedal = 0.f, brake = 0.f, nudge = 0.f;
    if (bot_) {
        pilot(pedal, brake, nudge);
    } else {
        if (pad.down(gs::BTN_UP)) nudge += 1.f;
        if (pad.down(gs::BTN_DOWN)) nudge -= 1.f;
        if (std::fabs(pad.axisY) > 0.25f) nudge = pad.axisY;
        nudge = clampf(nudge, -1.f, 1.f);
        if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z)) pedal = 1.f;
        if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X)) brake = 1.f;
        if (pad.accel > 0.12f) pedal = std::max(pedal, pad.accel);
        if (pad.brake > 0.12f) brake = std::max(brake, pad.brake);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
            draw();
            return;
        }
    }

    physics(pedal, brake, nudge);
    audio();
    draw();
}

}  // namespace bikeplat
