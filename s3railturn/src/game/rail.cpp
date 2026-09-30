#include "game/rail.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace railturn {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float PI = 3.14159265f;
constexpr float KLEAN = 0.088f;
constexpr float TIP = 0.58f;
constexpr float FINISH = 820.0f;
constexpr int HOR = 86;

float bump(float s, float a, float b, float peak) {
    if (s <= a || s >= b) return 0;
    float u = (s - a) / (b - a);
    return peak * std::sin(u * PI);
}

int skyAt(int y) {
    float t = y / 223.0f;
    int r = int(2 + t * 8);
    int g = int(3 + t * 7);
    int b = int(8 + t * 4);
    return gs::rgb4(std::min(r, 15), std::min(g, 15), std::min(b, 15));
}

}  // namespace

float Game::kappa(float s) const {
    return bump(s, 80, 200, 0.012f) + bump(s, 300, 460, -0.020f) + bump(s, 560, 760, 0.030f);
}

float Game::lateral(float dist) const {
    const int n = 12;
    float step = dist / float(n);
    float x = 0, hdg = 0;
    for (int i = 0; i < n; i++) {
        hdg += kappa(s_ + (i + 0.5f) * step) * step;
        x += hdg * step;
    }
    return x;
}

float Game::required() const { return -speed_ * speed_ * kappa(s_) * KLEAN; }

void Game::resetRun() {
    static const float cps[] = {0, 220, 490};
    s_ = cps[std::clamp(cp_, 0, 2)];
    speed_ = 18;
    bank_ = 0;
    lean_ = 0;
    slipT_ = 0;
    shake_ = 0;
    if (s_ < 200) cleared_ = 0;
    else if (s_ < 460) cleared_ = 1;
    else cleared_ = 2;
}

void Game::text(const std::string& s, float x, float y, int pal, bool center) {
    float width = 0;
    for (char ch : s) {
        int i = (ch >= 32 && ch < 127) ? ch - 32 : 0;
        const gs::Image& g = art_.glyph[i];
        width += (g.h > 0 ? g.w * (14.0f / g.h) : 8) + 1;
    }
    float cx = center ? x - width * 0.5f : x;
    for (char ch : s) {
        int i = (ch >= 32 && ch < 127) ? ch - 32 : 0;
        const gs::Image& g = art_.glyph[i];
        if (g.h <= 0) continue;
        float h = 14;
        float w = g.w * (h / g.h);
        spr(g, cx + w * 0.5f, y, h, pal);
        cx += w + 1;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float h, int pal, int fog, bool shadow) {
    if (h < 1.0f || img.h == 0) return;
    float sc = h / float(img.h);
    float w = img.w * sc;
    gs::Sprite s;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = img;
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::update(float dt) {
    float want = 0;
    if (!bot_) {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) want -= 1;
        if (pad.down(gs::BTN_RIGHT)) want += 1;
        want += pad.axisX;
        want = std::clamp(want, -1.0f, 1.0f);
        float thr = 0;
        if (pad.down(gs::BTN_A) || pad.accel > 0.25f) thr += 1;
        if (pad.down(gs::BTN_B) || pad.brake > 0.25f) thr -= 1;
        speed_ += thr * 12.0f * dt;
        speed_ -= 1.2f * dt;
    } else {
        float look = 0;
        for (float a = 6; a <= 48; a += 6) look = std::max(look, std::fabs(kappa(s_ + a)));
        float safe = look < 1e-5f ? 28.0f : std::sqrt(0.55f / (look * KLEAN));
        safe = std::clamp(safe, 14.0f, 28.0f);
        speed_ += (speed_ > safe ? -14.0f : 8.0f) * dt;
        want = std::clamp(required(), -1.0f, 1.0f);
    }
    speed_ = std::clamp(speed_, 10.0f, 34.0f);
    lean_ = want;
    if (bot_) bank_ = want;
    else bank_ += (want - bank_) * std::min(1.0f, dt * 4.5f);

    float req = required();
    float slip = std::fabs(bank_ - req);
    if (slip > TIP) slipT_ += dt;
    else slipT_ = std::max(0.0f, slipT_ - dt * 1.5f);

    s_ += speed_ * dt;
    if (cleared_ < 1 && s_ >= 200) cleared_ = 1;
    if (cleared_ < 2 && s_ >= 460) cleared_ = 2;
    if (cleared_ < 3 && s_ >= 760) cleared_ = 3;

    if (slipT_ > 0.40f) {
        mode_ = Mode::Tip;
        hold_ = 0;
        shake_ = 1;
        lives_--;
        sys_->apu.noiseBurst(0.4f, 1400, 0.35f);
        sys_->apu.tone(1, 90, 0.2f);
        return;
    }
    if (s_ >= FINISH && cleared_ >= 3) {
        mode_ = Mode::Win;
        hold_ = 0;
        won_ = true;
        sys_->apu.tone(1, 523, 0.18f);
    }
    float pitch = 55.0f + speed_ * 4.2f;
    sys_->apu.tone(0, pitch, 0.07f);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    float cam = s_;
    int shake = int(shake_ * 4 * std::sin(t_ * 40.0f));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = uint16_t(skyAt(std::clamp(y + shake, 0, gs::SCREEN_H - 1)));
        vdp.lineFog[y] = uint8_t(y < HOR ? (HOR - y) / 6 : 0);
        gs::RoadLine& r = vdp.road[y];
        if (y < HOR) {
            r.on = false;
            continue;
        }
        float p = (y - HOR) / float(gs::SCREEN_H - 1 - HOR);
        float denom = p + 0.05f;
        float meters = 42.0f * (1.0f - p) * (1.0f - p);
        float lat = lateral(meters) * 3.2f;
        r.on = true;
        r.hw = 78.0f * denom;
        r.cx = 160.0f - lat * (r.hw / 2.15f) + shake;
        r.v = (cam + meters) * 40.0f;
        r.pal = PAL_ROAD;
        r.band = (int(cam + meters) / 8) & 1;
        r.style = 1;
        r.left = gs::GROUND_DROP;
        r.right = gs::GROUND_DROP;
    }
    vdp.roadTime = int(cam * 3);

    auto project = [&](float dist, float side, float& ox, float& oy, float& sc, int& fog) {
        dist = std::max(dist, 0.4f);
        float p = 1.0f - std::sqrt(std::min(dist / 42.0f, 1.0f));
        float denom = p + 0.05f;
        float hw = 78.0f * denom;
        float lat = lateral(dist) * 3.2f;
        float cx = 160.0f - lat * (hw / 2.15f);
        float ppm = hw / 2.15f;
        ox = cx + side * ppm;
        oy = HOR + p * float(gs::SCREEN_H - 1 - HOR);
        sc = std::max(2.0f, hw * 0.9f);
        fog = int((1.0f - p) * 12);
    };

    for (int i = 8; i >= 0; i--) {
        float base = std::floor(s_ / 24.0f) * 24.0f + i * 24.0f;
        float dist = base - s_;
        if (dist < 1.5f || dist > 40.0f) continue;
        float x, y, sc;
        int fog;
        float side = (int(base / 24.0f) & 1) ? 5.2f : -5.2f;
        project(dist, side, x, y, sc, fog);
        spr(art_.block[int(base / 24.0f) % 3], x, y - sc * 0.35f, sc * 1.3f, PAL_CITY, fog);
    }
    for (int i = 6; i >= 0; i--) {
        float base = std::floor(s_ / 16.0f) * 16.0f + i * 16.0f;
        float dist = base - s_;
        if (dist < 1.2f || dist > 38.0f) continue;
        float x, y, sc;
        int fog;
        project(dist, 2.6f, x, y, sc, fog);
        spr(art_.pylon, x, y - sc * 0.2f, sc * 1.6f, PAL_STEEL, fog);
        project(dist, -2.6f, x, y, sc, fog);
        spr(art_.pylon, x, y - sc * 0.2f, sc * 1.6f, PAL_STEEL, fog);
    }

    int pose = int(std::lround((bank_ + 1.0f) * 2.0f));
    pose = std::clamp(pose, 0, 4);
    float bob = std::sin(t_ * speed_ * 0.35f) * 1.5f;
    spr(art_.shadow, 160, 176 + bob, 16, PAL_CAR, 0, true);
    spr(art_.car[pose], 160, 158 + bob, 78, PAL_CAR, 0);

    char buf[64];
    if (mode_ == Mode::Title) {
        text("S3 RAIL TURN", 160, 28, PAL_AMBER, true);
        text("MAKE THE THREE TURNS", 160, 52, PAL_HUD, true);
        text("WITHOUT TIPPING", 160, 68, PAL_HUD, true);
        if (int(t_ * 2) % 2 == 0) text("PRESS START", 160, 100, PAL_GREEN, true);
        text("ARROWS LEAN   A GAS   B BRAKE", 160, 200, PAL_HUD, true);
        text(S3_VERSION_STRING, 8, 210, PAL_HUD, false);
    } else if (mode_ == Mode::Run || mode_ == Mode::Tip) {
        std::snprintf(buf, sizeof buf, "TURN %d OF 3", std::min(cleared_ + 1, 3));
        text(buf, 8, 14, PAL_AMBER, false);
        std::snprintf(buf, sizeof buf, "SPEED %d", int(speed_));
        text(buf, 210, 14, PAL_HUD, false);
        std::snprintf(buf, sizeof buf, "CARS %d", std::max(0, lives_));
        text(buf, 8, 30, PAL_HUD, false);
        float slip = bank_ - required();
        text(slip < -0.25f ? "LEAN RIGHT" : slip > 0.25f ? "LEAN LEFT" : "ON THE RAIL", 160, 30, std::fabs(slip) > TIP ? PAL_RED : PAL_GREEN,
             true);
        if (mode_ == Mode::Tip) text(lives_ > 0 ? "TIPPED" : "OFF THE RAIL", 160, 90, PAL_RED, true);
    } else if (mode_ == Mode::Win) {
        text("THREE TURNS CLEAR", 160, 40, PAL_GREEN, true);
        text("THE RAIL HELD", 160, 58, PAL_AMBER, true);
        text("PRESS START", 160, 96, PAL_HUD, true);
    } else if (mode_ == Mode::Lose) {
        text("TIPPED OFF", 160, 48, PAL_RED, true);
        std::snprintf(buf, sizeof buf, "TURNS %d OF 3", cleared_);
        text(buf, 160, 70, PAL_HUD, true);
        text("PRESS START", 160, 100, PAL_AMBER, true);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    lives_ = 3;
    cleared_ = 0;
    cp_ = 0;
    over_ = false;
    won_ = false;
    t_ = 0;
    if (bot_) {
        mode_ = Mode::Run;
        resetRun();
    } else {
        mode_ = Mode::Title;
        s_ = 40;
        speed_ = 16;
        bank_ = 0;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    shake_ *= 0.92f;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        s_ += 10.0f * DT;
        if (s_ > 70) s_ = 20;
        bank_ = std::sin(t_ * 0.8f) * 0.35f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Run;
            lives_ = 3;
            cp_ = 0;
            cleared_ = 0;
            resetRun();
            sys.apu.tone(1, 660, 0.12f);
        }
    } else if (mode_ == Mode::Run) {
        update(DT);
    } else if (mode_ == Mode::Tip) {
        hold_ += DT;
        bank_ += (bank_ >= 0 ? 1.2f : -1.2f) * DT;
        sys.apu.tone(0, 40, 0.04f);
        if (hold_ > 1.1f) {
            if (lives_ <= 0) {
                mode_ = Mode::Lose;
                hold_ = 0;
                if (bot_) over_ = true;
            } else {
                cp_ = cleared_;
                resetRun();
                mode_ = Mode::Run;
            }
        }
    } else if (mode_ == Mode::Win) {
        hold_ += DT;
        s_ += 8.0f * DT;
        bank_ *= 0.96f;
        sys.apu.tone(0, 70, 0.05f);
        if (hold_ > 0.4f && int(hold_ / 0.18f) != int((hold_ - DT) / 0.18f)) {
            static const float notes[] = {523, 659, 784, 1046};
            sys.apu.tone(1, notes[std::min(3, int(hold_ / 0.18f) - 1)], 0.16f);
        }
        if (bot_ && hold_ > 1.6f) over_ = true;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            won_ = false;
            lives_ = 3;
            cp_ = 0;
            cleared_ = 0;
            mode_ = Mode::Run;
            resetRun();
        }
    } else if (mode_ == Mode::Lose) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            lives_ = 3;
            cp_ = 0;
            cleared_ = 0;
            mode_ = Mode::Run;
            resetRun();
        }
    }
    draw();
}

}  // namespace railturn
