#include "game/tower.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tower {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 168.f;
constexpr float kWorld = 1760.f;
constexpr float kSpawn = 64.f;
constexpr float kPouch0 = 200.f;
constexpr float kWatch = 72.f;
constexpr float kRun = 186.f;
constexpr float kDuckSp = 92.f;
constexpr float kAccel = 1500.f;
constexpr float kGrav = 1040.f;
constexpr float kJumpV = -470.f;
constexpr float kWellA = 690.f;
constexpr float kWellB = 808.f;
constexpr float kClock = 1120.f;
constexpr float kGoal = 1580.f;

float moveToward(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ && won_) return 3;
    if (over_) return 4;
    if (held_) return 2;
    return 1;
}

bool Game::inWell(float x) const { return x > kWellA + 10.f && x < kWellB - 10.f; }

bool Game::underHand() const { return std::abs(px_ - handX_) < 16.f && px_ > kClock - 90.f && px_ < kClock + 90.f; }

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
    watch_ = stepT_ = coyote_ = stun_ = inv_ = shake_ = 0;
    cam_ = 0;
    handX_ = kClock;
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    blip(420.f);
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
    shake_ = crossed ? 0.25f : 0.65f;
    if (crossed) {
        sys_->rumble(0.2f, 0.6f, 200);
        sys_->setLight(40, 140, 80);
        blip(720.f);
    } else {
        sys_->rumble(0.7f, 0.2f, 180);
        sys_->setLight(140, 30, 24);
        sys_->apu.noiseBurst(0.4f, 120.f, 0.28f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::bump() {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.65f;
    stun_ = 0.2f;
    shake_ = 0.7f;
    vx_ = px_ < handX_ ? -140.f : 140.f;
    face_ = vx_ < 0 ? 1 : -1;
    if (held_) {
        held_ = false;
        pouchX_ = std::clamp(px_ - (vx_ < 0 ? -18.f : 18.f), 40.f, kGoal - 30.f);
        if (inWell(pouchX_)) {
            finish(false, "THE POUCH FELL");
            return;
        }
    }
    sys_->apu.noiseBurst(0.3f, 360.f, 0.12f);
    sys_->rumble(0.45f, 0.2f, 70);
}

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (duck_) return art_.duck;
    if (std::abs(vx_) > 22.f) return (int(stepT_) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    left = right = jump = duck = false;
    if (stun_ > 0.f) return;
    if (!held_) {
        if (px_ < pouchX_ - 8.f) right = true;
        else if (px_ > pouchX_ + 8.f) left = true;
        return;
    }
    if (px_ > kClock - 78.f && px_ < kClock + 78.f) duck = onGround_;
    if (!onGround_) {
        right = true;
        return;
    }
    if (px_ < kWellB + 6.f) {
        right = true;
        if (px_ > kWellA - 46.f && px_ < kWellA - 8.f && vx_ > 120.f) jump = true;
        return;
    }
    right = px_ < kGoal + 6.f;
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    handX_ = kClock + std::sin(t_ * 2.4f) * 72.f;
    watch_ += DT;
    if (watch_ > kWatch) {
        finish(false, "THE WATCH IS OVER");
        return;
    }
    if (stun_ > 0.f) {
        stun_ -= DT;
        left = right = jump = duck = false;
        vx_ = moveToward(vx_, 0.f, 340.f * DT);
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
        if (left && right) target = 0.f;
        if (duck_) target = std::clamp(target, -kDuckSp, kDuckSp);
        float accel = onGround_ ? kAccel : kAccel * 0.5f;
        vx_ = moveToward(vx_, target, accel * DT);
    }

    if (jump && couldJump && !jumped_ && !duck_) {
        vy_ = kJumpV;
        onGround_ = false;
        jumped_ = true;
        coyote_ = 0.f;
        blip(360.f);
    }

    vy_ = std::min(640.f, vy_ + kGrav * DT);
    px_ += vx_ * DT;
    py_ += vy_ * DT;
    px_ = std::clamp(px_, 22.f, kWorld - 24.f);

    bool drop = inWell(px_);
    if (!drop && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else if (drop && py_ >= kFloor) {
        onGround_ = false;
        if (py_ > kFloor + 46.f) {
            finish(false, held_ ? "THE POUCH FELL" : "INTO THE WELL");
            return;
        }
    } else {
        onGround_ = false;
    }

    if (onGround_) {
        coyote_ = 0.1f;
        jumped_ = false;
        if (std::abs(vx_) > 20.f) stepT_ += DT * std::abs(vx_) / 26.f;
    } else if (!jumped_) {
        coyote_ = std::max(0.f, coyote_ - DT);
    }

    if (inv_ <= 0.f && !duck_ && underHand() && py_ > kFloor - 20.f) bump();
    if (mode_ != Mode::Play) return;

    if (!held_ && onGround_ && stun_ <= 0.f && std::abs(px_ - pouchX_) < 24.f && std::abs(py_ - kFloor) < 8.f) {
        held_ = true;
        blip(640.f);
        sys_->rumble(0.1f, 0.28f, 50);
    }

    if (onGround_ && px_ >= kGoal) {
        if (held_) finish(true, "CROSSED");
        else finish(false, "WITHOUT THE POUCH");
    }
}

const char* Game::hint() const {
    if (!held_ && px_ > pouchX_ + 36.f) return "GO BACK FOR THE POUCH";
    if (!held_) return "TAKE THE POUCH";
    if (px_ > kWellA - 110.f && px_ < kWellA) return "JUMP THE WELL";
    if (px_ > kClock - 120.f && px_ < kClock + 70.f) return "DUCK THE HAND";
    if (px_ > kGoal - 160.f) return "TO THE FAR DOOR";
    return "CARRY IT ACROSS";
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
    int hs = int(std::lround(-cam * 0.28f));
    int fg = int(std::lround(-cam));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int dusk = y < 36 ? 2 : (y < 88 ? 3 : 4);
        vdp.lineBackdrop[y] = gs::rgb4(dusk, dusk - 1, dusk + 3);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
        vdp.B.hscroll[y] = int16_t(hs);
        vdp.B.vscroll[y] = 0;
        vdp.A.hscroll[y] = int16_t(fg);
        vdp.A.vscroll[y] = 0;
    }
    vdp.HUD.scroll(0, 0);
}

void Game::drawWorld(float cam) {
    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true,
                     bool shadow = false) { spr(m, wx - cam, foot, h, pal, flip, feet, shadow); };
    int fi = int(t_ * 8.f) & 1;
    int bob = int(t_ * 3.f) & 1;

    world(art_.well, (kWellA + kWellB) * 0.5f, kFloor + 22.f, 26, PAL_VOID, false);
    world(art_.lip, kWellA, kFloor + 2.f, 18, PAL_ASHLAR, false);
    world(art_.lip, kWellB, kFloor + 2.f, 18, PAL_ASHLAR, true);

    const float merlons[] = {90.f, 240.f, 400.f, 540.f, 880.f, 1040.f, 1280.f, 1460.f};
    for (float x : merlons) world(art_.merlon, x, 118.f, 40, PAL_ASHLAR, false);
    world(art_.banner, 240.f, 78.f, 30, PAL_FLAG, false, false);
    world(art_.banner, 1280.f, 78.f, 30, PAL_FLAG, true, false);

    const float lamps[] = {140.f, 480.f, 960.f, 1400.f};
    for (float x : lamps) {
        world(art_.lamp, x, kFloor, 64, PAL_COPPER, false);
        world(art_.flame[fi], x, kFloor - 62.f, 12, PAL_LAMP, false, false);
    }

    world(art_.clock, kClock, 78.f, 44, PAL_COPPER, false, false);
    world(art_.hand, handX_, kFloor - 8.f, 36, PAL_COPPER, false);
    world(art_.stair, kPouch0, kFloor, 16, PAL_WALK, false);
    world(art_.door, kGoal + 40.f, kFloor, 78, PAL_DOOR, false);
    world(art_.flame[fi], kGoal + 40.f, kFloor - 84.f, 12.f, PAL_LAMP, false, false);

    bool show = inv_ <= 0.f || (int(inv_ * 16.f) & 1) == 0;
    if (show && mode_ != Mode::Title) {
        if (held_) world(art_.pouch[bob], px_ + face_ * 14.f, py_ - 30.f, 16, PAL_POUCH, face_ < 0, false);
        world(heroSprite(), px_, py_, duck_ ? 38.f : 54.f, PAL_WATCH, face_ < 0);
        if (py_ > kFloor - 28.f) world(art_.shadow, px_, kFloor + 2.f, 6, PAL_VOID, false, true, true);
    }
    if (!held_) world(art_.pouch[bob], pouchX_, kFloor - 14.f, 18, PAL_POUCH, false);
}

void Game::drawTitle() {
    int fi = int(t_ * 8.f) & 1;
    text("S3 TOWER POUC", 160, 16, 1.05f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160, 40, 0.85f, PAL_LAMP, 0);
    text("MISS THAT AND THE WATCH IS OVER", 160, 60, 0.7f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 86, 1.0f, PAL_GO, 0);
    spr(art_.merlon, 48, 168, 36, PAL_ASHLAR, false, true, false);
    spr(art_.stand, 100, kFloor, 54, PAL_WATCH, false, true, false);
    spr(art_.shadow, 100, kFloor + 2, 6, PAL_VOID, false, true, true);
    spr(art_.stair, 168, kFloor, 16, PAL_WALK, false, true, false);
    spr(art_.pouch[fi], 186, kFloor - 14, 18, PAL_POUCH, false, true, false);
    spr(art_.clock, 250, 120, 36, PAL_COPPER, false, false, false);
    spr(art_.door, 292, kFloor, 64, PAL_DOOR, false, true, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = std::clamp(px_ - 130.f, 0.f, kWorld - float(gs::SCREEN_W));
        if (shake_ > 0.f) cam += std::sin(t_ * 58.f) * shake_ * 3.f;
        cam_ = cam;
    }
    backdrop(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160, 14, 1.0f, PAL_HUD, 0);
        text("THE WATCH IS DONE", 160, 36, 0.85f, PAL_GO, 0);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 14, 1.0f, PAL_ALERT, 0);
        text(reason_, 160, 38, 0.85f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 22, 1.1f, PAL_HUD, 0);
    }
    drawWorld(cam);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_GO : PAL_ALERT);
        int left = std::max(0, int(std::ceil(kWatch - watch_)));
        char buf[16];
        std::snprintf(buf, sizeof buf, "%d", left);
        hud(36, 0, buf, left < 12 ? PAL_ALERT : PAL_HUD);
        hudC(1, hint(), PAL_LAMP);
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
        static const float good[] = {523.f, 659.f, 784.f, 1046.f};
        static const float bad[] = {294.f, 220.f, 165.f, 123.f};
        fanT_ += DT;
        if (fanT_ > 0.13f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.045f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0.f;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 88.f : 66.f, 0.015f);
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
            if (pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
            audio();
            draw();
            return;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(400.f);
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
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
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

    bool left = false, right = false, jump = false, duck = false;
    if (bot_) {
        bot(left, right, jump, duck);
    } else if (pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        blip(240.f);
        audio();
        draw();
        return;
    } else {
        left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
        right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO) || pad.pressed(gs::BTN_B);
        duck = pad.down(gs::BTN_DOWN) || pad.axisY <= -0.45f;
    }
    stepPlay(left, right, jump, duck);
    audio();
    draw();
}

}  // namespace tower
