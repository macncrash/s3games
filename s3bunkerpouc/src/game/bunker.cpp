#include "game/bunker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bunker {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 186.f;
constexpr float kWorld = 1720.f;
constexpr float kSpawn = 72.f;
constexpr float kPouch0 = 214.f;
constexpr float kGoal = 1560.f;
constexpr float kRun = 188.f;
constexpr float kDuckSp = 96.f;
constexpr float kAccel = 980.f;
constexpr float kGrav = 1120.f;
constexpr float kJumpV = -430.f;

constexpr float kPipeA = 390.f;
constexpr float kPipeB = 530.f;
constexpr float kPipeHead = 150.f;

constexpr float kVentA = 690.f;
constexpr float kVentB = 810.f;
constexpr float kSteamPeriod = 2.15f;
constexpr float kSteamOn = 0.85f;

constexpr float kSumpA = 980.f;
constexpr float kSumpB = 1090.f;

constexpr float kShutX = 1304.f;
constexpr float kShutHalf = 22.f;
constexpr float kShutPeriod = 3.1f;
constexpr float kShutOpen = 1.55f;

float moveToward(float v, float target, float step) {
    if (v < target) return std::min(target, v + step);
    return std::max(target, v - step);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (held_) return 2;
    return 1;
}

void Game::dump(const char* where) const {
    std::printf("bunkerpouc %s px %.1f py %.1f vx %.1f held %d gnd %d duck %d t %.2f pouch %.1f shut %.2f\n", where,
                px_, py_, vx_, held_ ? 1 : 0, onGround_ ? 1 : 0, duck_ ? 1 : 0, t_, pouchX_, shut_);
    std::fflush(stdout);
}

bool Game::gap(float x) const { return x > kSumpA && x < kSumpB; }

bool Game::solid(float x) const { return x > 28.f && x < kWorld - 28.f && !gap(x); }

bool Game::steamOn() const {
    float p = std::fmod(t_ + 0.35f, kSteamPeriod);
    return p < kSteamOn;
}

bool Game::shutOpen() const { return std::fmod(t_, kShutPeriod) < kShutOpen; }

float Game::shutRemain() const {
    float p = std::fmod(t_, kShutPeriod);
    if (p < kShutOpen) return kShutOpen - p;
    return 0.f;
}

void Game::tickGates() {
    float target = shutOpen() ? 1.f : 0.f;
    shut_ += (target - shut_) * std::min(1.f, DT * 7.f);
}

void Game::begin() {
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    pouchX_ = kPouch0;
    held_ = false;
    onGround_ = true;
    duck_ = false;
    jumped_ = false;
    face_ = 1;
    fan_ = -1;
    reason_ = "";
    stepT_ = coyote_ = stun_ = inv_ = shake_ = beep_ = fanT_ = 0;
    shut_ = 0;
    cam_ = 0;
    t_ = 0;
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    tickGates();
    blip(480.f);
}

void Game::finish(bool crossed, const char* why) {
    if (mode_ != Mode::Play) return;
    won_ = crossed;
    over_ = true;
    reason_ = why;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    vx_ = 0;
    vy_ = 0;
    fan_ = 0;
    fanT_ = 0;
    shake_ = crossed ? 0.2f : 0.8f;
    if (crossed) {
        sys_->rumble(0.2f, 0.65f, 200);
        sys_->setLight(40, 160, 70);
        blip(740.f);
    } else {
        sys_->rumble(0.75f, 0.25f, 180);
        sys_->setLight(160, 28, 20);
        sys_->apu.noiseBurst(0.42f, 120.f, 0.26f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::knock() {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.7f;
    stun_ = 0.22f;
    shake_ = 0.9f;
    vx_ = -150.f;
    face_ = 1;
    if (held_) {
        held_ = false;
        float drop = px_ - 26.f;
        if (!solid(drop)) drop = px_ + 20.f;
        if (!solid(drop)) {
            finish(false, "THE POUCH FELL");
            return;
        }
        pouchX_ = drop;
    }
    sys_->apu.noiseBurst(0.38f, 420.f, 0.12f);
    sys_->rumble(0.45f, 0.15f, 80);
    sys_->setLight(160, 50, 24);
}

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (duck_) return art_.duck;
    if (std::abs(vx_) > 24.f) return (int(stepT_) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    left = right = jump = duck = false;
    if (stun_ > 0.f) return;
    if (!held_) {
        if (px_ < pouchX_ - 6.f) right = true;
        else if (px_ > pouchX_ + 6.f && solid(px_ - 8.f)) left = true;
        return;
    }
    if (!onGround_) {
        right = true;
        jump = true;
        return;
    }
    if (px_ < kPipeB + 12.f) {
        if (px_ > kPipeA - 28.f) duck = true;
        right = true;
        return;
    }
    if (px_ < kVentB + 18.f) {
        if (steamOn() && px_ > kVentA - 36.f) return;
        right = true;
        return;
    }
    if (px_ < kSumpB + 20.f) {
        right = true;
        if (px_ > kSumpA - 42.f && px_ < kSumpA - 6.f && vx_ > 120.f) jump = true;
        return;
    }
    if (px_ < kShutX + 48.f) {
        float stop = kShutX - kShutHalf - 28.f;
        if (px_ < stop - 4.f) {
            right = true;
            return;
        }
        if (shut_ > 0.82f && shutRemain() > 0.62f) {
            right = true;
            return;
        }
        if (px_ > stop + 6.f) left = true;
        return;
    }
    if (px_ < kGoal + 8.f) right = true;
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    if (stun_ > 0.f) {
        stun_ -= DT;
        left = right = jump = duck = false;
        vx_ = moveToward(vx_, 0.f, 260.f * DT);
    }
    if (inv_ > 0.f) inv_ -= DT;

    bool couldJump = onGround_ || coyote_ > 0.f;
    duck_ = duck && onGround_ && stun_ <= 0.f;
    if (stun_ <= 0.f) {
        if (right && !left) face_ = 1;
        else if (left && !right) face_ = -1;
        float target = 0.f;
        if (right) target += kRun;
        if (left) target -= kRun;
        if (duck_) target = std::clamp(target, -kDuckSp, kDuckSp);
        float accel = onGround_ ? kAccel : kAccel * 0.45f;
        vx_ = moveToward(vx_, target, accel * DT);
    }

    vy_ = std::min(620.f, vy_ + kGrav * DT);
    float prevX = px_;
    px_ += vx_ * DT;
    py_ += vy_ * DT;

    if (jump && couldJump && !jumped_ && stun_ <= 0.f) {
        vy_ = kJumpV;
        py_ += vy_ * DT;
        onGround_ = false;
        duck_ = false;
        jumped_ = true;
        coyote_ = 0.f;
        blip(390.f);
    }

    bool blocked = shut_ < 0.72f && px_ > kShutX - kShutHalf && px_ < kShutX + kShutHalf;
    if (blocked) {
        if (prevX <= kShutX) px_ = kShutX - kShutHalf - 1.f;
        else px_ = kShutX + kShutHalf + 1.f;
        vx_ = 0.f;
    }
    px_ = std::clamp(px_, 20.f, kWorld - 24.f);

    bool overGap = gap(px_);
    if (!overGap && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else if (overGap && py_ >= kFloor) {
        onGround_ = false;
        if (py_ > kFloor + 48.f) {
            finish(false, held_ ? "THE POUCH FELL" : "INTO THE SUMP");
            return;
        }
    } else {
        onGround_ = false;
    }

    if (onGround_) {
        coyote_ = 0.09f;
        jumped_ = false;
        if (std::abs(vx_) > 20.f) stepT_ += DT * std::abs(vx_) / 28.f;
    } else if (!jumped_) {
        coyote_ = std::max(0.f, coyote_ - DT);
    }

    float head = py_ - bodyH();
    if (px_ > kPipeA && px_ < kPipeB && head < kPipeHead) knock();
    if (mode_ != Mode::Play) return;

    if (steamOn() && onGround_ && px_ > kVentA && px_ < kVentB) knock();
    if (mode_ != Mode::Play) return;

    if (!held_ && onGround_ && stun_ <= 0.f && std::abs(px_ - pouchX_) < 26.f && std::abs(py_ - kFloor) < 8.f) {
        held_ = true;
        blip(680.f);
        sys_->rumble(0.12f, 0.3f, 50);
    }

    if (!held_ && onGround_ && px_ >= kGoal) {
        finish(false, "LEFT THE POUCH");
        return;
    }
    if (held_ && onGround_ && px_ >= kGoal) finish(true, "CROSSED");
}

const char* Game::hint() const {
    if (!held_ && px_ > pouchX_ + 40.f) return "GO BACK FOR THE POUCH";
    if (!held_) return "TAKE THE POUCH";
    if (px_ > kPipeA - 70.f && px_ < kPipeB) return "DUCK THE PIPE";
    if (px_ > kVentA - 80.f && px_ < kVentB && steamOn()) return "WAIT OUT THE STEAM";
    if (px_ > kSumpA - 90.f && px_ < kSumpA) return "JUMP THE SUMP";
    if (px_ > kShutX - 120.f && px_ < kShutX && shut_ < 0.8f) return "WAIT FOR THE SHUTTER";
    if (px_ > kGoal - 140.f) return "THROUGH THE BLAST DOOR";
    return "CARRY IT ACROSS";
}

int Game::hintPal() const {
    const char* h = hint();
    if (h[0] == 'T' && h[1] == 'H') return PAL_GO;
    if (h[0] == 'C' || h[0] == 'T' || h[0] == 'G') return PAL_HUD;
    return PAL_ALERT;
}

void Game::hud(int col, int row, const char* s, int pal) {
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        int x = col + i;
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    float width = 0.f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float gh = g.h * scale;
        spr(g, x + g.w * scale * 0.5f, y + gh * 0.5f, gh, pal, false, false, false);
        x += g.w * scale;
    }
}

void Game::backdrop(float cam) {
    gs::VDP& vdp = sys_->vdp;
    (void)cam;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int shade = 1 + (y < 48 ? 0 : (y < 150 ? 1 : 2));
        vdp.lineBackdrop[y] = gs::rgb4(shade, shade + 1, shade);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
        vdp.A.hscroll[y] = 0;
        vdp.B.hscroll[y] = 0;
    }
    vdp.HUD.scroll(0, 0);
}

void Game::drawWorld(float cam) {
    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true,
                     bool shadow = false) { spr(m, wx - cam, foot, h, pal, flip, feet, shadow); };
    int bob = int(t_ * 5.f) & 1;
    int puff = int(t_ * 10.f) & 1;

    for (float x = 40.f; x < kWorld; x += 58.f) world(art_.floor, x, kFloor + 14.f, 16, PAL_CONC, false);
    const float ribs[] = {120.f, 300.f, 620.f, 900.f, 1180.f, 1460.f};
    for (float x : ribs) world(art_.rib, x, kFloor, 120, PAL_CONC, false);
    const float lamps[] = {160.f, 460.f, 760.f, 1140.f, 1500.f};
    for (float x : lamps) world(art_.lamp, x, 78.f, 36, PAL_LAMP, false, false);

    for (float x = kPipeA + 16.f; x < kPipeB; x += 36.f) world(art_.pipe, x, kPipeHead + 6.f, 18, PAL_STEEL, false, false);

    if (steamOn()) {
        world(art_.puff[puff], (kVentA + kVentB) * 0.5f - 16.f, kFloor - 8.f, 36, PAL_STEAM, false);
        world(art_.puff[1 - puff], (kVentA + kVentB) * 0.5f + 16.f, kFloor - 18.f, 28, PAL_STEAM, false);
    }
    world(art_.grate, (kVentA + kVentB) * 0.5f, kFloor + 2.f, 12, PAL_STEEL, false);

    world(art_.sump, (kSumpA + kSumpB) * 0.5f, kFloor + 28.f, 40, PAL_SUMP, false);

    float slabBottom = 28.f + (1.f - shut_) * (kFloor - 28.f);
    world(art_.slab, kShutX, slabBottom, std::max(16.f, slabBottom - 16.f), PAL_STEEL, false);

    world(art_.door, kGoal + 46.f, kFloor, 108, PAL_STEEL, false);
    world(art_.crate, kPouch0, kFloor, 30, PAL_CONC, false);

    bool show = inv_ <= 0.f || (int(inv_ * 16.f) & 1) == 0;
    if (show && mode_ != Mode::Title) {
        if (held_) {
            float bobY = std::sin(stepT_ * 0.7f) * 1.4f;
            world(art_.pouch[bob], px_ + face_ * 16.f, py_ - 28.f + bobY, 20, PAL_POUCH, face_ < 0, false);
        }
        world(heroSprite(), px_, py_, duck_ ? 40.f : 54.f, PAL_PLAYER, face_ < 0);
        if (py_ > kFloor - 30.f) world(art_.shadow, px_, kFloor + 2.f, 8, PAL_CONC, false, true, true);
    }
    if (!held_) world(art_.pouch[bob], pouchX_, kFloor - 28.f, 22, PAL_POUCH, false);
}

void Game::drawTitle() {
    int bob = int(t_ * 5.f) & 1;
    text("S3 BUNKER POUC", 160, 18, 1.1f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160, 46, 1.0f, PAL_LAMP, 0);
    text("ANYTHING ELSE IS A LOSS", 160, 68, 1.0f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 96, 1.0f, PAL_GO, 0);
    spr(art_.rib, 48, kFloor, 100, PAL_CONC, false, true, false);
    spr(art_.stand, 96, kFloor, 54, PAL_PLAYER, false, true, false);
    spr(art_.crate, 168, kFloor, 30, PAL_CONC, false, true, false);
    spr(art_.pouch[bob], 168, kFloor - 28, 22, PAL_POUCH, false, true, false);
    spr(art_.pipe, 230, 132, 16, PAL_STEEL, false, false, false);
    spr(art_.door, 276, kFloor, 90, PAL_STEEL, false, true, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = cam_;
        if (shake_ > 0.f) cam += std::sin(t_ * 64.f) * shake_ * 2.2f;
    }
    backdrop(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160, 16, 1.05f, PAL_HUD, 0);
        text("THE BUNKER", 160, 40, 1.05f, PAL_GO, 0);
    } else if (mode_ == Mode::Lost) {
        text("A LOSS", 160, 16, 1.15f, PAL_ALERT, 0);
        text(reason_, 160, 42, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 24, 1.15f, PAL_HUD, 0);
    }
    drawWorld(cam);
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_GO : PAL_ALERT);
        hudC(1, hint(), hintPal());
        hudC(26, "ARROWS  Z JUMP  DOWN DUCK", PAL_HUD);
    } else if (!bot_) {
        hudC(26, "START", PAL_HUD);
    }
}

void Game::audio() {
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (fan_ >= 0) {
        static const float good[] = {392.f, 523.f, 659.f, 784.f};
        static const float bad[] = {246.f, 196.f, 146.f, 110.f};
        fanT_ += DT;
        if (fanT_ > 0.13f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.05f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0.f;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 92.f : 68.f, 0.016f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 74.f, 0.01f);
    else sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.66f);
    t_ = 0.f;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    cam_ = 0.f;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        t_ += DT;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        if (mode_ == Mode::Title) {
            if (pad.pressed(gs::BTN_MODE)) sys.quit();
            audio();
            draw();
            return;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(420.f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
        audio();
        draw();
        return;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        t_ += DT;
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
        audio();
        draw();
        return;
    }

    t_ += DT;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.2f);
    tickGates();

    bool left = false, right = false, jump = false, duck = false;
    if (bot_) {
        bot(left, right, jump, duck);
    } else if (pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        blip(260.f);
    } else {
        left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
        right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_TURBO);
        duck = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
    }
    if (mode_ == Mode::Play) stepPlay(left, right, jump, duck);

    float want = std::clamp(px_ + vx_ * 0.1f - 130.f, 0.f, kWorld - float(gs::SCREEN_W));
    cam_ += (want - cam_) * std::min(1.f, DT * 6.5f);

    if (mode_ == Mode::Won) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Lost) sys.setLight(160, 28, 20);
    else if (steamOn() && px_ > kVentA - 40.f && px_ < kVentB + 20.f) sys.setLight(180, 170, 140);
    else if (held_) sys.setLight(160, 110, 40);
    else sys.setLight(50, 80, 70);

    audio();
    draw();
}

}  // namespace bunker
