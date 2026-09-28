#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace plowboom {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kStorm = 48.f;
constexpr float kStop = 0.22f;
constexpr float kHoldNeed = 0.5f;
constexpr float kStillNeed = 1.25f;

// The boom deck is a slot at the north end of the yard. The drive must sit in it.
constexpr float kDeckX = 8.2f;
constexpr float kDeckY0 = 118.f;
constexpr float kDeckY1 = 146.f;
constexpr float kRailIn = 11.0f;
constexpr float kRailOut = 15.4f;
constexpr float kRailY0 = 112.f;
constexpr float kRailY1 = 156.f;
constexpr float kHeadY0 = 148.f;
constexpr float kHeadY1 = 156.f;

constexpr float kLead = 13.0f;
constexpr float kDL = 5.6f;
constexpr float kDW = 2.35f;
constexpr float kBodyX = 5.3f;
constexpr float kBodyY = 6.1f;

struct V2 {
    float x, y;
};

constexpr V2 kBank[] = {{-30.f, 20.f}, {-34.f, 48.f}, {-32.f, 78.f}, {30.f, 24.f}, {34.f, 52.f}, {32.f, 80.f}};

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool inBox(float x, float y, float x0, float x1, float y0, float y1) {
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

}  // namespace

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    tone_ = 0.07f;
}

void Game::fail(const char* why) {
    std::snprintf(why_, sizeof why_, "%s", why);
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    blip(90.f);
}

void Game::begin() {
    x_ = 5.2f;
    y_ = 18.f;
    heading_ = 0.32f;
    speed_ = 0.f;
    blade_ = 0.f;
    you_ = 0.f;
    hold_ = 0.f;
    still_ = 0.f;
    won_ = false;
    over_ = false;
    seated_ = false;
    fanStep_ = -1;
    flakeN_ = 0;
    why_[0] = 0;
    camX_ = x_;
    camY_ = y_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    storm_ = kStorm;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.snowTile, PAL_SNOW));
    sys.vdp.setFogColor(gs::rgb4(9, 11, 13));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.14f, 0.06f);
    begin();
    if (bot_) {
        mode_ = Mode::Drive;
        zoom_ = 2.2f;
    } else {
        mode_ = Mode::Title;
        zoom_ = 1.35f;
        camX_ = 0.f;
        camY_ = 78.f;
    }
}

void Game::controls(float& gas, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    gas = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = clampf(p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_UP)) gas += 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.down(gs::BTN_DOWN)) gas -= 1.f;
    if (p.accel > 0.08f) gas += p.accel;
    if (p.brake > 0.08f) gas -= p.brake;
    gas = clampf(gas, -1.f, 1.f);
}

void Game::pilot(float& gas, float& steer) {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    if (driveInside() && std::fabs(speed_) < kStop + 0.35f) {
        gas = clampf(-speed_ * 4.f, -1.f, 1.f);
        steer = clampf(wrapPi(0.f - heading_) * 1.4f, -0.4f, 0.4f);
        if (std::fabs(speed_) < kStop) steer = 0.f;
        return;
    }

    const float gate = 96.f;
    float tx = 0.f;
    float ty = 108.f;
    if (y_ > gate && std::fabs(x_) < 3.2f) {
        const float targetY = 130.f - kLead;
        float err = wrapPi(0.f - heading_);
        float want = clampf((targetY - y_) * 0.62f, 0.f, 3.4f);
        if (std::fabs(err) > 0.28f || std::fabs(x_) > 1.6f) want = std::min(want, 1.3f);
        if (y_ > targetY - 1.5f) want = clampf((targetY - y_) * 0.9f, -2.2f, 1.6f);
        const float side = x_;
        steer = clampf(err * 2.6f - side * 0.22f, -1.f, 1.f);
        gas = clampf((want - speed_) * 1.7f, -1.f, 1.f);
        return;
    }

    float dx = tx - x_;
    float dy = ty - y_;
    float err = wrapPi(std::atan2(dx, dy) - heading_);
    float dist = std::hypot(dx, dy);
    float desired = std::min(8.2f, dist * 0.26f);
    if (std::fabs(err) > 0.55f) desired = std::min(desired, 2.2f);
    (void)s;
    (void)c;
    steer = clampf(err * 2.3f, -1.f, 1.f);
    gas = clampf((desired - speed_) * 1.45f, -1.f, 1.f);
}

void Game::physics(float dt, float gas, float steer) {
    speed_ += gas * 15.f * dt;
    speed_ -= speed_ * 1.45f * dt;
    speed_ = clampf(speed_, -5.5f, 9.5f);
    const float turn = 1.65f + std::fabs(speed_) * 0.04f;
    heading_ = wrapPi(heading_ + steer * turn * dt);
    x_ += std::sin(heading_) * speed_ * dt;
    y_ += std::cos(heading_) * speed_ * dt;
    if (x_ > 26.f) x_ = 26.f;
    if (x_ < -26.f) x_ = -26.f;
    if (y_ < 4.f) y_ = 4.f;
    if (gas > 0.1f) blade_ += dt * 2.f;
    if (std::fabs(speed_) > 1.1f) {
        Flake& f = flake_[flakeN_ % 14];
        f.x = x_ - std::sin(heading_) * 4.f + (flakeN_ & 1 ? 2.5f : -2.5f);
        f.y = y_ - std::cos(heading_) * 3.f;
        f.life = 1.f;
        flakeN_++;
    }
    for (int i = 0; i < 14; i++) flake_[i].life -= dt * 0.9f;
}

bool Game::driveInside() const {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    const float lys[2] = {kLead - kDL, kLead + kDL};
    const float lxs[2] = {-kDW, kDW};
    for (float ly : lys) {
        for (float lx : lxs) {
            float wx = x_ + lx * c + ly * s;
            float wy = y_ - lx * s + ly * c;
            if (std::fabs(wx) > kDeckX) return false;
            if (wy < kDeckY0 || wy > kDeckY1) return false;
        }
    }
    return true;
}

bool Game::plowHitsBoom() const {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    const float lxs[3] = {-kBodyX, 0.f, kBodyX};
    const float lys[3] = {-kBodyY, 0.f, kBodyY};
    for (float ly : lys) {
        for (float lx : lxs) {
            float wx = x_ + lx * c + ly * s;
            float wy = y_ - lx * s + ly * c;
            if (inBox(wx, wy, -kRailOut, -kRailIn, kRailY0, kRailY1)) return true;
            if (inBox(wx, wy, kRailIn, kRailOut, kRailY0, kRailY1)) return true;
            if (inBox(wx, wy, -kRailOut, kRailOut, kHeadY0, kHeadY1)) return true;
        }
    }
    return false;
}

bool Game::driveHitsHead() const {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    const float lys[2] = {kLead - kDL, kLead + kDL};
    const float lxs[2] = {-kDW, kDW};
    for (float ly : lys) {
        for (float lx : lxs) {
            float wx = x_ + lx * c + ly * s;
            float wy = y_ - lx * s + ly * c;
            if (inBox(wx, wy, -kRailOut, kRailOut, kHeadY0, kHeadY1)) return true;
            if (inBox(wx, wy, -kRailOut, -kRailIn, kRailY0, kRailY1)) return true;
            if (inBox(wx, wy, kRailIn, kRailOut, kRailY0, kRailY1)) return true;
        }
    }
    return false;
}

void Game::audio(float dt, float gas) {
    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Drive && std::fabs(gas) > 0.12f) {
        float phase = blade_ - std::floor(blade_);
        if (phase < dt * 3.f) sys_->apu.tone(1, 80.f + std::fabs(speed_) * 7.f, 0.05f);
        else if (phase > 0.5f && phase < 0.5f + dt * 3.f) sys_->apu.tone(1, 0, 0);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {262.f, 330.f, 392.f, 523.f};
        fanStep_++;
        if (fanStep_ % 9 == 1 && fanStep_ < 36) sys_->apu.tone(0, notes[fanStep_ / 9], 0.1f);
        if (fanStep_ > 50) {
            fanStep_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = (wx - camX_) * zoom_ + 160.f;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal);
}

int Game::face() const {
    float h = seated_ && mode_ != Mode::Drive ? seatH_ : heading_;
    if (h < 0.f) h += kTau;
    return int(h / (kTau / 8.f) + 0.5f) & 7;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    int scrollX = int(std::lround(camX_ * zoom_));
    int scrollY = int(std::lround(-camY_ * zoom_));
    vdp.B.scroll(scrollX, scrollY);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = 10 + ((y / 36) % 2);
        vdp.lineBackdrop[y] = gs::rgb4(band, band + 1, 14);
        vdp.road[y].on = false;
    }

    place(art_.shed, 0.f, -2.f, 14.f, PAL_BOOM);
    for (const V2& r : kBank) place(art_.bank, r.x, r.y, 8.f, PAL_BANK);

    for (int i = 0; i < 5; i++) {
        float u = (i + 0.5f) / 5.f;
        float yy = kDeckY0 + u * (kDeckY1 - kDeckY0);
        place(art_.plank, 0.f, yy, 2.2f, PAL_BOOM);
    }
    place(art_.post, -kRailIn - 1.6f, kRailY0 + 6.f, 9.f, PAL_BOOM);
    place(art_.post, kRailIn + 1.6f, kRailY0 + 6.f, 9.f, PAL_BOOM);
    place(art_.post, -kRailIn - 1.6f, (kRailY0 + kHeadY0) * 0.5f, 9.f, PAL_BOOM);
    place(art_.post, kRailIn + 1.6f, (kRailY0 + kHeadY0) * 0.5f, 9.f, PAL_BOOM);
    place(art_.beam, 0.f, kHeadY0 + 2.f, 3.4f, PAL_BOOM);

    float px = x_, py = y_, hx = heading_;
    if (mode_ == Mode::Title) {
        px = 0.f;
        py = 40.f + std::sin(t_ * 1.2f) * 0.3f;
        hx = 0.f;
    }
    const float c = std::cos(hx);
    const float s = std::sin(hx);
    float dx = px + kLead * s;
    float dy = py + kLead * c;
    int df = face();
    if (seated_ && mode_ != Mode::Drive && mode_ != Mode::Title) {
        dx = seatX_;
        dy = seatY_;
    }
    int plowFace = 0;
    {
        float h = hx;
        if (h < 0.f) h += kTau;
        plowFace = int(h / (kTau / 8.f) + 0.5f) & 7;
    }
    place(art_.drive[mode_ == Mode::Title ? 0 : df], dx, dy, 11.f, PAL_DRIVE);
    place(art_.plow[plowFace], px, py, 15.f, PAL_PLOW);

    for (int i = 0; i < 14; i++) {
        if (flake_[i].life > 0.f) place(art_.spray, flake_[i].x, flake_[i].y, 2.0f + flake_[i].life, PAL_SPRAY);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 PLOW BOOM", PAL_HUD);
        hudC(6, "DELIVER THE DRIVE", 2);
        hudC(8, "SET IT ON THE BOOM", PAL_HUD);
        hudC(10, "THE STORM IS THE CLOCK", 3);
        hudC(18, "A GAS   B BRAKE   ARROWS STEER", PAL_HUD);
        hudC(21, "START", 2);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", 2);
        hudC(15, "START CONTINUES", PAL_HUD);
    } else {
        int sec = int(you_);
        int cs = int(you_ * 100.f) % 100;
        std::snprintf(line, sizeof line, "YOU %d:%02d.%02d", sec / 60, sec % 60, cs);
        hud(1, 1, line, PAL_HUD);
        int csec = int(storm_ - you_);
        if (csec < 0) csec = 0;
        std::snprintf(line, sizeof line, "STORM %d:%02d", csec / 60, csec % 60);
        hud(24, 1, line, 4);
        if (mode_ == Mode::Drive) {
            if (driveInside()) hudC(25, "ON THE BOOM  HOLD", 3);
            else hudC(25, "THE BOOM IS UP THE YARD", 2);
        } else if (mode_ == Mode::Win) {
            hudC(11, "DELIVERED", 3);
            hudC(13, "THE DRIVE IS ON THE BOOM", 2);
            if (!bot_) hudC(16, "START PLOWS AGAIN", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(11, why_, 4);
            if (!bot_) hudC(16, "START PLOWS AGAIN", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    float gas = 0.f;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Drive;
            zoom_ = 2.2f;
            blip(520.f);
        }
    } else if (mode_ == Mode::Drive) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(240.f);
        } else {
            float steer = 0.f;
            if (bot_) pilot(gas, steer);
            else controls(gas, steer);
            you_ += kDt;
            physics(kDt, gas, steer);
            const bool in = driveInside();
            if (in && std::fabs(speed_) < kStop) hold_ += kDt;
            else hold_ = 0.f;
            if (std::fabs(speed_) < kStop && !in && y_ > 100.f) still_ += kDt;
            else still_ = 0.f;
            if (hold_ >= kHoldNeed) {
                seated_ = true;
                const float c = std::cos(heading_);
                const float s = std::sin(heading_);
                seatX_ = x_ + kLead * s;
                seatY_ = y_ + kLead * c;
                seatH_ = heading_;
                mode_ = Mode::Win;
                won_ = true;
                over_ = true;
                fanStep_ = 0;
                blip(640.f);
            } else if (plowHitsBoom() || driveHitsHead()) {
                fail("broke the boom");
            } else if (still_ >= kStillNeed) {
                const float c = std::cos(heading_);
                const float s = std::sin(heading_);
                float nose = y_ + (kLead + kDL) * c + 0.f * s;
                if (nose < kDeckY0) fail("stopped short of the boom");
                else fail("the drive is not on the boom");
            } else if (you_ >= storm_) {
                fail("the storm took the leg");
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Drive;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        begin();
        mode_ = Mode::Drive;
        zoom_ = 2.2f;
        blip(520.f);
    }

    if (mode_ == Mode::Title) {
        camX_ = std::sin(t_ * 0.15f) * 4.f;
        camY_ = 72.f + std::sin(t_ * 0.1f) * 3.f;
        zoom_ = 1.25f;
    } else {
        float gx = x_ + std::sin(heading_) * 8.f;
        float gy = y_ + std::cos(heading_) * 8.f;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ = 2.15f;
    }
    audio(kDt, gas);
    draw();
}

}  // namespace plowboom
