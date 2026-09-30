#include "game/wharf.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wharf {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 168.f;
constexpr float kWorld = 1720.f;
constexpr float kSpawn = 72.f;
constexpr float kPouch0 = 196.f;
constexpr float kWatch = 48.f;
constexpr float kRun = 156.f;
constexpr float kDuckSp = 74.f;
constexpr float kAccel = 1600.f;
constexpr float kGrav = 980.f;
constexpr float kJumpV = -400.f;
constexpr float kBoom = 980.f;
constexpr float kGoal = 1564.f;

struct Span {
    float a, b;
};
constexpr Span kGaps[] = {{440.f, 508.f}, {1200.f, 1276.f}};

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

bool Game::gap(float x) const {
    for (const Span& p : kGaps)
        if (x > p.a + 6.f && x < p.b - 6.f) return true;
    return false;
}

bool Game::hookLow() const {
    float c = std::fmod(t_, 2.4f);
    return c >= 0.85f && c <= 1.70f;
}

float Game::barrelX() const { return 720.f + 100.f * std::sin(t_ * 1.35f); }

void Game::dump(const char* where) const {
    std::fprintf(stderr, "wharfpouc %s px %.1f py %.1f vx %.1f held %d ground %d duck %d watch %.2f pouch %.1f %s\n",
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
    t_ = 0;
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
    shake_ = crossed ? 0.3f : 0.7f;
    if (crossed) {
        sys_->rumble(0.2f, 0.6f, 200);
        sys_->setLight(30, 140, 80);
        blip(720.f);
    } else {
        sys_->rumble(0.7f, 0.2f, 180);
        sys_->setLight(140, 30, 20);
        sys_->apu.noiseBurst(0.4f, 120.f, 0.28f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::knock(float fromX) {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.65f;
    stun_ = 0.2f;
    shake_ = 0.7f;
    float away = px_ <= fromX ? -1.f : 1.f;
    vx_ = away * 140.f;
    face_ = away < 0 ? 1 : -1;
    if (held_) {
        held_ = false;
        float drop = px_ + away * 26.f;
        if (gap(drop)) drop = px_ - away * 16.f;
        pouchX_ = std::clamp(drop, 40.f, kGoal - 36.f);
        if (gap(pouchX_)) {
            finish(false, "THE POUCH WENT IN");
            return;
        }
    }
    sys_->apu.noiseBurst(0.3f, 420.f, 0.12f);
    sys_->rumble(0.45f, 0.2f, 80);
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
        else if (px_ > pouchX_ + 6.f) left = true;
        return;
    }
    if (!onGround_) {
        right = true;
        return;
    }
    Span next{kWorld, kWorld};
    for (const Span& p : kGaps) {
        if (px_ < p.b - 4.f) {
            next = p;
            break;
        }
    }
    if (px_ < kBoom + 26.f && next.a > kBoom) {
        if (std::abs(px_ - barrelX()) < 54.f) jump = true;
        right = true;
        if (px_ > kBoom - 44.f) duck = true;
        return;
    }
    if (px_ < next.b - 4.f) {
        right = true;
        if (px_ > next.a - 34.f && px_ < next.a - 4.f && vx_ > 110.f) jump = true;
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
        float accel = onGround_ ? kAccel : kAccel * 0.55f;
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

    bool open = gap(px_);
    if (!open && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else if (open && py_ >= kFloor) {
        onGround_ = false;
        if (py_ > kFloor + 42.f) {
            finish(false, held_ ? "THE POUCH WENT IN" : "OFF THE WHARF");
            return;
        }
    } else {
        onGround_ = false;
    }

    if (onGround_) {
        coyote_ = 0.1f;
        jumped_ = false;
        if (std::abs(vx_) > 20.f) stepT_ += DT * std::abs(vx_) / 22.f;
    } else if (!jumped_) {
        coyote_ = std::max(0.f, coyote_ - DT);
    }

    if (inv_ <= 0.f && onGround_ && std::abs(px_ - barrelX()) < 18.f) knock(barrelX());
    if (mode_ != Mode::Play) return;
    if (inv_ <= 0.f && !duck_ && hookLow() && std::abs(px_ - kBoom) < 16.f && py_ > kFloor - 30.f) knock(kBoom);
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
    for (const Span& p : kGaps)
        if (px_ > p.a - 100.f && px_ < p.a) return "JUMP THE GAP";
    if (std::abs(px_ - barrelX()) < 90.f && px_ < barrelX()) return "CLEAR THE CASK";
    if (px_ > kBoom - 110.f && px_ < kBoom + 16.f) return "DUCK THE HOOK";
    if (px_ > kGoal - 150.f) return "TO THE SHED";
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
    int hs = int(std::lround(-cam * 0.2f));
    int fg = int(std::lround(-cam));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(3, 6, 10);
        vdp.lineFog[y] = 0;
        vdp.B.hscroll[y] = int16_t(hs);
        vdp.B.vscroll[y] = 0;
        vdp.A.hscroll[y] = int16_t(fg);
        vdp.A.vscroll[y] = 0;
        gs::RoadLine& r = vdp.road[y];
        if (y >= 184) {
            r.on = true;
            r.cx = 160;
            r.hw = 420;
            r.v = float(y) * 3.f + t_ * 40.f;
            r.pal = PAL_ROAD;
            r.band = uint8_t((y / 6) & 1);
            r.style = 2;
            r.left = gs::GROUND_WATER;
            r.right = gs::GROUND_WATER;
        } else {
            r.on = false;
        }
    }
    vdp.roadTime = int(t_ * 60.f);
    vdp.HUD.scroll(0, 0);
}

void Game::drawWorld(float cam) {
    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true,
                     bool shadow = false) { spr(m, wx - cam, foot, h, pal, flip, feet, shadow); };
    int fi = int(t_ * 6.f) & 1;
    int bob = int(t_ * 4.f) & 1;

    const float posts[] = {120.f, 300.f, 560.f, 760.f, 1040.f, 1360.f, 1500.f};
    for (float x : posts) world(art_.piling, x, kFloor + 52.f, 56, PAL_WOOD, false);

    for (const Span& p : kGaps) {
        world(art_.piling, p.a - 8.f, kFloor + 46.f, 50, PAL_WOOD, false);
        world(art_.piling, p.b + 8.f, kFloor + 46.f, 50, PAL_WOOD, false);
    }

    world(art_.mast, kBoom, kFloor, 120, PAL_IRON, false);
    float hookY = hookLow() ? kFloor - 34.f : kFloor - 96.f;
    world(art_.hook, kBoom + 6.f, hookY, hookLow() ? 26.f : 22.f, PAL_IRON, false, false);

    world(art_.barrel, barrelX(), kFloor, 28, PAL_WOOD, barrelX() > 720.f);

    const float gulls[] = {240.f, 680.f, 1100.f, 1460.f};
    for (int i = 0; i < 4; i++) {
        float gx = gulls[i] + std::sin(t_ * 0.7f + i) * 18.f;
        world(art_.gull[fi], gx, 48.f + float(i % 2) * 14.f, 14, PAL_HUD, false, false);
    }

    world(art_.shed, kGoal + 48.f, kFloor, 78, PAL_SHED, false);
    world(art_.hook, kGoal + 48.f, kFloor - 86.f, 10, PAL_LAMP, false, false);

    bool show = inv_ <= 0.f || (int(inv_ * 16.f) & 1) == 0;
    if (show && mode_ != Mode::Title) {
        if (held_) world(art_.pouch[bob], px_ + face_ * 14.f, py_ - 30.f, 16, PAL_POUCH, face_ < 0, false);
        world(heroSprite(), px_, py_, duck_ ? 38.f : 54.f, PAL_PLAYER, face_ < 0);
        if (py_ > kFloor - 28.f) world(art_.shadow, px_, kFloor + 3.f, 6, PAL_WATER, false, true, true);
    }
    if (!held_) world(art_.pouch[bob], pouchX_, kFloor - 8.f, 18, PAL_POUCH, false);
}

void Game::drawTitle() {
    int fi = int(t_ * 6.f) & 1;
    text("S3 WHARF POUC", 160, 16, 1.05f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160, 42, 0.85f, PAL_LAMP, 0);
    text("ANYTHING ELSE IS A LOSS", 160, 62, 0.75f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 88, 1.0f, PAL_GO, 0);
    spr(art_.piling, 50, kFloor + 40, 48, PAL_WOOD, false, true, false);
    spr(art_.stand, 90, kFloor, 54, PAL_PLAYER, false, true, false);
    spr(art_.shadow, 90, kFloor + 3, 6, PAL_WATER, false, true, true);
    spr(art_.pouch[fi], 150, kFloor - 8, 18, PAL_POUCH, false, true, false);
    spr(art_.barrel, 210, kFloor, 28, PAL_WOOD, false, true, false);
    spr(art_.shed, 286, kFloor, 64, PAL_SHED, false, true, false);
    spr(art_.gull[fi], 200, 36, 14, PAL_HUD, false, false, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = std::clamp(px_ - 130.f, 0.f, kWorld - float(gs::SCREEN_W));
        if (shake_ > 0.f) cam += std::sin(t_ * 60.f) * shake_ * 3.f;
        cam_ = cam;
    }
    backdrop(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160, 14, 1.0f, PAL_HUD, 0);
        text("THE WHARF IS DONE", 160, 36, 0.9f, PAL_GO, 0);
    } else if (mode_ == Mode::Lost) {
        text("A LOSS", 160, 14, 1.1f, PAL_ALERT, 0);
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
        static const float bad[] = {294.f, 220.f, 165.f, 110.f};
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

}  // namespace wharf
