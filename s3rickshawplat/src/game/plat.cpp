#include "game/plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace rickplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kFace = 72.f;
constexpr float kEnd = 104.f;
constexpr float kMark = 86.f;
constexpr float kPlat = 1.72f;
constexpr float kStopSpd = 0.22f;
constexpr float kHoldNeed = 0.48f;
constexpr float kXTol = 0.95f;
constexpr float kHTol = 0.11f;
constexpr float kLegLimit = 36.f;
constexpr float kBrake = 6.2f;
constexpr float kDrive = 2.35f;
constexpr float kLift = 1.25f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (beside_ || x_ > kFace - 6.f) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    beside_ = false;
    hold_ = 0.f;
    idle_ = 0.f;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    snapCam_ = true;
    x_ = kFace - 28.f;
    step_ = kPlat - 0.35f;
    v_ = 2.6f;
    liftV_ = 0.f;
    camX_ = kMark - 4.f;
    camH_ = kPlat + 0.8f;
    camS_ = 3.4f;
}

void Game::startRun() {
    x_ = 8.f;
    step_ = 0.95f;
    v_ = 4.6f;
    lift_ = 0.f;
    liftV_ = 0.f;
    pedal_ = 0.f;
    beside_ = false;
    hold_ = 0.f;
    idle_ = 0.f;
    legT_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    puffN_ = 0;
    shake_ = 0.f;
    for (Puff& p : puffs_) p = {};
    mode_ = Mode::Run;
    snapCam_ = true;
    camX_ = x_ + 10.f;
    camH_ = 1.6f;
    camS_ = 3.6f;
    blip(392.f);
}

void Game::pilot(float& pedal, float& lift) const {
    float err = kMark - x_;
    float hErr = kPlat - step_;
    lift = clampf(hErr * 5.2f - liftV_ * 0.9f, -1.f, 1.f);
    if (x_ > kFace - 14.f && step_ > kPlat + 0.06f) lift = -1.f;

    if (err < 1.15f && std::fabs(hErr) < 0.22f) {
        if (v_ > 0.06f) pedal = -1.f;
        else if (v_ < -0.03f) pedal = 0.4f;
        else pedal = 0.f;
        return;
    }
    float vStop = std::sqrt(std::max(0.f, 2.f * (kBrake * 0.82f) * std::max(0.f, err)));
    float vWant = err > 24.f ? 6.6f : std::min(5.4f, std::max(0.48f, vStop * 0.78f));
    if (v_ > vWant + 0.08f) pedal = -1.f;
    else if (v_ < vWant - 0.14f) pedal = 1.f;
    else pedal = 0.12f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    banner_ = "LEVEL";
    step_ = kPlat;
    v_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.2f, 0.05f, 120);
    sys_->setLight(40, 140, 50);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    shake_ = 1.f;
    sys_->rumble(0.4f, 0.18f, 150);
    sys_->setLight(140, 28, 16);
    sys_->apu.noiseBurst(0.32f, 220.f, 0.24f);
}

void Game::physics(float pedal, float lift) {
    pedal_ = clampf(pedal, -1.f, 1.f);
    lift_ = clampf(lift, -1.f, 1.f);
    legT_ += kDt;

    liftV_ += (lift_ - liftV_) * 7.f * kDt;
    step_ += liftV_ * kLift * kDt;
    step_ = clampf(step_, 0.55f, 3.1f);

    float accel = pedal_ > 0.f ? pedal_ * kDrive : pedal_ * kBrake;
    accel -= v_ * 0.09f;
    v_ += accel * kDt;
    v_ = clampf(v_, -1.8f, 8.5f);
    if (std::fabs(v_) < 0.03f && std::fabs(pedal_) < 0.04f) v_ = 0.f;
    x_ += v_ * kDt;

    beside_ = x_ >= kFace - 0.4f && x_ <= kEnd - 0.2f;

    if (x_ > kEnd + 0.15f) {
        fail("past the platform", "PAST");
        return;
    }
    if (x_ > kFace + 0.25f && x_ < kEnd && step_ > kPlat + 0.48f) {
        fail("clipped the lip", "CLIPPED");
        return;
    }
    if (x_ > kFace - 0.05f && x_ < kEnd && step_ < kPlat - 0.62f) {
        fail("under the lip", "TOO LOW");
        return;
    }
    if (v_ < -0.25f && x_ < kFace - 12.f && legT_ > 4.f) {
        fail("rolled back", "BACK");
        return;
    }

    bool atMark = std::fabs(x_ - kMark) <= kXTol;
    bool level = std::fabs(step_ - kPlat) <= kHTol;
    bool stopped = std::fabs(v_) <= kStopSpd;
    if (beside_ && atMark && level && stopped) {
        hold_ += kDt;
        idle_ = 0.f;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else {
        hold_ = 0.f;
    }

    bool settled = std::fabs(v_) <= 0.04f && pedal_ <= 0.02f;
    if (settled && !(beside_ && atMark && level)) {
        idle_ += kDt;
        if (idle_ > 0.85f) {
            if (!level && x_ > kFace) fail("not level", "NOT LEVEL");
            else fail("missed the platform", "MISSED");
            return;
        }
    } else if (!settled) {
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
    float roll = clampf(std::fabs(v_) / 8.f, 0.f, 1.f) * 0.04f;
    sys_->apu.noise(roll, 280.f + std::fabs(v_) * 30.f, false);
    if (std::fabs(liftV_) > 0.2f) sys_->apu.tone(2, 90.f + std::fabs(liftV_) * 40.f, 0.02f);
    else sys_->apu.tone(2, 0.f, 0.f);
    bool at = std::fabs(x_ - kMark) <= kXTol && std::fabs(step_ - kPlat) <= kHTol;
    if (at) sys_->setLight(40, 130, 50);
    else if (x_ > kFace) sys_->setLight(40, 90, 30);
    else sys_->setLight(80, 40, 16);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 6, 11);
    uint16_t mid = gs::rgb4(8, 10, 13);
    uint16_t hor = gs::rgb4(14, 11, 7);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 4, 3), 0.35f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(8, 13, 8), 0.3f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, hor, (t - 0.55f) / 0.45f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(6, 6, 5));
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
    float span = framing ? 42.f : 58.f;
    float wantS = 220.f / span;
    float lead = (mode_ == Mode::Run && !framing) ? clampf(v_ * 0.45f, 0.f, 8.f) : 0.f;
    float wantX = framing ? kMark : x_ + 2.f + lead;
    float wantH = framing ? kPlat + 0.7f : 1.7f;
    if (mode_ == Mode::Title) {
        wantX = kMark - 6.f;
        wantH = kPlat + 1.1f;
        wantS = 3.2f;
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
        shx_ = std::sin(legT_ * 70.f) * shake_ * 3.5f;
        shy_ = std::cos(legT_ * 48.f) * shake_ * 2.f;
        shake_ *= 0.9f;
        if (shake_ < 0.05f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }

    const float scale = camS_;
    const float ax = 148.f + shx_;
    const float ay = 128.f + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX_) * scale;
        sy = ay - (wy - camH_) * scale;
    };

    if (mode_ == Mode::Title) text("RICKSHAW PLAT", 160, 20, 0.72f, PAL_HUD);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 22, 1.0f, PAL_HUD);
    else if (mode_ == Mode::Fail) text(banner_, 160, 18, 0.95f, PAL_BAD);
    else if (mode_ == Mode::Win) text("LEVEL", 160, 16, 1.05f, PAL_GOOD);

    for (int i = 0; i < 3; i++) {
        float sxc = std::fmod(20.f + float(i) * 140.f - camX_ * scale * 0.04f + anim_ * 2.f, 460.f);
        if (sxc < -50.f) sxc += 460.f;
        spr(art_.cloud, sxc, 28.f + float(i) * 10.f, 12.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 268.f, 22.f, 18.f, PAL_SKY, false, 0);

    float sx, sy;
    project(kFace - 16.f, 2.4f, sx, sy);
    spr(art_.tree, sx, sy, std::clamp(3.6f * scale, 16.f, 70.f), PAL_TREE, false, 2);
    project(kEnd + 6.f, 2.2f, sx, sy);
    spr(art_.tree, sx, sy, std::clamp(3.2f * scale, 14.f, 64.f), PAL_TREE, true, 2);

    float deckPx = std::max(4.f, 0.55f * scale);
    float band0, band1, bandY;
    project(kMark - kXTol, kPlat, band0, bandY);
    project(kMark + kXTol, kPlat, band1, bandY);
    sprBox(art_.stripe, (band0 + band1) * 0.5f, bandY - deckPx * 0.15f, std::max(8.f, band1 - band0), deckPx * 0.7f,
           PAL_MARK);

    float stepW = 4.2f;
    float left = camX_ - (ax + 30.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 30.f) / scale;
    for (float wx = kFace; wx < kEnd; wx += stepW) {
        if (wx + stepW < left || wx > right) continue;
        float s0, y0, s1, y1;
        project(wx, kPlat, s0, y0);
        project(std::min(wx + stepW, kEnd), kPlat, s1, y1);
        sprBox(art_.stone, (s0 + s1) * 0.5f, y0, std::max(2.f, s1 - s0 + 1.f), deckPx, PAL_STONE);
        float hx, hy;
        project((wx + std::min(wx + stepW, kEnd)) * 0.5f, kPlat + 1.6f, hx, hy);
        spr(art_.wall, hx, hy, std::clamp(2.4f * scale, 12.f, 48.f), PAL_WALL, false, 0);
    }
    project(kFace + 1.2f, kPlat + 0.15f, sx, sy);
    spr(art_.lip, sx, sy, std::max(6.f, 0.45f * scale), PAL_STONE, false, 0);
    project((kFace + kEnd) * 0.5f, kPlat + 2.35f, sx, sy);
    spr(art_.awn, sx, sy, std::clamp(1.1f * scale, 8.f, 28.f), PAL_AWN, false, 0);
    project(kEnd - 2.5f, kPlat + 1.35f, sx, sy);
    spr(art_.lamp, sx, sy, std::clamp(1.7f * scale, 10.f, 36.f), PAL_LAMP, false, 0);

    float cabH = std::clamp(2.15f * scale, 18.f, 78.f);
    float bob = std::sin(anim_ * (2.f + std::fabs(v_) * 0.4f)) * 0.02f;
    float hsx, hsy;
    project(x_, step_ + 0.42f + bob, hsx, hsy);
    spr(art_.cab, hsx, hsy, cabH, PAL_CAB, false, 0);
    float wh = cabH * 0.38f;
    project(x_ - 1.55f, 0.28f, sx, sy);
    spr(art_.wheel, sx, sy, wh, PAL_MAN, false, 0);
    project(x_ + 1.45f, 0.28f, sx, sy);
    spr(art_.wheel, sx, sy, wh, PAL_MAN, false, 0);
    int stride = (int(anim_ * (3.f + std::fabs(v_))) & 1);
    project(x_ - 2.55f, 0.95f, sx, sy);
    spr(art_.pull[stride], sx, sy, cabH * 0.72f, PAL_MAN, false, 0);
    project(x_ + 0.15f, step_ + 0.72f, sx, sy);
    spr(art_.rider, sx, sy, cabH * 0.32f, PAL_MAN, false, 0);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float px, py;
        project(p.x, p.y, px, py);
        spr(art_.dust, px, py, 6.f + (1.f - p.life) * 8.f, PAL_DUST, false, int((1.f - p.life) * 10));
    }

    float gStep = std::clamp(28.f / scale, 3.5f, 8.f);
    float g0 = std::floor(left / gStep) * gStep;
    for (float wx = g0; wx < right; wx += gStep) {
        float px, py;
        project(wx + gStep * 0.5f, 0.f, px, py);
        sprBox(art_.stone, px, py, gStep * scale + 2.f, std::max(18.f, gs::SCREEN_H - py), PAL_ROAD);
    }

    for (int i = 0; i < 2; i++) {
        float bx = 18.f + float(i) * 30.f + std::sin(anim_ * 0.6f + float(i)) * 2.f;
        float px, py;
        project(bx, 4.4f + float(i) * 0.4f, px, py);
        spr(art_.bird[int(anim_ * 5.f + i) & 1], px, py, std::max(5.f, 0.35f * scale), PAL_BIRD, i & 1, 1);
    }

    if (mode_ == Mode::Title) {
        hudC(16, "UP RAISES THE STEP", PAL_HUD);
        hudC(17, "DOWN LOWERS THE STEP", PAL_HUD);
        hudC(18, "Z PEDALS    X BRAKES", PAL_AMBER);
        hudC(20, "STOP THE FLOOR LEVEL", PAL_HUD);
        hudC(21, "WITH THE PLATFORM", PAL_GOOD);
        hudC(22, "A CLIP OR A MISS FAILS", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START ROLLS   ESC TITLE", PAL_AMBER);
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
        float gap = step_ - kPlat;
        std::snprintf(buf, sizeof buf, "LVL %+5.2f", gap);
        hud(1, 1, buf, std::fabs(gap) <= kHTol ? PAL_GOOD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(14, 1, buf, PAL_HUD);
        float leftM = kMark - x_;
        std::snprintf(buf, sizeof buf, "MARK %+5.1f", leftM);
        hud(26, 1, buf, std::fabs(leftM) <= kXTol ? PAL_GOOD : PAL_AMBER);
        const char* line = "TO THE PORCH";
        int pal = PAL_HUD;
        if (hold_ > 0.02f) {
            line = "HOLD LEVEL";
            pal = PAL_GOOD;
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
        hudC(26, "UP DOWN STEP   Z X PEDAL", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 6, 5));
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
        if (p.life > 0.f) p.life = std::max(0.f, p.life - kDt);

    if (chime_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.15f) {
            if (chime_ < 4) sys.apu.tone(0, notes[chime_], 0.07f);
            else sys.apu.tone(0, 0.f, 0.f);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 6) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        x_ = kFace - 26.f + std::sin(anim_ * 0.45f) * 1.8f;
        step_ = kPlat - 0.42f + std::sin(anim_ * 0.8f) * 0.06f;
        v_ = 2.2f;
        beside_ = false;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(494.f);
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
            blip(360.f);
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

    float pedal = 0.f, lift = 0.f;
    if (bot_) {
        pilot(pedal, lift);
    } else {
        if (pad.down(gs::BTN_UP)) lift += 1.f;
        if (pad.down(gs::BTN_DOWN)) lift -= 1.f;
        if (std::fabs(pad.axisY) > 0.2f) lift = pad.axisY;
        lift = clampf(lift, -1.f, 1.f);
        if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z)) pedal = 1.f;
        if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X)) pedal = -1.f;
        if (pad.accel > 0.12f) pedal = std::max(pedal, pad.accel);
        if (pad.brake > 0.12f) pedal = -std::max(-pedal, pad.brake);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(280.f);
            draw();
            return;
        }
    }

    bool was = beside_;
    physics(pedal, lift);
    if (!was && beside_ && mode_ == Mode::Run) {
        puffs_[puffN_ % 6] = {x_ - 1.6f, 0.12f, 0.4f};
        puffN_++;
    }
    if (std::fabs(v_) > 1.4f && mode_ == Mode::Run && (sys.frame % 7) == 0) {
        puffs_[puffN_ % 6] = {x_ - 1.7f, 0.08f, 0.32f};
        puffN_++;
    }
    audio();
    draw();
}

}  // namespace rickplat
