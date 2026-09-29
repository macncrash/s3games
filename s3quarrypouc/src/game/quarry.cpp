#include "game/quarry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace quarry {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 176.f;
constexpr float kWorld = 1560.f;
constexpr float kSpawn = 72.f;
constexpr float kPouch0 = 230.f;
constexpr float kWatch = 48.f;
constexpr float kRun = 156.f;
constexpr float kDuckSp = 70.f;
constexpr float kAccel = 1500.f;
constexpr float kGrav = 860.f;
constexpr float kJumpV = -430.f;
constexpr float kPitA = 500.f;
constexpr float kPitB = 600.f;
constexpr float kBeam = 820.f;
constexpr float kSkip = 1080.f;
constexpr float kGoal = 1360.f;

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

bool Game::pit(float x) const { return x > kPitA + 10.f && x < kPitB - 10.f; }

float Game::skipFoot() const {
    float s = std::sin(t_ * 1.65f);
    return 108.f + 62.f * (0.5f + 0.5f * s);
}

void Game::dump(const char* where) const {
    std::fprintf(stderr, "quarrypouc %s px %.1f py %.1f vx %.1f held %d ground %d duck %d watch %.2f pouch %.1f %s\n",
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
    blip(440.f);
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
        sys_->rumble(0.2f, 0.6f, 180);
        sys_->setLight(40, 140, 60);
        blip(720.f);
    } else {
        sys_->rumble(0.7f, 0.2f, 160);
        sys_->setLight(140, 40, 20);
        sys_->apu.noiseBurst(0.4f, 120.f, 0.26f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::bump(float fromX) {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.65f;
    stun_ = 0.2f;
    shake_ = 0.7f;
    float away = px_ <= fromX ? -1.f : 1.f;
    vx_ = away * 140.f;
    face_ = away < 0 ? 1 : -1;
    if (held_) {
        held_ = false;
        float drop = px_ + away * 24.f;
        if (pit(drop)) drop = px_ - away * 16.f;
        pouchX_ = std::clamp(drop, 48.f, kGoal - 48.f);
        if (pit(pouchX_)) {
            finish(false, "THE POUCH FELL");
            return;
        }
    }
    sys_->apu.noiseBurst(0.3f, 420.f, 0.1f);
    sys_->rumble(0.45f, 0.18f, 70);
}

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (duck_) return art_.duck;
    if (std::abs(vx_) > 20.f) return (int(stepT_) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    left = right = jump = duck = false;
    if (stun_ > 0.f) return;
    const float target = held_ ? kGoal + 12.f : pouchX_;
    if (!held_) {
        if (px_ < target - 8.f) right = true;
        else if (px_ > target + 8.f) left = true;
        return;
    }
    if (px_ > kBeam - 70.f && px_ < kBeam + 70.f) duck = true;
    if (!onGround_) {
        right = true;
        return;
    }
    if (px_ < kPitB + 8.f) {
        right = true;
        if (px_ > kPitA - 48.f && px_ < kPitA - 10.f && vx_ > 80.f) jump = true;
        return;
    }
    float foot = skipFoot();
    bool rising = std::cos(t_ * 1.65f) < 0.f;
    if (px_ > kSkip - 80.f && px_ < kSkip - 4.f && !(rising && foot < 136.f)) return;
    if (px_ < kGoal + 16.f) right = true;
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
    px_ = std::clamp(px_, 28.f, kWorld - 28.f);

    bool overCut = pit(px_);
    if (!overCut && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0;
        onGround_ = true;
        jumped_ = false;
        coyote_ = 0.08f;
    } else if (overCut && py_ >= kFloor - 2.f) {
        finish(false, held_ ? "THE CUT TOOK THE POUCH" : "THE CUT TOOK YOU");
        return;
    } else {
        onGround_ = false;
        if (coyote_ > 0.f) coyote_ -= DT;
    }

    if (std::abs(vx_) > 8.f && onGround_) stepT_ += std::abs(vx_) * DT * 0.08f;

    const bool inBeam = px_ > kBeam - 36.f && px_ < kBeam + 36.f;
    if (inBeam && !duck_ && py_ > kFloor - 18.f && stun_ <= 0.f) bump(kBeam);

    float foot = skipFoot();
    if (std::abs(px_ - kSkip) < 22.f && foot > 142.f && py_ > foot - 18.f && stun_ <= 0.f) bump(kSkip);

    if (!held_ && onGround_ && stun_ <= 0.f && std::abs(px_ - pouchX_) < 24.f && std::abs(py_ - kFloor) < 12.f) {
        held_ = true;
        blip(560.f);
        sys_->rumble(0.15f, 0.35f, 60);
    }

    if (held_ && onGround_ && px_ >= kGoal) {
        finish(true, "THE POUCH CROSSED");
        return;
    }
    if (!held_ && onGround_ && px_ >= kGoal) {
        finish(false, "LEFT THE POUCH");
        return;
    }
}

const char* Game::hint() const {
    if (!held_) {
        if (px_ > pouchX_ + 36.f) return "GO BACK FOR THE POUCH";
        return "TAKE THE POUCH";
    }
    if (px_ < kPitB) return "JUMP THE CUT";
    if (px_ < kBeam + 40.f) return "DUCK THE BOOM";
    if (px_ < kSkip + 40.f) return "LET THE SKIP PASS";
    return "CARRY IT TO THE GATE";
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
        int sky = y < 28 ? 8 : (y < 70 ? 10 : 12);
        int g = y < 28 ? 7 : (y < 70 ? 8 : 7);
        vdp.lineBackdrop[y] = gs::rgb4(sky, g, 4);
        vdp.lineFog[y] = y < 36 ? 4 : 0;
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

    const float faces[] = {160.f, 420.f, 700.f, 980.f, 1240.f};
    for (float x : faces) world(art_.face, x, kFloor - 8.f, 48, PAL_ROCK, false);

    world(art_.lip, kPitA, kFloor + 4.f, 36, PAL_PIT, false);
    world(art_.lip, kPitB, kFloor + 4.f, 36, PAL_PIT, true);
    world(art_.beam, kBeam, kFloor - 46.f, 12, PAL_IRON, false, false);
    world(art_.cable, kSkip, 78.f, 70, PAL_IRON, false, false);
    float foot = skipFoot();
    world(art_.skip, kSkip, foot, 40, PAL_IRON, false);

    const float lamps[] = {140.f, 360.f, 680.f, 960.f, 1220.f};
    for (float x : lamps) {
        world(art_.lamp, x, kFloor, 70, PAL_IRON, false);
        world(art_.flame[fi], x, kFloor - 68.f, 12, PAL_LAMP, false, false);
    }
    world(art_.ramp, kGoal - 30.f, kFloor, 22, PAL_BENCH, false);
    world(art_.gate, kGoal + 28.f, kFloor, 78, PAL_GATE, false);
    world(art_.flame[fi], kGoal + 28.f, kFloor - 84.f, 12.f + (held_ ? 2.f : 0.f), PAL_LAMP, false, false);

    bool show = inv_ <= 0.f || (int(inv_ * 16.f) & 1) == 0;
    if (show && mode_ != Mode::Title) {
        if (held_) world(art_.pouch[bob], px_ + face_ * 14.f, py_ - 30.f, 16, PAL_POUCH, face_ < 0, false);
        world(heroSprite(), px_, py_, duck_ ? 38.f : 54.f, PAL_PLAYER, face_ < 0);
        if (py_ > kFloor - 28.f) world(art_.shadow, px_, kFloor + 2.f, 6, PAL_PIT, false, true, true);
    }
    if (!held_) world(art_.pouch[bob], pouchX_, kFloor - 14.f, 18, PAL_POUCH, false);
}

void Game::drawTitle() {
    int fi = int(t_ * 8.f) & 1;
    text("S3 QUARRY POUC", 160, 16, 1.05f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160, 42, 0.85f, PAL_LAMP, 0);
    text("ONE QUARRY  THEN IT IS DONE", 160, 64, 0.7f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 90, 1.0f, PAL_GO, 0);
    spr(art_.face, 48, kFloor - 8, 44, PAL_ROCK, false, true, false);
    spr(art_.stand, 96, kFloor, 54, PAL_PLAYER, false, true, false);
    spr(art_.shadow, 96, kFloor + 2, 6, PAL_PIT, false, true, true);
    spr(art_.pouch[fi], 150, kFloor - 14, 18, PAL_POUCH, false, true, false);
    spr(art_.lip, 196, kFloor + 4, 30, PAL_PIT, false, true, false);
    spr(art_.skip, 236, 150, 36, PAL_IRON, false, true, false);
    spr(art_.gate, 286, kFloor, 70, PAL_GATE, false, true, false);
    spr(art_.flame[fi], 286, kFloor - 76, 11, PAL_LAMP, false, false, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = std::clamp(px_ - 130.f, 0.f, kWorld - float(gs::SCREEN_W));
        if (shake_ > 0.f) cam += std::sin(t_ * 55.f) * shake_ * 3.f;
        cam_ = cam;
    }
    backdrop(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160, 16, 1.0f, PAL_HUD, 0);
        text("THE QUARRY IS DONE", 160, 38, 0.9f, PAL_GO, 0);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 16, 1.0f, PAL_ALERT, 0);
        text(reason_, 160, 40, 0.85f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 22, 1.1f, PAL_HUD, 0);
    }
    drawWorld(cam);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_GO : PAL_ALERT);
        int left = std::max(0, int(std::ceil(kWatch - watch_)));
        char buf[16];
        std::snprintf(buf, sizeof buf, "%d", left);
        hud(36, 0, buf, left < 10 ? PAL_ALERT : PAL_HUD);
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
        static const float good[] = {392.f, 523.f, 659.f, 784.f};
        static const float bad[] = {294.f, 220.f, 165.f, 130.f};
        fanT_ += DT;
        if (fanT_ > 0.13f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.04f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0.f;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 88.f : 64.f, 0.015f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 72.f, 0.01f);
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
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
        duck = pad.down(gs::BTN_DOWN) || pad.axisY <= -0.45f;
    }
    stepPlay(left, right, jump, duck);
    audio();
    draw();
}

}  // namespace quarry
