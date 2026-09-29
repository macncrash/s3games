#include "game/mush.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mushslip {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kTide = 52.f;
constexpr float kMouth = 118.f;
constexpr float kHead = 168.f;
constexpr float kHalf = 6.4f;
constexpr float kPierOut = 13.6f;
constexpr float kBank = 30.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
float len(float x, float y) { return std::sqrt(x * x + y * y); }

bool hitPier(float px, float py) {
    const bool along = py > kMouth - 6.f && py < kHead + 8.f;
    const bool side = (px > -kPierOut && px < -kHalf) || (px > kHalf && px < kPierOut);
    const bool head = py > kHead && py < kHead + 8.f && px > -kPierOut && px < kPierOut;
    return (along && side) || head;
}

}  // namespace

float Game::speed() const { return len(vx_, vy_); }

float Game::tideLeft() const { return std::max(0.f, kTide - race_); }

bool Game::insideSlip(float px, float py) const {
    return std::fabs(px) < kHalf - 0.35f && py > kMouth + 2.f && py < kHead - 1.f;
}

bool Game::inBerth(float px, float py) const {
    return std::fabs(px) < 4.2f && py > kHead - 22.f && py < kHead - 3.f;
}

int Game::sledFrame(float heading) const {
    float h = heading;
    const float pi = 3.14159265f;
    while (h < 0) h += pi * 2.f;
    while (h >= pi * 2.f) h -= pi * 2.f;
    int f = int(std::floor(h / (pi / 4.f) + 0.5f)) & 7;
    return f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (holding_) return 3;
    if (inSlip_) return 2;
    return 1;
}

void Game::showTitle() { mode_ = Mode::Title; }

void Game::begin() {
    over_ = false;
    won_ = false;
    inSlip_ = false;
    holding_ = false;
    race_ = 0;
    hold_ = 0;
    t_ = 0;
    x_ = 16.f;
    y_ = 10.f;
    heading_ = 0.55f;
    vx_ = 0;
    vy_ = 0;
    rx_ = -8.f;
    ry_ = 36.f;
    rv_ = 4.15f;
    rH_ = 0.15f;
    rivalIn_ = false;
    rivalDone_ = false;
    camX_ = x_;
    camY_ = y_;
    why_[0] = 0;
    for (auto& p : spray_) p.life = 0;
    mode_ = Mode::Run;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    showTitle();
    if (bot_) begin();
}

void Game::controls(float& steer, float& throttle, float& brake) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    throttle = 0;
    brake = 0;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = p.axisX;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.accel > 0.2f) throttle = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.2f) brake = 1.f;
    steer = clampf(steer, -1.f, 1.f);
}

void Game::pilot(float& steer, float& throttle, float& brake) {
    float aim = 0.f;
    if (y_ < 70.f) aim = 0.f;
    float want = std::atan2(aim - x_, 26.f);
    float err = want - heading_;
    while (err > 3.14159265f) err -= 6.2831853f;
    while (err < -3.14159265f) err += 6.2831853f;
    float lateral = aim - x_;
    steer = clampf(err * 2.4f + lateral * 0.04f, -1.f, 1.f);
    float sp = speed();
    throttle = 1.f;
    brake = 0.f;
    if (y_ > 142.f) throttle = 0.35f;
    if (y_ > 150.f || (sp > 7.f && y_ > 146.f)) {
        throttle = 0.f;
        brake = 1.f;
    }
    if (inBerth(x_, y_) && sp < 3.f) {
        throttle = 0.f;
        brake = 1.f;
        steer = clampf(-heading_ * 3.f, -1.f, 1.f);
    }
}

void Game::physics(float dt, float steer, float throttle, float brake) {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    float fwd = vx_ * s + vy_ * c;
    float lat = vx_ * c - vy_ * s;
    fwd += throttle * 16.5f * dt;
    float drag = insideSlip(x_, y_) ? 1.35f : 0.42f;
    if (std::fabs(x_) > kBank && y_ < kMouth) drag += 1.6f;
    fwd -= fwd * drag * dt;
    lat -= lat * (insideSlip(x_, y_) ? 4.2f : 1.7f) * dt;
    if (brake > 0.f) fwd -= fwd * 3.4f * dt;
    if (fwd > 14.5f) fwd = 14.5f;
    if (fwd < -3.2f) fwd = -3.2f;
    float yaw = steer * (1.15f + std::fabs(fwd) * 0.11f);
    heading_ += yaw * dt;
    vx_ = s * fwd + c * lat;
    vy_ = c * fwd - s * lat;
    float nx = x_ + vx_ * dt;
    float ny = y_ + vy_ * dt;
    if (hitPier(nx, y_)) {
        vx_ *= -0.25f;
        nx = x_;
    }
    if (hitPier(x_, ny)) {
        vy_ *= -0.25f;
        ny = y_;
    }
    if (hitPier(nx, ny)) {
        nx = x_;
        ny = y_;
        vx_ *= 0.4f;
        vy_ *= 0.4f;
    }
    x_ = nx;
    y_ = ny;
    if (speed() > 2.4f) {
        Puff& p = spray_[puffCursor_];
        p.x = x_ - s * 3.2f;
        p.y = y_ - c * 3.2f;
        p.life = 0.45f;
        puffCursor_ = (puffCursor_ + 1) % 10;
    }
    for (auto& p : spray_) p.life -= dt;
}

void Game::rivalStep(float dt) {
    if (rivalDone_) return;
    float aim = 0.f;
    rH_ += clampf((aim - rx_) * 0.08f - rH_, -1.f, 1.f) * dt * 1.4f;
    float cap = 4.35f;
    if (ry_ > 148.f) cap = 1.6f;
    if (ry_ > 156.f) cap = 0.35f;
    rv_ += (cap - rv_) * std::min(1.f, dt * 1.8f);
    if (rv_ < 0.f) rv_ = 0.f;
    rx_ += std::sin(rH_) * rv_ * dt;
    ry_ += std::cos(rH_) * rv_ * dt;
    rx_ = clampf(rx_, -5.2f, 5.2f);
    if (ry_ > kMouth) rivalIn_ = true;
    if (inBerth(rx_, ry_) && rv_ < 0.45f && ry_ > 152.f) rivalDone_ = true;
}

void Game::win() {
    if (won_) return;
    won_ = true;
    over_ = true;
    holding_ = true;
    mode_ = Mode::Win;
    std::snprintf(why_, sizeof why_, "berthed");
    sys_->apu.tone(0, 523.f, 0.18f);
    sys_->apu.tone(1, 784.f, 0.14f);
    tone0_ = 0.35f;
}

void Game::fail(const char* why) {
    if (over_) return;
    std::snprintf(why_, sizeof why_, "%s", why);
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 90.f, 0.2f);
    tone0_ = 0.4f;
}

void Game::judge() {
    inSlip_ = insideSlip(x_, y_);
    float sp = speed();
    bool lined = std::fabs(heading_) < 0.62f || std::fabs(heading_ - 6.2831853f) < 0.62f;
    if (inBerth(x_, y_) && sp < 0.55f && lined && inSlip_) {
        hold_ += DT;
        holding_ = hold_ > 0.12f;
        if (hold_ > 0.42f) win();
    } else {
        hold_ = std::max(0.f, hold_ - DT * 2.f);
        holding_ = false;
    }
    if (std::fabs(x_) > 46.f || y_ < -12.f || y_ > kHead + 16.f) fail("off the ice");
    if (rivalDone_) fail("other crew berthed");
    if (race_ >= kTide) fail("tide turned");
}

void Game::audio(float dt) {
    if (tone0_ > 0.f) tone0_ -= dt;
    else if (mode_ == Mode::Run) {
        float sp = speed();
        if (sp > 1.f) sys_->apu.tone(0, 70.f + sp * 6.f, 0.04f);
        else sys_->apu.tone(0, 0, 0);
    } else {
        sys_->apu.tone(0, 0, 0);
    }
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 126.f - (wy - camY_) * zoom_;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 2.f) return;
    gs::Sprite s;
    s.img = m.pick(h);
    s.h = std::max(1, int(h));
    s.w = std::max(1, int(m.w * h / float(std::max(1, m.h))));
    s.x = int(cx - s.w * 0.5f);
    s.y = int(cy - s.h * 0.5f);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    if (sx < -40 || sx > 360 || sy < -40 || sy > 260) return;
    spr(m, sx, sy, worldH * zoom_, pal);
}

void Game::hud(int col, int row, const char* s, int pal) {
    int x = 8 + col * 6;
    int y = 6 + row * 10;
    for (const char* p = s; *p; ++p) {
        unsigned c = unsigned(*p);
        if (c < 32 || c > 127) c = '?';
        gs::Sprite sp;
        sp.img = art_.glyph[c - 32];
        sp.x = int16_t(x);
        sp.y = int16_t(y);
        sp.w = 5;
        sp.h = 7;
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += 6;
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    int x = 160 - n * 3;
    int y = 6 + row * 10;
    for (const char* p = s; *p; ++p) {
        unsigned c = unsigned(*p);
        if (c < 32 || c > 127) c = '?';
        gs::Sprite sp;
        sp.img = art_.glyph[c - 32];
        sp.x = int16_t(x);
        sp.y = int16_t(y);
        sp.w = 5;
        sp.h = 7;
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += 6;
    }
}

void Game::drawHud() {
    char buf[64];
    int left = int(std::ceil(tideLeft() - 0.001f));
    if (left < 0) left = 0;
    std::snprintf(buf, sizeof buf, "TIDE %02d", left);
    hud(0, 0, buf, left < 10 ? PAL_ALERT : PAL_HUD);
    const char* crew = rivalDone_ ? "CREW IN" : (rivalIn_ ? "CREW SLIP" : "CREW OUT");
    hud(28, 0, crew, rivalIn_ ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Run) {
        const char* hint = holding_ ? "HOLD THE BERTH" : (inSlip_ ? "STOP IN THE SLIP" : "MUSH TO THE SLIP");
        hudC(20, hint, PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(18, "ARROWS MUSH   B BRAKE", PAL_HUD);
        hudC(19, "START TO RUN", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(18, "START TO RUN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(18, why_, PAL_ALERT);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.A.clear();
    v.B.clear();
    v.HUD.clear();
    float tide = clampf(race_ / kTide, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        float u = y / float(gs::SCREEN_H);
        int sky = y < 36;
        if (sky) {
            v.lineBackdrop[y] = gs::rgb4(4, 6, 9);
            v.lineFog[y] = 0;
        } else {
            int ice = 11 - int(tide * 4.f);
            int water = 6 + int((1.f - u) * 3.f);
            bool deep = u > 0.55f && tide > 0.15f;
            if (deep) v.lineBackdrop[y] = gs::rgb4(3, 5 + int(tide * 2), 8);
            else v.lineBackdrop[y] = gs::rgb4(ice, ice + 1, water);
            v.lineFog[y] = uint8_t(tide > 0.75f ? 2 : 0);
        }
    }
    camX_ += (x_ - camX_) * 0.12f;
    camY_ += (y_ + 6.f - camY_) * 0.12f;

    for (int i = 0; i < 8; i++) {
        float py = kMouth - 4.f + i * 7.2f;
        place(art_.post, -kHalf - 1.3f, py, 5.2f, PAL_WOOD);
        place(art_.post, kHalf + 1.3f, py, 5.2f, PAL_WOOD);
    }
    place(art_.post, -kPierOut + 1.f, kMouth, 5.2f, PAL_WOOD);
    place(art_.post, kPierOut - 1.f, kMouth, 5.2f, PAL_WOOD);
    place(art_.lamp, -kHalf - 2.4f, kHead - 4.f, 6.4f, PAL_LAMP);
    place(art_.lamp, kHalf + 2.4f, kHead - 4.f, 6.4f, PAL_LAMP);
    place(art_.flag, 0.f, kHead + 3.f, 6.f, PAL_FLAG);

    for (auto& p : spray_) {
        if (p.life > 0.f) place(art_.puff, p.x, p.y, 1.6f + (0.45f - p.life), PAL_HUD);
    }

    place(art_.sled[sledFrame(rH_)], rx_, ry_, 7.2f, PAL_RIVAL);
    place(art_.sled[sledFrame(heading_)], x_, y_, 8.0f, PAL_TEAM);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 48, 28, PAL_INK);
        spr(art_.sub, 160, 78, 14, PAL_INK);
    } else if (mode_ == Mode::Win) {
        spr(art_.berthed, 160, 36, 22, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped& ban = std::strcmp(why_, "tide turned") == 0 ? art_.tide : art_.beaten;
        spr(ban, 160, 36, 20, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160, 36, 20, PAL_INK);
    }
    drawHud();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (p.pressed(gs::BTN_START) && !bot_) mode_ = Mode::Pause;
        float steer = 0, throttle = 0, brake = 0;
        if (bot_) pilot(steer, throttle, brake);
        else controls(steer, throttle, brake);
        physics(DT, steer, throttle, brake);
        rivalStep(DT);
        race_ += DT;
        t_ += DT;
        judge();
    } else if ((mode_ == Mode::Win || mode_ == Mode::Fail) && !bot_) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) showTitle();
    }
    audio(DT);
    draw();
    if (mode_ == Mode::Win) sys.setLight(40, 180, 80);
    else if (mode_ == Mode::Fail) sys.setLight(180, 40, 30);
    else if (tideLeft() < 10.f && mode_ == Mode::Run) sys.setLight(180, 90, 20);
    else sys.setLight(20, 40, 70);
}

}  // namespace mushslip
