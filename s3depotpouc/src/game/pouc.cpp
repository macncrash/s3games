#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace pouc {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 176.f;
constexpr float kWorld = 1720.f;
constexpr float kSpawn = 72.f;
constexpr float kPouch0 = 188.f;
constexpr float kGoal = 1544.f;
constexpr float kRun = 156.f;
constexpr float kDuckSp = 74.f;
constexpr float kAccel = 1100.f;
constexpr float kGrav = 900.f;
constexpr float kJumpV = -440.f;

struct Pit {
    float a, b;
};
constexpr Pit kPits[2] = {{408.f, 464.f}, {1172.f, 1228.f}};

constexpr float kBridgeX = 690.f;
constexpr float kBridgeHalf = 64.f;
constexpr float kBridgePeriod = 5.6f;
constexpr float kBridgeUp = 3.8f;
constexpr float kBridgePhase = 0.5f;

constexpr float kDoorX = 958.f;
constexpr float kDoorHalf = 18.f;
constexpr float kDoorPeriod = 4.2f;
constexpr float kDoorOpen = 2.6f;
constexpr float kDoorPhase = 1.4f;

constexpr float kHookX = 1396.f;
constexpr float kHookPeriod = 4.6f;
constexpr float kHookHigh = 2.7f;
constexpr float kHookPhase = 0.8f;

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

void Game::dump(const char* where) const {
    std::fprintf(stderr,
                 "depotpouc %s px %.1f py %.1f vx %.1f held %d gnd %d duck %d t %.2f pouch %.1f "
                 "bridge %.2f door %.2f hook %.2f %s\n",
                 where, px_, py_, vx_, held_ ? 1 : 0, onGround_ ? 1 : 0, duck_ ? 1 : 0, t_, pouchX_, bridgeLift_,
                 doorLift_, hookLift_, reason_);
}

bool Game::spanOn(float period, float openFor, float phase) const {
    float u = std::fmod(t_ + phase, period);
    if (u < 0.f) u += period;
    return u < openFor;
}

float Game::openRemain(float period, float openFor, float phase, float lift) const {
    float u = std::fmod(t_ + phase, period);
    if (u < 0.f) u += period;
    if (u >= openFor || lift < 0.90f) {
        float wait = u >= openFor ? (period - u) : 0.12f;
        return -wait;
    }
    return openFor - u;
}

void Game::syncLifts() {
    bridgeLift_ = spanOn(kBridgePeriod, kBridgeUp, kBridgePhase) ? 1.f : 0.f;
    doorLift_ = spanOn(kDoorPeriod, kDoorOpen, kDoorPhase) ? 1.f : 0.f;
    hookLift_ = spanOn(kHookPeriod, kHookHigh, kHookPhase) ? 1.f : 0.f;
}

void Game::tickGates() {
    bridgeLift_ = moveToward(bridgeLift_, spanOn(kBridgePeriod, kBridgeUp, kBridgePhase) ? 1.f : 0.f, 6.5f * DT);
    doorLift_ = moveToward(doorLift_, spanOn(kDoorPeriod, kDoorOpen, kDoorPhase) ? 1.f : 0.f, 6.5f * DT);
    hookLift_ = moveToward(hookLift_, spanOn(kHookPeriod, kHookHigh, kHookPhase) ? 1.f : 0.f, 5.5f * DT);
}

bool Game::hole(float x) const {
    for (const Pit& p : kPits)
        if (x > p.a + 1.f && x < p.b - 1.f) return true;
    if (x > kBridgeX - kBridgeHalf + 8.f && x < kBridgeX + kBridgeHalf - 8.f) {
        if (!(spanOn(kBridgePeriod, kBridgeUp, kBridgePhase) && bridgeLift_ > 0.88f)) return true;
    }
    return false;
}

bool Game::doorBlocks(float foot) const {
    if (doorLift_ >= 0.74f) return false;
    float bottom = kFloor - doorLift_ * 84.f;
    float top = bottom - 102.f;
    float head = foot - bodyH();
    return foot > top + 6.f && head < bottom - 2.f;
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
    pouchOnDesk_ = true;
    face_ = 1;
    fan_ = -1;
    reason_ = "";
    stepT_ = coyote_ = stun_ = inv_ = shake_ = beep_ = fanT_ = 0;
    cam_ = 0;
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    syncLifts();
    blip(520.f);
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
    shake_ = crossed ? 0.25f : 0.8f;
    if (crossed) {
        sys_->rumble(0.25f, 0.7f, 220);
        sys_->setLight(40, 170, 80);
        blip(784.f);
    } else {
        sys_->rumble(0.8f, 0.3f, 200);
        sys_->setLight(170, 30, 24);
        sys_->apu.noiseBurst(0.45f, 140.f, 0.28f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::hit() {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.85f;
    stun_ = 0.28f;
    shake_ = 1.f;
    vx_ = -170.f;
    face_ = 1;
    if (held_) {
        held_ = false;
        pouchOnDesk_ = false;
        float drop = px_ - 22.f;
        if (hole(drop)) drop = px_ + 16.f;
        if (hole(drop)) {
            finish(false, "THE POUCH FELL");
            return;
        }
        pouchX_ = drop;
    }
    sys_->apu.noiseBurst(0.4f, 480.f, 0.14f);
    sys_->rumble(0.5f, 0.2f, 90);
    sys_->setLight(170, 40, 30);
}

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (duck_) return art_.duck;
    if (std::abs(vx_) > 22.f) return (int(stepT_) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

bool Game::runPit(float a, float b, bool& right, bool& jump) const {
    if (px_ >= b + 10.f) return false;
    right = true;
    if (px_ > a - 36.f && px_ < a - 6.f && vx_ > 115.f) jump = true;
    return true;
}

bool Game::cross(float stopX, float clearX, float remain, bool& right) const {
    if (px_ >= clearX) return false;
    if (px_ >= stopX) {
        right = true;
        return true;
    }
    float eta = (clearX - px_) / kRun + 0.36f;
    if (remain < eta && px_ > stopX - 46.f) return true;
    right = true;
    return true;
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    left = right = jump = duck = false;
    if (stun_ > 0.f) return;
    if (!held_) {
        if (px_ < pouchX_ - 5.f) right = true;
        else if (px_ > pouchX_ + 5.f && !hole(px_ - 5.f)) left = true;
        return;
    }
    if (!onGround_) {
        jump = true;
        right = true;
        return;
    }
    if (runPit(kPits[0].a, kPits[0].b, right, jump)) return;
    float bridgeStop = kBridgeX - kBridgeHalf - 10.f;
    float bridgeClear = kBridgeX + kBridgeHalf + 16.f;
    float bridgeRem = openRemain(kBridgePeriod, kBridgeUp, kBridgePhase, bridgeLift_);
    if (cross(bridgeStop, bridgeClear, bridgeRem, right)) return;
    float doorStop = kDoorX - kDoorHalf - 16.f;
    float doorClear = kDoorX + kDoorHalf + 20.f;
    float doorRem = openRemain(kDoorPeriod, kDoorOpen, kDoorPhase, doorLift_);
    if (cross(doorStop, doorClear, doorRem, right)) return;
    if (runPit(kPits[1].a, kPits[1].b, right, jump)) return;
    if (px_ < kHookX + 34.f) {
        if (px_ > kHookX - 48.f) duck = true;
        right = true;
        return;
    }
    if (px_ < kGoal + 12.f) right = true;
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    if (stun_ > 0.f) {
        stun_ -= DT;
        left = right = jump = duck = false;
        vx_ = moveToward(vx_, 0.f, 280.f * DT);
    }
    if (inv_ > 0.f) inv_ -= DT;

    bool couldJump = onGround_ || coyote_ > 0.f;
    duck_ = duck && onGround_;
    if (stun_ <= 0.f) {
        if (right && !left) face_ = 1;
        else if (left && !right) face_ = -1;
        float target = 0.f;
        if (right) target += kRun;
        if (left) target -= kRun;
        if (left && right) target = 0.f;
        if (duck_) target = std::clamp(target, -kDuckSp, kDuckSp);
        float accel = onGround_ ? kAccel : kAccel * 0.4f;
        vx_ = moveToward(vx_, target, accel * DT);
    }

    vy_ = std::min(560.f, vy_ + kGrav * DT);
    float prevX = px_;
    px_ += vx_ * DT;
    py_ += vy_ * DT;

    if (jump && couldJump && !jumped_) {
        vy_ = kJumpV;
        py_ += vy_ * DT;
        onGround_ = false;
        duck_ = false;
        jumped_ = true;
        coyote_ = 0.f;
        blip(440.f);
    }

    if (doorBlocks(py_) && px_ > kDoorX - kDoorHalf && px_ < kDoorX + kDoorHalf) {
        if (prevX <= kDoorX) px_ = kDoorX - kDoorHalf - 0.5f;
        else px_ = kDoorX + kDoorHalf + 0.5f;
        vx_ = 0.f;
    }
    px_ = std::clamp(px_, 18.f, kWorld - 22.f);

    bool drop = hole(px_);
    if (!drop && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else if (drop && py_ >= kFloor) {
        onGround_ = false;
        if (py_ > kFloor + 54.f) {
            finish(false, held_ ? "THE POUCH FELL" : "MISSED THE FLOOR");
            return;
        }
    } else {
        onGround_ = false;
    }

    if (onGround_) {
        coyote_ = 0.10f;
        jumped_ = false;
        if (std::abs(vx_) > 22.f) stepT_ += DT * std::abs(vx_) / 26.f;
    } else if (!jumped_) {
        coyote_ = std::max(0.f, coyote_ - DT);
    }

    if (held_ && onGround_ && px_ > kGoal + 14.f) {
        px_ = kGoal + 14.f;
        vx_ = std::min(vx_, 0.f);
    }

    float tip = kFloor - 32.f - hookLift_ * 90.f;
    float head = py_ - bodyH();
    if (inv_ <= 0.f && std::abs(px_ - kHookX) < 16.f && head < tip - 2.f) hit();
    if (mode_ != Mode::Play) return;

    if (!held_ && onGround_ && stun_ <= 0.f && std::abs(px_ - pouchX_) < 28.f && std::abs(py_ - kFloor) < 8.f) {
        held_ = true;
        pouchOnDesk_ = false;
        blip(720.f);
        sys_->rumble(0.15f, 0.35f, 60);
    }
    if (held_ && onGround_ && px_ >= kGoal) finish(true, "CROSSED");
}

const char* Game::hint() const {
    if (!held_ && px_ > pouchX_ + 36.f) return "GO BACK FOR THE POUCH";
    if (!held_) return "TAKE THE POUCH";
    for (const Pit& p : kPits)
        if (px_ > p.a - 78.f && px_ < p.a) return "JUMP THE PIT";
    if (px_ > kBridgeX - kBridgeHalf - 70.f && px_ < kBridgeX - kBridgeHalf && bridgeLift_ < 0.9f)
        return "WAIT FOR THE BRIDGE";
    if (px_ > kDoorX - 80.f && px_ < kDoorX - kDoorHalf && doorLift_ < 0.75f) return "WAIT FOR THE DOOR";
    if (px_ > kHookX - 80.f && px_ < kHookX + 16.f && hookLift_ < 0.75f) return "DUCK THE HOOK";
    if (px_ > kGoal - 110.f) return "INTO THE BAY";
    return "CARRY IT ACROSS";
}

int Game::hintPal() const {
    const char* h = hint();
    if (h[0] == 'I') return PAL_GO;
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
    int hs = int(std::lround(-cam));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(3, 2, 1);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
        vdp.A.hscroll[y] = 0;
        vdp.A.vscroll[y] = 0;
        vdp.B.hscroll[y] = int16_t(hs);
        vdp.B.vscroll[y] = 0;
    }
    vdp.HUD.scroll(0, 0);
}

void Game::drawWorld(float cam) {
    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true,
                     bool shadow = false) { spr(m, wx - cam, foot, h, pal, flip, feet, shadow); };
    int fi = int(t_ * 9.f) & 1;
    int bob = int(t_ * 5.f) & 1;

    float doorFoot = kFloor - doorLift_ * 84.f;
    world(art_.door, kDoorX, doorFoot, 100, PAL_DOOR, false);

    float tip = kFloor - 32.f - hookLift_ * 90.f;
    world(art_.hook, kHookX, tip + 8.f, 28, PAL_STEEL, false, false);
    float cableH = std::max(8.f, tip - 46.f);
    world(art_.cable, kHookX, 46.f + cableH * 0.5f, cableH, PAL_STEEL, false, false);

    bool show = inv_ <= 0.f || (int(inv_ * 16.f) & 1) == 0;
    if (show && mode_ != Mode::Title) {
        if (held_) {
            float bobY = std::sin(stepT_ * 0.8f) * 1.5f;
            world(art_.pouch[bob], px_ + face_ * 14.f, py_ - 30.f + bobY, 20, PAL_POUCH, face_ < 0, false);
        }
        world(heroSprite(), px_, py_, 54, PAL_PLAYER, face_ < 0);
        if (py_ > kFloor - 40.f) world(art_.shadow, px_, kFloor + 3.f, 8, PAL_FX, false, true, true);
    }
    if (!held_) {
        float foot = pouchOnDesk_ ? kFloor - 14.f : kFloor - 1.f;
        world(art_.pouch[bob], pouchX_, foot, 22, PAL_POUCH, false);
    }

    float deckFoot = kFloor + 4.f + (1.f - bridgeLift_) * 34.f;
    world(art_.deck, kBridgeX, deckFoot, 18, PAL_STEEL, false);

    world(art_.signal, kBridgeX - kBridgeHalf - 8.f, kFloor - 58.f, 12, bridgeLift_ > 0.9f ? PAL_GO : PAL_ALERT, false,
          false);
    world(art_.signal, kDoorX, kFloor - 108.f, 12, doorLift_ > 0.75f ? PAL_GO : PAL_ALERT, false, false);
    world(art_.signal, kHookX + 28.f, 58.f, 12, hookLift_ > 0.75f ? PAL_GO : PAL_ALERT, false, false);

    const float lamps[] = {130.f, 330.f, 560.f, 880.f, 1100.f, 1320.f, 1508.f};
    for (float x : lamps) {
        world(art_.flame[fi], x, kFloor - 52.f, 12, PAL_FX, false, false);
        world(art_.lamp, x, kFloor, 48, PAL_STEEL, false);
    }
    world(art_.desk, 168.f, kFloor, 36, PAL_WOOD, false);
    const float crates[] = {248.f, 520.f, 860.f, 1064.f, 1304.f};
    for (float x : crates) world(art_.crate, x, kFloor, 28, PAL_WOOD, false);
    world(art_.barrel, 300.f, kFloor, 30, PAL_WOOD, false);
    world(art_.barrel, 820.f, kFloor, 30, PAL_WOOD, false);

    world(art_.mat, kGoal + 8.f, kFloor + 2.f, 10, PAL_GO, false);
    float pulse = 12.f;
    if (held_ && px_ > kGoal - 120.f) pulse += 3.f * std::sin(t_ * 8.f);
    world(art_.flame[fi], kGoal + 8.f, kFloor - 62.f, pulse, PAL_FX, false, false);
    world(art_.bay, kGoal + 28.f, kFloor, 70, PAL_WOOD, false);

    for (const Pit& p : kPits) {
        world(art_.lip, p.a, kFloor + 2.f, 18, PAL_STEEL, false);
        world(art_.lip, p.b, kFloor + 2.f, 18, PAL_STEEL, true);
        float mid = (p.a + p.b) * 0.5f;
        world(art_.hole, mid, kFloor + 46.f, 42, PAL_PIT, false);
    }
    world(art_.hole, kBridgeX, kFloor + 48.f, 44, PAL_PIT, false);

    world(art_.post, kHookX - 78.f, kFloor, 120, PAL_STEEL, false);
    world(art_.post, kHookX + 78.f, kFloor, 120, PAL_STEEL, false);
    world(art_.beam, kHookX, 52.f, 16, PAL_STEEL, false, false);
}

void Game::drawTitle() {
    int fi = int(t_ * 9.f) & 1;
    int bob = int(t_ * 5.f) & 1;
    text("S3 DEPOT POUC", 160, 16, 1.15f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160, 40, 1.0f, PAL_FX, 0);
    text("THEN IT IS DONE", 160, 62, 1.0f, PAL_GO, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 88, 1.0f, PAL_HUD, 0);

    spr(art_.beam, 250, 118, 14, PAL_STEEL, false, false, false);
    spr(art_.hook, 250, 132, 22, PAL_STEEL, false, false, false);
    spr(art_.door, 286, kFloor - 8, 78, PAL_DOOR, false, true, false);
    spr(art_.stand, 78, kFloor, 54, PAL_PLAYER, false, true, false);
    spr(art_.shadow, 78, kFloor + 2, 8, PAL_FX, false, true, true);
    spr(art_.desk, 148, kFloor, 34, PAL_WOOD, false, true, false);
    spr(art_.pouch[bob], 168, kFloor - 12, 22, PAL_POUCH, false, true, false);
    spr(art_.crate, 214, kFloor, 28, PAL_WOOD, false, true, false);
    spr(art_.lamp, 40, kFloor, 46, PAL_STEEL, false, true, false);
    spr(art_.flame[fi], 40, kFloor - 50, 11, PAL_FX, false, false, false);
    spr(art_.bay, 250, kFloor, 58, PAL_WOOD, false, true, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = cam_;
        if (shake_ > 0.f) cam += std::sin(t_ * 70.f) * shake_ * 2.5f;
    }
    backdrop(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160, 18, 1.05f, PAL_HUD, 0);
        text("IT IS DONE", 160, 40, 1.05f, PAL_GO, 0);
    } else if (mode_ == Mode::Lost) {
        text("NOT DONE", 160, 18, 1.15f, PAL_ALERT, 0);
        text(reason_, 160, 42, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 28, 1.2f, PAL_HUD, 0);
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
        static const float good[] = {523.f, 659.f, 784.f, 1046.f};
        static const float bad[] = {294.f, 220.f, 174.f, 130.f};
        fanT_ += DT;
        if (fanT_ > 0.14f) {
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
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 98.f : 73.f, 0.018f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 82.f, 0.012f);
    else sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.68f);
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
            if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.2f);
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
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
        audio();
        draw();
        return;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        t_ += DT;
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.2f);
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.4f);
    tickGates();

    bool left = false, right = false, jump = false, duck = false;
    if (bot_) {
        bot(left, right, jump, duck);
    } else if (pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        blip(280.f);
    } else {
        left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
        right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_TURBO);
        duck = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
    }
    if (mode_ == Mode::Play) stepPlay(left, right, jump, duck);

    float look = vx_ * 0.12f;
    float want = std::clamp(px_ + look - 120.f, 0.f, kWorld - float(gs::SCREEN_W));
    cam_ += (want - cam_) * std::min(1.f, DT * 7.f);

    if (mode_ == Mode::Won) sys.setLight(40, 180, 80);
    else if (mode_ == Mode::Lost) sys.setLight(170, 30, 24);
    else if (stun_ > 0.f) sys.setLight(170, 40, 30);
    else if (held_) sys.setLight(180, 120, 40);
    else sys.setLight(70, 90, 140);

    audio();
    draw();
}

}  // namespace pouc
