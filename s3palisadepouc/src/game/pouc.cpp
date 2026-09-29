#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace palisadepouc {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 178.f;
constexpr float kWorld = 860.f;
constexpr float kRun = 148.f;
constexpr float kAccel = 900.f;
constexpr float kGrav = 980.f;
constexpr float kJumpV = -330.f;
constexpr float kDitchA = 214.f;
constexpr float kDitchB = 278.f;
constexpr float kGateA = 430.f;
constexpr float kGateB = 500.f;
constexpr float kGoal = 720.f;
constexpr float kPouch0 = 118.f;

float toward(float v, float target, float maxStep) {
    if (v < target) return std::min(target, v + maxStep);
    return std::max(target, v - maxStep);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.5f);
    mode_ = Mode::Title;
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    held_ = false;
    onGround_ = true;
    face_ = 1;
    reason_ = "";
    px_ = 52.f;
    py_ = kFloor;
    vx_ = vy_ = 0.f;
    pouchX_ = kPouch0;
    t_ = stepT_ = coyote_ = stun_ = 0.f;
    sys_->setLight(40, 90, 40);
}

void Game::finish(bool crossed, const char* why) {
    if (mode_ != Mode::Play) return;
    won_ = crossed;
    over_ = true;
    reason_ = why;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    vx_ = vy_ = 0.f;
    if (crossed) {
        sys_->rumble(0.2f, 0.55f, 180);
        sys_->setLight(40, 160, 70);
        blip(720.f);
    } else {
        sys_->rumble(0.6f, 0.2f, 160);
        sys_->setLight(160, 30, 20);
        sys_->apu.noiseBurst(0.4f, 90.f, 0.22f);
    }
}

bool Game::barDown() const {
    float cyc = std::fmod(t_, 2.2f);
    return cyc < 0.85f;
}

bool Game::overDitch(float x) const { return x > kDitchA && x < kDitchB; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (held_) return 2;
    return 1;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = 0.08f;
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = right = jump = false;
    if (stun_ > 0.f) return;
    if (!held_) {
        if (px_ < pouchX_ - 8.f) right = true;
        else if (px_ > pouchX_ + 8.f) left = true;
        return;
    }
    if (!onGround_) {
        right = true;
        return;
    }
    if (px_ < kDitchA - 16.f) {
        right = true;
        if (px_ > kDitchA - 28.f) jump = true;
        return;
    }
    if (px_ < kGateA - 18.f) {
        if (barDown() && px_ > kGateA - 56.f) return;
        right = true;
        return;
    }
    if (px_ < kGoal + 6.f) right = true;
}

void Game::stepPlay(bool left, bool right, bool jump) {
    if (stun_ > 0.f) {
        stun_ -= DT;
        left = right = jump = false;
        vx_ = toward(vx_, 0.f, 420.f * DT);
    }
    bool couldJump = onGround_ || coyote_ > 0.f;
    if (stun_ <= 0.f) {
        if (right && !left) face_ = 1;
        else if (left && !right) face_ = -1;
        float target = 0.f;
        if (right) target += kRun;
        if (left) target -= kRun;
        float accel = onGround_ ? kAccel : kAccel * 0.55f;
        vx_ = toward(vx_, target, accel * DT);
    }
    bool jumped = false;
    if (jump && couldJump && stun_ <= 0.f) {
        vy_ = kJumpV;
        onGround_ = false;
        coyote_ = 0.f;
        jumped = true;
        blip(360.f);
    }
    vy_ = std::min(640.f, vy_ + kGrav * DT);
    px_ += vx_ * DT;
    py_ += vy_ * DT;
    px_ = std::clamp(px_, 18.f, kWorld - 20.f);

    if (!overDitch(px_) && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else if (overDitch(px_) && py_ >= kFloor) {
        onGround_ = false;
        if (py_ > kFloor + 36.f) {
            finish(false, held_ ? "THE POUCH FELL" : "INTO THE DITCH");
            return;
        }
    } else {
        onGround_ = false;
    }
    if (onGround_) {
        coyote_ = 0.1f;
        if (std::abs(vx_) > 18.f) stepT_ += DT * 8.f;
    } else if (!jumped) {
        coyote_ = std::max(0.f, coyote_ - DT);
    }

    bool inGate = px_ > kGateA - 6.f && px_ < kGateB;
    if (barDown() && onGround_ && inGate) {
        stun_ = 0.28f;
        vx_ = -160.f;
        face_ = 1;
        px_ = kGateA - 22.f;
        if (held_) {
            held_ = false;
            pouchX_ = kPouch0;
        }
        sys_->apu.noiseBurst(0.32f, 240.f, 0.1f);
        sys_->rumble(0.35f, 0.1f, 70);
        return;
    }

    if (!held_ && onGround_ && stun_ <= 0.f && std::abs(px_ - pouchX_) < 22.f && std::abs(py_ - kFloor) < 10.f) {
        held_ = true;
        blip(640.f);
        sys_->rumble(0.1f, 0.28f, 40);
    }
    if (!held_ && onGround_ && px_ >= kGoal - 8.f) {
        finish(false, "LEFT THE POUCH");
        return;
    }
    if (held_ && onGround_ && px_ >= kGoal) finish(true, "CROSSED");
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (m.h <= 0 || h < 1.f) return;
    float s = h / float(m.h);
    float w = float(m.w) * s;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.x = int(std::lround(cx - w * 0.5f));
    sp.y = int(std::lround(cy - h * 0.5f));
    sp.w = int16_t(std::max(1, int(std::lround(w))));
    sp.h = int16_t(std::max(1, int(std::lround(h))));
    if (sp.x > gs::SCREEN_W + 40 || sp.y > gs::SCREEN_H + 40 || sp.x + sp.w < -40 || sp.y + sp.h < -40) return;
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const char* s, float x, float y, int pal) {
    for (int i = 0; s[i]; i++) {
        unsigned char ch = (unsigned char)s[i];
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        if (ch < 32 || ch > 127) continue;
        if (ch != ' ') spr(art_.glyph[ch - 32], x + 2.f, y, 7.f, pal, false);
        x += 6.f;
    }
}

void Game::backdrop() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = 2 + y / 48;
        int g = 3 + y / 40;
        int b = 5 + (y < 90 ? 2 : 0);
        vdp.lineBackdrop[y] = gs::rgb4(std::min(r, 8), std::min(g, 9), std::min(b, 11));
        vdp.lineFog[y] = uint8_t(y < 30 ? 2 : 0);
        vdp.road[y].on = false;
    }
    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

void Game::draw() {
    sys_->vdp.clearSprites();
    backdrop();
    int blink = (sys_->frame / 24) & 1;

    if (mode_ == Mode::Title) {
        text("PALISADE", 118, 42, PAL_HUD);
        text("POUCH", 136, 56, PAL_POUCH);
        text("ONE WALL. CARRY IT ACROSS.", 64, 84, PAL_HUD);
        for (int i = 0; i < 8; i++) spr(art_.stake, 48.f + float(i) * 30.f, 150, 64, PAL_WOOD, false);
        spr(art_.pouch, 168, 168, 18, PAL_POUCH, false);
        if (blink) text("START", 142, 196, PAL_GO);
        text("ARROWS MOVE   A JUMP", 82, 210, PAL_HUD);
        return;
    }

    cam_ = std::clamp(px_ - 120.f, 0.f, kWorld - float(gs::SCREEN_W));
    auto foot = [&](const gs::Mipped& m, float wx, float fy, float h, int pal, bool flip) {
        spr(m, wx - cam_, fy - h * 0.5f, h, pal, flip);
    };

    for (float x = 0; x < kWorld; x += 32.f) {
        if (x + 16.f > kDitchA && x + 16.f < kDitchB) foot(art_.ditch, x + 16.f, kFloor + 16.f, 28, PAL_EARTH, false);
        else foot(art_.turf, x + 16.f, kFloor + 6.f, 18, PAL_EARTH, false);
    }
    for (float x = 300.f; x < 680.f; x += 22.f) {
        if (x > kGateA - 8.f && x < kGateB - 8.f) continue;
        foot(art_.stake, x, kFloor + 2.f, 78, PAL_WOOD, false);
    }
    foot(art_.post, kGateA - 10.f, kFloor + 2.f, 92, PAL_WOOD, false);
    foot(art_.post, kGateB + 4.f, kFloor + 2.f, 92, PAL_WOOD, false);
    float barY = barDown() ? kFloor - 36.f : kFloor - 92.f;
    foot(art_.bar, (kGateA + kGateB) * 0.5f, barY, 12, PAL_BAR, false);
    foot(art_.mark, kGoal + 16.f, kFloor + 2.f, 30, PAL_GO, false);

    const gs::Mipped& body = !onGround_ ? art_.leap : (std::abs(vx_) > 20.f ? ((int(stepT_) & 1) ? art_.runA : art_.runB) : art_.stand);
    foot(body, px_, py_, 48, PAL_RUNNER, face_ < 0);
    if (held_) foot(art_.pouch, px_ + face_ * 12.f, py_ - 30.f, 16, PAL_POUCH, face_ < 0);
    else foot(art_.pouch, pouchX_, kFloor - 8.f, 18, PAL_POUCH, false);

    text(held_ ? "CARRY" : "POUCH", 8, 12, held_ ? PAL_GO : PAL_POUCH);
    if (mode_ == Mode::Won) text("THE POUCH CROSSED", 78, 36, PAL_GO);
    else if (mode_ == Mode::Lost) text(reason_, 90, 36, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 136, 36, PAL_HUD);
    else if (barDown() && px_ > kGateA - 90.f && px_ < kGateA && held_) text("WAIT FOR THE BAR", 96, 28, PAL_ALERT);
    else if (!held_) text("TAKE THE POUCH", 104, 28, PAL_HUD);
    else text("ACROSS THE PALISADE", 88, 28, PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_Z)) begin();
        draw();
        return;
    }
    if ((mode_ == Mode::Won || mode_ == Mode::Lost) && !bot_ && sys.pad.pressed(gs::BTN_START)) {
        begin();
        draw();
        return;
    }
    if (mode_ == Mode::Play && !bot_ && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        draw();
        return;
    }
    if (mode_ == Mode::Play) {
        t_ += DT;
        bool left = false, right = false, jump = false;
        if (bot_) bot(left, right, jump);
        else {
            left = sys.pad.down(gs::BTN_LEFT);
            right = sys.pad.down(gs::BTN_RIGHT);
            jump = sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_Z) || sys.pad.pressed(gs::BTN_B);
        }
        stepPlay(left, right, jump);
    }
    draw();
}

}  // namespace palisadepouc
