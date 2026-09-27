#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace spanpouc {
namespace {

constexpr float GOAL = 440.f;
constexpr float FOCAL = 92.f;
constexpr float CAM_H = 30.f;

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Run) return 1;
    return 2;
}

float Game::curve(float z) const {
    float amp = 1.2f;
    float tail = GOAL - z;
    if (tail < 90.f) amp *= std::clamp(tail / 90.f, 0.f, 1.f);
    return std::sin(z * 0.030f) * amp + std::sin(z * 0.011f + 0.6f) * 0.38f * amp;
}

float Game::half(float z) const { return 2.45f + 0.12f * std::sin(z * 0.05f); }

void Game::resetRun() {
    z_ = 0;
    x_ = curve(0);
    vx_ = 0;
    speed_ = 17.5f;
    modeT_ = 0;
    fallSide_ = 1;
    won_ = false;
    over_ = false;
    reason_.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    t_ = 0;
    modeT_ = 0;
    resetRun();
    speed_ = 4.f;
}

void Game::blit(const gs::Image& img, float x, float y, float h, int pal, bool feet, int fog) {
    if (img.h < 1 || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.h = std::max(1, int(std::lround(h)));
    s.w = std::max(1, int(std::lround(h * float(img.w) / float(img.h))));
    s.x = int(std::lround(x - s.w * 0.5f));
    s.y = int(std::lround(feet ? y - s.h : y));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::textAt(const std::string& s, float x, float y, int pal) {
    float pen = x;
    for (char ch : s) {
        int i = int(static_cast<unsigned char>(ch)) - 32;
        if (i < 0 || i >= 96) {
            pen += 8;
            continue;
        }
        const gs::Image& g = art_.glyph[i];
        gs::Sprite sp;
        sp.img = g;
        sp.w = art_.glyphW[i];
        sp.h = art_.glyphH;
        sp.x = int(std::lround(pen));
        sp.y = int(std::lround(y));
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        pen += float(art_.glyphW[i]) + 1.f;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(7, 9, 11));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= hor_) {
            float u = float(y) / float(hor_);
            int r = int(std::lround(3 + 9 * u));
            int g = int(std::lround(5 + 6 * u));
            int b = int(std::lround(9 + 3 * u));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(std::clamp((1.f - u) * 4.f, 0.f, 4.f));
        } else {
            float u = float(y - hor_) / float(gs::SCREEN_H - hor_);
            int g = int(std::lround(5 + 4 * (1.f - u)));
            int b = int(std::lround(7 + 3 * (1.f - u)));
            v.lineBackdrop[y] = gs::rgb4(1, g, b);
            float dz = FOCAL * CAM_H / float(y - hor_);
            v.lineFog[y] = uint8_t(std::clamp((dz - 40.f) / 28.f, 0.f, 12.f));
        }
    }
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.enabled = false;
}

void Game::deck() {
    gs::VDP& v = sys_->vdp;
    const float camX = (mode_ == Mode::Fall) ? x_ + fallSide_ * modeT_ * 3.f : x_;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 520.f) continue;
        float zw = z_ + dz;
        float hw = FOCAL * half(zw) / dz;
        L.on = true;
        L.cx = 160.f + (curve(zw) - camX) * (FOCAL / dz);
        L.hw = hw;
        L.v = zw * 18.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor(zw * 0.35f)) & 1) ? 1 : 0;
        L.style = gs::ROAD_RUTS;
        L.left = gs::GROUND_DROP;
        L.right = gs::GROUND_DROP;
    }
    v.roadTime = int(t_ * 60.f);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    sky();
    deck();

    auto imageTop = [&](const gs::Image& img, float cx, float top, int pal) {
        blit(img, cx, top, float(img.h), pal, false, 0);
    };

    if (mode_ == Mode::Title) {
        imageTop(art_.title, 160, 28, PAL_HUD);
        imageTop(art_.line1, 160, 78, PAL_HUD);
        imageTop(art_.line2, 160, 100, PAL_HUD);
        imageTop(art_.hint, 160, 188, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        imageTop(art_.win, 160, 24, PAL_HUD);
    } else if (mode_ == Mode::Fall) {
        imageTop(art_.fell, 160, 24, PAL_HUD);
    }

    if (mode_ != Mode::Title) {
        int pct = int(std::clamp(z_ / GOAL, 0.f, 1.f) * 100.f);
        std::string hud = "SPAN " + std::to_string(pct);
        textAt(hud, 8, 8, PAL_HUD);
        textAt("POUCH", 250, 8, PAL_HUD);
    }

    blit(art_.sun, 250, 18, 28, PAL_SUN, false, 2);

    const float camX = (mode_ == Mode::Fall) ? x_ + fallSide_ * modeT_ * 3.f : x_;
    float gateZ = GOAL - z_;
    if (gateZ > 6.f && gateZ < 240.f) {
        float y = float(hor_) + FOCAL * CAM_H / gateZ;
        float px = FOCAL / gateZ;
        float sx = 160.f + (curve(GOAL) - camX) * px;
        float h = std::clamp(220.f / gateZ, 8.f, 150.f);
        int fog = int(std::clamp((gateZ - 30.f) / 14.f, 0.f, 12.f));
        blit(art_.gate, sx, y, h, PAL_GATE, true, fog);
    }

    for (int i = 14; i >= 1; --i) {
        float dz = 10.f + float(i) * 12.f;
        float zw = z_ + dz;
        if (zw > GOAL + 4.f) continue;
        float y = float(hor_) + FOCAL * CAM_H / dz;
        if (y >= gs::SCREEN_H - 2) continue;
        float px = FOCAL / dz;
        float cx = 160.f + (curve(zw) - camX) * px;
        float hw = half(zw) * px;
        float ph = std::clamp(150.f / dz, 6.f, 90.f);
        int fog = int(std::clamp((dz - 24.f) / 16.f, 0.f, 12.f));
        blit(art_.post, cx - hw, y, ph, PAL_TIMBER, true, fog);
        blit(art_.post, cx + hw, y, ph, PAL_TIMBER, true, fog);
    }

    float bodyX = 160.f;
    float bodyY = 206.f;
    float pouchX = 184.f;
    float pouchY = 168.f;
    if (mode_ == Mode::Fall) {
        bodyX += fallSide_ * modeT_ * 70.f;
        bodyY += modeT_ * 48.f;
        pouchX += fallSide_ * modeT_ * 110.f;
        pouchY += modeT_ * 90.f;
    } else if (mode_ == Mode::Won) {
        bodyX = 168.f;
    }
    blit(art_.pouch, pouchX, pouchY, 26, PAL_POUCH, false, 0);
    blit(art_.walker, bodyX, bodyY, 84, PAL_WALKER, true, 0);
}

void Game::tick(float dt) {
    const gs::Pad& pad = sys_->pad;
    float steer = 0;
    if (bot_) {
        float look = z_ + 22.f + speed_ * 0.45f;
        if (look > GOAL) look = GOAL;
        float err = curve(look) - x_;
        steer = std::clamp(err * 1.55f - vx_ * 0.42f, -1.f, 1.f);
    } else {
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
        steer = std::clamp(steer, -1.f, 1.f);
    }

    vx_ += steer * 28.f * dt;
    vx_ *= std::pow(0.08f, dt);
    x_ += vx_ * dt;
    z_ += speed_ * dt;

    float off = x_ - curve(z_);
    if (std::fabs(off) > half(z_)) {
        fallSide_ = off >= 0 ? 1.f : -1.f;
        mode_ = Mode::Fall;
        modeT_ = 0;
        reason_ = "the pouch left the span";
        sys_->apu.tone(0, 140, 0.2f);
        return;
    }
    if (z_ >= GOAL) {
        z_ = GOAL;
        x_ = curve(GOAL);
        mode_ = Mode::Won;
        modeT_ = 0;
        won_ = true;
        reason_ = "the pouch crossed the span";
        sys_->apu.tone(0, 520, 0.18f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    modeT_ += dt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        z_ = std::fmod(t_ * 4.f, 80.f);
        x_ = curve(z_);
        vx_ = 0;
        bool go = bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
        if (go) {
            mode_ = Mode::Run;
            resetRun();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            // a pause would be something other than the crossing
        }
        tick(dt);
        float wind = 90.f + 40.f * std::sin(t_ * 0.7f);
        sys.apu.tone(1, wind, 0.03f);
    } else if (mode_ == Mode::Fall) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        if (modeT_ > 1.4f) over_ = true;
    } else if (mode_ == Mode::Won) {
        if (modeT_ > 0.4f) sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        if (modeT_ > 1.6f) over_ = true;
    }

    draw();
}

}  // namespace spanpouc
