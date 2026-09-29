#include "game/memorygold.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace memorygold {
namespace {

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.15f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.16f, 0.f, 0.18f};
    p.op[1] = {2.f, 0.4f, 0.01f, 0.2f, 0.f, 0.18f};
    p.op[2] = {3.f, 0.22f, 0.01f, 0.18f, 0.f, 0.16f};
    p.op[3] = {4.5f, 0.12f, 0.01f, 0.14f, 0.f, 0.14f};
    p.vol = 0.2f;
    return p;
}

uint32_t entropy() {
    using namespace std::chrono;
    auto n = high_resolution_clock::now().time_since_epoch().count();
    uint32_t x = uint32_t(n) ^ uint32_t(n >> 32);
    x ^= x << 13;
    x ^= x >> 17;
    return x ? x : 0xA5A5u;
}

}  // namespace

uint32_t Game::rndu() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

void Game::mapSlots() {
    int n[PAIRS] = {};
    for (int i = 0; i < CARDS; i++) {
        int f = cards_[i].face;
        if (n[f] == 0) slotA_[f] = i;
        else slotB_[f] = i;
        n[f]++;
    }
}

void Game::freshDeal() {
    int ids[CARDS];
    for (int i = 0; i < CARDS; i++) ids[i] = i >> 1;
    for (int i = CARDS - 1; i > 0; i--) {
        int j = int(rndu() % uint32_t(i + 1));
        std::swap(ids[i], ids[j]);
    }
    for (int i = 0; i < CARDS; i++) {
        cards_[i].face = ids[i];
        cards_[i].up = false;
        cards_[i].matched = false;
        cards_[i].show = 0;
    }
    mapSlots();
    golds_ = cream_ = score_ = turns_ = refused_ = 0;
    nOpen_ = shut_ = 0;
    cursor_ = 0;
    botFace_ = 0;
    botPhase_ = 0;
    clockFrames_ = kClock;
    over_ = won_ = left_ = finisherGold_ = false;
    why_ = "";
    fanFrame_ = -1;
    lastSec_ = -1;
    beep_ = tick_ = ready_ = 0;
    holdX_ = holdY_ = holdXT_ = holdYT_ = 0;
}

void Game::dealIn(int studyFrames) {
    freshDeal();
    for (auto& c : cards_) c.up = true;
    studyFrames_ = studyFrames;
    mode_ = Mode::Study;
}

void Game::approach(Card& c) {
    float target = (c.up || c.matched) ? 1.f : 0.f;
    float d = target - c.show;
    const float step = 0.28f;
    if (std::fabs(d) <= step) c.show = target;
    else c.show += d > 0 ? step : -step;
}

bool Game::closing() const {
    if (shut_ > 0) return true;
    for (const Card& c : cards_) {
        float target = (c.up || c.matched) ? 1.f : 0.f;
        if (c.show > target + 0.02f) return true;
    }
    return false;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 4;
}

void Game::matchChime() {
    sys_->apu.keyOn(1, 880.f, 0.18f);
    sys_->apu.keyOn(2, 1320.f, 0.12f);
}

void Game::miss() {
    sys_->apu.tone(0, 150.f, 0.06f);
    beep_ = 6;
    sys_->apu.noiseBurst(0.12f, 500.f, 0.12f);
}

void Game::leaveTable() {
    mode_ = Mode::Leave;
    over_ = true;
    won_ = true;
    left_ = true;
    finisherGold_ = true;
    why_ = "only the gold counts double";
    fanFrame_ = 0;
    sys_->rumble(0.35f, 0.7f, 160);
}

void Game::lose(const char* why) {
    clockFrames_ = 0;
    mode_ = Mode::Lose;
    over_ = true;
    won_ = false;
    left_ = false;
    why_ = why;
    sys_->apu.noiseBurst(0.35f, 240.f, 0.35f);
    sys_->rumble(0.45f, 0.15f, 180);
}

bool Game::tryFlip(int idx) {
    if (mode_ != Mode::Play || over_ || closing()) return false;
    if (idx < 0 || idx >= CARDS) return false;
    Card& c = cards_[idx];
    if (c.matched || c.up) return false;
    c.up = true;
    if (nOpen_ < 2) open_[nOpen_++] = idx;
    blip(740.f);
    if (nOpen_ == 2) {
        turns_++;
        Card& a = cards_[open_[0]];
        Card& b = cards_[open_[1]];
        bool pair = a.face == b.face && open_[0] != open_[1];
        if (!pair) {
            shut_ = 40;
            miss();
            return true;
        }
        bool gold = goldFace(a.face);
        int add = gold ? 2 : 1;
        // Cream cannot buy the line. A gold double can, and only while the
        // undoubled count of pairs is still short of the line.
        if (!gold && score_ + add >= LINE) {
            refused_++;
            shut_ = 40;
            miss();
            why_ = "cream is not a double";
            return true;
        }
        a.matched = true;
        b.matched = true;
        nOpen_ = 0;
        if (gold) golds_++;
        else cream_++;
        score_ += add;
        matchChime();
        sys_->rumble(0.12f, 0.28f, 60);
        if (score_ >= LINE && bare() < LINE && gold) {
            leaveTable();
        } else if (score_ >= LINE && !gold) {
            lose("cream bought the line");
        } else if (score_ >= LINE && bare() >= LINE) {
            lose("the bare count met the line");
        }
    }
    return true;
}

void Game::moveCursor(int dx, int dy) {
    int x = std::clamp(cursor_ % COLS + dx, 0, COLS - 1);
    int y = std::clamp(cursor_ / COLS + dy, 0, ROWS - 1);
    cursor_ = y * COLS + x;
}

void Game::nudge(int& dx, int& dy) {
    const gs::Pad& pad = sys_->pad;
    auto axis = [&](gs::Button neg, gs::Button pos, int& mem, int& timer) {
        int v = (pad.down(pos) ? 1 : 0) - (pad.down(neg) ? 1 : 0);
        if (v == 0) {
            mem = 0;
            timer = 0;
            return 0;
        }
        if (v != mem) {
            mem = v;
            timer = 0;
            return v;
        }
        if (++timer >= 12 && ((timer - 12) % 5) == 0) return v;
        return 0;
    };
    dx = axis(gs::BTN_LEFT, gs::BTN_RIGHT, holdX_, holdXT_);
    dy = axis(gs::BTN_UP, gs::BTN_DOWN, holdY_, holdYT_);
}

void Game::readPad() {
    const gs::Pad& pad = sys_->pad;
    const bool action = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
    const bool start = pad.pressed(gs::BTN_START);
    const bool back = pad.pressed(gs::BTN_MODE);
    int dx = 0, dy = 0;

    if (mode_ == Mode::Title) {
        if (start || action) dealIn(180);
        else if (back) sys_->quit();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (start || action) mode_ = Mode::Play;
        else if (back) mode_ = Mode::Title;
        return;
    }
    if (mode_ == Mode::Leave || mode_ == Mode::Lose) {
        if (start || action) dealIn(180);
        else if (back) mode_ = Mode::Title;
        return;
    }
    if (mode_ == Mode::Ready) {
        nudge(dx, dy);
        moveCursor(dx, dy);
        if (start || action) mode_ = Mode::Play;
        else if (back) mode_ = Mode::Title;
        return;
    }
    nudge(dx, dy);
    moveCursor(dx, dy);
    if (mode_ == Mode::Play && action) tryFlip(cursor_);
    if (start || back) {
        if (mode_ == Mode::Play) mode_ = Mode::Pause;
        else mode_ = Mode::Title;
    }
}

void Game::botAct() {
    if (mode_ != Mode::Play || over_ || closing()) return;
    if (botFace_ >= GOLD_FACES) return;
    int target = botPhase_ == 0 ? slotA_[botFace_] : slotB_[botFace_];
    int tx = target % COLS, ty = target / COLS;
    int cx = cursor_ % COLS, cy = cursor_ / COLS;
    int dx = 0, dy = 0;
    bool flip = false;
    if (cx != tx) dx = tx > cx ? 1 : -1;
    else if (cy != ty) dy = ty > cy ? 1 : -1;
    else flip = true;
    moveCursor(dx, dy);
    if (flip && tryFlip(cursor_)) {
        botPhase_ ^= 1;
        if (botPhase_ == 0) botFace_++;
    }
}

void Game::physics() {
    for (auto& c : cards_) approach(c);
    if (shut_ > 0 && --shut_ == 0) {
        if (nOpen_ == 2) {
            cards_[open_[0]].up = false;
            cards_[open_[1]].up = false;
        }
        nOpen_ = 0;
    }
    if (mode_ == Mode::Study && --studyFrames_ <= 0) {
        for (auto& c : cards_)
            if (!c.matched) c.up = false;
        mode_ = Mode::Cover;
    }
    if (mode_ == Mode::Ready) {
        if (--ready_ <= 0) mode_ = Mode::Play;
    } else if (mode_ == Mode::Cover && !closing()) {
        mode_ = Mode::Ready;
        ready_ = bot_ ? 4 : 30;
    }
}

void Game::clockTick() {
    if (clockFrames_ > 0) clockFrames_--;
    if (clockFrames_ <= 0) lose("the clock won");
    int sec = clockSec();
    if (!over_ && sec != lastSec_ && sec <= 10) {
        sys_->apu.tone(1, sec <= 3 ? 680.f : 440.f, 0.045f);
        tick_ = 3;
    }
    lastSec_ = sec;
}

void Game::audio() {
    if (beep_ > 0 && --beep_ == 0) sys_->apu.tone(0, 0, 0);
    if (tick_ > 0 && --tick_ == 0) sys_->apu.tone(1, 0, 0);
    if (fanFrame_ < 0) return;
    static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f, 1318.5f};
    if (fanFrame_ < 40 && fanFrame_ % 8 == 0) {
        int i = fanFrame_ / 8;
        if (i < 5) sys_->apu.keyOn(1, notes[i], 0.18f);
        if (i == 4) sys_->apu.keyOn(2, 1568.f, 0.12f);
    }
    if (++fanFrame_ > 48) fanFrame_ = -1;
}

int Game::clockSec() const {
    if (clockFrames_ <= 0) return 0;
    return (clockFrames_ + 59) / 60;
}

void Game::cell(int i, float& cx, float& cy) const {
    int x = i % COLS;
    int y = i / COLS;
    cx = float(GRID_X + x * (CARD_W + GAP_X)) + CARD_W * 0.5f;
    cy = float(GRID_Y + y * (CARD_H + GAP_Y)) + CARD_H * 0.5f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, float xscale, int pal, bool shadow) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * (float(m.w) / float(m.h)) * std::max(xscale, 0.04f);
    gs::Sprite s;
    s.h = int16_t(std::lround(h));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (shadow) {
        s.x = int16_t(s.x + 3);
        s.y = int16_t(s.y + 3);
    }
    s.img = m.pick(h);
    s.pal = uint8_t(pal & 15);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::bar(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || art_.bar.h < 1) return;
    gs::Sprite s;
    s.img = art_.bar.pick(h);
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.pal = uint8_t(pal & 15);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::felt() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int dy = y - 112;
        if (dy < 0) dy = -dy;
        int g = 8 - dy / 30;
        if (g < 4) g = 4;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(1, g, 3);
    }
}

void Game::drawBar() {
    float frac = std::clamp(float(score_) / float(LINE), 0.f, 1.f);
    bar(float(GRID_X), 18, float(GRID_W), 4, PAL_WOOD);
    if (frac > 0.004f) bar(float(GRID_X), 18, GRID_W * frac, 4, mode_ == Mode::Leave ? PAL_OK : PAL_GOLD);
}

void Game::drawTitle() {
    spr(art_.logo, 160, 42, float(art_.logo.h), 1.f, PAL_GOLD, false);
    spr(art_.lineA, 160, 78, float(art_.lineA.h), 1.f, PAL_INK, false);
    spr(art_.lineB, 160, 96, float(art_.lineB.h), 1.f, PAL_INK, false);
    const float gh = 34.f;
    const float gw = gh * float(CARD_W) / float(CARD_H);
    const float gap = 4.f;
    const float total = PAIRS * gw + (PAIRS - 1) * gap;
    float x = (gs::SCREEN_W - total) * 0.5f + gw * 0.5f;
    for (int i = 0; i < PAIRS; i++) {
        float cx = x + i * (gw + gap);
        spr(art_.face[i], cx, 150, gh, 1.f, PAL_FACE, true);
        spr(art_.face[i], cx, 150, gh, 1.f, PAL_FACE, false);
    }
}

void Game::drawTable() {
    float ccx, ccy;
    cell(cursor_, ccx, ccy);
    spr(art_.cursor, ccx, ccy, float(CARD_H + 8), 1.f, PAL_MARK, false);
    for (int i = 0; i < CARDS; i++) {
        if (!cards_[i].matched || cards_[i].show < 0.85f) continue;
        float cx, cy;
        cell(i, cx, cy);
        int pal = goldFace(cards_[i].face) ? PAL_GOLD : PAL_INK;
        spr(art_.pip, cx + CARD_W * 0.5f - 8.f, cy - CARD_H * 0.5f + 8.f, 10.f, 1.f, pal, false);
    }
    for (int i = 0; i < CARDS; i++) {
        float cx, cy;
        cell(i, cx, cy);
        const Card& c = cards_[i];
        float xs = std::fabs(std::cos(c.show * 3.14159265f));
        if (xs < 0.08f) xs = 0.08f;
        bool face = c.show >= 0.5f;
        const gs::Mipped& img = face ? art_.face[c.face] : art_.back;
        spr(img, cx, cy, float(CARD_H), xs, face ? PAL_FACE : PAL_BACK, true);
        spr(img, cx, cy, float(CARD_H), xs, face ? PAL_FACE : PAL_BACK, false);
    }
}

void Game::chrome() {
    sys_->vdp.HUD.clear();
    spr(art_.wood, gs::SCREEN_W * 0.5f, 8.f, 16.f, 1.f, PAL_WOOD, false);
    spr(art_.wood, gs::SCREEN_W * 0.5f, gs::SCREEN_H - 8.f, 16.f, 1.f, PAL_WOOD, false);
    if (mode_ == Mode::Title) {
        hudC(22, "PRESS START", PAL_GOLD);
        hudC(24, "GOLD 2   CREAM 1   LINE 6", PAL_INK);
        hudC(26, "ARROWS MOVE   C FLIPS", PAL_INK);
        hudC(27, S3_VERSION_STRING, PAL_DIM);
        return;
    }
    if (mode_ == Mode::Study) {
        hudC(0, "STUDY  GOLD COUNTS DOUBLE", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Cover || mode_ == Mode::Ready) {
        hudC(0, "READY", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_GOLD);
        hudC(26, "START RESUMES", PAL_INK);
        return;
    }
    if (mode_ == Mode::Leave) {
        char top[40];
        std::snprintf(top, sizeof top, "SCORE %d  BARE %d  GOLD %d", score_, bare(), golds_);
        hudC(0, top, PAL_OK);
        hudC(26, "LEFT  ONLY THE GOLD", PAL_OK);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(0, why_ ? why_ : "LOST", PAL_ALERT);
        hudC(26, "THE TABLE STAYS", PAL_ALERT);
        return;
    }
    char left[40];
    std::snprintf(left, sizeof left, "S %d/%d  B %d  G %d", score_, LINE, bare(), golds_);
    hud(1, 0, left, PAL_GOLD);
    char right[16];
    int sec = clockSec();
    std::snprintf(right, sizeof right, "T %02d", sec);
    int n = int(std::strlen(right));
    hud(39 - n, 0, right, sec <= 10 ? PAL_ALERT : PAL_INK);
    if (why_ && why_[0] && refused_ > 0) hudC(26, why_, PAL_ALERT);
    else hudC(26, "CREAM CANNOT BUY THE LINE", PAL_DIM);
}

void Game::draw() {
    felt();
    sys_->vdp.clearSprites();
    drawBar();
    if (mode_ == Mode::Title) drawTitle();
    else drawTable();
    chrome();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    gs::FMPatch chime = chimePatch();
    sys.apu.setPatch(1, chime);
    sys.apu.setPatch(2, chime);
    rng_ = bot_ ? 0x4D454D47u : entropy();
    if (bot_) dealIn(8);
    else {
        freshDeal();
        mode_ = Mode::Title;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) physics();
    if (bot_) botAct();
    else readPad();
    if (mode_ == Mode::Play && !over_) clockTick();
    audio();
    draw();
}

}  // namespace memorygold
