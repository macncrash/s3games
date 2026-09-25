#include "game/juggle.h"

#include "../version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace juggle {
namespace {

// Siteswap 3: a throw lasts three beats and the hands alternate, so three
// balls stay airborne. The minute starts when the third ball leaves the
// hand and resets if a catch misses the landing. The autopilot taps 10
// frames before each landing, inside the player's window.
constexpr int kBeat = 30;
constexpr int kFlight = 90;
constexpr int kEarly = 22;
constexpr int kBotEta = 10;
constexpr int kBuffer = 8;
constexpr int kPreview = 29;
constexpr int kGoal = 60 * 60;
constexpr float kHandY = 150.f;
constexpr float kArc = 100.f;
constexpr float kFloor = 176.f;
constexpr int kLeft = 0;
constexpr int kRight = 1;

const float kMel[4] = {523.25f, 659.25f, 783.99f, 659.25f};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int u, int v) { return int(std::lround(u + (v - u) * t)); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

}  // namespace

int Game::ballPal(int ball) const {
    static const int kP[3] = {PAL_BALL0, PAL_BALL1, PAL_BALL2};
    return kP[ball % 3];
}

void Game::bodyCenter(float& x, float& y) const {
    x = 160.f - (kAnchorX - kBodyW * 0.5f);
    y = kHandY - (kAnchorY - kBodyH * 0.5f);
}

void Game::handPos(int hand, float& x, float& y) const {
    float cx, cy;
    bodyCenter(cx, cy);
    float gx = hand == kRight ? kGloveR : kGloveL;
    x = cx + (gx - kBodyW * 0.5f);
    y = cy + (kGloveY - kBodyH * 0.5f);
}

void Game::arcPos(const Flight& f, float& x, float& y) const {
    float x0, y0, x1, y1;
    handPos(f.from, x0, y0);
    handPos(f.to, x1, y1);
    float u = float(pf_ - f.launch) / float(kFlight);
    if (u < 0.f) u = 0.f;
    if (u > 1.f) u = 1.f;
    x = x0 + (x1 - x0) * u;
    y = y0 - kArc * (4.f * u * (1.f - u));
}

void Game::heldPos(int ball, float& x, float& y) const {
    int hand = (ball % 2 == 0) ? kRight : kLeft;
    int slot = 0;
    for (int b = 0; b < ball; b++) {
        if (live_[b]) continue;
        int h = (b % 2 == 0) ? kRight : kLeft;
        if (h == hand) slot++;
    }
    handPos(hand, x, y);
    float side = hand == kRight ? -1.f : 1.f;
    x += side * slot * 10.f;
    y -= slot * 16.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    if (s.w < 1) s.w = 1;
    if (s.h < 1) s.h = 1;
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    const float gw = 16.f * scale;
    float width = s.empty() ? 0.f : float(s.size() - 1) * adv + gw;
    x -= width * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::panel() {
    for (int r = 24; r <= 27; r++)
        for (int c = 0; c < 40; c++) sys_->vdp.HUD.set(c, r, gs::entry(art_.font[0], PAL_HUD));
}

void Game::wood() {
    sys_->apu.tone(0, 196.f, 0.05f);
    toneLeft_[0] = 4;
}

void Game::melody() {
    sys_->apu.tone(1, kMel[mel_ & 3], 0.075f);
    toneLeft_[1] = 8;
    mel_++;
}

void Game::blip(bool high) {
    sys_->apu.tone(1, high ? 880.f : 440.f, 0.05f);
    toneLeft_[1] = 4;
}

void Game::ageTones() {
    for (int c = 0; c < 2; c++) {
        if (toneLeft_[c] > 0 && --toneLeft_[c] == 0) sys_->apu.tone(c, 0, 0);
    }
}

void Game::burstAt(float x, float y) {
    for (Burst& b : burst_) {
        if (b.on) continue;
        b.x = x;
        b.y = y;
        b.life = 12;
        b.on = true;
        return;
    }
}

void Game::ageBursts() {
    for (Burst& b : burst_) {
        if (!b.on) continue;
        if (--b.life <= 0) b.on = false;
    }
}

void Game::stepFan() {
    if (fanStep_ < 0 || fanStep_ > 3) return;
    if (fanWait_ % 8 == 0) {
        static const float n[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
        sys_->apu.tone(0, n[fanStep_], 0.09f);
        sys_->apu.tone(1, n[fanStep_] * 0.5f, 0.045f);
        toneLeft_[0] = toneLeft_[1] = 12;
        fanStep_++;
    }
    fanWait_++;
}

void Game::lights() {
    if (mode_ == Mode::Win) sys_->setLight(40, 200, 90);
    else if (mode_ == Mode::Drop || mode_ == Mode::Over) sys_->setLight(200, 40, 36);
    else if (mode_ == Mode::Count) sys_->setLight(200, 200, 220);
    else sys_->setLight(200, 150, 40);
}

void Game::beginPattern() {
    pf_ = 0;
    next_ = 0;
    airFrames_ = 0;
    mel_ = 0;
    tapAt_[0] = tapAt_[1] = -100000;
    stick_ = 0;
    for (int i = 0; i < 3; i++) {
        live_[i] = false;
        fall_[i].on = false;
    }
}

void Game::readTaps() {
    const gs::Pad& p = sys_->pad;
    bool tapL = p.pressed(gs::BTN_LEFT) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_X);
    bool tapR = p.pressed(gs::BTN_RIGHT) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C);
    float ax = p.axisX;
    if (ax < -0.55f && stick_ >= -0.55f) tapL = true;
    if (ax > 0.55f && stick_ <= 0.55f) tapR = true;
    stick_ = ax;
    if (tapL) tapAt_[kLeft] = pf_;
    if (tapR) tapAt_[kRight] = pf_;
}

void Game::updatePattern(bool scored) {
    const bool autoCatch = !scored || bot_;
    if (scored && !autoCatch) readTaps();

    for (int b = 0; b < 3; b++) {
        if (!live_[b]) continue;
        Flight& f = flight_[b];
        if (f.caught) continue;
        int eta = f.land - pf_;
        if (eta < 0 || eta > kEarly) continue;
        bool hit = false;
        if (autoCatch) {
            hit = eta == kBotEta;
        } else if (tapAt_[f.to] >= 0 && pf_ - tapAt_[f.to] <= kBuffer && pf_ >= tapAt_[f.to]) {
            hit = true;
            tapAt_[f.to] = -100000;
        }
        if (!hit) continue;
        f.caught = true;
        float x, y;
        handPos(f.to, x, y);
        burstAt(x, y - 6.f);
        melody();
        if (scored && !bot_) sys_->rumble(0.06f, 0.16f, 18);
    }

    bool missed = false;
    if (next_ < 3) {
        while (next_ < 3 && pf_ >= next_ * kBeat) {
            int i = next_++;
            int ball = i % 3;
            Flight& f = flight_[ball];
            f.ball = ball;
            f.from = (i & 1) ? kLeft : kRight;
            f.to = f.from ^ 1;
            f.launch = i * kBeat;
            f.land = f.launch + kFlight;
            f.caught = false;
            live_[ball] = true;
            wood();
        }
    } else if (pf_ >= next_ * kBeat) {
        int ball = next_ % 3;
        if (live_[ball] && flight_[ball].caught && flight_[ball].land == next_ * kBeat) {
            int i = next_++;
            Flight& f = flight_[ball];
            f.ball = ball;
            f.from = (i & 1) ? kLeft : kRight;
            f.to = f.from ^ 1;
            f.launch = i * kBeat;
            f.land = f.launch + kFlight;
            f.caught = false;
            live_[ball] = true;
            wood();
        } else {
            missed = true;
        }
    }

    if (missed) {
        if (!scored) {
            beginPattern();
            return;
        }
        startDrop();
        return;
    }

    if (scored) {
        int flying = 0;
        for (int b = 0; b < 3; b++)
            if (live_[b]) flying++;
        if (flying == 3) {
            airFrames_++;
            if (airFrames_ >= kGoal) {
                enterWin();
                return;
            }
        }
    }
    pf_++;
}

void Game::updateCount() {
    const int steps = fullCount_ ? 4 : 2;
    const int limit = steps * kBeat;
    if (countF_ % kBeat == 0) wood();
    countF_++;
    if (countF_ >= limit) mode_ = Mode::Play;
}

void Game::startDrop() {
    for (int b = 0; b < 3; b++) {
        Fall& f = fall_[b];
        f.ball = b;
        f.on = true;
        if (live_[b]) arcPos(flight_[b], f.x, f.y);
        else heldPos(b, f.x, f.y);
        f.vy = -1.8f;
        f.vx = (b - 1) * 0.45f;
        live_[b] = false;
    }
    airFrames_ = 0;
    drops_++;
    dropF_ = 0;
    mode_ = Mode::Drop;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    toneLeft_[0] = toneLeft_[1] = 0;
    sys_->apu.noiseBurst(0.5f, 640.f, 0.32f);
    sys_->rumble(0.7f, 0.9f, 160);
}

void Game::updateDrop() {
    for (Fall& f : fall_) {
        if (!f.on) continue;
        f.vy += 0.42f;
        f.x += f.vx;
        f.y += f.vy;
        if (f.y > kFloor) {
            f.y = kFloor;
            if (f.vy > 0.f) f.vy *= -0.4f;
            f.vx *= 0.75f;
            if (std::fabs(f.vy) < 0.7f) f.vy = 0.f;
        }
        if (f.x < 16.f) f.x = 16.f;
        if (f.x > 304.f) f.x = 304.f;
    }
    dropF_++;
    if (dropF_ < 72) return;
    if (drops_ >= 3) {
        mode_ = Mode::Over;
        over_ = true;
        return;
    }
    beginPattern();
    fullCount_ = false;
    countF_ = 0;
    mode_ = Mode::Count;
}

void Game::enterWin() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    fanStep_ = 0;
    fanWait_ = 0;
    std::printf("three in the air for a minute\n");
    std::fflush(stdout);
    sys_->rumble(0.45f, 0.75f, 420);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(3, 1, 6));
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.22f, 0.12f);
    sys.apu.tone(2, 130.81f, 0.02f);
    for (Burst& b : burst_) b.on = false;
    drops_ = 0;
    won_ = false;
    over_ = false;
    anim_ = 0;
    fanStep_ = -1;
    if (bot_) {
        mode_ = Mode::Play;
        beginPattern();
    } else {
        mode_ = Mode::Title;
        beginPattern();
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_++;
    ageTones();
    ageBursts();
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        updatePattern(false);
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            drops_ = 0;
            won_ = false;
            over_ = false;
            beginPattern();
            fullCount_ = true;
            countF_ = 0;
            mode_ = Mode::Count;
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Count) {
        updateCount();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            blip(false);
            mode_ = Mode::Pause;
        } else {
            updatePattern(true);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            mode_ = Mode::Play;
        } else if (pad.pressed(gs::BTN_MODE)) {
            blip(false);
            drops_ = 0;
            won_ = false;
            over_ = false;
            beginPattern();
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Drop) {
        updateDrop();
    } else if (mode_ == Mode::Over) {
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            drops_ = 0;
            won_ = false;
            over_ = false;
            beginPattern();
            mode_ = Mode::Title;
        }
    }

    if (mode_ == Mode::Win) stepFan();
    draw();
    lights();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const uint16_t skyTop = gs::rgb4(2, 1, 6);
    const uint16_t skyMid = gs::rgb4(7, 2, 10);
    const uint16_t skyWarm = gs::rgb4(13, 6, 5);
    const uint16_t woodA = gs::rgb4(9, 5, 2);
    const uint16_t woodB = gs::rgb4(7, 4, 2);
    const uint16_t seam = gs::rgb4(4, 2, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 78) v.lineBackdrop[y] = lerpC(skyTop, skyMid, y / 78.f);
        else if (y < 148) v.lineBackdrop[y] = lerpC(skyMid, skyWarm, (y - 78) / 70.f);
        else if (y < 192) {
            int rel = y - 148;
            v.lineBackdrop[y] = (rel % 7 == 6) ? seam : ((rel / 7) % 2 ? woodA : woodB);
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 1, 4);
        }
    }
}

void Game::drawActors(float bob) {
    float cx, cy;
    bodyCenter(cx, cy);

    // Back of the stage first in the list would cover the juggler.
    // Earlier sprites win, so the performer and the balls go in before the set.
    int heat[2] = {0, 0};
    if (mode_ == Mode::Play || mode_ == Mode::Title || mode_ == Mode::Pause || mode_ == Mode::Win) {
        for (int b = 0; b < 3; b++) {
            if (!live_[b] || flight_[b].caught) continue;
            int eta = flight_[b].land - pf_;
            if (eta < 0 || eta > kPreview) continue;
            int level = eta <= kEarly ? 2 : 1;
            int hand = flight_[b].to;
            if (level > heat[hand]) heat[hand] = level;
        }
    }
    for (int hand = 0; hand < 2; hand++) {
        if (!heat[hand]) continue;
        float x, y;
        handPos(hand, x, y);
        float rh = heat[hand] == 2 ? 28.f + 3.f * std::sin(anim_ * 0.45f) : 20.f;
        spr(art_.ring, x, y + bob, rh, heat[hand] == 2 ? PAL_GOLD : PAL_DIM, false, false);
    }

    for (const Burst& b : burst_) {
        if (!b.on) continue;
        spr(art_.star, b.x, b.y + bob, 6.f + b.life, PAL_GOLD, false, false);
    }

    auto drawBall = [&](int ball, float x, float y, int spin) {
        spr(art_.shadow, x, 178.f, 10.f, PAL_PROP, false, true);
        spr(art_.ball[spin & 3], x, y, 30.f, ballPal(ball), false, false);
    };

    if (mode_ == Mode::Drop || mode_ == Mode::Over) {
        for (const Fall& f : fall_) {
            if (!f.on) continue;
            int spin = (anim_ / 3 + f.ball) & 3;
            drawBall(f.ball, f.x, f.y + bob, spin);
        }
    } else {
        int order[3] = {0, 1, 2};
        std::sort(order, order + 3, [&](int a, int c) {
            float ax, ay, bx, by;
            bool la = live_[a], lb = live_[c];
            if (la) arcPos(flight_[a], ax, ay);
            else heldPos(a, ax, ay);
            if (lb) arcPos(flight_[c], bx, by);
            else heldPos(c, bx, by);
            return ay > by;
        });
        for (int i = 0; i < 3; i++) {
            int b = order[i];
            float x, y;
            int spin;
            if (live_[b]) {
                arcPos(flight_[b], x, y);
                spin = ((pf_ - flight_[b].launch) / 4 + b) & 3;
            } else {
                heldPos(b, x, y);
                spin = b;
            }
            // Shadows sit on the floor; skip them for a tucked second ball.
            if (live_[b] || y > kHandY - 8.f) spr(art_.shadow, x, 178.f, live_[b] ? 8.f + (y - 50.f) * 0.04f : 11.f, PAL_PROP, false, true);
            spr(art_.ball[spin], x, y + bob, 30.f, ballPal(b), false, false);
        }
    }

    spr(art_.body, cx, cy + bob, float(kBodyH), PAL_BODY, false, false);
    spr(art_.pool, 160.f, 172.f, 26.f, PAL_FX, false, false);
    spr(art_.lamp, 70.f, 162.f, 20.f, PAL_PROP, false, false);
    spr(art_.lamp, 250.f, 162.f, 20.f, PAL_PROP, true, false);
    spr(art_.curtain, 28.f, 108.f, 148.f, PAL_PROP, false, false);
    spr(art_.curtain, 292.f, 108.f, 148.f, PAL_PROP, true, false);
    spr(art_.valance, 160.f, 18.f, 36.f, PAL_PROP, false, false);

    static const float kStars[6][2] = {{66, 50}, {80, 68}, {246, 48}, {260, 66}, {58, 84}, {268, 86}};
    for (const auto& s : kStars) spr(art_.star, s[0], s[1] + std::sin(anim_ * 0.04f + s[0]) * 0.6f, 7.f, PAL_GOLD, false, false);

    if (mode_ == Mode::Win) {
        for (int i = 0; i < 12; i++) {
            float x = std::fmod(18.f + i * 41.f + anim_ * (0.7f + (i % 3) * 0.2f), 320.f);
            float y = std::fmod(40.f + i * 29.f + anim_ * (1.3f + (i % 4) * 0.15f), 168.f);
            spr(art_.star, x, y, 8.f + (i % 3) * 3.f, (i & 1) ? PAL_GOLD : PAL_RED, false, false);
        }
    }
}

void Game::drawHud() {
    panel();
    if (mode_ == Mode::Title) {
        hudC(24, "THREE UP FOR A MINUTE", PAL_GOLD);
        hudC(25, "TAP THE GLOWING HAND", PAL_HUD);
        hud(1, 26, "LEFT ARROW OR Z", PAL_HUD);
        hud(22, 26, "RIGHT ARROW OR X", PAL_HUD);
        hudC(27, "START", PAL_GOLD);
        std::string ver = S3_VERSION_STRING;
        hud(40 - int(ver.size()), 27, ver, PAL_DIM);
        return;
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Win || mode_ == Mode::Count || mode_ == Mode::Drop ||
        mode_ == Mode::Over) {
        int sec = airFrames_ / 60;
        if (sec > 60) sec = 60;
        char buf[24];
        std::snprintf(buf, sizeof buf, "AIR %d:%02d", sec / 60, sec % 60);
        hud(1, 0, buf, mode_ == Mode::Win ? PAL_GREEN : PAL_GOLD);
        std::snprintf(buf, sizeof buf, "MISS %d", drops_);
        hud(40 - int(std::strlen(buf)), 0, buf, drops_ ? PAL_RED : PAL_DIM);
        int filled = airFrames_ * 24 / kGoal;
        if (filled > 24) filled = 24;
        if (mode_ == Mode::Win) filled = 24;
        std::string bar(24, '-');
        for (int i = 0; i < filled; i++) bar[size_t(i)] = '=';
        hud(8, 1, bar, filled >= 24 ? PAL_GREEN : PAL_GOLD);
    }

    int heat[2] = {0, 0};
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        for (int b = 0; b < 3; b++) {
            if (!live_[b] || flight_[b].caught) continue;
            int eta = flight_[b].land - pf_;
            if (eta < 0 || eta > kEarly) continue;
            heat[flight_[b].to] = 1;
        }
    }

    if (mode_ == Mode::Count) {
        if (fullCount_) {
            int i = countF_ / kBeat;
            hudC(24, i >= 3 ? "GO" : "READY", PAL_GOLD);
        } else {
            hudC(24, "UP", PAL_GOLD);
        }
        hudC(25, "SAME BEAT", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        if (heat[kLeft]) hud(2, 24, "TAP", PAL_GOLD);
        if (heat[kRight]) hud(34, 24, "TAP", PAL_GOLD);
        if (!heat[kLeft] && !heat[kRight]) hudC(24, "KEEP THREE UP", PAL_DIM);
        hud(2, 26, "Z LEFT", heat[kLeft] ? PAL_GOLD : PAL_DIM);
        hud(30, 26, "X RIGHT", heat[kRight] ? PAL_GOLD : PAL_DIM);
        hudC(27, "START PAUSES", PAL_DIM);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_GOLD);
        hudC(25, "START CONTINUES", PAL_HUD);
        hudC(26, "ESC TO TITLE", PAL_DIM);
    } else if (mode_ == Mode::Drop) {
        hudC(24, "DROPPED", PAL_RED);
        hudC(25, drops_ >= 3 ? "THAT WAS THE THIRD" : "CLOCK RESET", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(24, "DROPPED", PAL_RED);
        hudC(25, "THREE MISSES", PAL_HUD);
        hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(24, "ONE MINUTE", PAL_GREEN);
        hudC(25, "THREE STILL UP", PAL_GOLD);
        hudC(27, "START", PAL_HUD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const float bob = std::sin(anim_ * 0.08f) * 1.1f;
    const char* banner = nullptr;
    float bscale = 1.f;
    int bpal = PAL_HUD;
    if (mode_ == Mode::Title) {
        banner = "S3 JUGGLE";
        bscale = 1.05f;
        bpal = PAL_GOLD;
    } else if (mode_ == Mode::Count) {
        if (fullCount_) {
            static const char* kLab[4] = {"3", "2", "1", "GO"};
            int i = countF_ / kBeat;
            if (i < 0) i = 0;
            if (i > 3) i = 3;
            banner = kLab[i];
            bscale = (i == 3) ? 1.3f : 1.6f;
        } else {
            banner = "UP";
            bscale = 1.4f;
        }
        bpal = PAL_GOLD;
    } else if (mode_ == Mode::Drop) {
        banner = "DROP";
        bscale = 1.4f;
        bpal = PAL_RED;
    } else if (mode_ == Mode::Over) {
        banner = "DROPPED";
        bscale = 1.05f;
        bpal = PAL_RED;
    } else if (mode_ == Mode::Win) {
        banner = "ONE MINUTE";
        bscale = 1.f;
        bpal = PAL_GREEN;
    } else if (mode_ == Mode::Pause) {
        banner = "PAUSE";
        bscale = 1.3f;
        bpal = PAL_GOLD;
    }
    if (banner) text(banner, 160.f, 16.f, bscale, bpal);

    drawActors(bob);
    drawHud();
}

}  // namespace juggle
