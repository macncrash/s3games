#include "game/game.h"

#include "game/world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cranebox {
namespace {

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.B.resize(128, 32);
    sys.vdp.B.clear();
    for (int x = 0; x < 128; x++) {
        sys.vdp.B.set(x, 21, gs::entry(art_.curb, PAL_YARD));
        for (int y = 22; y < 28; y++) sys.vdp.B.set(x, y, gs::entry(art_.dirt, PAL_YARD));
    }
    sys.vdp.A.enabled = false;
    sys.vdp.HUD.clear();
    sys.apu.setMaster(0.5f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    x_ = START_X;
    v_ = 0;
    time_ = 0;
    hold_ = 0;
    t_ = 0;
    melody_ = -1;
}

void Game::begin() {
    x_ = START_X;
    v_ = 0;
    time_ = 0;
    hold_ = 0;
    melody_ = -1;
    over_ = false;
    won_ = false;
    mode_ = Mode::Run;
    why_ = "the leg ran out";
    blip(440.f);
}

bool Game::inside() const {
    return x_ - REAR >= BOX_L && x_ + NOSE <= BOX_R;
}

void Game::botPlan(float& gas, float& brake) const {
    float lo = BOX_L + REAR;
    float hi = BOX_R - NOSE;
    float target = (lo + hi) * 0.5f;
    float dist = target - x_;
    float vWant = clampf(std::sqrt(std::max(0.f, dist) * 90.f), 0.f, 96.f);
    gas = 0;
    brake = 0;
    if (dist < 1.5f) brake = 1.f;
    else if (v_ > vWant + 1.5f) brake = 1.f;
    else if (v_ < vWant - 4.f) gas = 1.f;
    else gas = 0.25f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "stopped inside the box";
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
    float gas = 0, brake = 0;
    const gs::Pad& p = sys_->pad;
    if (bot_) botPlan(gas, brake);
    else {
        if (p.down(gs::BTN_A) || p.down(gs::BTN_RIGHT) || p.accel > 0.2f) gas = 1.f;
        if (p.down(gs::BTN_B) || p.down(gs::BTN_LEFT) || p.down(gs::BTN_DOWN) || p.brake > 0.2f) brake = 1.f;
    }
    time_ += dt;
    float a = gas * GAS_A - brake * BRAKE_A - DRAG * v_;
    v_ = std::max(0.f, v_ + a * dt);
    if (v_ > V_MAX) v_ = V_MAX;
    if (brake > 0.5f && v_ < 4.f) v_ = 0;
    x_ += v_ * dt;

    if (x_ + NOSE > BOX_R + 1.f) {
        fail("missed the end");
        return;
    }
    if (v_ <= STOP_V && inside()) hold_ += dt;
    else hold_ = 0;
    if (hold_ >= HOLD_NEED) {
        win();
        return;
    }
    if (v_ <= 0.5f && !inside() && time_ > 0.45f) {
        if (x_ + NOSE < BOX_L) fail("short of the box");
        else fail("stopped outside the box");
        return;
    }
    if (time_ > LEG_LIMIT) fail("the leg ran out");
}

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.07f); }

void Game::chime(float dt) {
    if (melody_ < 0) return;
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
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
    float c = x_ - 108.f;
    if (c < 0.f) c = 0.f;
    if (c > 720.f) c = 720.f;
    return c;
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = y < 70 ? 0 : (y < 130 ? 1 : 2);
        uint16_t c = band == 0 ? gs::rgb4(5, 8, 13) : band == 1 ? gs::rgb4(8, 11, 15) : gs::rgb4(12, 13, 14);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    sky();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    float c = cam();
    auto wx = [&](float world) { return world - c; };

    spr(art_.shed, wx(180.f), GROUND - 28.f, 36.f, PAL_YARD);
    spr(art_.shed, wx(300.f), GROUND - 40.f, 48.f, PAL_YARD);
    spr(art_.lamp, wx(360.f), GROUND - 22.f, 28.f, PAL_YARD);
    spr(art_.lamp, wx(BOX_L - 16.f), GROUND - 22.f, 28.f, PAL_YARD);

    float span = BOX_R - BOX_L;
    for (float u = 8.f; u < span - 8.f; u += 16.f) spr(art_.stripe, wx(BOX_L + u), GROUND - 2.f, 8.f, PAL_BOX);
    spr(art_.post, wx(BOX_L), GROUND - 22.f, 40.f, PAL_BOX);
    spr(art_.post, wx(BOX_R), GROUND - 22.f, 40.f, PAL_BOX);

    float bob = (v_ > 2.f) ? std::sin(t_ * 18.f) * 0.6f : 0.f;
    spr(art_.crane, wx(x_), GROUND - 32.f + bob, 62.f, PAL_CRANE);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CRANEBOX", PAL_AMBER);
        hudC(6, "STOP INSIDE THE BOX", PAL_WHITE);
        hudC(8, "MISS THE END AND THE LEG FAILS", PAL_WHITE);
        hudC(12, "A GAS    B BRAKE", PAL_AMBER);
        hudC(16, "PRESS START", PAL_GREEN);
    } else if (mode_ == Mode::Run) {
        std::snprintf(buf, sizeof buf, "LEG %ds", int(std::ceil(std::max(0.f, LEG_LIMIT - time_))));
        hud(1, 1, buf, PAL_WHITE);
        if (inside() && v_ <= STOP_V) hudC(3, "HOLD", PAL_GREEN);
        else if (x_ + NOSE > BOX_L) hudC(3, "BOX", PAL_AMBER);
        else hudC(3, "YARD", PAL_WHITE);
    } else if (mode_ == Mode::Win) {
        hudC(4, "IN THE BOX", PAL_GREEN);
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

}  // namespace cranebox
