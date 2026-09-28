#include "scull.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace scullboom {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kG = 11.4;
constexpr double kThrow = 8.6;
constexpr double kHeave = 6.6;
constexpr double kHold = 0.55;
constexpr double kLimit = 36.0;

enum class Hit { None, Bed, Edge, Smash, Water, Bank };

struct Body {
    double x, y, vx, vy;
};

struct Info {
    Hit hit = Hit::None;
    double land = 0;
};

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

double deckCenter() { return kDeck + kDriveH * 0.5; }

Body carried(double sx, double spd) {
    Body b;
    b.x = sx + kDriveSeat;
    b.y = kHullY + 0.72;
    b.vx = spd + kThrow;
    b.vy = kHeave;
    return b;
}

void stepBody(Body& b) {
    b.vy -= kG * kDt;
    b.x += b.vx * kDt;
    b.y += b.vy * kDt;
}

Info classify(double land, double vy) {
    Info info;
    info.land = land;
    const double lo = kBed0 + kDriveHalf + 0.18;
    const double hi = kBed1 - kDriveHalf - 0.18;
    const double edgeL = kPostL - 0.2;
    const double edgeR = kPostR + 0.2;
    if (land >= lo && land <= hi) {
        info.hit = (vy < -9.2) ? Hit::Smash : Hit::Bed;
        return info;
    }
    if (land >= edgeL && land <= edgeR) {
        info.hit = Hit::Edge;
        return info;
    }
    if (land < kPostL) info.hit = Hit::Water;
    else if (land < kFarBank) info.hit = Hit::Water;
    else info.hit = Hit::Bank;
    return info;
}

Info forecast(Body b) {
    double prevX = b.x, prevY = b.y;
    const double stopY = deckCenter();
    for (int i = 0; i < 180; i++) {
        stepBody(b);
        if (prevY >= stopY && b.y <= stopY && b.vy < 0) {
            double u = (prevY - b.y) > 1e-6 ? (prevY - stopY) / (prevY - b.y) : 1.0;
            double land = prevX + (b.x - prevX) * u;
            return classify(land, b.vy);
        }
        if (b.y - kDriveH * 0.5 <= 0.0) {
            Info info;
            info.land = b.x;
            info.hit = (b.x < 8.0 || b.x > kFarBank) ? Hit::Bank : Hit::Water;
            return info;
        }
        prevX = b.x;
        prevY = b.y;
    }
    Info info;
    info.land = b.x;
    info.hit = Hit::None;
    return info;
}

const char* hitName(Hit h) {
    switch (h) {
    case Hit::Bed: return "ON THE BED";
    case Hit::Edge: return "EDGE";
    case Hit::Smash: return "TOO HARD";
    case Hit::Water: return "IN THE WATER";
    case Hit::Bank: return "ON THE BANK";
    default: return "NO SHOT";
    }
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (phase_ == Phase::Bed) return 3;
    if (phase_ == Phase::Air) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    phase_ = Phase::Carry;
    over_ = false;
    won_ = false;
    why_ = "";
    banner_ = "";
    time_ = 0;
    hold_ = 0;
    x_ = 118;
    speed_ = 5.4;
    stroke_ = 0.65;
    oar_ = 0;
    chime_ = -1;
    camX_ = 150;
    camY_ = 2.4;
    camS_ = 300.0 / 52.0;
}

void Game::startRun() {
    mode_ = Mode::Row;
    phase_ = Phase::Carry;
    over_ = false;
    won_ = false;
    why_ = "";
    banner_ = "";
    time_ = 0;
    hold_ = 0;
    x_ = 18;
    speed_ = 2.4;
    stroke_ = 0;
    oar_ = 0;
    dx_ = dy_ = dvx_ = dvy_ = 0;
    chime_ = -1;
    camX_ = 28;
    camY_ = 2.2;
    camS_ = 300.0 / 46.0;
    blip(520.f);
}

void Game::pilot(double& stroke, bool& heave) const {
    stroke = 1.0;
    heave = false;
    if (phase_ != Phase::Carry) {
        stroke = 0.15;
        return;
    }
    const Info info = forecast(carried(x_, speed_));
    const double mid = 0.5 * (kBed0 + kBed1);
    if (info.hit == Hit::Bed) heave = true;
    else if (info.land > mid) stroke = 0.0;
    else stroke = 1.0;
    const double bow = x_ + kBow;
    if (bow > kPostL - 14.0 && info.hit != Hit::Bed) stroke = std::min(stroke, 0.35);
}

void Game::win() {
    if (mode_ != Mode::Row) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "the drive is on the boom";
    banner_ = "DELIVERED";
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.2f, 0.08f, 120);
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Row) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    banner_ = "MISSED";
    sys_->rumble(0.5f, 0.25f, 160);
    sys_->setLight(170, 40, 30);
    sys_->apu.noiseBurst(0.42f, 380.f, 0.26f);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.06f;
}

void Game::physics(double stroke, bool heave) {
    if (mode_ != Mode::Row) return;
    time_ += kDt;
    stroke_ = clampd(stroke, 0.0, 1.0);
    const double cruise = 1.7 + stroke_ * 6.1;
    speed_ += (cruise - speed_) * std::min(1.0, 2.1 * kDt);
    speed_ = clampd(speed_, 0.0, 9.5);
    x_ += speed_ * kDt;
    oar_ += (0.55 + stroke_ * 2.4) * kDt;
    if (oar_ > 1.0) {
        oar_ -= 1.0;
        if (stroke_ > 0.2) {
            sys_->apu.tone(2, 90.f + float(stroke_) * 40.f, 0.03f);
            splash_ = 0.18f;
        }
    }

    if (heave && phase_ == Phase::Carry) {
        Body b = carried(x_, speed_);
        dx_ = b.x;
        dy_ = b.y;
        dvx_ = b.vx;
        dvy_ = b.vy;
        phase_ = Phase::Air;
        blip(880.f);
        sys_->rumble(0.12f, 0.04f, 40);
    }

    if (phase_ == Phase::Carry) {
        if (x_ + kBow >= kPostL - 0.15) {
            fail("still carrying the drive");
            return;
        }
    }

    if (phase_ == Phase::Air) {
        Body b{dx_, dy_, dvx_, dvy_};
        double prevX = b.x, prevY = b.y;
        stepBody(b);
        dx_ = b.x;
        dy_ = b.y;
        dvx_ = b.vx;
        dvy_ = b.vy;
        const double stopY = deckCenter();
        if (prevY >= stopY && b.y <= stopY && b.vy < 0) {
            double u = (prevY - b.y) > 1e-6 ? (prevY - stopY) / (prevY - b.y) : 1.0;
            double land = prevX + (b.x - prevX) * u;
            Info info = classify(land, b.vy);
            dx_ = land;
            dy_ = stopY;
            dvx_ = dvy_ = 0;
            if (info.hit == Hit::Bed) {
                phase_ = Phase::Bed;
                hold_ = 0;
                blip(240.f);
                sys_->apu.noiseBurst(0.22f, 160.f, 0.14f);
                sys_->rumble(0.28f, 0.1f, 80);
            } else if (info.hit == Hit::Smash) {
                fail("smashed the drive");
                return;
            } else if (info.hit == Hit::Edge) {
                fail("on the edge of the boom");
                return;
            } else if (info.hit == Hit::Bank) {
                fail(land < kPostL ? "dropped short" : "dropped long");
                return;
            } else {
                fail(land + kDriveHalf > kPostL - 6.0 && land - kDriveHalf < kPostR + 6.0 ? "beside the boom"
                                                                                           : "in the water");
                return;
            }
        } else if (b.y - kDriveH * 0.5 <= 0.0) {
            dy_ = kDriveH * 0.35;
            if (b.x < 10.0 || b.x > kFarBank) fail(b.x < kPostL ? "dropped short" : "dropped long");
            else if (b.x + kDriveHalf > kPostL - 6.0 && b.x - kDriveHalf < kPostR + 6.0) fail("beside the boom");
            else fail("in the water");
            return;
        }
    }

    if (phase_ == Phase::Bed) {
        hold_ += kDt;
        dy_ = deckCenter() + std::sin(time_ * 3.0) * 0.03;
        if (hold_ >= kHold) {
            win();
            return;
        }
    }

    if (time_ > kLimit) fail("too late");
}

void Game::sky() {
    uint16_t zen = gs::rgb4(5, 8, 13);
    uint16_t mid = gs::rgb4(8, 12, 15);
    uint16_t hor = gs::rgb4(13, 12, 9);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 6, 5), 0.35f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(10, 14, 9), 0.3f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.62f ? lerpC(zen, mid, t / 0.62f) : lerpC(mid, hor, (t - 0.62f) / 0.38f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip) {
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
    sys_->vdp.sprite(s);
}

void Game::sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal, bool flip) {
    if (destH < 1.2f || m.h < 1) return;
    float sc = destH / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(sx - ax * sc)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(sy - ay * sc)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal) {
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H || s.y + s.h < 0) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::blit(const Spr& s, float wx, float wy, float scale, int pal, bool flip) {
    if (s.ppm < 0.05f || s.img.h < 1) return;
    float sx = 160.f + (wx - float(camX_)) * scale;
    float sy = 132.f - (wy - float(camY_)) * scale;
    float dest = float(s.img.h) / s.ppm * scale;
    sprAnchor(s.img, s.ax, s.ay, sx, sy, dest, pal, flip);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    const float adv = 18.f * scale;
    const int n = int(std::strlen(s));
    x -= float(n) * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    double focus = x_ + 6.0;
    double span = 48.0;
    if (phase_ != Phase::Carry || mode_ == Mode::Win || mode_ == Mode::Fail) {
        focus = dx_;
        span = 36.0;
    }
    if (mode_ == Mode::Title) {
        camX_ = 152;
        camY_ = 2.5;
        camS_ = 300.0 / 54.0;
    } else {
        camX_ += (focus - camX_) * 0.08;
        camY_ += (2.3 - camY_) * 0.08;
        camS_ += ((300.0 / span) - camS_) * 0.08;
    }
    const float sc = float(camS_);

    if (mode_ == Mode::Title) {
        text("SCULL BOOM", 160, 22, 1.05f, PAL_HUD);
        text("DELIVER THE DRIVE", 160, 48, 0.58f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 26, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 22, 1.05f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("DELIVERED", 160, 22, 1.05f, PAL_GOOD);
    }

    Body load = (phase_ == Phase::Carry) ? carried(x_, speed_) : Body{dx_, dy_, dvx_, dvy_};
    if (phase_ == Phase::Carry) {
        load.vx = speed_;
        load.vy = 0;
    }

    blit(art_.drive, float(load.x), float(load.y), sc, PAL_DRIVE);
    if (mode_ != Mode::Fail || phase_ != Phase::Carry) {
        float bob = float(std::sin(x_ * 0.7) * 0.04);
        blit(art_.shell, float(x_), float(kHullY + bob), sc, PAL_SHELL);
        blit(art_.rower, float(x_ - 0.15f), float(kHullY + 0.22f + bob), sc, PAL_SHELL);
        int oi = (oar_ < 0.45) ? 0 : 1;
        blit(art_.oar[oi], float(x_ + 0.4f), float(kHullY + 0.35f + bob), sc, PAL_OAR, false);
        blit(art_.oar[oi], float(x_ + 0.4f), float(kHullY + 0.15f + bob), sc, PAL_OAR, true);
    }

    if (mode_ == Mode::Row && phase_ == Phase::Carry) {
        Info info = forecast(carried(x_, speed_));
        int pal = info.hit == Hit::Bed ? PAL_GOOD : (info.hit == Hit::Edge || info.hit == Hit::Smash ? PAL_BAD : PAL_AMBER);
        double cy = info.hit == Hit::Bed ? kDeck + 0.35 : 0.35;
        blit(art_.chev, float(info.land), float(cy), sc, pal);
    } else if (mode_ == Mode::Title) {
        blit(art_.chev, float(0.5 * (kBed0 + kBed1)), float(kDeck + 0.45), sc, PAL_GOOD);
    }

    blit(art_.post, float(kPostL + kPostW * 0.5), 0.f, sc, PAL_BOOM);
    blit(art_.post, float(kPostR - kPostW * 0.5), 0.f, sc, PAL_BOOM, true);
    blit(art_.sign, float(kPostL - 2.2), 0.f, sc, PAL_SIGN);

    float bedL = 160.f + (float(kBed0) - float(camX_)) * sc;
    float bedR = 160.f + (float(kBed1) - float(camX_)) * sc;
    float bedY = 132.f - (float(kDeck) - float(camY_)) * sc;
    sprBox(art_.plank, (bedL + bedR) * 0.5f, bedY, std::max(4.f, bedR - bedL), std::max(3.f, 0.28f * sc), PAL_BOOM);

    const double trees[] = {6, 24, 40, 188, 206};
    for (int i = 0; i < 5; i++) blit(art_.willow, float(trees[i]), 0.f, sc, PAL_TREE, i & 1);
    const double reeds[] = {12.0, 56.0, 92.0, 140.0, 176.0};
    for (double r : reeds) blit(art_.reed, float(r), 0.f, sc, PAL_TREE);

    if (splash_ > 0.02f) blit(art_.splash, float(x_ - 1.2f), 0.15f, sc, PAL_WATER);

    const int tileFrame = (int(sys_->frame) / 10) & 1;
    const double step = 4.2;
    const double left = camX_ - 200.0 / std::max(0.2, camS_);
    const double right = camX_ + 200.0 / std::max(0.2, camS_);
    for (double wx = std::floor(left / step) * step; wx < right; wx += step) {
        const bool water = wx > 2.0 && wx < kFarBank;
        const float sx = 160.f + (float(wx + step * 0.5) - float(camX_)) * sc;
        const float sy = 132.f - (0.f - float(camY_)) * sc;
        const float sw = float(step) * sc + 1.5f;
        const float sh = 3.2f * sc + 1.f;
        const gs::Mipped& tile = water ? art_.water[tileFrame] : art_.grass;
        int rows = 0;
        for (float y = sy; y < gs::SCREEN_H + 2.f && rows < 7; y += sh - 1.f, ++rows)
            sprBox(tile, sx, y, sw, sh, water ? PAL_WATER : PAL_BANK);
    }

    if (mode_ == Mode::Row || mode_ == Mode::Pause) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "STROKE %d", int(stroke_ * 100));
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%4.1f M/S", speed_);
        hud(30, 1, buf, PAL_AMBER);
        if (phase_ == Phase::Carry) {
            Info info = forecast(carried(x_, speed_));
            hudC(2, hitName(info.hit), info.hit == Hit::Bed ? PAL_GOOD : PAL_AMBER);
            std::snprintf(buf, sizeof buf, "BOOM %3.0f M", std::max(0.0, kPostL - (x_ + kBow)));
            hudC(3, buf, PAL_HUD);
        } else if (phase_ == Phase::Bed) {
            int n = int(hold_ / kHold * 5.0 + 0.001);
            std::snprintf(buf, sizeof buf, "HOLD %d/5", std::clamp(n, 0, 5));
            hudC(3, buf, PAL_GOOD);
        } else {
            hudC(3, "DRIVE IN THE AIR", PAL_AMBER);
        }
        hudC(26, "A STROKE    B HEAVE    START PAUSE", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(24, "A START", PAL_HUD);
        hudC(25, "HEAVE THE DRIVE ONTO THE BOOM", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(25, "THE DRIVE IS ON THE BOOM", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, why_, PAL_BAD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 9, 12));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.1f, 0.16f, 0.1f);
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (beep_ > 0.f) {
        beep_ -= float(kDt);
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (splash_ > 0.f) splash_ -= float(kDt);
    sys.apu.tone(2, 0.f, 0.f);

    if (chime_ >= 0) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        chimeT_ += float(kDt);
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.18f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        oar_ += 0.02;
        if (oar_ > 1.0) oar_ -= 1.0;
        x_ = 122.0 + std::sin(sys.frame * 0.02) * 1.5;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) startRun();
        else if (pad.pressed(gs::BTN_MODE) || pad.pressed(gs::BTN_C)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Row;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) showTitle();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        return;
    }

    double stroke = 0;
    bool heave = false;
    if (bot_) pilot(stroke, heave);
    else {
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.2f) stroke = 1.0;
        else if (pad.down(gs::BTN_DOWN)) stroke = 0.0;
        else stroke = 0.25;
        if (pad.axisY > 0.35f) stroke = 1.0;
        if (pad.axisY < -0.35f) stroke = 0.0;
        heave = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_Y);
    }
    physics(stroke, heave);
    draw();
}

}  // namespace scullboom
