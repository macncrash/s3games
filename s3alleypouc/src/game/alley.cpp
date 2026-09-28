#include "game/alley.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace alley {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 176.f;
constexpr float kWorld = 1680.f;
constexpr float kSpawn = 70.f;
constexpr float kPouch0 = 210.f;
constexpr float kWatch = 70.f;
constexpr float kRun = 148.f;
constexpr float kDuckSp = 78.f;
constexpr float kAccel = 1400.f;
constexpr float kGrav = 920.f;
constexpr float kJumpV = -430.f;
constexpr float kLineX = 780.f;
constexpr float kLineY = 146.f;
constexpr float kGoal = 1488.f;

struct Span {
    float a, b;
};
constexpr Span kDrains[] = {{460.f, 548.f}, {1040.f, 1132.f}};

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

bool Game::drain(float x) const {
    for (const Span& p : kDrains)
        if (x > p.a + 8.f && x < p.b - 8.f) return true;
    return false;
}

void Game::dump(const char* where) const {
    std::fprintf(stderr, "alleypouc %s px %.1f py %.1f vx %.1f held %d ground %d duck %d watch %.2f pouch %.1f %s\n",
                 where, px_, py_, vx_, held_ ? 1 : 0, onGround_ ? 1 : 0, duck_ ? 1 : 0, watch_, pouchX_, reason_);
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
    watch_ = stepT_ = coyote_ = stun_ = inv_ = shake_ = 0;
    cam_ = 0;
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
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
    shake_ = crossed ? 0.3f : 0.7f;
    if (crossed) {
        sys_->rumble(0.2f, 0.65f, 200);
        sys_->setLight(40, 160, 70);
        blip(740.f);
    } else {
        sys_->rumble(0.75f, 0.25f, 180);
        sys_->setLight(160, 28, 22);
        sys_->apu.noiseBurst(0.4f, 140.f, 0.28f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::bump(float fromX) {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.7f;
    stun_ = 0.22f;
    shake_ = 0.8f;
    float away = px_ <= fromX ? -1.f : 1.f;
    vx_ = away * 150.f;
    face_ = away < 0 ? 1 : -1;
    if (held_) {
        held_ = false;
        float drop = px_ + away * 22.f;
        if (drain(drop)) drop = px_ - away * 18.f;
        pouchX_ = std::clamp(drop, 40.f, kGoal - 40.f);
        if (drain(pouchX_)) {
            finish(false, "THE POUCH FELL");
            return;
        }
    }
    sys_->apu.noiseBurst(0.32f, 480.f, 0.12f);
    sys_->rumble(0.5f, 0.2f, 80);
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
        if (px_ < pouchX_ - 6.f) right = true;
        else if (px_ > pouchX_ + 6.f && !drain(px_ - 10.f)) left = true;
        return;
    }
    if (!onGround_) {
        right = true;
        return;
    }
    for (const Span& p : kDrains) {
        if (px_ >= p.b + 8.f) continue;
        right = true;
        if (px_ > p.a - 36.f && px_ < p.a - 6.f && vx_ > 100.f) jump = true;
        return;
    }
    if (px_ < kLineX + 28.f) {
        if (px_ > kLineX - 46.f) duck = true;
        right = true;
        return;
    }
    if (px_ < kGoal + 8.f) right = true;
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    watch_ += DT;
    if (watch_ > kWatch) {
        finish(false, "THE WATCH IS OVER");
        return;
    }
    if (stun_ > 0.f) {
        stun_ -= DT;
        left = right = jump = duck = false;
        vx_ = moveToward(vx_, 0.f, 320.f * DT);
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
        float accel = onGround_ ? kAccel : kAccel * 0.45f;
        vx_ = moveToward(vx_, target, accel * DT);
    }

    if (jump && couldJump && !jumped_ && !duck_) {
        vy_ = kJumpV;
        onGround_ = false;
        jumped_ = true;
        coyote_ = 0.f;
        blip(390.f);
    }

    vy_ = std::min(620.f, vy_ + kGrav * DT);
    px_ += vx_ * DT;
    py_ += vy_ * DT;
    px_ = std::clamp(px_, 24.f, kWorld - 28.f);

    bool drop = drain(px_);
    if (!drop && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else if (drop && py_ >= kFloor) {
        onGround_ = false;
        if (py_ > kFloor + 48.f) {
            finish(false, held_ ? "THE POUCH FELL" : "IN THE DRAIN");
            return;
        }
    } else {
        onGround_ = false;
    }

    if (onGround_) {
        coyote_ = 0.09f;
        jumped_ = false;
        if (std::abs(vx_) > 20.f) stepT_ += DT * std::abs(vx_) / 24.f;
    } else if (!jumped_) {
        coyote_ = std::max(0.f, coyote_ - DT);
    }

    float head = py_ - bodyH();
    if (inv_ <= 0.f && std::abs(px_ - kLineX) < 14.f && head < kLineY) bump(kLineX);
    if (mode_ != Mode::Play) return;

    if (!held_ && onGround_ && stun_ <= 0.f && std::abs(px_ - pouchX_) < 26.f && std::abs(py_ - kFloor) < 10.f) {
        held_ = true;
        blip(680.f);
        sys_->rumble(0.12f, 0.3f, 50);
    }

    if (onGround_ && px_ >= kGoal) {
        if (held_) finish(true, "CROSSED");
        else finish(false, "WITHOUT THE POUCH");
    }
}

const char* Game::hint() const {
    if (!held_ && px_ > pouchX_ + 40.f) return "GO BACK FOR THE POUCH";
    if (!held_) return "TAKE THE POUCH";
    for (const Span& p : kDrains)
        if (px_ > p.a - 90.f && px_ < p.a) return "JUMP THE DRAIN";
    if (px_ > kLineX - 90.f && px_ < kLineX + 10.f) return "DUCK THE LINE";
    if (px_ > kGoal - 140.f) return "TO THE FAR DOOR";
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
    int hs = int(std::lround(-cam * 0.35f));
    int fg = int(std::lround(-cam));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 40 ? 1 : (y < 90 ? 2 : 3);
        vdp.lineBackdrop[y] = gs::rgb4(sky, sky, sky + 2);
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
    int bob = int(t_ * 4.f) & 1;

    for (const Span& p : kDrains) {
        world(art_.grate, (p.a + p.b) * 0.5f, kFloor + 36.f, 30, PAL_DRAIN, false);
        world(art_.lip, p.a, kFloor + 2.f, 16, PAL_IRON, false);
        world(art_.lip, p.b, kFloor + 2.f, 16, PAL_IRON, true);
    }

    world(art_.peg, kLineX - 70.f, kFloor, 150, PAL_IRON, false);
    world(art_.peg, kLineX + 70.f, kFloor, 150, PAL_IRON, false);
    world(art_.line, kLineX, kLineY, 8, PAL_ALERT, false, false);

    const float lamps[] = {120.f, 340.f, 620.f, 900.f, 1220.f, 1420.f};
    for (float x : lamps) {
        world(art_.lamp, x, kFloor, 78, PAL_IRON, false);
        world(art_.flame[fi], x, kFloor - 74.f, 12, PAL_LAMP, false, false);
    }
    const float bins[] = {280.f, 700.f, 960.f, 1280.f};
    for (float x : bins) world(art_.bin, x, kFloor, 32, PAL_WOOD, false);
    world(art_.stair, kPouch0, kFloor, 18, PAL_STONE, false);

    world(art_.door, kGoal + 36.f, kFloor, 86, PAL_DOOR, false);
    float glow = 12.f + (held_ ? 2.f * std::sin(t_ * 7.f) : 0.f);
    world(art_.flame[fi], kGoal + 36.f, kFloor - 92.f, glow, PAL_LAMP, false, false);

    bool show = inv_ <= 0.f || (int(inv_ * 16.f) & 1) == 0;
    if (show && mode_ != Mode::Title) {
        if (held_) world(art_.pouch[bob], px_ + face_ * 12.f, py_ - 28.f, 18, PAL_POUCH, face_ < 0, false);
        world(heroSprite(), px_, py_, duck_ ? 40.f : 56.f, PAL_PLAYER, face_ < 0);
        if (py_ > kFloor - 30.f) world(art_.shadow, px_, kFloor + 2.f, 7, PAL_DRAIN, false, true, true);
    }
    if (!held_) world(art_.pouch[bob], pouchX_, kFloor - 16.f, 20, PAL_POUCH, false);
}

void Game::drawTitle() {
    int fi = int(t_ * 8.f) & 1;
    text("S3 ALLEY POUC", 160, 18, 1.1f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160, 44, 0.9f, PAL_LAMP, 0);
    text("MISS THAT AND THE WATCH IS OVER", 160, 66, 0.75f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 92, 1.0f, PAL_GO, 0);
    spr(art_.stand, 78, kFloor, 56, PAL_PLAYER, false, true, false);
    spr(art_.shadow, 78, kFloor + 2, 7, PAL_DRAIN, false, true, true);
    spr(art_.stair, 150, kFloor, 18, PAL_STONE, false, true, false);
    spr(art_.pouch[fi], 168, kFloor - 16, 20, PAL_POUCH, false, true, false);
    spr(art_.bin, 220, kFloor, 32, PAL_WOOD, false, true, false);
    spr(art_.lamp, 40, kFloor, 70, PAL_IRON, false, true, false);
    spr(art_.flame[fi], 40, kFloor - 66, 11, PAL_LAMP, false, false, false);
    spr(art_.door, 280, kFloor, 72, PAL_DOOR, false, true, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = std::clamp(px_ - 140.f, 0.f, kWorld - float(gs::SCREEN_W));
        if (shake_ > 0.f) cam += std::sin(t_ * 60.f) * shake_ * 3.f;
        cam_ = cam;
    }
    backdrop(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160, 16, 1.0f, PAL_HUD, 0);
        text("THE WATCH IS DONE", 160, 38, 0.9f, PAL_GO, 0);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 16, 1.0f, PAL_ALERT, 0);
        text(reason_, 160, 40, 0.9f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 24, 1.1f, PAL_HUD, 0);
    }
    drawWorld(cam);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_GO : PAL_ALERT);
        int left = std::max(0, int(std::ceil(kWatch - watch_)));
        char buf[16];
        std::snprintf(buf, sizeof buf, "%d", left);
        hud(36, 0, buf, left < 12 ? PAL_ALERT : PAL_HUD);
        hudC(1, hint(), hint()[0] == 'T' || hint()[0] == 'C' || hint()[0] == 'G' ? PAL_HUD : PAL_LAMP);
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
        static const float good[] = {494.f, 659.f, 784.f, 988.f};
        static const float bad[] = {311.f, 233.f, 175.f, 130.f};
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
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 92.f : 70.f, 0.016f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 78.f, 0.01f);
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
        blip(260.f);
        audio();
        draw();
        return;
    } else {
        left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
        right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
        duck = pad.down(gs::BTN_DOWN) || pad.axisY <= -0.45f;
    }
    stepPlay(left, right, jump, duck);
    audio();
    draw();
}

}  // namespace alley
