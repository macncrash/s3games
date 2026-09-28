#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace mpouc {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GROUND = 168.f;
constexpr float SPEED = 80.f;
constexpr float JUMP = 180.f;
constexpr float GRAV = 560.f;
constexpr float LIMIT = 20.f;
constexpr float GAP_L = 136.f;
constexpr float GAP_R = 196.f;
constexpr float CH0 = 74.f;
constexpr float CH1 = 228.f;
constexpr float DOOR = 292.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    sys.apu.setMaster(0.4f);
}

void Game::begin() {
    mode_ = Mode::Carry;
    over_ = false;
    won_ = false;
    carry_ = true;
    reason_ = "";
    playT_ = 0;
    px_ = 28.f;
    py_ = GROUND;
    vy_ = 0;
    onGround_ = true;
    face_ = 1;
    fan_ = -1;
}

void Game::finish(bool win, const char* why) {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return;
    won_ = win;
    over_ = true;
    carry_ = win;
    reason_ = why;
    mode_ = win ? Mode::Victory : Mode::Fail;
    fan_ = 0;
    fanT_ = 0;
    if (!win) sys_->apu.noiseBurst(0.35f, 900.f, 0.25f);
}

float Game::gatePhase() const { return std::fmod(t_ + 40.f, 2.8f); }

bool Game::gateOpen() const { return gatePhase() < 1.62f; }

bool Game::chuteHits(float x0) const {
    if (px_ < x0 || px_ > x0 + 12.f) return false;
    return (GROUND - py_) < 12.f;
}

void Game::bot(bool& right, bool& left, bool& jump) {
    right = true;
    left = false;
    jump = false;
    if (px_ > CH0 - 28.f && px_ < CH0 - 6.f && onGround_) jump = true;
    if (px_ > GAP_L - 8.f && px_ < GAP_L) {
        float p = gatePhase();
        if (!(p < 0.55f)) right = false;
    }
    if (px_ > CH1 - 28.f && px_ < CH1 - 6.f && onGround_) jump = true;
}

void Game::update(float dt) {
    bool right = false, left = false, jump = false;
    if (bot_) bot(right, left, jump);
    else {
        const gs::Pad& pad = sys_->pad;
        right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.3f;
        left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.3f;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    }
    if (right && !left) {
        px_ += SPEED * dt;
        face_ = 1;
    } else if (left) {
        px_ -= SPEED * dt;
        face_ = -1;
    }
    px_ = std::clamp(px_, 16.f, 308.f);

    if (jump && onGround_) {
        vy_ = -JUMP;
        onGround_ = false;
        sys_->apu.tone(2, 420.f, 0.12f);
    } else if (!onGround_) {
        sys_->apu.tone(2, 0.f, 0.f);
    }

    vy_ += GRAV * dt;
    py_ += vy_ * dt;

    bool overGap = px_ > GAP_L && px_ < GAP_R;
    bool solid = !overGap || gateOpen();
    if (py_ >= GROUND && solid) {
        py_ = GROUND;
        vy_ = 0;
        onGround_ = true;
    } else if (py_ >= GROUND && overGap && !gateOpen()) {
        onGround_ = false;
        finish(false, "the pouch fell in the race");
        return;
    } else {
        onGround_ = false;
    }
    if (py_ > 230.f) {
        finish(false, "the pouch fell in the race");
        return;
    }

    if (chuteHits(CH0) || chuteHits(CH1)) {
        finish(false, "a sack knocked the pouch off");
        return;
    }

    if (playT_ >= LIMIT) {
        finish(false, "the clock died before the door");
        return;
    }
    if (px_ >= DOOR && onGround_ && carry_) finish(true, "the pouch crossed the mill");
}

int Game::marker() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (px_ >= GAP_L) return 2;
    return 1;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    sail_ += DT * 0.8f;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Carry) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            playT_ += DT;
            update(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Carry;
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        begin();
    }

    if (fan_ >= 0) {
        fanT_ += DT;
        if (fanT_ > 0.14f) {
            fanT_ = 0;
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            if (won_ && fan_ < 4) sys.apu.tone(3, notes[fan_], 0.15f);
            fan_++;
            if (fan_ > 6) {
                sys.apu.tone(3, 0, 0);
                fan_ = -1;
            }
        }
    }
    bool walking = mode_ == Mode::Carry && onGround_ && (bot_ || sys.pad.down(gs::BTN_LEFT) || sys.pad.down(gs::BTN_RIGHT));
    sys.apu.tone(0, walking ? 70.f : 0.f, walking ? 0.04f : 0.f);
    sys.apu.tone(1, 48.f + 3.f * std::sin(sail_ * 2.f), mode_ == Mode::Fail ? 0.f : 0.05f);
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 2.f || m.h <= 0) return;
    float s = h / float(m.h);
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.w = std::max(1, int(m.w * s));
    sp.h = std::max(1, int(h));
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(feet ? cy - sp.h : cy - sp.h * 0.5f));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float pen = x;
    float sc = std::max(1.f, scale);
    for (char ch : s) {
        unsigned c = unsigned(ch);
        if (c < 32 || c > 127) c = 32;
        int gi = int(c) - 32;
        gs::Image img = art_.glyph[gi];
        int w = std::max(1, art_.gw[gi]);
        gs::Sprite sp;
        sp.img = img;
        sp.w = int(w * sc);
        sp.h = int(art_.gh * sc);
        sp.x = int(pen);
        sp.y = int(y);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        pen += (w + 1) * sc;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.hudEnabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.road[y].on = false;
        vdp.lineFog[y] = 0;
        uint16_t c;
        if (y < 78) {
            int r = 6 + y / 16;
            int g = 8 + y / 20;
            int b = 11 - y / 30;
            c = gs::rgb4(r, g, b);
        } else if (y < 158) {
            c = gs::rgb4(5, 7, 3);
        } else {
            int bob = int(2 + std::sin(sail_ * 3.f + y * 0.2f));
            c = gs::rgb4(2, 5 + bob, 7);
        }
        vdp.lineBackdrop[y] = c;
    }

    if (mode_ == Mode::Title) {
        text("S3 MILL POUC", 104, 16, 1.f, PAL_HUD);
        text("CARRY THE POUCH ACROSS", 68, 32, 1.f, PAL_OK);
        text("ANYTHING ELSE IS A LOSS", 72, 46, 1.f, PAL_HUD);
        text("ARROWS WALK   A JUMP", 78, 188, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 140, 64, 1.f, PAL_POUCH);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 144, 16, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("THE POUCH CROSSED THE MILL", 48, 14, 1.f, PAL_OK);
    } else if (mode_ == Mode::Fail) {
        text(reason_, 36, 14, 1.f, PAL_ALERT);
    } else {
        text(playT_ > LIMIT - 5.f ? "DOOR IS CLOSING" : "KEEP THE POUCH", 96, 14, 1.f, playT_ > LIMIT - 5.f ? PAL_ALERT : PAL_HUD);
    }

    int secs = std::max(0, int(std::ceil(LIMIT - (mode_ == Mode::Title ? 0.f : playT_) - 0.001f)));
    char clock[24];
    std::snprintf(clock, sizeof clock, "CLOCK %02d", mode_ == Mode::Title ? int(LIMIT) : secs);
    text(clock, 12, 204, 1.f, secs <= 5 && mode_ == Mode::Carry ? PAL_ALERT : PAL_HUD);
    text(carry_ ? "POUCH HELD" : "POUCH LOST", 200, 204, 1.f, carry_ ? PAL_POUCH : PAL_ALERT);

    float feet = (mode_ == Mode::Title) ? GROUND : py_;
    float manX = (mode_ == Mode::Title) ? 48.f : px_;
    bool air = mode_ != Mode::Title && !onGround_;
    spr(air ? art_.millerJ : art_.miller, manX, feet, 32, PAL_MILLER, face_ < 0, true);
    if (carry_ || mode_ == Mode::Title) spr(art_.pouch, manX + (face_ < 0 ? -10.f : 10.f), feet - 16.f, 12, PAL_POUCH, false, false);

    auto chute = [&](float x) {
        spr(art_.sack, x + 6.f, GROUND, 22, PAL_WOOD, false, true);
    };
    chute(CH0);
    chute(CH1);

    bool open = gateOpen();
    if (open || mode_ == Mode::Title) {
        for (float x = GAP_L + 8.f; x < GAP_R; x += 16.f) spr(art_.plank, x, GROUND + 2.f, 8, PAL_WOOD, false, true);
    }
    spr(art_.plank, GAP_L - 10.f, GROUND + 2.f, 8, PAL_WOOD, false, true);
    spr(art_.plank, GAP_R + 6.f, GROUND + 2.f, 8, PAL_WOOD, false, true);

    spr(art_.door, 304.f, GROUND, 40, PAL_WOOD, false, true);

    spr(art_.mill, 168.f, 128.f, 86, PAL_STONE, false, true);
    float ang = sail_ * 2.4f;
    for (int i = 0; i < 4; i++) {
        float a = ang + i * 1.5708f;
        float sx = 168.f + std::sin(a) * 28.f;
        float sy = 62.f - std::cos(a) * 26.f;
        spr(art_.sail, sx, sy, 28, PAL_SAIL, i & 1, false);
    }
    spr(art_.hub, 168.f, 118.f, 16, PAL_WOOD, false, false);
    float wang = sail_ * 3.1f;
    for (int i = 0; i < 4; i++) {
        float a = wang + i * 1.5708f;
        float bx = 168.f + std::sin(a) * 18.f;
        float by = 146.f - std::cos(a) * 16.f;
        spr(art_.bucket, bx, by, 10, PAL_WATER, false, false);
    }
    for (int i = 0; i < 8; i++) {
        float x = 8.f + i * 18.f;
        if (x > 120.f && x < 210.f) continue;
        spr(art_.reed, x, 160.f, 14, PAL_WHEAT, i & 1, true);
    }
}

}  // namespace mpouc
