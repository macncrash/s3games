#include "game/plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace bargeplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kFace = 70.f;
constexpr float kEnd = 138.f;
constexpr float kMark = 110.f;
constexpr float kBow = 15.f;
constexpr float kStern = 13.f;
constexpr float kPlat = 2.48f;
constexpr float kStopSpd = 0.26f;
constexpr float kHoldNeed = 0.55f;
constexpr float kXTol = 1.15f;
constexpr float kHTol = 0.15f;
constexpr float kLegLimit = 32.f;
constexpr float kBrake = 4.6f;
constexpr float kDrive = 2.15f;
constexpr float kCurrent = 0.18f;
constexpr float kPump = 1.15f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float bowOf(float x) { return x + kBow; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (alongside_ || bowOf(x_) > kFace - 4.f) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    alongside_ = false;
    hold_ = 0.f;
    idle_ = 0.f;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    snapCam_ = true;
    x_ = kFace - 36.f;
    deck_ = kPlat - 0.4f;
    v_ = 3.2f;
    pumpV_ = 0.f;
    camX_ = kMark - 6.f;
    camH_ = kPlat + 1.1f;
    camS_ = 3.3f;
}

void Game::startRun() {
    x_ = 6.f;
    deck_ = 1.85f;
    v_ = 5.2f;
    pump_ = 0.f;
    pumpV_ = 0.f;
    thrust_ = 0.f;
    alongside_ = false;
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
    camX_ = x_ + 14.f;
    camH_ = 2.2f;
    camS_ = 3.1f;
    blip(440.f);
}

void Game::pilot(float& thrust, float& pump) const {
    float bow = bowOf(x_);
    float err = kMark - bow;
    float hErr = kPlat - deck_;
    pump = clampf(hErr * 4.8f - pumpV_ * 0.8f, -1.f, 1.f);
    if (bow > kFace - 18.f && deck_ > kPlat + 0.08f) pump = -1.f;

    if (err < 1.4f && std::fabs(deck_ - kPlat) < 0.28f) {
        if (v_ > 0.08f) thrust = -1.f;
        else if (v_ < -0.04f) thrust = 0.45f;
        else thrust = -0.045f;
        return;
    }
    float vStop = std::sqrt(std::max(0.f, 2.f * (kBrake * 0.9f) * std::max(0.f, err)));
    float vWant = err > 26.f ? 7.4f : std::min(6.4f, std::max(0.55f, vStop * 0.86f));
    if (v_ > vWant + 0.1f) thrust = -1.f;
    else if (v_ < vWant - 0.16f) thrust = 1.f;
    else thrust = 0.15f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    banner_ = "LEVEL";
    deck_ = kPlat;
    v_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.22f, 0.06f, 140);
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
    sys_->rumble(0.45f, 0.2f, 160);
    sys_->setLight(150, 30, 20);
    sys_->apu.noiseBurst(0.35f, 240.f, 0.26f);
}

void Game::physics(float thrust, float pump) {
    thrust_ = clampf(thrust, -1.f, 1.f);
    pump_ = clampf(pump, -1.f, 1.f);
    legT_ += kDt;

    pumpV_ += (pump_ - pumpV_) * 6.f * kDt;
    deck_ += pumpV_ * kPump * kDt;
    deck_ = clampf(deck_, 0.9f, 4.6f);

    float accel = thrust_ > 0.f ? thrust_ * kDrive : thrust_ * kBrake;
    accel -= v_ * 0.07f;
    accel += kCurrent * (1.f - std::fabs(thrust_) * 0.4f);
    v_ += accel * kDt;
    v_ = clampf(v_, -2.2f, 10.f);
    if (std::fabs(v_) < 0.035f && std::fabs(thrust_) < 0.05f) v_ = 0.f;
    x_ += v_ * kDt;

    float bow = bowOf(x_);
    float stern = x_ - kStern;
    alongside_ = stern >= kFace - 0.5f && bow <= kEnd + 0.1f && bow >= kFace;

    if (bow > kEnd + 0.05f) {
        fail("past the platform", "PAST");
        return;
    }
    if (bow > kFace + 0.4f && bow < kEnd && deck_ > kPlat + 0.62f) {
        fail("scraped the platform", "SCRAPED");
        return;
    }
    if (bow > kFace - 0.2f && bow < kEnd && deck_ < kPlat - 0.85f) {
        fail("under the lip", "TOO LOW");
        return;
    }
    if (v_ < -0.2f && bow < kFace - 10.f && legT_ > 5.f) {
        fail("turned back", "TURNED");
        return;
    }

    bool atMark = std::fabs(bow - kMark) <= kXTol;
    bool level = std::fabs(deck_ - kPlat) <= kHTol;
    bool stopped = std::fabs(v_) <= kStopSpd;
    bool aboard = stern >= kFace - 0.2f && bow <= kEnd - 0.1f;
    if (aboard && atMark && level && stopped) {
        hold_ += kDt;
        idle_ = 0.f;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else {
        hold_ = 0.f;
    }

    bool settled = std::fabs(v_) <= 0.05f && thrust_ <= 0.02f;
    if (settled && !(aboard && atMark && level)) {
        idle_ += kDt;
        if (idle_ > 0.9f) {
            if (!level && bow > kFace) fail("not level", "NOT LEVEL");
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
    float wash = clampf(std::fabs(v_) / 9.f, 0.f, 1.f) * 0.045f;
    sys_->apu.noise(wash, 360.f + std::fabs(v_) * 24.f, false);
    if (std::fabs(pumpV_) > 0.15f) sys_->apu.tone(2, 70.f + std::fabs(pumpV_) * 50.f, 0.028f);
    else sys_->apu.tone(2, 0.f, 0.f);
    bool at = std::fabs(bowOf(x_) - kMark) <= kXTol && std::fabs(deck_ - kPlat) <= kHTol;
    if (at) sys_->setLight(30, 140, 70);
    else if (bowOf(x_) > kFace) sys_->setLight(30, 80, 130);
    else sys_->setLight(16, 40, 80);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(2, 5, 9);
    uint16_t mid = gs::rgb4(5, 9, 11);
    uint16_t hor = gs::rgb4(11, 12, 9);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(11, 4, 3), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(8, 13, 9), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.5f ? lerpC(zen, mid, t / 0.5f) : lerpC(mid, hor, (t - 0.5f) / 0.5f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(4, 7, 8));
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
    float span = framing ? 52.f : 68.f;
    float wantS = 240.f / span;
    float lead = (mode_ == Mode::Run && !framing) ? clampf(v_ * 0.5f, 0.f, 10.f) : 0.f;
    float wantX = framing ? kMark - 4.f : x_ + 4.f + lead;
    float wantH = framing ? kPlat + 0.9f : 2.3f;
    if (mode_ == Mode::Title) {
        wantX = kMark - 8.f;
        wantH = kPlat + 1.4f;
        wantS = 3.05f;
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
    } else if (snapCam_) {
        camX_ = wantX;
        camH_ = wantH;
        camS_ = wantS;
        snapCam_ = false;
    } else {
        camX_ += (wantX - camX_) * 0.1f;
        camH_ += (wantH - camH_) * 0.1f;
        camS_ += (wantS - camS_) * 0.1f;
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(legT_ * 74.f) * shake_ * 4.f;
        shy_ = std::cos(legT_ * 52.f) * shake_ * 2.4f;
        shake_ *= 0.9f;
        if (shake_ < 0.05f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }

    const float scale = camS_;
    const float ax = 150.f + shx_;
    const float ay = 120.f + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX_) * scale;
        sy = ay - (wy - camH_) * scale;
    };

    if (mode_ == Mode::Title) text("BARGE PLAT", 160, 22, 0.95f, PAL_HUD);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 24, 1.05f, PAL_HUD);
    else if (mode_ == Mode::Fail) text(banner_, 160, 20, 1.0f, PAL_BAD);
    else if (mode_ == Mode::Win) text("LEVEL", 160, 18, 1.1f, PAL_GOOD);

    for (int i = 0; i < 3; i++) {
        float sxc = std::fmod(16.f + float(i) * 150.f - camX_ * scale * 0.06f, 500.f);
        if (sxc < -60.f) sxc += 500.f;
        spr(art_.bank, sxc, 148.f, 22.f + float(i % 2) * 5.f, PAL_BANK, false, 7);
    }
    for (int i = 0; i < 3; i++) {
        float sxc = std::fmod(24.f + float(i) * 130.f - camX_ * scale * 0.03f + anim_ * 3.f, 440.f);
        if (sxc < -40.f) sxc += 440.f;
        spr(art_.cloud, sxc, 32.f + float(i) * 12.f, 14.f, PAL_SKY, i & 1, 1);
    }
    spr(art_.sun, 46.f, 26.f, 16.f, PAL_SKY, false, 0);

    float deckPx = std::max(3.5f, 0.42f * scale);
    float band0, band1, bandY;
    project(kMark - kXTol, kPlat, band0, bandY);
    project(kMark + kXTol, kPlat, band1, bandY);
    sprBox(art_.stripe, (band0 + band1) * 0.5f, bandY, std::max(6.f, band1 - band0), deckPx, PAL_MARK);

    float sx, sy;
    project((kFace + kEnd) * 0.5f, kPlat + 3.4f, sx, sy);
    spr(art_.crane, sx, sy, std::clamp(4.2f * scale, 18.f, 80.f), PAL_CRANE, false, 0);
    project(kEnd - 3.f, kPlat + 1.5f, sx, sy);
    spr(art_.lamp, sx, sy, std::clamp(1.6f * scale, 10.f, 32.f), PAL_MARK, false, 0);

    float step = 5.5f;
    float left = camX_ - (ax + 40.f) / scale;
    float right = camX_ + (gs::SCREEN_W - ax + 40.f) / scale;
    float plank0 = std::max(kFace, std::floor(left / step) * step);
    for (float wx = plank0; wx < kEnd && wx < right + step; wx += step) {
        float x0 = std::max(wx, kFace);
        float x1 = std::min(wx + step, kEnd);
        float s0, y0, s1, y1;
        project(x0, kPlat, s0, y0);
        project(x1, kPlat, s1, y1);
        sprBox(art_.plank, (s0 + s1) * 0.5f, y0, std::max(2.f, s1 - s0 + 1.f), deckPx, PAL_QUAY);
    }
    for (float wx = kFace + 3.f; wx < kEnd - 1.f; wx += 8.5f) {
        float gx, gy, dx, dy;
        project(wx, -1.8f, gx, gy);
        project(wx, kPlat, dx, dy);
        float ht = gy - dy;
        if (ht < 4.f) continue;
        spr(art_.pile, dx, dy + ht * 0.5f, ht, PAL_PILE, false, 0);
    }

    float hullH = std::clamp(3.4f * scale, 22.f, 96.f);
    float bob = std::sin(anim_ * 1.4f) * 0.04f;
    float hsx, hsy;
    project(x_, deck_ + bob, hsx, hsy);
    spr(art_.hull, hsx, hsy, hullH, PAL_HULL, false, 0);
    float crateH = hullH * 0.28f;
    project(x_ - 2.f, deck_ + 0.55f, sx, sy);
    spr(art_.crate, sx, sy, crateH, PAL_CRATE, false, 0);
    project(x_ + 3.2f, deck_ + 0.55f, sx, sy);
    spr(art_.crate, sx, sy, crateH * 0.9f, PAL_CRATE, true, 0);

    if (std::fabs(v_) > 0.35f) {
        float wx, wy;
        project(x_ - kStern - 1.f, 0.12f, wx, wy);
        spr(art_.wake, wx, wy, std::max(5.f, 0.55f * scale), PAL_WAKE, v_ < 0.f, 1);
    }
    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float px, py;
        project(p.x, p.y, px, py);
        spr(art_.foam, px, py, 5.f + (1.f - p.life) * 7.f, PAL_FOAM, false, int((1.f - p.life) * 8));
    }

    float gStep = std::clamp(32.f / scale, 4.f, 9.f);
    float g0 = std::floor(left / gStep) * gStep;
    for (float wx = g0; wx < right; wx += gStep) {
        float px, py;
        project(wx + gStep * 0.5f, 0.f, px, py);
        sprBox(art_.water, px, py, gStep * scale + 2.f, std::max(16.f, gs::SCREEN_H - py), PAL_WATER);
    }

    for (int i = 0; i < 2; i++) {
        float bx = 40.f + float(i) * 36.f + std::sin(anim_ * 0.5f + float(i)) * 3.f;
        float by = 6.2f + float(i) * 0.8f;
        float px, py;
        project(bx, by, px, py);
        spr(art_.bird[int(anim_ * 4.f + i) & 1], px, py, std::max(5.f, 0.4f * scale), PAL_BIRD, i & 1, 1);
    }

    if (mode_ == Mode::Title) {
        hudC(16, "UP FILLS THE TANKS", PAL_HUD);
        hudC(17, "DOWN DUMPS BALLAST", PAL_HUD);
        hudC(18, "Z AHEAD     X ASTERN", PAL_AMBER);
        hudC(20, "STOP THE DECK LEVEL", PAL_HUD);
        hudC(21, "WITH THE PLATFORM", PAL_GOOD);
        hudC(22, "A SCRAPE OR A MISS FAILS", PAL_AMBER);
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
        float gap = deck_ - kPlat;
        std::snprintf(buf, sizeof buf, "LVL %+5.2f", gap);
        hud(1, 1, buf, std::fabs(gap) <= kHTol ? PAL_GOOD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(14, 1, buf, PAL_HUD);
        float leftM = kMark - bowOf(x_);
        std::snprintf(buf, sizeof buf, "MARK %+5.1f", leftM);
        hud(26, 1, buf, std::fabs(leftM) <= kXTol ? PAL_GOOD : PAL_AMBER);

        const char* line = "TO THE QUAY";
        int pal = PAL_HUD;
        bool atMark = std::fabs(bowOf(x_) - kMark) <= kXTol;
        bool level = std::fabs(gap) <= kHTol;
        if (atMark && level && std::fabs(v_) <= kStopSpd) {
            line = "LEVEL";
            pal = PAL_GOOD;
        } else if (atMark && level) {
            line = "HOLD STILL";
            pal = PAL_GOOD;
        } else if (bowOf(x_) > kFace && !level) {
            line = gap > 0.f ? "TOO HIGH" : "TOO LOW";
            pal = PAL_AMBER;
        } else if (bowOf(x_) > kMark + kXTol) {
            line = "PAST THE MARK";
            pal = PAL_BAD;
        } else if (bowOf(x_) > kFace) {
            line = "TO THE MARK";
            pal = PAL_AMBER;
        }
        hudC(2, line, pal);
        if (hold_ > 0.02f) {
            int n = std::clamp(int(hold_ / kHoldNeed * 5.f + 0.001f), 0, 5);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
            hudC(3, buf, PAL_GOOD);
        }
        hudC(26, "UP DOWN BALLAST   Z X DRIVE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(4, 7, 8));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.1f, 0.18f, 0.08f);
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
        static const float notes[] = {330.f, 415.f, 494.f, 659.f};
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
        x_ = kFace - 34.f + std::sin(anim_ * 0.4f) * 2.5f;
        deck_ = kPlat - 0.5f + std::sin(anim_ * 0.9f) * 0.1f;
        v_ = 2.4f;
        alongside_ = false;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(520.f);
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
            blip(300.f);
            draw();
            return;
        }
    }

    bool was = alongside_;
    physics(thrust, pump);
    if (!was && alongside_ && mode_ == Mode::Run) {
        puffs_[puffN_ % 6] = {x_ + kBow - 1.f, 0.15f, 0.45f};
        puffN_++;
    }
    if (std::fabs(v_) > 1.6f && mode_ == Mode::Run && (sys.frame % 8) == 0) {
        puffs_[puffN_ % 6] = {x_ - kStern, 0.1f, 0.35f};
        puffN_++;
    }
    audio();
    draw();
}

}  // namespace bargeplat
