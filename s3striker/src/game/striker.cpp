#include "striker.h"

#include "../version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace striker {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float TAU = 6.2831853f;
constexpr float PERIOD = 1.9f;
constexpr float BELL_AT = 0.92f;
constexpr float GRAV = 3.6f;
constexpr float FOOT_SX = 112.f;
constexpr float FOOT_SY = 200.f;
constexpr float MAN_DEST = 196.f;
constexpr float TOWER_X = 208.f;
constexpr float TOWER_TOP = 16.f;
constexpr float TOWER_H = 172.f;

struct Place {
    float slotX, slotTop, slotBot;
    float anvilX, anvilY;
    float bellX, bellY;
    float towerLeft, towerTop;
};

Place placeOf(const Art& a) {
    Place p;
    float sc = TOWER_H / float(a.tower.h);
    p.towerLeft = TOWER_X - a.tower.w * sc * 0.5f;
    p.towerTop = TOWER_TOP;
    p.slotX = p.towerLeft + a.towerSlotX * sc;
    p.slotTop = TOWER_TOP + a.towerSlotTop * sc;
    p.slotBot = TOWER_TOP + a.towerSlotBot * sc;
    p.anvilX = TOWER_X;
    p.anvilY = TOWER_TOP + a.towerAnvilY * sc;
    p.bellX = p.slotX;
    p.bellY = TOWER_TOP + 6.f;
    return p;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto c = [&](int u, int v) { return int(std::lround(u + (v - u) * t)); };
    return gs::rgb4(c(ar, br), c(ag, bg), c(ab, bb));
}

gs::FMPatch calliope() {
    gs::FMPatch p;
    p.alg = 5;
    p.vol = 0.11f;
    p.echo = 0.25f;
    p.op[0] = {1, 0.9f, 0.02f, 0.16f, 0.75f, 0.14f};
    p.op[1] = {2, 0.32f, 0.02f, 0.22f, 0.45f, 0.18f};
    p.op[2] = {3, 0.18f, 0.03f, 0.2f, 0.35f, 0.2f};
    p.op[3] = {1, 0.12f, 0.02f, 0.28f, 0.25f, 0.22f};
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.vol = 0.2f;
    p.echo = 0.45f;
    p.op[0] = {3.5f, 0.5f, 0.001f, 0.35f, 0.0f, 0.22f};
    p.op[1] = {1, 0.85f, 0.001f, 1.05f, 0.0f, 0.65f};
    p.op[2] = {7, 0.26f, 0.001f, 0.16f, 0.0f, 0.25f};
    p.op[3] = {2, 0.38f, 0.001f, 0.75f, 0.0f, 0.5f};
    return p;
}

gs::FMPatch drumPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.vol = 0.32f;
    p.op[0] = {1, 1, 0.001f, 0.07f, 0, 0.04f};
    p.op[1] = {0.5f, 1, 0.001f, 0.1f, 0, 0.06f};
    p.op[2] = {2.2f, 0.25f, 0.001f, 0.05f, 0, 0.04f};
    p.op[3] = {1, 0.35f, 0.001f, 0.09f, 0, 0.05f};
    return p;
}

}  // namespace

float Game::meter() const { return 0.5f * (1.f - std::cos(phase_)); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Ready || mode_ == Mode::Strike) return 1;
    if (mode_ == Mode::Bell) return 3;
    if (mode_ == Mode::Win || mode_ == Mode::Lose) return 4;
    return 2;
}

int Game::pose() const {
    if (mode_ == Mode::Strike) {
        float u = std::min(1.f, modeT_ / 0.22f);
        u = u * u;
        return std::clamp(int(std::lround(strikeFrom_ + (6 - strikeFrom_) * u)), 0, 6);
    }
    if (mode_ == Mode::Rise || mode_ == Mode::Fall || mode_ == Mode::Bell) return 6;
    if (mode_ == Mode::Between || mode_ == Mode::Win || mode_ == Mode::Lose) return modeT_ < 0.28f ? 6 : 0;
    return std::clamp(int(meter() * 5.99f), 0, 5);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::anchor(const gs::Mipped& m, float ax, float ay, float lx, float ly, float destH, int pal) {
    if (destH < 1.5f || m.h < 1) return;
    float sc = destH / float(m.h);
    float dw = float(m.w) * sc;
    float left = ax - lx * sc;
    float top = ay - ly * sc;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(dw)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::lround(left));
    s.y = int16_t(std::lround(top));
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::meterBar(int row) {
    const int cols = 32;
    const int x0 = 4;
    float m = (mode_ == Mode::Title || mode_ == Mode::Ready) ? meter() : power_;
    int mark = std::clamp(int(std::lround(m * (cols - 1))), 0, cols - 1);
    int gold = int(std::ceil(BELL_AT * (cols - 1)));
    for (int i = 0; i < cols; i++) {
        char ch[2] = {char(i == mark ? 'O' : i >= gold ? '=' : '-'), 0};
        int pal = i >= gold ? PAL_GOLD : PAL_HUD;
        if (i == mark) pal = m >= BELL_AT ? PAL_GREEN : PAL_RED;
        hud(x0 + i, row, ch, pal);
    }
}

void Game::blip(bool high) {
    sys_->apu.tone(1, high ? 980.f : 520.f, 0.07f);
    beepT_ = 0.06f;
}

void Game::thunk() {
    sys_->apu.noiseBurst(0.62f, 160.f, 0.14f);
    sys_->apu.setPatch(3, drumPatch());
    sys_->apu.keyOn(3, 62.f, 0.4f);
    sys_->rumble(0.7f, 0.35f, 90);
}

void Game::chime() {
    gs::FMPatch bell = bellPatch();
    const float notes[] = {784.f, 1175.f, 1568.f};
    for (int i = 0; i < 3; i++) {
        sys_->apu.setPatch(4 + i, bell);
        sys_->apu.setPan(4 + i, (i - 1) * 0.55f);
        sys_->apu.keyOn(4 + i, notes[i], 0.22f);
    }
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::whistle(float h) {
    if (h < 0.03f) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    sys_->apu.tone(0, 200.f + std::clamp(h, 0.f, 1.f) * 920.f, 0.045f);
}

void Game::resetAttempt() {
    swings_ = 3;
    used_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    winSwing_ = 0;
    bestMark_ = 0;
    power_ = 0;
    apex_ = 0;
    puck_ = 0;
    puckV_ = 0;
    phase_ = 0;
    modeT_ = 0;
    bellT_ = 0;
    flashT_ = 0;
    shake_ = 0;
    strikeFrom_ = 0;
    fanStep_ = -1;
    bits_.clear();
    mode_ = Mode::Ready;
    sys_->apu.tone(0, 0, 0);
    sys_->setLight(180, 40, 50);
}

void Game::launch(float power) {
    if (mode_ != Mode::Ready || swings_ <= 0) return;
    power_ = std::clamp(power, 0.f, 1.f);
    used_++;
    swings_--;
    strikeFrom_ = std::clamp(int(power_ * 5.99f), 0, 5);
    bool will = power_ >= BELL_AT;
    apex_ = will ? 1.12f : (power_ / BELL_AT) * 0.97f;
    puck_ = 0;
    puckV_ = std::sqrt(2.f * GRAV * std::max(0.04f, apex_));
    mode_ = Mode::Strike;
    modeT_ = 0;
    sys_->apu.noiseBurst(0.18f, 2400.f, 0.06f);
}

void Game::ring() {
    if (rung_) return;
    rung_ = true;
    won_ = true;
    winSwing_ = used_;
    bestMark_ = 100;
    puck_ = 1.f;
    puckV_ = 0;
    mode_ = Mode::Bell;
    bellT_ = 0;
    modeT_ = 0;
    sys_->apu.tone(0, 0, 0);
    chime();
    sys_->rumble(0.45f, 0.95f, 220);
    sys_->setLight(255, 190, 40);
    Place p = placeOf(art_);
    for (int i = 0; i < 26; i++) {
        rng_ = rng_ * 1664525u + 1013904223u;
        float u = (rng_ >> 8) * (1.f / 16777216.f);
        rng_ = rng_ * 1664525u + 1013904223u;
        float v = (rng_ >> 8) * (1.f / 16777216.f);
        float ang = u * TAU;
        float sp = 28.f + v * 110.f;
        Bit b;
        b.x = p.bellX;
        b.y = p.bellY + 10.f;
        b.vx = std::cos(ang) * sp;
        b.vy = std::sin(ang) * sp - 50.f;
        b.life = 0.7f + v * 0.7f;
        b.kind = int(u * 3.f);
        bits_.push_back(b);
    }
}

void Game::update(float dt) {
    t_ += dt;
    if (beepT_ > 0) {
        beepT_ -= dt;
        if (beepT_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (flashT_ > 0) flashT_ -= dt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt * 2.2f);

    if (mode_ == Mode::Title || mode_ == Mode::Ready) {
        phase_ += dt * (TAU / PERIOD);
        if (phase_ > TAU * 8.f) phase_ -= TAU * 8.f;
        rising_ = std::sin(phase_) > 0.02f;
    }

    // The autopilot's first swing is a real miss. Only the bell wins.
    if (bot_ && mode_ == Mode::Ready && modeT_ > 0.28f) {
        float m = meter();
        if (used_ == 0) {
            if (rising_ && m >= 0.70f && m <= 0.78f) launch(m);
        } else if (rising_ && m >= 0.97f) {
            launch(m);
        }
    }

    if (mode_ == Mode::Strike) {
        modeT_ += dt;
        if (modeT_ >= 0.22f) {
            mode_ = Mode::Rise;
            modeT_ = 0;
            flashT_ = 0.16f;
            shake_ = 1.f;
            thunk();
        }
    } else if (mode_ == Mode::Rise) {
        modeT_ += dt;
        puck_ += puckV_ * dt;
        puckV_ -= GRAV * dt;
        whistle(puck_);
        bool will = power_ >= BELL_AT;
        if (will && puck_ >= 1.f) {
            ring();
        } else if (!will && (puck_ >= apex_ || puckV_ <= 0.f)) {
            puck_ = std::min(puck_, apex_);
            puckV_ = 0;
            mode_ = Mode::Fall;
            modeT_ = 0;
            sys_->apu.noiseBurst(puck_ > 0.8f ? 0.28f : 0.16f, 500.f, 0.08f);
        }
        if (!rung_) bestMark_ = std::max(bestMark_, int(std::lround(std::min(1.f, puck_) * 100.f)));
    } else if (mode_ == Mode::Fall) {
        modeT_ += dt;
        puck_ += puckV_ * dt;
        puckV_ -= GRAV * dt;
        whistle(0);
        if (puck_ <= 0.f) {
            puck_ = 0;
            puckV_ = 0;
            bestMark_ = std::max(bestMark_, int(std::lround(std::min(1.f, apex_) * 100.f)));
            mode_ = swings_ > 0 ? Mode::Between : Mode::Lose;
            modeT_ = 0;
        }
    } else if (mode_ == Mode::Between) {
        modeT_ += dt;
        if (modeT_ > 0.8f) {
            mode_ = Mode::Ready;
            phase_ = 0;
            rising_ = true;
            modeT_ = 0;
        }
    } else if (mode_ == Mode::Bell) {
        bellT_ += dt;
        if (bellT_ > 1.2f) {
            mode_ = Mode::Win;
            modeT_ = 0;
        }
    } else if (mode_ == Mode::Ready || mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) {
        modeT_ += dt;
    }

    if ((mode_ == Mode::Win || mode_ == Mode::Lose) && bot_) over_ = true;

    for (Bit& b : bits_) {
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.vy += 220.f * dt;
        b.life -= dt;
    }

    if (mode_ != Mode::Rise && mode_ != Mode::Fall && mode_ != Mode::Bell && mode_ != Mode::Win) {
        noteT_ += dt;
        if (noteT_ >= 0.2f) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f, 659.f, 880.f, 698.f};
            sys_->apu.keyOn(7, notes[noteI_ & 7], 0.07f);
            noteI_++;
            noteT_ = 0;
        }
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 1319.f};
        fanT_ += dt;
        if (fanT_ > 0.14f) {
            if (fanStep_ < 5) sys_->apu.keyOn(2, notes[fanStep_], 0.16f);
            else sys_->apu.keyOff(2);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > 7) fanStep_ = -1;
        }
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();

    const uint16_t skyTop = gs::rgb4(2, 1, 6);
    const uint16_t skyMid = gs::rgb4(9, 2, 8);
    const uint16_t skyHor = gs::rgb4(15, 8, 3);
    const uint16_t dirt = gs::rgb4(3, 2, 2);
    const int horizon = 168;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < horizon / 2) v.lineBackdrop[y] = lerpC(skyTop, skyMid, y / float(horizon / 2));
        else if (y < horizon) v.lineBackdrop[y] = lerpC(skyMid, skyHor, (y - horizon / 2) / float(horizon / 2));
        else v.lineBackdrop[y] = dirt;
    }

    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(t_ * 73.f) * shake_ * 3.5f;
        shy = std::cos(t_ * 61.f) * shake_ * 2.2f;
    }
    Place p = placeOf(art_);
    float puckY = p.slotBot + (p.slotTop - p.slotBot) * std::clamp(puck_, 0.f, 1.f);

    if (mode_ == Mode::Title) text("S3 STRIKER", 160 + shx, 20, 0.92f, PAL_GOLD);
    if (mode_ == Mode::Bell || mode_ == Mode::Win) text("RING", 108, 34, 1.35f, PAL_GOLD);
    if (mode_ == Mode::Lose) text("QUIET", 100, 36, 1.15f, PAL_RED);
    if (mode_ == Mode::Between) text(bestMark_ >= 80 ? "CLOSE" : "SHORT", 96, 40, 0.8f, PAL_RED);

    for (const Bit& b : bits_) {
        if (b.life <= 0) continue;
        spr(art_.spark, b.x + shx, b.y + shy, 5.f + b.kind * 2.f, PAL_FX, false);
    }
    if (flashT_ > 0) spr(art_.flash, p.anvilX + shx, p.anvilY + shy, 30, PAL_FX, false);

    spr(art_.puck, p.slotX + shx, puckY + shy, 18, PAL_BRASS, false);

    int bframe = 1;
    float sway = 0;
    if (mode_ == Mode::Bell || mode_ == Mode::Win) {
        float w = mode_ == Mode::Bell ? bellT_ : modeT_ + 1.2f;
        sway = std::sin(w * 42.f) * 6.f * std::exp(-w * 0.7f);
        int step = int(w * 16.f);
        bframe = (step % 3 == 0) ? 0 : (step % 3 == 1) ? 2 : 1;
    }
    spr(art_.bell[bframe], p.bellX + sway + shx, p.bellY + shy, 34, PAL_BRASS, sway < 0);

    anchor(art_.man[pose()], FOOT_SX + shx, FOOT_SY + shy, float(art_.footX), float(art_.footY), MAN_DEST, PAL_MAN);
    spr(art_.tower, TOWER_X + shx, TOWER_TOP + TOWER_H * 0.5f + shy, TOWER_H, PAL_WOOD, false);

    spr(art_.person[0], 34, 176, 52, PAL_PROP, false);
    spr(art_.person[1], 58, 180, 44, PAL_PROP, true);
    spr(art_.person[2], 300, 174, 56, PAL_PROP, true);

    for (int i = 0; i < (mode_ == Mode::Title ? 3 : swings_); i++)
        spr(art_.bulb, 18.f + i * 14.f, 186, 12, PAL_PROP, false);

    spr(art_.ground, 160, 200, 48, PAL_WOOD, false);
    spr(art_.bunting, 160, 30, 20, PAL_PROP, false);
    spr(art_.lights[int(t_ * 3.f) & 1], 160, 16, 16, PAL_PROP, false);
    spr(art_.tent, 78, 132, 90, PAL_PROP, false);
    spr(art_.moon, 292, 42, 22, PAL_FX, false);

    static const float stars[][2] = {{20, 18}, {48, 40}, {140, 16}, {188, 48}, {250, 22}, {308, 70}, {230, 58}, {96, 28}};
    for (const auto& s : stars) {
        float tw = 0.65f + 0.35f * std::sin(t_ * 3.f + s[0]);
        spr(art_.star, s[0], s[1], 4.f + tw * 3.f, PAL_FX, false);
    }

    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(21, "THREE SWINGS", PAL_HUD);
        hudC(22, "RING THE BELL", PAL_GOLD);
        hudC(23, "Z, C, OR SPACE IN THE GOLD", PAL_HUD);
        if (int(t_ * 2) % 2 == 0) hudC(24, "PRESS START", PAL_GREEN);
        hud(1, 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(8, "THE BELL RINGS", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SWING %d OF 3", std::max(1, winSwing_));
        hudC(10, buf, PAL_HUD);
        if (!bot_) hudC(20, "START AGAIN", PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        hudC(8, "THE BELL STAYS QUIET", PAL_RED);
        std::snprintf(buf, sizeof buf, "BEST %d", bestMark_);
        hudC(10, buf, PAL_HUD);
        if (!bot_) hudC(20, "START AGAIN", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "LEFT %d", swings_);
        hud(1, 0, buf, swings_ > 1 ? PAL_HUD : PAL_RED);
        int live = int(std::lround(std::min(1.f, puck_) * 100.f));
        if (mode_ == Mode::Ready || mode_ == Mode::Strike || mode_ == Mode::Between) live = bestMark_;
        std::snprintf(buf, sizeof buf, "%d", live);
        hud(39 - int(std::strlen(buf)), 0, buf, live >= 100 ? PAL_GOLD : PAL_HUD);
        if (mode_ == Mode::Ready) {
            if (meter() >= BELL_AT) hudC(23, "NOW", PAL_GREEN);
            else hudC(23, "WAIT FOR THE GOLD", PAL_HUD);
        } else if (mode_ == Mode::Between) {
            std::snprintf(buf, sizeof buf, "%d  SHORT", bestMark_);
            hudC(23, buf, PAL_RED);
        } else if (mode_ == Mode::Rise || mode_ == Mode::Fall) {
            hudC(23, puck_ >= 1.f ? "RING" : "UP", PAL_GOLD);
        }
        hudC(27, "Z  C  SPACE", PAL_HUD);
    }
    if (mode_ != Mode::Title) meterBar(25);
    else meterBar(26);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.22f, 0.32f, 0.24f);
    sys.apu.setPatch(7, calliope());
    sys.apu.setPatch(2, bellPatch());
    sys.setLight(180, 40, 50);
    if (bot_) resetAttempt();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool back = !bot_ && pad.pressed(gs::BTN_MODE);
    bool start = !bot_ && pad.pressed(gs::BTN_START);
    bool swing = !bot_ && (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO));

    if (back) {
        if (mode_ == Mode::Title) sys.quit();
        else {
            mode_ = Mode::Title;
            modeT_ = 0;
            over_ = false;
            sys.apu.tone(0, 0, 0);
            blip(false);
        }
    } else if (mode_ == Mode::Title) {
        if (start) {
            resetAttempt();
            blip(true);
        }
    } else if (mode_ == Mode::Ready) {
        if (swing && modeT_ > 0.18f) {
            launch(meter());
            blip(meter() >= BELL_AT);
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (start) {
            resetAttempt();
            blip(true);
        }
    }

    update(DT);
    draw();
}

}  // namespace striker
