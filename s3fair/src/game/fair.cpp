#include "game/fair.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace fair {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float WORLD = 1200.f;
constexpr float START_X = 96.f;
constexpr float FERRIS_X = 176.f;
constexpr float FEET = 188.f;
constexpr float BOOTH_X[3] = {268.f, 548.f, 828.f};
constexpr float GATE_SPOT = 1024.f;
constexpr float GATE_X = 1080.f;
constexpr int NEED = 160;
constexpr float RING_CX = 160.f;
constexpr float RING_AMP = 86.f;
constexpr float RING_W = 2.45f;
constexpr float RING_HIT = 22.f;
constexpr float LINE_Y = 98.f;
constexpr const char* NAME[3] = {"RING", "DART", "BELL"};

float bottleX(int i) { return RING_CX + (i - 1) * RING_AMP; }
int bottlePts(int i) { return i == 2 ? 50 : 25; }

const int STAR[12][2] = {{16, 14}, {46, 36}, {78, 16}, {118, 30}, {168, 12}, {214, 26},
                          {258, 14}, {304, 22}, {96, 50}, {186, 46}, {240, 52}, {30, 58}};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 0, 4));
    px_ = START_X;
    face_ = 1;
    mode_ = Mode::Title;
    clock_ = 0;
    for (int i = 0; i < 3; i++) {
        boothScore_[i] = 0;
        played_[i] = false;
    }
    won_ = false;
    over_ = false;
}

void Game::newGame() {
    px_ = START_X;
    face_ = 1;
    slide_ = 0;
    for (int i = 0; i < 3; i++) {
        boothScore_[i] = 0;
        played_[i] = false;
    }
    won_ = false;
    over_ = false;
    acc_ = 0;
    thrown_ = 0;
    booth_ = -1;
    flight_ = 0;
    show_ = 0;
    cool_ = 0.2f;
    shake_ = 0;
    for (Pop& p : pops_) p.on = false;
    mode_ = Mode::Walk;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    px_ = START_X;
    slide_ = 0;
    cool_ = 0;
}

void Game::openBooth(int i) {
    booth_ = i;
    thrown_ = 0;
    acc_ = 0;
    flight_ = 0;
    show_ = 0;
    playT_ = 0;
    nudge_ = 0;
    aim_ = 160;
    hitId_ = -1;
    swung_ = false;
    rung_ = false;
    wasSweet_ = false;
    struck_ = 0;
    burstT_ = 0;
    shake_ = 0;
    boothScore_[i] = 0;
    played_[i] = false;
    if (i == 1) {
        const float home[5] = {48, 104, 160, 216, 272};
        const float phase[5] = {0.4f, 1.7f, 2.9f, 4.2f, 5.5f};
        const float freq[5] = {0.9f, 1.2f, 0.75f, 1.3f, 1.05f};
        const int pts[5] = {20, 40, 60, 40, 20};
        const int pal[5] = {PAL_RED, PAL_BLUE, PAL_GOLD, PAL_BLUE, PAL_RED};
        for (int k = 0; k < 5; k++) {
            bal_[k] = Bal{home[k], phase[k], freq[k], pts[k], pal[k], true};
        }
        mode_ = Mode::Dart;
    } else if (i == 2) {
        mode_ = Mode::Bell;
    } else {
        mode_ = Mode::Ring;
    }
    blip(340.f, 0.06f);
}

void Game::bank() {
    if (booth_ < 0 || booth_ > 2) return;
    boothScore_[booth_] = acc_;
    played_[booth_] = true;
    mode_ = Mode::Walk;
    cool_ = 0.35f;
    flight_ = 0;
    show_ = 0;
}

void Game::leave() {
    mode_ = Mode::Leave;
    won_ = played_[0] && played_[1] && played_[2] && score() >= NEED;
    over_ = true;
    if (won_) {
        chord(392.f, 523.f, 659.f);
        beep_ = 1.1f;
        sys_->rumble(0.35f, 0.65f, 200);
        sys_->setLight(255, 200, 70);
    }
}

void Game::say(const char* s) {
    std::snprintf(bark_, sizeof bark_, "%s", s);
    mode_ = Mode::Bark;
    show_ = 1.15f;
    cool_ = 0.35f;
}

float Game::goal() const {
    for (int i = 0; i < 3; i++)
        if (!played_[i]) return BOOTH_X[i];
    if (score() < NEED) {
        int w = 0;
        for (int i = 1; i < 3; i++)
            if (boothScore_[i] < boothScore_[w]) w = i;
        return BOOTH_X[w];
    }
    return GATE_SPOT;
}

void Game::tryUse() {
    if (cool_ > 0) return;
    for (int i = 0; i < 3; i++) {
        if (std::fabs(px_ - BOOTH_X[i]) <= 36.f) {
            openBooth(i);
            return;
        }
    }
    if (std::fabs(px_ - GATE_SPOT) <= 30.f) {
        if (!played_[0] || !played_[1] || !played_[2]) say("THREE BOOTHS");
        else if (score() < NEED) say("NEED 160");
        else leave();
    }
}

float Game::ringBase() const { return RING_CX + RING_AMP * std::sin(playT_ * RING_W); }
float Game::ringX() const { return ringBase() + nudge_; }

float Game::meter() const {
    float p = std::fmod(playT_ / 1.35f, 1.f);
    if (p < 0) p += 1.f;
    return p < 0.5f ? p * 2.f : 2.f - p * 2.f;
}

int Game::strikeScore(float m) const {
    if (m >= 0.82f) return 100;
    if (m >= 0.62f) return 60;
    if (m >= 0.38f) return 30;
    return 10;
}

const char* Game::strikeName(float m) const {
    if (m >= 0.82f) return "BELL";
    if (m >= 0.62f) return "HIGH";
    if (m >= 0.38f) return "MID";
    return "TAP";
}

float Game::balX(int i) const { return bal_[i].home + 14.f * std::sin(playT_ * bal_[i].freq + bal_[i].phase); }

float Game::balY(int i) const { return LINE_Y + 22.f * std::sin(playT_ * 1.15f + bal_[i].phase * 1.3f); }

int Game::richest() const {
    int best = -1, pts = -1;
    float bestD = 1e9f;
    for (int i = 0; i < 5; i++) {
        if (!bal_[i].live) continue;
        float d = std::fabs(balX(i) - aim_);
        if (bal_[i].pts > pts || (bal_[i].pts == pts && d < bestD)) {
            best = i;
            pts = bal_[i].pts;
            bestD = d;
        }
    }
    return best;
}

int Game::atAim(float x) const {
    int best = -1, pts = -1;
    float bestD = 1e9f;
    for (int i = 0; i < 5; i++) {
        if (!bal_[i].live) continue;
        float dx = std::fabs(balX(i) - x);
        float dy = std::fabs(balY(i) - LINE_Y);
        if (dx > 12.f || dy > 13.f) continue;
        if (bal_[i].pts > pts || (bal_[i].pts == pts && dx < bestD)) {
            best = i;
            pts = bal_[i].pts;
            bestD = dx;
        }
    }
    return best;
}

void Game::popup(float x, float y, int pts) {
    for (Pop& p : pops_) {
        if (p.on) continue;
        p.x = x;
        p.y = y;
        p.life = 0.7f;
        p.pts = pts;
        p.on = true;
        return;
    }
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    if (beep_ < 0.12f) beep_ = 0.12f;
}

void Game::chord(float a, float b, float c) {
    sys_->apu.tone(0, a, 0.11f);
    sys_->apu.tone(1, b, 0.09f);
    sys_->apu.tone(2, c, 0.08f);
    beep_ = 0.55f;
}

void Game::missSnd() { sys_->apu.noiseBurst(0.2f, 260.f, 0.14f); }

void Game::updateWalk(bool action, float slide) {
    float g = goal();
    if (bot_) {
        float dx = g - px_;
        if (std::fabs(dx) > 10.f) slide = dx > 0.f ? 1.f : -1.f;
        else {
            slide = 0;
            action = true;
        }
    }
    px_ += slide * 156.f * DT;
    px_ = std::clamp(px_, 48.f, WORLD - 48.f);
    if (slide < -0.2f) face_ = -1;
    else if (slide > 0.2f) face_ = 1;
    if (bot_ && std::fabs(g - px_) <= 10.f) px_ = g;
    slide_ = slide;
    if (action) tryUse();
}

void Game::updateRing(bool action, float slide) {
    playT_ += DT;
    if (burstT_ > 0) burstT_ -= DT;
    if (show_ > 0) {
        show_ -= DT;
        if (show_ > 0) return;
        if (thrown_ >= 3) {
            bank();
            return;
        }
    }
    if (flight_ > 0) {
        flight_ -= DT;
        if (flight_ > 0) return;
        int hit = -1;
        float best = RING_HIT;
        for (int i = 0; i < 3; i++) {
            float d = std::fabs(landX_ - bottleX(i));
            if (d <= best) {
                best = d;
                hit = i;
            }
        }
        if (hit >= 0) {
            int pts = bottlePts(hit);
            acc_ += pts;
            popup(bottleX(hit), 96.f, pts);
            burstX_ = bottleX(hit);
            burstY_ = 112.f;
            burstT_ = 0.35f;
            chord(523.f, 659.f, 784.f);
        } else {
            popup(landX_, 96.f, 0);
            missSnd();
        }
        thrown_++;
        show_ = 0.46f;
        return;
    }
    nudge_ = bot_ ? 0.f : slide * 30.f;
    bool sweet = ringBase() >= bottleX(2) - 14.f;
    if (sweet && !wasSweet_) blip(880.f, 0.05f);
    wasSweet_ = sweet;
    if (bot_ && sweet && thrown_ < 3) action = true;
    if (action && thrown_ < 3) {
        landX_ = ringX();
        flight_ = 0.30f;
        blip(440.f, 0.07f);
    }
}

void Game::updateDart(bool action, float slide) {
    playT_ += DT;
    if (burstT_ > 0) burstT_ -= DT;
    if (show_ > 0) {
        show_ -= DT;
        if (show_ > 0) return;
        int live = 0;
        for (const Bal& b : bal_)
            if (b.live) live++;
        if (thrown_ >= 3 || live == 0) {
            bank();
            return;
        }
    }
    if (flight_ > 0) {
        flight_ -= DT;
        if (flight_ > 0) return;
        if (hitId_ >= 0 && hitId_ < 5 && bal_[hitId_].live) {
            int pts = bal_[hitId_].pts;
            float x = balX(hitId_);
            float y = balY(hitId_);
            bal_[hitId_].live = false;
            acc_ += pts;
            popup(x, y - 18.f, pts);
            burstX_ = x;
            burstY_ = y;
            burstT_ = 0.4f;
            chord(660.f, 880.f, 1046.f);
        } else {
            popup(aim_, LINE_Y, 0);
            missSnd();
        }
        thrown_++;
        show_ = 0.4f;
        hitId_ = -1;
        return;
    }
    if (bot_) {
        int t = richest();
        slide = 0;
        if (t >= 0) {
            float dx = balX(t) - aim_;
            if (dx > 4.f) slide = 1.f;
            else if (dx < -4.f) slide = -1.f;
        }
    }
    aim_ = std::clamp(aim_ + slide * 240.f * DT, 36.f, 286.f);
    if (bot_) {
        int t = richest();
        if (t >= 0 && std::fabs(balX(t) - aim_) <= 7.f && std::fabs(balY(t) - LINE_Y) <= 11.f) action = true;
    }
    if (action && thrown_ < 3) {
        hitId_ = atAim(aim_);
        flight_ = 0.26f;
        blip(520.f, 0.07f);
    }
}

void Game::updateBell(bool action) {
    playT_ += DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);
    if (show_ > 0) {
        show_ -= DT;
        if (show_ <= 0) bank();
        return;
    }
    if (swung_) return;
    float m = meter();
    bool sweet = m >= 0.82f;
    if (sweet && !wasSweet_) blip(980.f, 0.05f);
    wasSweet_ = sweet;
    if (bot_ && m >= 0.93f) action = true;
    if (!action) return;
    swung_ = true;
    struck_ = m;
    acc_ = strikeScore(m);
    rung_ = m >= 0.82f;
    show_ = 1.05f;
    popup(176.f, 46.f, acc_);
    if (rung_) {
        chord(523.f, 784.f, 1046.f);
        shake_ = 0.5f;
        burstX_ = 176.f;
        burstY_ = 58.f;
        burstT_ = 0.55f;
        sys_->rumble(0.55f, 0.9f, 180);
        sys_->setLight(255, 220, 90);
    } else {
        blip(140.f, 0.06f);
        missSnd();
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    if (cool_ > 0) cool_ -= DT;
    for (Pop& p : pops_) {
        if (!p.on) continue;
        p.life -= DT;
        if (p.life <= 0) p.on = false;
    }

    bool action = sys.pad.pressed(gs::BTN_C) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_TURBO);
    bool start = sys.pad.pressed(gs::BTN_START);
    bool back = sys.pad.pressed(gs::BTN_MODE);
    float slide = 0;
    if (sys.pad.down(gs::BTN_LEFT)) slide -= 1;
    if (sys.pad.down(gs::BTN_RIGHT)) slide += 1;
    if (std::fabs(sys.pad.axisX) > 0.2f) slide = sys.pad.axisX;
    if (bot_) {
        action = false;
        start = false;
        back = false;
        slide = 0;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        if (bot_ && clock_ > 0.45f) start = true;
        if (back && !bot_) sys.quit();
        else if (start || action) newGame();
    } else if (mode_ == Mode::Leave) {
        if (start || action) newGame();
        else if (back) toTitle();
    } else if (start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Bark) {
        show_ -= DT;
        if (show_ <= 0) {
            mode_ = Mode::Walk;
            cool_ = 0.25f;
        }
    } else if (mode_ == Mode::Walk) {
        updateWalk(action, slide);
    } else if (mode_ == Mode::Ring) {
        updateRing(action, slide);
    } else if (mode_ == Mode::Dart) {
        updateDart(action, slide);
    } else if (mode_ == Mode::Bell) {
        updateBell(action);
    }
    draw();
}

int Game::marker() const {
    switch (mode_) {
        case Mode::Title: return 0;
        case Mode::Walk: return 1;
        case Mode::Ring: return 2;
        case Mode::Dart: return 3;
        case Mode::Bell: return 4;
        case Mode::Bark: return 5;
        case Mode::Leave: return 6;
        case Mode::Pause: return 7;
    }
    return 0;
}

const char* Game::phase() const {
    switch (mode_) {
        case Mode::Title: return "title";
        case Mode::Walk: return "walk";
        case Mode::Ring: return "ring";
        case Mode::Dart: return "dart";
        case Mode::Bell: return "bell";
        case Mode::Bark: return "bark";
        case Mode::Leave: return "leave";
        case Mode::Pause: return "pause";
    }
    return "?";
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.1f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + adv * 0.5f, y, g.h * scale, pal, false);
    }
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

float Game::sx(float x) const { return x - cam_; }

void Game::drawPops() {
    for (const Pop& p : pops_) {
        if (!p.on) continue;
        char b[8];
        if (p.pts > 0) std::snprintf(b, sizeof b, "+%d", p.pts);
        else std::snprintf(b, sizeof b, "MISS");
        text(b, p.x, p.y - (0.7f - p.life) * 16.f, 0.62f, p.pts > 0 ? PAL_GOLD : PAL_RED);
    }
}

void Game::drawSky(Mode view) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (view == Mode::Ring) {
            int band = (y / 7) & 1;
            if (y < 156) v.lineBackdrop[y] = gs::rgb4(band ? 9 : 6, band ? 1 : 0, band ? 2 : 1);
            else if (y < 190) v.lineBackdrop[y] = gs::rgb4(7, 4, 2);
            else v.lineBackdrop[y] = gs::rgb4(3, 2, 1);
        } else if (view == Mode::Dart) {
            v.lineBackdrop[y] = y < 176 ? gs::rgb4(1, 1, 5) : gs::rgb4(4, 2, 1);
        } else if (view == Mode::Bell) {
            float u = y / 223.f;
            v.lineBackdrop[y] = gs::rgb4(int(1 + u * 4), int(1 + u), int(3 + u));
        } else if (y < 152) {
            float u = y / 152.f;
            int r = int(1 + u * 9);
            int g = int(u * 3);
            int b = int(5 - u * 3);
            if (view == Mode::Leave) r = std::min(15, r + 2);
            v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        } else {
            v.lineBackdrop[y] = gs::rgb4(3, 2, 1);
        }
    }
}

void Game::drawMidway(Mode view) {
    if (mode_ == Mode::Pause) text("PAUSE", 160, 22, 1.15f, PAL_GOLD);
    if (view == Mode::Title) {
        text("S3 FAIR", 160, 22, 1.22f, PAL_GOLD);
        text("THREE BOOTHS", 160, 50, 0.76f, PAL_HUD);
    } else if (view == Mode::Leave) {
        text("LEAVE", 160, 26, 1.4f, PAL_GOLD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "WITH %d", score());
        text(buf, 160, 58, 0.95f, PAL_HUD);
    } else if (view == Mode::Bark) {
        text(bark_, 160, 58, 0.9f, PAL_GOLD);
    }
    drawPops();

    int pose = (std::fabs(slide_) > 0.2f && (int(clock_ * 8.f) & 1)) ? 1 : 0;
    float bob = std::fabs(slide_) > 0.2f ? std::sin(clock_ * 12.f) * 1.1f : 0;
    spr(art_.kid[pose], sx(px_), FEET + bob, 80, PAL_PLAYER, face_ < 0, 0, true);
    stamp(art_.shadow, sx(px_), FEET + 2, 34, 10, PAL_HUD, 0, true);

    for (float x = 18.f; x < WORLD; x += 26.f) {
        float s = sx(x);
        if (s < -12 || s > 332) continue;
        int fog = (int(x / 26.f) + int(clock_ * 5.f)) % 5 == 0 ? 8 : 0;
        spr(art_.bulb, s, 70, 11, PAL_GOLD, false, fog);
    }
    for (float x = 30.f; x < WORLD; x += 26.f) {
        float s = sx(x);
        if (s < -12 || s > 332) continue;
        int k = int(x / 26.f) % 3;
        int pal = k == 0 ? PAL_RED : (k == 1 ? PAL_GOLD : PAL_BLUE);
        spr(art_.bunt, s, 84, 15, pal, false);
    }

    for (int i = 0; i < 3; i++) {
        float x = sx(BOOTH_X[i]);
        if (x < -90 || x > 410) continue;
        bool near = std::fabs(px_ - BOOTH_X[i]) < 42.f;
        int pal = (near && (int(clock_ * 6.f) & 1)) ? PAL_RED : PAL_GOLD;
        text(NAME[i], x, FEET - float(art_.awning.h) - 2.f, 0.55f, pal);
    }
    text("EXIT", sx(GATE_X), 118, 0.48f, PAL_GOLD);

    for (int i = 0; i < 3; i++) {
        float x = sx(BOOTH_X[0] + (i - 1) * 26.f);
        spr(art_.bottle, x, FEET - 18, 34, i == 2 ? PAL_GOLD : PAL_CREAM, false, 0, true);
    }
    spr(art_.fluff, sx(430.f), FEET, 42, PAL_PINK, false, 0, true);
    spr(art_.fluff, sx(700.f), FEET, 42, PAL_PINK, false, 0, true);

    for (int i = 0; i < 3; i++) spr(art_.awning, sx(BOOTH_X[i]), FEET, float(art_.awning.h), PAL_WOOD, false, 0, true);
    spr(art_.gate, sx(GATE_X), FEET, float(art_.gate.h), PAL_WOOD, false, 0, true);

    float fx = sx(FERRIS_X);
    for (int i = 0; i < 8; i++) {
        float a = clock_ * 0.48f + i * 6.2831853f / 8.f;
        spr(art_.car, fx + std::cos(a) * 54.f, 106.f + std::sin(a) * 42.f, 18, (i & 1) ? PAL_BLUE : PAL_RED, false, 1);
    }
    spr(art_.hub, fx, 106, 22, PAL_BRASS, false, 1);
    spr(art_.stand, fx, 170, 100, PAL_NIGHT, false, 2, true);

    for (const auto& st : STAR) spr(art_.bulb, float(st[0]), float(st[1]), 4, PAL_HUD, false, 3);
    spr(art_.moon, 26, 30, 30, PAL_GOLD, false, 0);
}

void Game::drawRing() {
    bool aiming = flight_ <= 0 && show_ <= 0;
    float rx = aiming ? ringX() : landX_;
    float ry = 70.f;
    if (flight_ > 0) {
        float u = 1.f - flight_ / 0.30f;
        ry = 70.f + u * u * 54.f;
    } else if (!aiming) {
        ry = 122.f;
    }
    bool sweet = ringBase() >= bottleX(2) - 14.f;
    drawPops();
    if (sweet && aiming) text("NOW", bottleX(2), 78, 0.55f, PAL_GOLD);
    text("25", bottleX(0), 96, 0.48f, PAL_HUD);
    text("25", bottleX(1), 96, 0.48f, PAL_HUD);
    text("50", bottleX(2), 90, 0.52f, PAL_GOLD);
    spr(art_.kid[0], 40, 210, 70, PAL_PLAYER, false, 0, true);
    spr(art_.ring, rx, ry, 26, PAL_GOLD, false);
    if (burstT_ > 0) spr(art_.burst, burstX_, burstY_, 28 + (0.35f - burstT_) * 20.f, PAL_GOLD, false);
    if (aiming) spr(art_.bulb, ringX(), 176, sweet ? 10.f : 7.f, sweet ? PAL_GOLD : PAL_HUD, false);
    for (int i = 0; i < 3; i++) {
        spr(art_.bottle, bottleX(i), 168, 52, i == 2 ? PAL_GOLD : PAL_CREAM, false, 0, true);
        if (i == 2) spr(art_.bulb, bottleX(i), 104, sweet ? 16.f : 12.f, PAL_GOLD, false);
    }
    stamp(art_.plank, 160, 186, 300, 22, PAL_WOOD);
    stamp(art_.awning, 160, 36, 280, 58, PAL_WOOD);
}

void Game::drawDart() {
    drawPops();
    spr(art_.kid[0], 36, 208, 68, PAL_PLAYER, false, 0, true);
    float dartY = 190.f;
    float dartX = aim_;
    if (flight_ > 0) {
        float u = 1.f - flight_ / 0.26f;
        dartY = 190.f + (LINE_Y - 190.f) * u;
        if (hitId_ >= 0) dartX = balX(hitId_);
    }
    spr(art_.dart, dartX, dartY, 16, PAL_BRASS, false);
    if (flight_ <= 0 && show_ <= 0) spr(art_.bulb, aim_, LINE_Y, 12, PAL_GOLD, false, 2);
    if (burstT_ > 0) spr(art_.burst, burstX_, burstY_, 30, PAL_GOLD, false);
    for (int i = 0; i < 5; i++) {
        if (!bal_[i].live) continue;
        spr(art_.balloon, balX(i), balY(i), 52, bal_[i].pal, false);
    }
    stamp(art_.board, 160, 108, 286, 124, PAL_WOOD);
}

void Game::drawBell() {
    float m = swung_ ? struck_ : meter();
    float sh = shake_ > 0 ? std::sin(clock_ * 78.f) * 5.f * shake_ : 0;
    drawPops();
    if (show_ > 0) text(strikeName(struck_), 176, 30, 1.05f, rung_ ? PAL_GOLD : PAL_HUD);
    else if (!swung_ && m >= 0.82f) text("NOW", 176, 30, 0.7f, PAL_GOLD);
    spr(art_.kid[0], 52 + sh, 208, 72, PAL_PLAYER, false, 0, true);
    bool raised = !swung_ ? (m > 0.4f) : (show_ > 0.75f);
    spr(art_.hammer[raised ? 1 : 0], raised ? 86.f : 74.f, raised ? 150.f : 178.f, 42, PAL_BRASS, false);
    spr(art_.puck, 188 + sh, 180.f - m * 100.f, 16, PAL_RED, false);
    int bellPal = (rung_ || (!swung_ && m >= 0.82f)) ? PAL_GOLD : PAL_BRASS;
    spr(art_.bell, 188 + sh, 64, 36, bellPal, false);
    if (burstT_ > 0) spr(art_.burst, burstX_ + sh, burstY_, 34, PAL_GOLD, false);
    spr(art_.tower, 188 + sh, 210, 150, PAL_WOOD, false, 0, true);
}

void Game::drawHelp(Mode view) {
    char buf[40];
    std::snprintf(buf, sizeof buf, "%d", score());
    if (view == Mode::Title) {
        hud(1, 0, "S3 FAIR", PAL_GOLD);
        hudC(24, "LEAVE WITH A SCORE");
        hudC(25, "ENTER START");
        hudC(26, "ARROWS WALK");
        hudC(27, "Z TOSS   C TOSS");
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(26, "PAUSED");
        hudC(27, "ENTER RESUME  ESC TITLE");
    }
    if (view == Mode::Walk || view == Mode::Bark || view == Mode::Leave || view == Mode::Title) {
        hud(1, 0, "S3 FAIR", PAL_GOLD);
        hud(40 - int(std::char_traits<char>::length(buf)), 0, buf, PAL_GOLD);
        char a[8], b[8], c[8], line[32];
        std::snprintf(a, sizeof a, "%s", played_[0] ? "" : "");
        if (played_[0]) std::snprintf(a, sizeof a, "%d", boothScore_[0]);
        else std::snprintf(a, sizeof a, "--");
        if (played_[1]) std::snprintf(b, sizeof b, "%d", boothScore_[1]);
        else std::snprintf(b, sizeof b, "--");
        if (played_[2]) std::snprintf(c, sizeof c, "%d", boothScore_[2]);
        else std::snprintf(c, sizeof c, "--");
        std::snprintf(line, sizeof line, "R%s  D%s  B%s", a, b, c);
        hud(1, 1, line, PAL_HUD);
        if (played_[0] && played_[1] && played_[2] && score() >= NEED) hud(22, 1, "GATE OPEN", PAL_GOLD);
        else {
            std::snprintf(line, sizeof line, "%d TO LEAVE", NEED);
            hud(26, 1, line, PAL_HUD);
        }
    }
    if (view == Mode::Leave) {
        hudC(25, "YOU LEAVE WITH A SCORE");
        std::snprintf(buf, sizeof buf, "RING %d  DART %d  BELL %d", ring(), dart(), bell());
        hudC(26, buf);
        hudC(27, "ENTER PLAYS AGAIN");
        return;
    }
    if (view == Mode::Bark) {
        hudC(27, "THE GATE STAYS SHUT");
        return;
    }
    if (view == Mode::Walk) {
        int near = -1;
        for (int i = 0; i < 3; i++)
            if (std::fabs(px_ - BOOTH_X[i]) <= 36.f) near = i;
        if (near >= 0) {
            std::snprintf(buf, sizeof buf, played_[near] ? "Z RETRY %s" : "Z PLAY %s", NAME[near]);
            hudC(26, buf);
        } else if (std::fabs(px_ - GATE_SPOT) <= 34.f) {
            hudC(26, score() >= NEED && played_[0] && played_[1] && played_[2] ? "Z LEAVE WITH YOUR SCORE" : "NOT YET");
        } else if (played_[0] && played_[1] && played_[2] && score() >= NEED) {
            hudC(26, "WALK TO THE GATE");
        } else {
            hudC(26, "THREE BOOTHS ON THE MIDWAY");
        }
        hudC(27, "ARROWS WALK   Z PLAY");
        return;
    }
    int shown = thrown_;
    if (flight_ <= 0 && show_ <= 0) shown = thrown_ + 1;
    shown = std::clamp(shown, 1, 3);
    if (view == Mode::Ring) {
        hud(1, 0, "RING TOSS", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "POT %d", acc_);
        hud(30, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "TOSS %d OF 3", shown);
        hud(1, 1, buf, PAL_HUD);
        hudC(26, "WAIT ON THE RIGHT BOTTLE");
        hudC(27, "Z DROPS THE RING");
    } else if (view == Mode::Dart) {
        hud(1, 0, "BALLOON DART", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "POT %d", acc_);
        hud(30, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "DART %d OF 3", shown);
        hud(1, 1, buf, PAL_HUD);
        hudC(26, "GOLD 60  BLUE 40  RED 20");
        hudC(27, "ARROWS AIM   Z THROW");
    } else if (view == Mode::Bell) {
        hud(1, 0, "HIGH STRIKER", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "POT %d", acc_);
        hud(30, 0, buf, PAL_HUD);
        hud(1, 1, "BELL 100  HIGH 60", PAL_HUD);
        hudC(26, "STOP THE PUCK AT THE TOP");
        hudC(27, "Z STRIKE");
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    Mode view = mode_ == Mode::Pause ? held_ : mode_;
    bool indoor = view == Mode::Ring || view == Mode::Dart || view == Mode::Bell;
    v.A.enabled = false;
    v.B.enabled = !indoor;
    if (!indoor) {
        cam_ = px_ - 150.f;
        if (view == Mode::Title) cam_ = 0;
        cam_ = std::clamp(cam_, 0.f, WORLD - 320.f);
        v.B.scroll(int(std::lround(-cam_)), 0);
    }
    drawSky(view);
    if (view == Mode::Ring) drawRing();
    else if (view == Mode::Dart) drawDart();
    else if (view == Mode::Bell) drawBell();
    else drawMidway(view);
    drawHelp(view);
}

}  // namespace fair
