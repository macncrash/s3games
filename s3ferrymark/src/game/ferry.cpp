#include "game/ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace fmark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kHalfX = 1.05f;
constexpr float kHalfY = 1.25f;
constexpr float kHdg = 0.11f;
constexpr float kStop = 0.18f;
constexpr float kHold = 0.42f;
constexpr float kPier = 8.6f;
constexpr float kQuay = 10.4f;
constexpr float kLen = 7.2f;
constexpr float kBeam = 2.2f;
constexpr float PI = 3.14159265f;

float clamp(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float wrap(float a) {
    while (a > PI) a -= 2.f * PI;
    while (a < -PI) a += 2.f * PI;
    return a;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float currentAt(float y) {
    if (y < -8.f) return 0.42f;
    if (y < -1.5f) return 0.42f * ((-1.5f - y) / 6.5f);
    return 0.f;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.18f, 0.5f, 0.22f};
    p.op[1] = {2.f, 0.28f, 0.02f, 0.2f, 0.3f, 0.2f};
    p.op[2] = {3.f, 0.08f, 0.02f, 0.22f, 0.18f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.vol = 0.2f;
    p.tone = 1600.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (std::fabs(x_) <= kHalfX + 1.2f && std::fabs(y_) <= kHalfY + 1.6f) return 2;
    return 1;
}

bool Game::onMark() const {
    return std::fabs(x_) <= kHalfX && std::fabs(y_) <= kHalfY && std::fabs(wrap(hdg_)) <= kHdg && spd_ <= kStop;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    hold_ = 0;
    x_ = -2.2f;
    y_ = -6.f;
    hdg_ = 0.08f;
    camX_ = 1.f;
    camY_ = 0.f;
    camS_ = 7.2f;
}

void Game::startRun() {
    x_ = -6.4f;
    y_ = -36.f;
    hdg_ = 0.18f;
    surge_ = 0.9f;
    sway_ = 0;
    yaw_ = 0;
    spd_ = 0.9f;
    time_ = 0;
    hold_ = 0;
    idle_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    thrustIn_ = 0;
    mode_ = Mode::Run;
    camX_ = x_;
    camY_ = y_ + 4.f;
    camS_ = 6.4f;
    blip(420.f);
}

void Game::pilot(float& thrust, float& rudder) const {
    float cur = currentAt(y_);
    float wantVx = clamp(-x_ * 0.62f, -0.9f, 0.9f);
    float denom = std::max(0.55f, std::fabs(surge_));
    float wantSin = clamp((wantVx - cur) / denom, -0.65f, 0.65f);
    float wantH = std::asin(wantSin);
    if (y_ > -7.f) wantH = clamp(-x_ * 0.55f, -0.4f, 0.4f);
    if (y_ > -2.4f) wantH = clamp(-x_ * 1.15f, -0.3f, 0.3f);
    float err = wrap(wantH - hdg_);
    rudder = clamp(err * 3.4f - yaw_ * 0.85f, -1.f, 1.f);

    float wantS = 2.15f;
    if (y_ > -16.f) wantS = 1.25f;
    if (y_ > -6.f) wantS = 0.72f;
    if (y_ > -3.2f) wantS = 0.48f;
    if (std::fabs(x_) > 2.4f && y_ > -14.f) wantS = std::min(wantS, 0.85f);
    bool lined = std::fabs(x_) < 0.7f && std::fabs(wrap(hdg_)) < 0.07f;
    if (y_ > -1.6f && lined) wantS = clamp(-y_ * 0.55f, 0.12f, 0.4f);
    if (std::fabs(y_) < 0.45f && std::fabs(x_) < 0.7f && std::fabs(wrap(hdg_)) < 0.08f)
        wantS = clamp(-surge_ * 2.4f, -0.5f, 0.15f);
    if (y_ > 0.7f && std::fabs(x_) < 2.f) wantS = -0.5f;
    thrust = clamp((wantS - surge_) * 1.7f, -1.f, 1.f);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "set down on the mark";
    banner_ = "SET DOWN";
    chime_ = 0;
    chimeT_ = 0;
    surge_ = 0;
    sway_ = 0;
    yaw_ = 0;
    spd_ = 0;
    sys_->rumble(0.16f, 0.05f, 140);
    sys_->setLight(30, 160, 80);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    banner_ = banner;
    sys_->rumble(0.45f, 0.22f, 160);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.4f, 360.f, 0.24f);
}

void Game::physics(float thrust, float rudder) {
    if (mode_ != Mode::Run) return;
    thrust = clamp(thrust, -1.f, 1.f);
    rudder = clamp(rudder, -1.f, 1.f);
    thrustIn_ = thrust;
    time_ += DT;

    float eff = clamp(std::fabs(surge_) * 0.62f + 0.28f, 0.28f, 1.5f);
    yaw_ += rudder * eff * 1.25f * DT;
    yaw_ += -yaw_ * 2.4f * DT;
    hdg_ = wrap(hdg_ + yaw_ * DT);
    surge_ += thrust * 3.4f * DT;
    surge_ += -surge_ * 0.95f * DT;
    sway_ += -sway_ * 1.8f * DT;
    sway_ += yaw_ * -0.35f * DT;
    surge_ = clamp(surge_, -2.2f, 3.4f);

    float cur = currentAt(y_);
    float vx = std::sin(hdg_) * surge_ + std::cos(hdg_) * sway_ + cur;
    float vy = std::cos(hdg_) * surge_ - std::sin(hdg_) * sway_;
    x_ += vx * DT;
    y_ += vy * DT;
    spd_ = std::sqrt(vx * vx + vy * vy);

    const float ca = std::cos(hdg_), sa = std::sin(hdg_);
    for (int i = 0; i < 4; ++i) {
        float n = (i & 1) ? kLen : -kLen;
        float r = (i & 2) ? kBeam : -kBeam;
        float wx = x_ + r * ca + n * sa;
        float wy = y_ - r * sa + n * ca;
        if (wx > kPier) {
            fail("scraped the pier", "PIER");
            return;
        }
        if (wy > kQuay) {
            fail("into the quay", "QUAY");
            return;
        }
    }
    if (std::fabs(x_) > 28.f || y_ < -70.f) {
        fail("lost the berth", "LOST");
        return;
    }

    if (onMark()) {
        hold_ += DT;
        idle_ = 0;
        if (hold_ >= kHold) {
            win();
            return;
        }
    } else {
        hold_ = 0;
        if (spd_ < 0.16f && y_ > -12.f) {
            idle_ += DT;
            if (idle_ > 1.7f) {
                fail("off the mark", "OFF");
                return;
            }
        } else {
            idle_ = 0;
        }
    }
    if (time_ > 42.f) fail("too late", "TOO LATE");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::sky() {
    uint16_t deep = gs::rgb4(1, 4, 8);
    uint16_t mid = gs::rgb4(2, 7, 11);
    uint16_t lit = gs::rgb4(4, 10, 12);
    if (mode_ == Mode::Fail) lit = lerpC(lit, gs::rgb4(10, 5, 4), 0.35f);
    if (mode_ == Mode::Win) lit = lerpC(lit, gs::rgb4(6, 12, 8), 0.3f);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.5f ? lerpC(deep, mid, t / 0.5f) : lerpC(mid, lit, (t - 0.5f) / 0.5f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float adv = 0;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            adv += 14.f * scale;
            continue;
        }
        adv += (float(art_.glyph[c - 32].w) + 3.f) * scale;
    }
    x -= adv * 0.5f;
    float pen = x;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            pen += 14.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float ht = float(g.h) * scale;
        spr(g, pen + float(g.w) * scale * 0.5f, y, ht, pal, false);
        pen += (float(g.w) + 3.f) * scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    if (mode_ == Mode::Title) {
        text("FERRY MARK", 160, 22, 1.05f, PAL_HUD);
        text("SET DOWN ON IT", 160, 46, 0.58f, PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 24, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 22, 1.05f, PAL_BAD);
    } else if (mode_ == Mode::Win) {
        text("SET DOWN", 160, 22, 1.15f, PAL_GOOD);
    }

    const bool framing = mode_ == Mode::Win || mode_ == Mode::Title || hold_ > 0.02f;
    float wantX = framing ? 0.6f : x_ + std::sin(hdg_) * 3.2f;
    float wantY = framing ? -0.4f : y_ + std::cos(hdg_) * 3.2f;
    float wantS = framing ? 7.4f : 6.5f;
    if (mode_ == Mode::Title) {
        camX_ = wantX;
        camY_ = wantY;
        camS_ = wantS;
    } else {
        float k = framing ? 0.18f : 0.12f;
        camX_ += (wantX - camX_) * k;
        camY_ += (wantY - camY_) * k;
        camS_ += (wantS - camS_) * k;
    }
    const float scale = camS_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = 160.f + (wx - camX_) * scale;
        sy = 118.f - (wy - camY_) * scale;
    };

    auto box = [&](const gs::Mipped& m, float wx, float wy, float ww, float wh, int pal) {
        float sx, sy;
        project(wx, wy, sx, sy);
        sprBox(m, sx, sy, std::max(2.f, ww * scale), std::max(2.f, wh * scale), pal);
    };

    for (int i = 0; i < 6; ++i) {
        float py = -2.f + float(i) * 2.1f;
        box(art_.pier, 13.2f, py, 8.4f, 2.2f, PAL_PIER);
    }
    box(art_.shed, 12.4f, 4.2f, 4.6f, 3.2f, PAL_SHED);
    box(art_.post, kPier - 0.2f, 2.f, 0.7f, 2.2f, PAL_PIER);
    box(art_.post, kPier - 0.2f, 7.2f, 0.7f, 2.2f, PAL_PIER);

    float mx, my;
    project(0.f, 0.f, mx, my);
    sprBox(art_.mark, mx, my, 7.6f * scale, 9.4f * scale, PAL_MARK);
    spr(art_.cross, mx, my, 3.2f * scale, PAL_MARK, false);

    float bx, by;
    project(-5.5f, -14.f, bx, by);
    spr(art_.buoy, bx, by, 16.f, PAL_BUOY, false);
    project(4.2f, -22.f, bx, by);
    spr(art_.buoy, bx, by, 14.f, PAL_BUOY, false);

    if (spd_ > 0.25f && mode_ != Mode::Title) {
        float wx = x_ - std::sin(hdg_) * 6.2f;
        float wy = y_ - std::cos(hdg_) * 6.2f;
        float sx, sy;
        project(wx, wy, sx, sy);
        spr(art_.wake, sx, sy, 10.f + spd_ * 4.f, PAL_WAKE, hdg_ < 0);
        project(x_ - std::cos(hdg_) * 1.6f, y_ + std::sin(hdg_) * 1.6f, sx, sy);
        spr(art_.foam, sx, sy, 8.f, PAL_FOAM, false);
    }

    int fi = int(std::lround(hdg_ / (2.f * PI / 16.f)));
    fi &= 15;
    float sx, sy;
    project(x_, y_, sx, sy);
    spr(art_.hull[fi], sx, sy, 14.8f * scale, PAL_HULL, false);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "X %+.1f  Y %+.1f", x_, y_);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "HDG %+.0f", wrap(hdg_) * 57.3f);
        hud(28, 1, buf, std::fabs(wrap(hdg_)) <= kHdg ? PAL_GOOD : PAL_AMBER);
        const char* line = "BRING THE HULL ONTO THE PAINT";
        int pal = PAL_HUD;
        if (onMark()) {
            line = "HOLD THE MARK";
            pal = PAL_GOOD;
        } else if (std::fabs(x_) <= kHalfX + 1.4f && std::fabs(y_) <= kHalfY + 2.f) {
            line = "CLOSE IS STILL OFF IT";
            pal = PAL_AMBER;
        }
        hudC(25, line, pal);
        if (hold_ > 0.02f) {
            int n = int(hold_ / kHold * 5.f + 0.001f);
            n = std::clamp(n, 0, 5);
            std::snprintf(buf, sizeof buf, "SET %d/5", n);
            hudC(26, buf, PAL_GOOD);
        } else {
            hudC(26, "ARROWS STEER   A AHEAD  B ASTERN", PAL_HUD);
        }
    } else if (mode_ == Mode::Title) {
        hudC(24, "START  SET DOWN ON THE MARK", PAL_HUD);
        hudC(26, "CLOSE TO THE PAINT IS STILL OFF IT", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(25, "THE HULL IS ON THE MARK", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, why_, PAL_BAD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 6, 9));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.16f, 0.08f);
    sys.apu.setPatch(0, bellPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) time_ += DT;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (chime_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += DT;
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 7) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        x_ = -2.2f + std::sin(time_ * 0.7f) * 0.35f;
        y_ = -5.5f + std::sin(time_ * 0.45f) * 0.4f;
        hdg_ = 0.06f + std::sin(time_ * 0.5f) * 0.04f;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(620.f);
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
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    float thrust = 0, rudder = 0;
    if (bot_) {
        pilot(thrust, rudder);
    } else {
        if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A)) thrust += 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B)) thrust -= 1.f;
        if (pad.axisY > 0.2f) thrust = pad.axisY;
        if (pad.axisY < -0.2f) thrust = pad.axisY;
        if (pad.accel > 0.08f) thrust = std::max(thrust, pad.accel);
        if (pad.brake > 0.08f) thrust = std::min(thrust, -pad.brake);
        if (pad.down(gs::BTN_LEFT)) rudder -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) rudder += 1.f;
        if (std::fabs(pad.axisX) > 0.15f) rudder = pad.axisX;
        thrust = clamp(thrust, -1.f, 1.f);
        rudder = clamp(rudder, -1.f, 1.f);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(360.f);
            draw();
            return;
        }
    }

    if (std::fabs(thrust) > 0.2f) sys.apu.tone(2, 70.f + std::fabs(thrust) * 40.f, 0.03f);
    else sys.apu.tone(2, 0.f, 0.f);
    physics(thrust, rudder);
    draw();
}

}  // namespace fmark
