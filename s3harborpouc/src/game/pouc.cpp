#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace harborpouc {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 176.f;
constexpr float kWorld = 1560.f;
constexpr float kSpawn = 70.f;
constexpr float kPouch0 = 168.f;
constexpr float kGoal = 1408.f;
constexpr float kRun = 168.f;
constexpr float kDuckSp = 76.f;
constexpr float kGrav = 980.f;
constexpr float kJumpV = -390.f;

constexpr float kTideA = 312.f, kTideB = 448.f;
constexpr float kTidePeriod = 4.4f, kTideUp = 2.7f, kTidePhase = 0.15f;

constexpr float kSpanA = 860.f, kSpanB = 1036.f;
constexpr float kSpanPeriod = 5.2f, kSpanUp = 3.3f, kSpanPhase = 1.7f;

constexpr float kHookX = 640.f;
constexpr float kHookPeriod = 3.6f, kHookHigh = 1.9f, kHookPhase = 0.55f;

float moveToward(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

float phaseU(float t, float period, float phase) {
    float u = std::fmod(t + phase, period);
    if (u < 0.f) u += period;
    return u;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ && won_) return 3;
    if (over_) return 4;
    if (held_) return 2;
    return 1;
}

void Game::dump(const char* where) const {
    std::fprintf(stderr,
                 "harborpouc %s px %.1f py %.1f vx %.1f held %d gnd %d duck %d t %.2f pouch %.1f "
                 "tide %.2f span %.2f hook %.2f %s\n",
                 where, px_, py_, vx_, held_ ? 1 : 0, onGround_ ? 1 : 0, duck_ ? 1 : 0, t_, pouchX_, tideLift_,
                 spanLift_, hookLift_, reason_);
}

bool Game::spanOn(float period, float openFor, float phase) const {
    return phaseU(t_, period, phase) < openFor;
}

float Game::openRemain(float period, float openFor, float phase) const {
    float u = phaseU(t_, period, phase);
    if (u < openFor) return openFor - u;
    return -(period - u);
}

void Game::tickLifts() {
    tideLift_ = moveToward(tideLift_, spanOn(kTidePeriod, kTideUp, kTidePhase) ? 1.f : 0.f, 5.5f * DT);
    spanLift_ = moveToward(spanLift_, spanOn(kSpanPeriod, kSpanUp, kSpanPhase) ? 1.f : 0.f, 5.5f * DT);
    hookLift_ = moveToward(hookLift_, spanOn(kHookPeriod, kHookHigh, kHookPhase) ? 1.f : 0.f, 5.f * DT);
}

bool Game::hole(float x) const {
    if (x > kTideA + 6.f && x < kTideB - 6.f && tideLift_ < 0.88f) return true;
    if (x > kSpanA + 6.f && x < kSpanB - 6.f && spanLift_ < 0.88f) return true;
    return false;
}

bool Game::hookHits() const {
    if (std::abs(px_ - kHookX) > 26.f) return false;
    if (hookLift_ > 0.72f) return false;
    return bodyH() > 30.f;
}

void Game::begin() {
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    pouchX_ = kPouch0;
    held_ = false;
    onGround_ = true;
    duck_ = false;
    face_ = 1;
    fan_ = -1;
    reason_ = "";
    stepT_ = stun_ = inv_ = shake_ = beep_ = fanT_ = 0;
    cam_ = 0;
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    t_ = 0;
    tideLift_ = spanLift_ = hookLift_ = 0;
    tickLifts();
    blip(494.f);
}

void Game::finish(bool crossed, const char* why) {
    if (mode_ != Mode::Play) return;
    won_ = crossed;
    over_ = true;
    reason_ = why;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    vx_ = vy_ = 0;
    fan_ = 0;
    fanT_ = 0;
    shake_ = crossed ? 0.2f : 0.7f;
    if (crossed) {
        sys_->rumble(0.25f, 0.65f, 200);
        sys_->setLight(30, 150, 90);
        blip(740.f);
    } else {
        sys_->rumble(0.75f, 0.25f, 180);
        sys_->setLight(160, 30, 24);
        sys_->apu.noiseBurst(0.4f, 160.f, 0.26f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::hit() {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.7f;
    stun_ = 0.22f;
    shake_ = 0.8f;
    vx_ = -140.f;
    face_ = 1;
    if (held_) {
        held_ = false;
        float drop = px_ - 28.f;
        if (hole(drop)) {
            finish(false, "THE POUCH SANK");
            return;
        }
        pouchX_ = drop;
    }
    sys_->apu.noiseBurst(0.35f, 420.f, 0.12f);
}

const char* Game::hint() const {
    if (!held_) return "TAKE THE POUCH";
    if (px_ > kTideA - 90.f && px_ < kTideA && tideLift_ < 0.9f) return "WAIT FOR THE PONTOON";
    if (px_ > kHookX - 90.f && px_ < kHookX + 30.f && hookLift_ < 0.75f) return "DUCK THE HOOK";
    if (px_ > kSpanA - 90.f && px_ < kSpanA && spanLift_ < 0.9f) return "WAIT FOR THE SPAN";
    if (px_ > kGoal - 120.f) return "ONTO THE FAR QUAY";
    return "CARRY IT ACROSS";
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    left = right = jump = duck = false;
    if (stun_ > 0.f) return;
    if (!held_) {
        if (px_ < pouchX_ - 8.f) right = true;
        else if (px_ > pouchX_ + 8.f) left = true;
        return;
    }
    auto gate = [&](float a, float b, float lift, float remainNeed, float period, float openFor, float phase) {
        if (px_ > a - 48.f && px_ < a + 4.f) {
            float r = openRemain(period, openFor, phase);
            if (lift < 0.92f || r < remainNeed) return true;
        }
        if (px_ >= a + 4.f && px_ <= b - 4.f) {
            right = true;
            return true;
        }
        return false;
    };
    if (gate(kTideA, kTideB, tideLift_, 1.15f, kTidePeriod, kTideUp, kTidePhase)) return;
    if (px_ > kHookX - 56.f && px_ < kHookX + 40.f) duck = true;
    if (gate(kSpanA, kSpanB, spanLift_, 1.35f, kSpanPeriod, kSpanUp, kSpanPhase)) return;
    right = true;
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    t_ += DT;
    tickLifts();
    if (stun_ > 0.f) {
        stun_ -= DT;
        left = right = jump = duck = false;
    }
    if (inv_ > 0.f) inv_ -= DT;
    duck_ = duck && onGround_ && stun_ <= 0.f;
    if (left) face_ = -1;
    if (right) face_ = 1;
    float target = 0.f;
    if (right) target = duck_ ? kDuckSp : kRun;
    if (left) target = duck_ ? -kDuckSp : -kRun;
    vx_ = moveToward(vx_, target, 1500.f * DT);
    if (!onGround_) vy_ += kGrav * DT;
    else if (jump) {
        vy_ = kJumpV;
        onGround_ = false;
        blip(620.f);
    }
    float nx = px_ + vx_ * DT;
    if (onGround_ && hole(nx) && !hole(px_) && vy_ >= 0.f) {
        vx_ = 0.f;
        nx = px_;
    }
    px_ = std::clamp(nx, 18.f, kWorld - 18.f);
    py_ += vy_ * DT;
    if (!hole(px_) && py_ >= kFloor && vy_ >= 0.f) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else {
        onGround_ = false;
        if (py_ > kFloor + 36.f) {
            finish(false, held_ ? "THE POUCH SANK" : "IN THE HARBOR");
            return;
        }
    }
    if (std::abs(vx_) > 20.f && onGround_) stepT_ += DT * 10.f;
    if (hookHits()) hit();
    if (mode_ != Mode::Play) return;
    if (!held_ && onGround_ && std::abs(px_ - pouchX_) < 24.f && std::abs(py_ - kFloor) < 8.f) {
        held_ = true;
        blip(880.f);
    }
    if (held_ && onGround_ && px_ >= kGoal) finish(true, "CROSSED");
    else if (!held_ && onGround_ && px_ >= kGoal) finish(false, "THE POUCH STAYED");
    float want = std::clamp(px_ - 130.f, 0.f, kWorld - float(gs::SCREEN_W));
    cam_ = moveToward(cam_, want, 420.f * DT);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.f);
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
    if (s.x > gs::SCREEN_W + 90 || s.x + s.w < -90 || s.y > gs::SCREEN_H + 90 || s.y + s.h < -90) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale;
    }
    x -= width * 0.5f;
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

void Game::sky() {
    gs::VDP& vdp = sys_->vdp;
    int bob = int(t_ * 18.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) c = gs::rgb4(5, 8, 13);
        else if (y < 128) c = gs::rgb4(6, 10, 14);
        else if (y < 168) c = gs::rgb4(3, 7, 11);
        else c = gs::rgb4(2, 5, 9);
        if (y > 168 && ((y + bob) & 7) == 0) c = gs::rgb4(4, 8, 12);
        vdp.lineBackdrop[y] = c;
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

    bool show = inv_ <= 0.f || (int(inv_ * 16.f) & 1) == 0;
    if (show && mode_ != Mode::Title) {
        if (held_) world(art_.pouch, px_ + face_ * 16.f, py_ - 28.f, 16, PAL_POUCH, face_ < 0, false);
        world(duck_ ? art_.duck : art_.stand, px_, py_, duck_ ? 26.f : 52.f, PAL_PLAYER, face_ < 0);
        if (onGround_) world(art_.shadow, px_, kFloor + 2.f, 7, PAL_FOAM, false, true, true);
    }
    if (!held_ && mode_ != Mode::Title) world(art_.pouch, pouchX_, kFloor - 26.f, 16, PAL_POUCH, false);

    float hookY = kFloor - 36.f - hookLift_ * 78.f;
    world(art_.hook, kHookX, hookY, 18, PAL_STEEL, false, false);
    float cableH = std::max(6.f, hookY - 58.f);
    world(art_.cable, kHookX, 52.f + cableH * 0.5f, cableH, PAL_STEEL, false, false);
    world(art_.beam, kHookX, 50.f, 12, PAL_STEEL, false, false);
    world(art_.post, kHookX - 40.f, kFloor, 128, PAL_STEEL, false);
    world(art_.post, kHookX + 40.f, kFloor, 128, PAL_STEEL, false);
    int lampPal = hookLift_ > 0.75f ? PAL_GO : PAL_ALERT;
    world(art_.lamp, kHookX + 28.f, 42.f, 10, lampPal, false, false);

    float tideY = kFloor + (1.f - tideLift_) * 30.f;
    world(art_.pontoon, (kTideA + kTideB) * 0.5f, tideY, 16, PAL_WOOD, false);
    world(art_.lamp, kTideA - 10.f, kFloor - 36.f, 10, tideLift_ > 0.9f ? PAL_GO : PAL_ALERT, false, false);
    float wave = std::sin(t_ * 3.f) * 3.f;
    world(art_.buoy, (kTideA + kTideB) * 0.5f + 18.f, kFloor + 18.f + wave, 20, PAL_ALERT, false);
    world(art_.wave, (kTideA + kTideB) * 0.5f - 10.f, kFloor + 22.f, 8, PAL_WATER, false, false);

    float spanY = kFloor + (1.f - spanLift_) * 32.f;
    world(art_.pontoon, (kSpanA + kSpanB) * 0.5f - 40.f, spanY, 16, PAL_STEEL, false);
    world(art_.pontoon, (kSpanA + kSpanB) * 0.5f + 40.f, spanY, 16, PAL_STEEL, false);
    world(art_.lamp, kSpanA - 12.f, kFloor - 36.f, 10, spanLift_ > 0.9f ? PAL_GO : PAL_ALERT, false, false);
    world(art_.wave, (kSpanA + kSpanB) * 0.5f, kFloor + 24.f, 8, PAL_WATER, false, false);
    world(art_.buoy, kSpanB - 20.f, kFloor + 16.f - wave, 18, PAL_GO, false);

    for (float x = 24.f; x < kWorld; x += 46.f) {
        bool gap = (x > kTideA - 8.f && x < kTideB + 8.f) || (x > kSpanA - 8.f && x < kSpanB + 8.f);
        if (!gap) world(art_.plank, x, kFloor + 6.f, 14, PAL_WOOD, false);
    }
    world(art_.bollard, kPouch0, kFloor, 30, PAL_STEEL, false);
    world(art_.bollard, 250.f, kFloor, 30, PAL_WOOD, false);
    world(art_.shed, kGoal + 36.f, kFloor, 62, PAL_HOUSE, false);
    world(art_.lamp, kGoal + 28.f, kFloor - 58.f, 12 + 2.f * std::sin(t_ * 7.f), PAL_GO, false, false);

    float gullY = 36.f + std::sin(t_ * 1.7f) * 6.f;
    world(art_.gull, std::fmod(t_ * 28.f, kWorld), gullY, 12, PAL_FOAM, false, false);
    world(art_.gull, std::fmod(180.f + t_ * 18.f, kWorld), 52.f, 10, PAL_FOAM, true, false);
}

void Game::drawTitle() {
    text("S3 HARBOR POUC", 160, 18, 1.05f, PAL_HUD);
    text("CARRY THE POUCH ACROSS", 160, 46, 0.85f, PAL_WATER);
    text("THEN IT IS DONE", 160, 70, 0.9f, PAL_GO);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 96, 0.9f, PAL_HUD);
    spr(art_.stand, 78, kFloor, 52, PAL_PLAYER, false, true, false);
    spr(art_.shadow, 78, kFloor + 2, 7, PAL_FOAM, false, true, true);
    spr(art_.bollard, 138, kFloor, 30, PAL_STEEL, false, true, false);
    spr(art_.pouch, 138, kFloor - 26, 16, PAL_POUCH, false, true, false);
    spr(art_.plank, 200, kFloor + 6, 14, PAL_WOOD, false, true, false);
    spr(art_.pontoon, 250, kFloor, 16, PAL_WOOD, false, true, false);
    spr(art_.shed, 286, kFloor, 54, PAL_HOUSE, false, true, false);
    spr(art_.gull, 40, 118, 12, PAL_FOAM, false, false, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.clear();
    vdp.B.clear();
    sky();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = cam_;
        if (shake_ > 0.f) cam += std::sin(t_ * 60.f) * shake_ * 3.f;
    }
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160, 16, 0.95f, PAL_HUD);
        text("IT IS DONE", 160, 40, 0.95f, PAL_GO);
    } else if (mode_ == Mode::Lost) {
        text("NOT DONE", 160, 16, 1.05f, PAL_ALERT);
        text(reason_, 160, 40, 0.85f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 24, 1.05f, PAL_HUD);
    }
    drawWorld(cam);
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_GO : PAL_ALERT);
        hudC(1, hint(), PAL_HUD);
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
        static const float bad[] = {311.f, 233.f, 174.f, 130.f};
        fanT_ += DT;
        if (fanT_ > 0.13f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.08f : 0.05f);
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
    sys.apu.setMaster(0.65f);
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
            blip(440.f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
        audio();
        draw();
        return;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        t_ += DT;
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
        audio();
        draw();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        audio();
        draw();
        return;
    }
    bool left = false, right = false, jump = false, duck = false;
    if (bot_) bot(left, right, jump, duck);
    else {
        left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
        right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
        duck = pad.down(gs::BTN_DOWN) || pad.axisY < -0.35f;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_UP);
    }
    stepPlay(left, right, jump, duck);
    audio();
    draw();
}

}  // namespace harborpouc
