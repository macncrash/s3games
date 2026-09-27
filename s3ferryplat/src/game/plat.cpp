#include "game/plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace ferryplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kNear = 96.f;
constexpr float kFar = 150.f;
constexpr float kMark = 132.f;
constexpr float kBow = 18.f;
constexpr float kStern = 15.f;
constexpr float kDeck = 3.55f;
constexpr float kStop = 0.28f;
constexpr float kHoldNeed = 0.48f;
constexpr float kXTol = 1.25f;
constexpr float kHTol = 0.16f;
constexpr float kLegLimit = 34.f;
constexpr float kBrake = 5.4f;
constexpr float kDrive = 2.6f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.16f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.2f, 0.55f, 0.2f};
    p.op[1] = {2.f, 0.28f, 0.02f, 0.22f, 0.3f, 0.16f};
    p.op[2] = {3.f, 0.08f, 0.02f, 0.24f, 0.2f, 0.18f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.2f;
    p.tone = 1400.f;
    return p;
}

float bowOf(float x) { return x + kBow; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (alongside_ || bowOf(x_) > kNear - 6.f) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    alongside_ = false;
    hold_ = 0.f;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    snapCam_ = true;
    x_ = kNear - 28.f;
    h_ = kDeck - 0.35f;
    v_ = 4.f;
    camX_ = kMark - 8.f;
    camH_ = kDeck + 1.2f;
    camS_ = 3.4f;
}

void Game::startRun() {
    x_ = 4.f;
    h_ = 2.15f;
    v_ = 6.4f;
    pump_ = 0.f;
    thrust_ = 0.f;
    alongside_ = false;
    hold_ = 0.f;
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
    camX_ = x_ + 16.f;
    camH_ = 2.4f;
    camS_ = 3.2f;
    blip(520.f);
}

void Game::pilot(float& thrust, float& pump) const {
    float bow = bowOf(x_);
    float err = kMark - bow;
    float dist = kNear - bow;
    float hWant = dist > 36.f ? 2.45f : kDeck;
    pump = clampf((hWant - h_) * 3.4f, -1.f, 1.f);

    float vStop = std::sqrt(std::max(0.f, 2.f * (kBrake * 0.92f) * std::max(0.f, err)));
    float vWant = err > 22.f ? 9.2f : std::min(8.4f, std::max(0.55f, vStop));
    if (err < 0.15f) vWant = 0.f;
    if (v_ > vWant + 0.12f) {
        thrust = -1.f;
    } else if (v_ < vWant - 0.18f && err > 0.4f) {
        thrust = 1.f;
    } else {
        thrust = 0.f;
    }
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    banner_ = "LEVEL";
    h_ = kDeck;
    v_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.25f, 0.08f, 150);
    sys_->setLight(40, 170, 90);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    shake_ = 1.f;
    sys_->rumble(0.5f, 0.25f, 170);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.4f, 280.f, 0.28f);
}

void Game::physics(float thrust, float pump) {
    thrust_ = clampf(thrust, -1.f, 1.f);
    pump_ = clampf(pump, -1.f, 1.f);
    legT_ += kDt;

    float rise = pump_ * 1.05f;
    h_ += rise * kDt;
    h_ = clampf(h_, 1.15f, 5.4f);

    float accel = thrust_ > 0.f ? thrust_ * kDrive : thrust_ * kBrake;
    accel -= v_ * 0.08f;
    v_ += accel * kDt;
    v_ = clampf(v_, -2.4f, 12.f);
    if (std::fabs(v_) < 0.04f && thrust_ == 0.f) v_ = 0.f;
    x_ += v_ * kDt;

    float bow = bowOf(x_);
    float stern = x_ - kStern;
    alongside_ = stern >= kNear - 0.4f && bow <= kFar + 0.05f && bow >= kNear;

    if (bow > kFar + 0.02f) {
        fail("missed the end", "MISSED");
        return;
    }
    if (bow > kNear - 0.2f && bow < kFar && h_ < kDeck - 0.72f) {
        fail("missed the end", "MISSED");
        return;
    }
    if (v_ < -0.15f && bow < kNear - 8.f && legT_ > 4.f) {
        fail("missed the end", "MISSED");
        return;
    }

    bool atMark = std::fabs(bow - kMark) <= kXTol;
    bool level = std::fabs(h_ - kDeck) <= kHTol;
    bool stopped = std::fabs(v_) <= kStop;
    bool aboard = stern >= kNear - 0.15f && bow <= kFar - 0.05f;
    if (aboard && atMark && level && stopped) {
        hold_ += kDt;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else {
        hold_ = 0.f;
    }
    if (std::fabs(v_) <= 0.05f && thrust_ <= 0.f && !(aboard && atMark && level)) {
        if (!aboard || bow < kMark - kXTol) fail("missed the end", "MISSED");
        else if (!level) fail("not level", "NOT LEVEL");
        else fail("missed the end", "MISSED");
        return;
    }
    if (mode_ == Mode::Run && legT_ > kLegLimit) fail("too late", "TOO LATE");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::audio() {
    if (mode_ != Mode::Run) {
        sys_->apu.noise(0.f, 500.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
        return;
    }
    float wash = clampf(std::fabs(v_) / 10.f, 0.f, 1.f) * 0.05f;
    sys_->apu.noise(wash, 420.f + std::fabs(v_) * 30.f, false);
    if (std::fabs(pump_) > 0.2f) sys_->apu.tone(2, 90.f + std::fabs(pump_) * 40.f, 0.03f);
    else sys_->apu.tone(2, 0.f, 0.f);
    bool at = std::fabs(bowOf(x_) - kMark) <= kXTol && std::fabs(h_ - kDeck) <= kHTol;
    if (at) sys_->setLight(40, 160, 80);
    else if (bowOf(x_) > kNear) sys_->setLight(40, 90, 150);
    else sys_->setLight(20, 50, 110);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 6, 12);
    uint16_t mid = gs::rgb4(6, 10, 14);
    uint16_t hor = gs::rgb4(12, 13, 11);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 5, 4), 0.35f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(10, 14, 11), 0.3f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.5f ? lerpC(zen, mid, t / 0.5f) : lerpC(mid, hor, (t - 0.5f) / 0.5f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(6, 9, 12));
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
    float span = framing ? 58.f : 72.f;
    float wantS = 250.f / span;
    float lead = (mode_ == Mode::Run && !framing) ? clampf(v_ * 0.55f, 0.f, 12.f) : 0.f;
    float wantX = framing ? kMark - 6.f : x_ + 6.f + lead;
    float wantH = framing ? kDeck + 1.1f : 2.6f;
    if (mode_ == Mode::Title) {
        wantX = kMark - 10.f;
        wantH = kDeck + 1.6f;
        wantS = 3.15f;
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
        camS_ += (wantS - camS_) * 0.12f;
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(legT_ * 80.f) * shake_ * 4.f;
        shy_ = std::cos(legT_ * 60.f) * shake_ * 2.5f;
        shake_ *= 0.9f;
        if (shake_ < 0.05f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }

    const float scale = camS_;
    const float ax = 150.f + shx_;
    const float ay = 118.f + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX_) * scale;
        sy = ay - (wy - camH_) * scale;
    };

    if (mode_ == Mode::Title) {
        text("FERRY PLAT", 160, 22, 1.0f, PAL_HUD);
        text("STOP LEVEL", 160, 46, 0.62f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 26, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 22, 1.05f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("LEVEL", 160, 20, 1.15f, PAL_GOOD);
    }

    float waterY;
    float dummy;
    project(0.f, 0.f, dummy, waterY);
    float left = camX_ - (ax + 30.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 30.f) / scale;

    for (int i = 0; i < 3; i++) {
        float sxc = std::fmod(20.f + float(i) * 140.f - camX_ * scale * 0.08f, 520.f);
        if (sxc < -50.f) sxc += 520.f;
        spr(art_.hill, sxc, 150.f, 26.f + float(i % 2) * 6.f, PAL_HILL, false, 8);
    }
    for (int i = 0; i < 3; i++) {
        float sxc = std::fmod(30.f + float(i) * 120.f - camX_ * scale * 0.04f + anim_ * 4.f, 460.f);
        if (sxc < -40.f) sxc += 460.f;
        spr(art_.cloud, sxc, 34.f + float(i) * 14.f, 16.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 40.f, 28.f, 18.f, PAL_SKY, false, 0);

    float deckH = std::max(4.f, 0.55f * scale);
    float band0, band1, bandY;
    project(kMark - kXTol, kDeck, band0, bandY);
    project(kMark + kXTol, kDeck, band1, bandY);
    sprBox(art_.stripe, (band0 + band1) * 0.5f, bandY, std::max(6.f, band1 - band0), deckH, PAL_END);

    float sx, sy;
    project(kMark, kDeck + 2.1f, sx, sy);
    spr(art_.sign, sx, sy, std::clamp(2.2f * scale, 12.f, 40.f), PAL_SIGN, false, 0);
    project(kFar - 2.f, kDeck + 1.6f, sx, sy);
    spr(art_.lamp, sx, sy, std::clamp(1.8f * scale, 12.f, 36.f), PAL_END, false, 0);
    project(kNear + 10.f, kDeck, sx, sy);
    float hut = std::clamp(3.6f * scale, 16.f, 70.f);
    spr(art_.shed, sx, sy - hut * 0.42f, hut, PAL_SHED, false, 0);

    float step = 6.f;
    float plank0 = std::max(kNear, std::floor(left / step) * step);
    for (float wx = plank0; wx < kFar && wx < right + step; wx += step) {
        float x0 = std::max(wx, kNear);
        float x1 = std::min(wx + step, kFar);
        float s0, y0, s1, y1;
        project(x0, kDeck, s0, y0);
        project(x1, kDeck, s1, y1);
        sprBox(art_.plank, (s0 + s1) * 0.5f, y0, std::max(2.f, s1 - s0 + 1.f), deckH, PAL_PIER);
    }
    for (float wx = kNear + 4.f; wx < kFar - 2.f; wx += 9.f) {
        float gx, gy, dx, dy;
        project(wx, -1.6f, gx, gy);
        project(wx, kDeck, dx, dy);
        float ht = gy - dy;
        if (ht < 4.f) continue;
        spr(art_.pile, dx, dy + ht * 0.5f, ht, PAL_PILE, false, 0);
    }

    float ferryH = std::clamp(4.6f * scale, 28.f, 120.f);
    float fsx, fsy;
    float bob = std::sin(anim_ * 1.6f) * 0.06f;
    project(x_, h_ + bob, fsx, fsy);
    spr(art_.ferry, fsx, fsy, ferryH, PAL_SHIP, false, 0);

    if (std::fabs(v_) > 0.4f) {
        float wx, wy;
        project(x_ - kStern - 1.2f, 0.15f, wx, wy);
        spr(art_.wake, wx, wy, std::max(6.f, 0.7f * scale), PAL_WAKE, v_ < 0.f, 1);
    }
    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float px, py;
        project(p.x, p.y, px, py);
        spr(art_.foam, px, py, 6.f + (1.f - p.life) * 8.f, PAL_FOAM, false, int((1.f - p.life) * 8));
    }

    float gStep = std::clamp(36.f / scale, 4.f, 10.f);
    float g0 = std::floor(left / gStep) * gStep;
    for (float wx = g0; wx < right; wx += gStep) {
        float px, py;
        project(wx + gStep * 0.5f, 0.f, px, py);
        float sw = gStep * scale + 2.f;
        sprBox(art_.water, px, py, sw, std::max(18.f, gs::SCREEN_H - py), PAL_WATER);
    }

    for (int i = 0; i < 2; i++) {
        float bx = 30.f + float(i) * 40.f + std::sin(anim_ * 0.6f + float(i)) * 4.f;
        float by = 5.5f + float(i) * 1.2f;
        float px, py;
        project(bx, by, px, py);
        spr(art_.gull[int(anim_ * 3.f + i) & 1], px, py, std::max(6.f, 0.55f * scale), PAL_GULL, i & 1, 1);
    }

    if (mode_ == Mode::Title) {
        hudC(16, "UP RAISES THE DECK", PAL_HUD);
        hudC(17, "DOWN LOWERS IT", PAL_HUD);
        hudC(18, "Z AHEAD     X ASTERN", PAL_AMBER);
        hudC(20, "STOP WITH THE DECK LEVEL", PAL_HUD);
        hudC(21, "WITH THE END OF THE PLATFORM", PAL_GOOD);
        hudC(22, "MISSING THE END FAILS THE LEG", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "START SAILS   ESC TITLE", PAL_AMBER);
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
        float gap = h_ - kDeck;
        std::snprintf(buf, sizeof buf, "LVL %+5.2f", gap);
        hud(1, 1, buf, std::fabs(gap) <= kHTol ? PAL_GOOD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(14, 1, buf, PAL_HUD);
        float leftM = kMark - bowOf(x_);
        std::snprintf(buf, sizeof buf, "END %+5.1f", leftM);
        hud(26, 1, buf, std::fabs(leftM) <= kXTol ? PAL_GOOD : PAL_AMBER);

        const char* line = "TO THE PLATFORM";
        int pal = PAL_HUD;
        bool atMark = std::fabs(bowOf(x_) - kMark) <= kXTol;
        bool level = std::fabs(gap) <= kHTol;
        if (atMark && level && std::fabs(v_) <= kStop) {
            line = "LEVEL";
            pal = PAL_GOOD;
        } else if (atMark && level) {
            line = "LEVEL  HOLD";
            pal = PAL_GOOD;
        } else if (bowOf(x_) > kNear && !level) {
            line = gap > 0.f ? "TOO HIGH" : "TOO LOW";
            pal = PAL_AMBER;
        } else if (bowOf(x_) > kMark + kXTol) {
            line = "PAST THE END";
            pal = PAL_BAD;
        } else if (bowOf(x_) > kNear) {
            line = "TO THE END";
            pal = PAL_AMBER;
        }
        hudC(2, line, pal);
        if (hold_ > 0.02f) {
            int n = std::clamp(int(hold_ / kHoldNeed * 5.f + 0.001f), 0, 5);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
            hudC(3, buf, PAL_GOOD);
        }
        hudC(26, "UP DOWN DECK    Z X DRIVE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 9, 12));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.2f, 0.1f);
    sys.apu.setPatch(0, bellPatch());
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
        if (chimeT_ > 0.16f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.18f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        x_ = kNear - 30.f + std::sin(anim_ * 0.45f) * 3.f;
        h_ = kDeck - 0.45f + std::sin(anim_ * 1.1f) * 0.12f;
        v_ = 3.f;
        alongside_ = false;
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
            blip(480.f);
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

    float thrust = 0.f, pump = 0.f;
    if (bot_) {
        pilot(thrust, pump);
    } else {
        if (pad.down(gs::BTN_UP)) pump += 1.f;
        if (pad.down(gs::BTN_DOWN)) pump -= 1.f;
        if (std::fabs(pad.axisY) > 0.2f) pump = pad.axisY;
        pump = clampf(pump, -1.f, 1.f);
        if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z)) thrust = 1.f;
        if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X)) thrust = -1.f;
        if (pad.accel > 0.12f) thrust = std::max(thrust, pad.accel);
        if (pad.brake > 0.12f) thrust = -std::max(-thrust, pad.brake);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(360.f);
            draw();
            return;
        }
    }

    bool was = alongside_;
    physics(thrust, pump);
    if (!was && alongside_ && mode_ == Mode::Run) {
        puffs_[puffN_ % 6] = {x_ + kBow - 1.f, 0.2f, 0.5f};
        puffN_++;
    }
    if (std::fabs(v_) > 2.f && mode_ == Mode::Run && (sys.frame % 7) == 0) {
        puffs_[puffN_ % 6] = {x_ - kStern, 0.15f, 0.4f};
        puffN_++;
    }
    audio();
    draw();
}

}  // namespace ferryplat
