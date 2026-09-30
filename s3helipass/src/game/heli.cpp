#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace helipass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHx = 14.f;
constexpr float kHy = 8.f;
constexpr float kLimit = 38.f;
constexpr float kStormV = 26.f;
constexpr float kEndL = 1148.f;
constexpr float kEndR = 1268.f;
constexpr float kMiss = 1340.f;
constexpr float kPadY = 98.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

// Side view of the saddle. Altitude rises into a roofed throat, then opens.
float floorAt(float x) {
    if (x < 240.f) return 22.f;
    if (x < 460.f) return 22.f + (x - 240.f) * 0.32f;
    if (x < 700.f) return 92.4f;
    if (x < 860.f) return 92.4f - (x - 700.f) * 0.28f;
    if (x < 1080.f) return 47.6f + (x - 860.f) * 0.12f;
    return 74.f;
}

float ceilAt(float x) {
    if (x < 500.f) return 230.f;
    if (x < 720.f) return 230.f - (x - 500.f) * 0.42f;
    if (x < 980.f) return 137.6f;
    if (x < 1120.f) return 137.6f + (x - 980.f) * 0.55f;
    return 230.f;
}

float stormAt(float t) { return -120.f + t * kStormV; }

}  // namespace

float Game::worldY(float alt) const { return 188.f - alt * 1.15f; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.vdp.setFogColor(gs::rgb4(4, 5, 7));
    sys.apu.setMaster(0.42f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    x_ = 110.f;
    y_ = 48.f;
    cam_ = x_;
    time_ = 0;
}

void Game::begin() {
    x_ = 110.f;
    y_ = 48.f;
    vx_ = vy_ = 0;
    time_ = 0;
    hold_ = 0;
    cam_ = x_;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(220.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.12f);
    beep_ = 0.08f;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    vx_ *= 0.12f;
    vy_ *= 0.12f;
    sys_->apu.noiseBurst(0.35f, 700.f, 0.22f);
    sys_->apu.tone(1, 70.f, 0.24f);
    beep_ = 0.3f;
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    why_ = "cleared the pass";
    chime_ = 0;
    chimeT_ = 0;
}

void Game::pilot(float& thrust, float& climb) {
    const gs::Pad& pad = sys_->pad;
    thrust = 0;
    climb = 0;
    if (pad.down(gs::BTN_RIGHT)) thrust += 1.f;
    if (pad.down(gs::BTN_LEFT)) thrust -= 1.f;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) climb += 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B)) climb -= 1.f;
    if (std::fabs(pad.axisX) > 0.18f) thrust = pad.axisX;
    if (std::fabs(pad.axisY) > 0.18f) climb = pad.axisY;
    if (!bot_) return;

    float look = x_ + 90.f;
    float lo = floorAt(look) + 22.f;
    float hi = ceilAt(look) - 20.f;
    if (hi < lo + 8.f) hi = lo + 8.f;
    float ty = (lo + hi) * 0.5f;
    float wantV = 64.f;
    if (x_ > 1040.f) {
        ty = kPadY;
        wantV = clampf((1200.f - x_) * 0.45f, 6.f, 48.f);
        if (x_ > kEndL && x_ < kEndR) wantV = clampf((1204.f - x_) * 0.8f, -8.f, 18.f);
    }
    thrust = clampf((wantV - vx_) * 0.08f, -1.f, 1.f);
    climb = clampf((ty - y_) * 0.11f - vy_ * 0.18f, -1.f, 1.f);
}

void Game::step(float thrust, float climb) {
    time_ += kDt;
    vx_ += thrust * 48.f * kDt;
    vy_ += climb * 70.f * kDt;
    vx_ *= std::exp(-kDt * 0.7f);
    vy_ *= std::exp(-kDt * 1.8f);
    vx_ = clampf(vx_, -36.f, 72.f);
    vy_ = clampf(vy_, -46.f, 46.f);
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;

    float fl = floorAt(x_);
    float cl = ceilAt(x_);
    if (y_ - kHy < fl) {
        fail("hit the ridge");
        y_ = fl + kHy;
        return;
    }
    if (y_ + kHy > cl) {
        fail("hit the roof");
        y_ = cl - kHy;
        return;
    }
    if (x_ < stormAt(time_) + kHx) {
        fail("storm clock");
        return;
    }
    if (time_ > kLimit) {
        fail("storm clock");
        return;
    }
    if (x_ > kMiss) {
        fail("missed the end");
        return;
    }

    bool onEnd = x_ > kEndL + 6.f && x_ < kEndR - 6.f && std::fabs(y_ - kPadY) < 12.f &&
                 std::fabs(vx_) < 14.f && std::fabs(vy_) < 12.f && y_ > floorAt(x_) + kHy;
    if (onEnd) hold_ += kDt;
    else hold_ = 0;
    if (hold_ > 0.28f) finish();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float thrust = 0, climb = 0;
            pilot(thrust, climb);
            step(thrust, climb);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }

    float want = x_ - 20.f;
    cam_ += (want - cam_) * (1.f - std::exp(-kDt * 3.2f));
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.4f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    rotor_ = int((time_ + sys.frame * kDt) * 24.f) % 3;
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        int stepN = int(chimeT_ / 0.16f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(2, notes[stepN], 0.16f);
        }
        if (stepN >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::quad(float x0, float y0, float x1, float y1, const gs::Mipped& m, int pal) {
    if (m.h < 1) return;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    float jx = (shake_ > 0.f) ? std::sin(time_ * 80.f) * shake_ * 3.f : 0.f;
    auto sx = [&](float x) { return (x - cam_) + 96.f + jx; };
    float px0 = sx(x0), px1 = sx(x1);
    float py0 = worldY(y1), py1 = worldY(y0);
    if (px1 < -40.f || px0 > gs::SCREEN_W + 40.f || py1 < -40.f || py0 > gs::SCREEN_H + 40.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(px1 - px0), 1L, 500L));
    s.h = int16_t(std::clamp(std::lround(py1 - py0), 1L, 500L));
    s.x = int16_t(std::lround(px0));
    s.y = int16_t(std::lround(py0));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    for (const char* p = s; *p; ++p) {
        char ch = *p;
        if (ch < 32 || ch > 126) {
            x += 6.f * scale;
            continue;
        }
        const gs::Mipped& m = art_.glyph[ch - 32];
        float h = std::max(7.f * scale, 1.f);
        spr(m, x + h * 0.45f, y, h, pal);
        x += (ch == ' ' ? 4.6f : 6.0f) * scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    float storm = (mode_ == Mode::Title) ? 0.f : clampf(time_ / kLimit, 0.f, 1.f);
    float front = stormAt(mode_ == Mode::Run || mode_ == Mode::Fail || mode_ == Mode::Win ? time_ : 0.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int((3 + 5 * (1.f - u)) * (1.f - storm * 0.65f));
        int g = int((5 + 4 * (1.f - u)) * (1.f - storm * 0.55f));
        int b = int((8 + 5 * (1.f - u)) * (1.f - storm * 0.35f) + storm * 2.f);
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = uint8_t(storm > 0.72f && y < 40 ? 4 : (y > 200 ? 2 : 0));
        vdp.road[y].on = false;
    }

    float base = cam_ - 180.f;
    int i0 = int(base / 36.f) - 1;
    for (int i = i0; i < i0 + 16; i++) {
        float wx = i * 36.f;
        float fl = floorAt(wx + 18.f);
        float cl = ceilAt(wx + 18.f);
        quad(wx, 0.f, wx + 36.f, fl, art_.rock, PAL_ROCK);
        quad(wx, fl - 6.f, wx + 36.f, fl + 4.f, art_.snow, PAL_SNOW);
        if (cl < 210.f) {
            quad(wx, cl, wx + 36.f, cl + 70.f, art_.rock, PAL_ROCK);
            quad(wx, cl - 2.f, wx + 36.f, cl + 8.f, art_.snow, PAL_SNOW);
        }
    }

    quad(70.f, 22.f, 168.f, 36.f, art_.pad, PAL_SNOW);
    quad(kEndL, kPadY - 16.f, kEndR, kPadY - 4.f, art_.pad, PAL_SNOW);

    float sxStorm = (front - cam_) + 96.f;
    for (int c = 0; c < 5; c++) {
        float cy = 30.f + c * 36.f;
        spr(art_.cloud, sxStorm - 10.f - c * 18.f, cy, 28.f + (c & 1) * 8.f, PAL_FX);
        spr(art_.cloud, sxStorm - 70.f, cy + 12.f, 22.f, PAL_FX);
    }
    spr(art_.cloud, 250.f + std::sin(time_ * 0.3f) * 6.f, 26.f, 16.f, PAL_FX);

    float jx = (shake_ > 0.f) ? std::sin(time_ * 80.f) * shake_ * 3.f : 0.f;
    float sx = (x_ - cam_) + 96.f + jx;
    float sy = worldY(y_);
    bool faceL = vx_ < -4.f;
    spr(art_.body, sx, sy, 34.f, PAL_SHIP, faceL);
    spr(art_.rotor[rotor_], sx + (faceL ? -4.f : 4.f), sy - 15.f, 11.f, PAL_SHIP, faceL);

    if (mode_ == Mode::Title) {
        text("S3 HELIPASS", 84, 58, 2.0f, PAL_HUD);
        text("CLEAR THE PASS", 78, 86, 1.35f, PAL_HUD);
        text("BEFORE THE STORM CLOCK", 48, 106, 1.15f, PAL_HUD);
        text("ARROWS FLY   ENTER START", 46, 132, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 118, 90, 2.f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("PASS CLEAR", 96, 28, 1.8f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(why_, 70, 24, 1.4f, PAL_HUD);
    } else {
        int left = int(std::ceil(std::max(0.f, kLimit - time_)));
        char buf[24];
        std::snprintf(buf, sizeof(buf), "STORM %02d", left);
        text(buf, 8, 14, 1.1f, left < 10 ? PAL_HUD : PAL_HUD);
        text(x_ < 1000.f ? "HOLD THE GAP" : "MAKE THE END", 150, 14, 1.05f, PAL_HUD);
    }
}

}  // namespace helipass
