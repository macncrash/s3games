#include "game/game.h"

#include "game/world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace craneboom {
namespace {

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::driveX() const { return x_ + JIB; }

bool Game::overBoom() const {
    float d = driveX();
    return d >= BOOM_L && d <= END_R;
}

bool Game::overEnd() const {
    float d = driveX();
    return d >= END_L + END_IN && d <= END_R - END_IN;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.B.resize(128, 32);
    sys.vdp.B.clear();
    for (int x = 0; x < 128; x++) {
        sys.vdp.B.set(x, 22, gs::entry(art_.curb, PAL_YARD));
        for (int y = 23; y < 28; y++) sys.vdp.B.set(x, y, gs::entry(art_.dirt, PAL_YARD));
    }
    sys.vdp.A.enabled = false;
    sys.vdp.HUD.clear();
    sys.apu.setMaster(0.5f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    x_ = START_X;
    v_ = 0;
    cable_ = CABLE_MIN;
    time_ = 0;
    hold_ = 0;
    t_ = 0;
    melody_ = -1;
}

void Game::begin() {
    x_ = START_X;
    v_ = 0;
    cable_ = CABLE_MIN;
    time_ = 0;
    hold_ = 0;
    melody_ = -1;
    over_ = false;
    won_ = false;
    mode_ = Mode::Run;
    why_ = "the leg ran out";
    blip(392.f);
}

void Game::botPlan(float& gas, float& brake, float& hoist, float& raise) const {
    float target = endMid() - JIB;
    float dist = target - x_;
    float vWant = clampf(std::sqrt(std::max(0.f, dist) * 78.f), 0.f, 86.f);
    gas = 0;
    brake = 0;
    hoist = 0;
    raise = 0;
    bool lined = std::fabs(dist) < 2.2f && v_ < 3.f;
    if (lined) {
        brake = 1.f;
        if (cable_ < restCable() - 0.4f) hoist = 1.f;
        return;
    }
    if (dist < 1.2f) brake = 1.f;
    else if (v_ > vWant + 1.4f) brake = 1.f;
    else if (v_ < vWant - 4.f) gas = 1.f;
    else gas = 0.22f;
    if (cable_ > CABLE_MIN + 1.f) raise = 1.f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "drive set on the end of the boom";
    v_ = 0;
    melody_ = 0;
    melodyT_ = 0;
    sys_->rumble(0.16f, 0.05f, 90);
    sys_->setLight(40, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    sys_->rumble(0.45f, 0.2f, 140);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.35f, 420.f, 0.22f);
}

void Game::update(float dt) {
    float gas = 0, brake = 0, hoist = 0, raise = 0;
    const gs::Pad& p = sys_->pad;
    if (bot_) botPlan(gas, brake, hoist, raise);
    else {
        if (p.down(gs::BTN_A) || p.down(gs::BTN_RIGHT) || p.accel > 0.2f) gas = 1.f;
        if (p.down(gs::BTN_B) || p.down(gs::BTN_LEFT) || p.brake > 0.2f) brake = 1.f;
        if (p.down(gs::BTN_DOWN)) hoist = 1.f;
        if (p.down(gs::BTN_UP)) raise = 1.f;
    }
    time_ += dt;
    float a = gas * GAS_A - brake * BRAKE_A - DRAG * v_;
    v_ = std::max(0.f, v_ + a * dt);
    if (v_ > V_MAX) v_ = V_MAX;
    if (brake > 0.5f && v_ < 4.f) v_ = 0;
    x_ += v_ * dt;
    if (hoist > 0.f && raise <= 0.f) cable_ += HOIST * dt;
    if (raise > 0.f && hoist <= 0.f) cable_ -= HOIST * dt;
    cable_ = clampf(cable_, CABLE_MIN, CABLE_MAX);

    float bottom = TIP_Y + cable_ + DRIVE_H * 0.5f;
    if (driveX() > PAST) {
        fail("missed the end");
        return;
    }
    if (bottom >= BOOM_TOP - 0.6f) {
        if (overEnd() && v_ <= STOP_V) {
            cable_ = restCable();
            hold_ += dt;
        } else if (overEnd()) {
            cable_ = restCable() - 1.f;
            hold_ = 0;
            if (v_ > 36.f) {
                fail("missed the end");
                return;
            }
        } else if (overBoom()) {
            fail("missed the end");
            return;
        } else if (cable_ >= CABLE_MAX - 0.5f) {
            fail(driveX() < BOOM_L ? "short of the boom" : "missed the end");
            return;
        }
    } else {
        hold_ = 0;
        if (cable_ >= CABLE_MAX - 0.5f && !overBoom()) {
            fail(driveX() < BOOM_L ? "short of the boom" : "missed the end");
            return;
        }
    }
    if (hold_ >= HOLD_NEED) {
        win();
        return;
    }
    if (time_ > LEG_LIMIT) fail("the leg ran out");
}

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.07f); }

void Game::chime(float dt) {
    if (melody_ < 0) return;
    static const float notes[] = {392.f, 494.f, 587.f, 784.f};
    melodyT_ -= dt;
    if (melodyT_ > 0.f) return;
    if (melody_ >= 4) {
        sys_->apu.tone(0, 0, 0);
        melody_ = -1;
        return;
    }
    sys_->apu.tone(0, notes[melody_], 0.08f);
    melody_++;
    melodyT_ = 0.16f;
}

float Game::cam() const {
    float c = x_ - 70.f;
    if (c < 0.f) c = 0.f;
    if (c > 520.f) c = 520.f;
    return c;
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = y < 64 ? 0 : (y < 120 ? 1 : 2);
        uint16_t c = band == 0 ? gs::rgb4(4, 7, 12) : band == 1 ? gs::rgb4(7, 10, 14) : gs::rgb4(11, 12, 13);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.B.hscroll[y] = int16_t(std::lround(cam()));
        v.B.vscroll[y] = 0;
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    sky();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    float c = cam();
    auto wx = [&](float world) { return world - c; };

    spr(art_.shed, wx(150.f), GROUND - 28.f, 40.f, PAL_YARD);
    spr(art_.shed, wx(280.f), GROUND - 34.f, 48.f, PAL_YARD);

    spr(art_.pier, wx(BOOM_L + 8.f), GROUND - 28.f, 56.f, PAL_BOOM);
    spr(art_.pier, wx((BOOM_L + END_L) * 0.5f), GROUND - 28.f, 56.f, PAL_BOOM);
    spr(art_.pier, wx(END_R - 8.f), GROUND - 28.f, 56.f, PAL_BOOM);
    for (float u = BOOM_L; u < END_L - 4.f; u += 22.f)
        spr(art_.plank, wx(u + 11.f), BOOM_TOP + 2.f, 12.f, PAL_BOOM);
    for (float u = END_L; u < END_R; u += 22.f)
        spr(art_.saddle, wx(u + 11.f), BOOM_TOP + 1.f, 14.f, PAL_BOOM);

    float tip = driveX();
    spr(art_.boom, wx(x_ + JIB * 0.48f), TIP_Y + 6.f, 18.f, PAL_CRANE);
    float bob = (v_ > 2.f) ? std::sin(t_ * 16.f) * 0.5f : 0.f;
    spr(art_.cab, wx(x_), GROUND - 26.f + bob, 52.f, PAL_CRANE);

    float hookY = TIP_Y + cable_;
    for (float y = TIP_Y + 4.f; y < hookY - 4.f; y += 6.f) spr(art_.link, wx(tip), y, 8.f, PAL_DRIVE);
    spr(art_.hook, wx(tip), hookY - 2.f, 10.f, PAL_DRIVE);
    spr(art_.drive, wx(tip), hookY + DRIVE_H * 0.35f, DRIVE_H, PAL_DRIVE);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CRANE BOOM", PAL_AMBER);
        hudC(6, "SET THE DRIVE ON THE BOOM", PAL_WHITE);
        hudC(8, "MISS THE END AND THE LEG FAILS", PAL_WHITE);
        hudC(12, "A GAS   B BRAKE   DOWN HOIST", PAL_AMBER);
        hudC(16, "PRESS START", PAL_GREEN);
    } else if (mode_ == Mode::Run) {
        std::snprintf(buf, sizeof buf, "LEG %ds", int(std::ceil(std::max(0.f, LEG_LIMIT - time_))));
        hud(1, 1, buf, PAL_WHITE);
        if (hold_ > 0.f) hudC(3, "HOLD", PAL_GREEN);
        else if (overEnd()) hudC(3, "END", PAL_AMBER);
        else if (overBoom()) hudC(3, "BOOM", PAL_WHITE);
        else hudC(3, "YARD", PAL_WHITE);
    } else if (mode_ == Mode::Win) {
        hudC(4, "DRIVE ON THE BOOM", PAL_GREEN);
        hudC(8, "THE LEG IS MADE", PAL_WHITE);
        hudC(16, "START TO RUN AGAIN", PAL_AMBER);
    } else {
        hudC(4, "LEG FAILED", PAL_RED);
        hudC(7, why_, PAL_WHITE);
        hudC(16, "START TO TRY AGAIN", PAL_AMBER);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C);
    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.3f) begin();
        else if (start) begin();
    } else if (mode_ == Mode::Run) {
        update(DT);
    } else {
        chime(DT);
        if (!bot_ && start) begin();
    }
    draw();
}

}  // namespace craneboom
