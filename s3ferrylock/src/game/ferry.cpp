#include "ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ferrylock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GATE_LO = 58.f;
constexpr float GATE_HI = 118.f;
constexpr float WIN_Y = 176.f;
constexpr float CREW = 100.f;
constexpr float WALL = 9.f;
constexpr float ZOOM = 7.2f;
constexpr float HIT = 0.42f;

float segDist(float px, float py, float ax, float ay, float bx, float by) {
    float dx = bx - ax, dy = by - ay;
    float len2 = dx * dx + dy * dy;
    float t = len2 < 1e-4f ? 0.f : std::clamp(((px - ax) * dx + (py - ay) * dy) / len2, 0.f, 1.f);
    float qx = ax + dx * t - px, qy = ay + dy * t - py;
    return std::sqrt(qx * qx + qy * qy);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    why_ = "";
    playT_ = 0;
    x_ = 0;
    y_ = 10.f;
    hdg_ = 0;
    speed_ = 0;
    yawV_ = 0;
    thrust_ = 0;
    steer_ = 0;
    openLo_ = tgtLo_ = 0;
    openHi_ = tgtHi_ = 0;
    camY_ = y_;
}

void Game::fail(const char* why) {
    if (over_) return;
    over_ = true;
    won_ = false;
    why_ = why;
    mode_ = Mode::Fail;
}

void Game::win() {
    if (over_) return;
    over_ = true;
    won_ = true;
    why_ = "";
    mode_ = Mode::Win;
}

float Game::bowY() const { return y_ + std::cos(hdg_) * kHalfL; }
float Game::sternY() const { return y_ - std::cos(hdg_) * kHalfL; }
float Game::sx(float wx) const { return gs::SCREEN_W * 0.5f + (wx - x_) * ZOOM; }
float Game::sy(float wy) const { return gs::SCREEN_H * 0.5f - (wy - camY_) * ZOOM; }
int Game::yawOf(float h) const {
    int i = int(std::lround(-h / (3.14159265f / 8.f)));
    return (i % 16 + 16) % 16;
}

void Game::pilot() {
    float gate = sternY() < GATE_LO + 1.f ? GATE_LO : (sternY() < GATE_HI + 1.f ? GATE_HI : 1e9f);
    float open = gate == GATE_LO ? openLo_ : openHi_;
    float ahead = gate - bowY();
    bool hold = gate < 1e8f && ahead < 9.f && ahead > -2.f && open < 0.86f;
    if (hold) thrust_ = speed_ > 0.15f ? -0.85f : 0.f;
    else thrust_ = 1.f;
    steer_ = std::clamp(-x_ * 1.15f - hdg_ * 2.1f, -1.f, 1.f);
}

void Game::drive(float dt) {
    speed_ += (thrust_ * 5.4f - speed_ * 0.55f) * dt;
    yawV_ += steer_ * 2.2f * dt;
    yawV_ *= 0.9f;
    hdg_ += yawV_ * dt;
    x_ += std::sin(hdg_) * speed_ * dt;
    y_ += std::cos(hdg_) * speed_ * dt;
    if (x_ > 4.6f) x_ = 4.6f;
    if (x_ < -4.6f) x_ = -4.6f;
}

const char* Game::gateHit() const {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[5] = {0.95f, 0.45f, 0.f, -0.45f, -0.95f};
    const float side[3] = {-0.9f, 0.f, 0.9f};
    for (int which = 0; which < 2; which++) {
        float open = which == 0 ? openLo_ : openHi_;
        float gy = which == 0 ? GATE_LO : GATE_HI;
        float span = WALL * (1.f - open);
        float segs[2][4] = {{-WALL, gy, -WALL + span, gy}, {WALL - span, gy, WALL, gy}};
        for (auto& seg : segs) {
            if (span < 0.4f) continue;
            for (float a : along) {
                for (float b : side) {
                    float wx = x_ + fx * a * kHalfL + rx * b * kHalfB;
                    float wy = y_ + fy * a * kHalfL + ry * b * kHalfB;
                    if (segDist(wx, wy, seg[0], seg[1], seg[2], seg[3]) < HIT)
                        return which == 0 ? "scraped the lower gate" : "scraped the upper gate";
                }
            }
        }
    }
    return nullptr;
}

void Game::step() {
    playT_ += DT;
    if (playT_ >= CREW) {
        fail("missed the end");
        return;
    }
    if (bot_) pilot();
    else {
        const gs::Pad& pad = sys_->pad;
        thrust_ = pad.down(gs::BTN_UP) ? 1.f : (pad.down(gs::BTN_DOWN) ? -0.7f : 0.f);
        if (pad.accel > 0.05f) thrust_ = pad.accel;
        steer_ = pad.axisX;
        if (pad.down(gs::BTN_LEFT)) steer_ = -1.f;
        if (pad.down(gs::BTN_RIGHT)) steer_ = 1.f;
    }
    drive(DT);

    auto ease = [&](float& o, float target) {
        float rate = 0.7f * DT;
        if (o < target) o = std::min(target, o + rate);
        else o = std::max(target, o - rate);
    };
    bool lined = std::fabs(x_) < 1.6f && std::fabs(hdg_) < 0.45f;
    if (bowY() > GATE_LO - 16.f && bowY() < GATE_LO && lined) tgtLo_ = 1.f;
    if (sternY() > GATE_LO + 2.f) tgtLo_ = 0.f;
    if (bowY() > GATE_HI - 16.f && bowY() < GATE_HI && lined) tgtHi_ = 1.f;
    if (sternY() > GATE_HI + 2.f) tgtHi_ = 0.f;
    ease(openLo_, tgtLo_);
    ease(openHi_, tgtHi_);

    if (const char* hit = gateHit()) {
        fail(hit);
        return;
    }
    if (bowY() > WIN_Y) win();
    camY_ += (y_ - camY_) * 0.12f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(m.h, 1));
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 400L);
    long sh = std::clamp(std::lround(h), 1L, 400L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::lround(cx - sw * 0.5f));
    s.y = int16_t(std::lround(cy - sh * 0.5f));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    spr(m, sx(wx), sy(wy), worldH * ZOOM, pal);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        int x = col + i;
        if (x < 0 || x > 39) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    uint16_t water = gs::rgb4(2, 6, 11);
    uint16_t bank = gs::rgb4(5, 8, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.lineBackdrop[y] = water;

    for (float wy = std::floor((camY_ - 20.f) / 12.f) * 12.f; wy < camY_ + 24.f; wy += 12.f) {
        place(art_.wall, -WALL - 1.2f, wy, 6.f, PAL_BANK);
        place(art_.wall, WALL + 1.2f, wy, 6.f, PAL_BANK);
        place(art_.shore, -WALL - 3.4f, wy, 2.2f, PAL_BANK);
        place(art_.shore, WALL + 3.4f, wy, 2.2f, PAL_BANK);
    }
    place(art_.house, WALL + 3.6f, GATE_LO + 8.f, 4.2f, PAL_HOUSE);
    place(art_.lamp, -WALL - 0.4f, GATE_LO, 2.4f, PAL_MARK);
    place(art_.lamp, -WALL - 0.4f, GATE_HI, 2.4f, PAL_MARK);
    place(art_.buoy[0], -3.2f, 28.f, 1.6f, PAL_BUOY);
    place(art_.buoy[1], 3.2f, 150.f, 1.6f, PAL_BUOY);
    place(art_.endSign, 0.f, WIN_Y + 4.f, 2.2f, PAL_WIN);
    place(art_.gull[int(playT_ * 2.f) & 1], -6.f + std::sin(playT_) * 2.f, camY_ + 8.f, 1.1f, PAL_GULL);

    auto beam = [&](float gy, float open) {
        float span = WALL * (1.f - open);
        if (span < 0.35f) return;
        place(art_.leaf[0], -WALL + span * 0.5f, gy, 1.5f, PAL_GATE);
        place(art_.leaf[0], WALL - span * 0.5f, gy, 1.5f, PAL_GATE);
        place(art_.post, -WALL, gy, 2.2f, PAL_GATE);
        place(art_.post, WALL, gy, 2.2f, PAL_GATE);
    };
    beam(GATE_LO, openLo_);
    beam(GATE_HI, openHi_);

    const gs::Mipped& hull = art_.hull[yawOf(hdg_)];
    float hh = float(hull.h) * ((kHalfL * 2.f) * ZOOM / float(kHullPx));
    spr(art_.foam, sx(x_ - std::sin(hdg_) * kHalfL), sy(y_ - std::cos(hdg_) * kHalfL), 8.f, PAL_FOAM);
    spr(hull, sx(x_), sy(y_), hh, PAL_FERRY);

    char line[64];
    std::snprintf(line, sizeof line, "FERRY LOCK  %ds", int(std::max(0.f, CREW - playT_)));
    hud(1, 1, line, PAL_HUD);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 78, 28, PAL_BANNER);
        hudC(16, "PASS THE LOCK", PAL_HUD);
        hudC(18, "A SCRAPED GATE FAILS THE LEG", PAL_ALERT);
        hudC(24, "UP THROTTLE   ARROWS STEER", PAL_DIM);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160, 100, 26, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        spr(art_.clearWord, 160, 86, 32, PAL_WIN);
        hudC(16, "THE LOCK IS CLEAR", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(why_ && std::strstr(why_, "scrape") ? art_.scraped : art_.missed, 160, 86, 22, PAL_ALERT);
        hudC(16, why_, PAL_ALERT);
    }
    (void)bank;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || bot_) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else step();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        begin();
    }
    draw();
}

}  // namespace ferrylock
