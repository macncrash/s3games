#include "game/sally.h"

#include <cmath>
#include <cstdio>

namespace sally {

static void putSpr(gs::VDP& vdp, gs::Image img, int x, int y, int pal, bool flip) {
    if (img.w < 1 || img.h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    vdp.sprite(s);
}

static void cellAt(int c, int r, int& x, int& y) {
    x = 48 + c * 68;
    y = 86 + r * 38;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    art_.build(sys.vdp);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.HUD.enabled = true;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
    reason_.clear();
    for (int i = 0; i < kHeaps; i++) heap_[i] = true;
    pc_ = pr_ = 0;
    clock_ = kClock;
}

void Game::begin() {
    mode_ = Mode::Play;
    age_ = 0;
    over_ = false;
    won_ = false;
    reason_.clear();
    for (int i = 0; i < kHeaps; i++) heap_[i] = true;
    pc_ = pr_ = fromC_ = fromR_ = toC_ = toR_ = 0;
    walk_ = 0;
    work_ = 0;
    sweep_ = 0;
    clock_ = kClock;
}

int Game::left() const { return dirtyCount(); }

int Game::dirtyCount() const {
    int n = 0;
    for (int i = 0; i < kHeaps; i++)
        if (heap_[i]) n++;
    return n;
}

void Game::finish(bool win, const char* why) {
    won_ = win;
    reason_ = why;
    mode_ = Mode::Over;
    over_ = true;
    blip(1, win ? 520.f : 70.f, win ? 0.14f : 0.16f);
}

bool Game::wantClear() const {
    if (bot_) {
        int i = pr_ * kCols + pc_;
        return walk_ == 0 && heap_[i];
    }
    const gs::Pad& p = sys_->pad;
    return p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z);
}

void Game::steer(int& dc, int& dr) const {
    dc = dr = 0;
    if (bot_) {
        if (walk_ > 0 || work_ > 0) return;
        int i = pr_ * kCols + pc_;
        if (heap_[i]) return;
        if (i + 1 >= kHeaps) return;
        // Snake: even rows left to right, odd rows right to left.
        int cleared = kHeaps - dirtyCount();
        int row = cleared / kCols;
        int along = cleared % kCols;
        int tc = (row & 1) ? (kCols - 1 - along) : along;
        int tr = row;
        if (tc == pc_ && tr == pr_) return;
        if (tc != pc_) dc = tc > pc_ ? 1 : -1;
        else dr = tr > pr_ ? 1 : -1;
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) dc = -1;
    else if (p.pressed(gs::BTN_RIGHT)) dc = 1;
    else if (p.pressed(gs::BTN_UP)) dr = -1;
    else if (p.pressed(gs::BTN_DOWN)) dr = 1;
}

void Game::blip(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    tone_ = 8;
}

void Game::update() {
    age_++;
    if (tone_ > 0 && --tone_ == 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
    if (clock_ <= 0) {
        finish(false, "THE CLOCK DIED");
        return;
    }
    clock_--;

    if (walk_ > 0) {
        walk_--;
        if (walk_ == 0) {
            pc_ = toC_;
            pr_ = toR_;
        }
        return;
    }

    int here = pr_ * kCols + pc_;
    if (work_ > 0 || (wantClear() && heap_[here])) {
        work_++;
        sweep_ = 8;
        if ((work_ % 8) == 1) blip(0, 180.f + float(work_), 0.08f);
        if (work_ >= kWork) {
            heap_[here] = false;
            work_ = 0;
            blip(1, 360.f, 0.1f);
            if (dirtyCount() == 0) {
                if (clock_ > 0) finish(true, "THE GROUND IS CLEAR");
                else finish(false, "CLEARED AFTER THE CLOCK");
            }
        }
        return;
    }

    int dc = 0, dr = 0;
    steer(dc, dr);
    int nc = pc_ + dc;
    int nr = pr_ + dr;
    if ((dc || dr) && nc >= 0 && nr >= 0 && nc < kCols && nr < kRows) {
        fromC_ = pc_;
        fromR_ = pr_;
        toC_ = nc;
        toR_ = nr;
        walk_ = kWalk;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = float(y) / float(gs::SCREEN_H - 1);
        int r = int(2 + 8.f * (1.f - t));
        int g = int(3 + 4.f * (1.f - t));
        int b = int(6 + 5.f * t);
        if (r > 15) r = 15;
        if (g > 15) g = 15;
        if (b > 15) b = 15;
        if (y > 64) {
            r = 3;
            g = 6;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::ground() {
    gs::VDP& v = sys_->vdp;
    v.B.clear();
    for (int y = 8; y < 28; y++) {
        int tile = y == 8 ? 201 : 200;
        for (int x = 0; x < 40; x++) v.B.set(x, y, gs::entry(tile, PAL_GND));
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.fontBase + (c - 32), pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::figure() {
    float u = walk_ > 0 ? 1.f - float(walk_) / float(kWalk) : 1.f;
    int x0, y0, x1, y1;
    cellAt(fromC_, fromR_, x0, y0);
    cellAt(walk_ > 0 ? toC_ : pc_, walk_ > 0 ? toR_ : pr_, x1, y1);
    if (walk_ == 0) {
        cellAt(pc_, pr_, x0, y0);
        x1 = x0;
        y1 = y0;
    }
    int x = int(std::lround(x0 + (x1 - x0) * u));
    int y = int(std::lround(y0 + (y1 - y0) * u));
    int bob = (walk_ > 0 && ((age_ / 3) & 1)) ? -2 : 0;
    bool flip = (walk_ > 0 ? toC_ < fromC_ : false);
    int sweep = 0;
    if (sweep_ > 0) {
        sweep = (age_ & 4) ? 4 : -2;
        sweep_--;
    }
    putSpr(sys_->vdp, art_.body, x - art_.body.w / 2, y - 40 + bob, PAL_YOU, flip);
    int bx = flip ? x - 18 : x + 2;
    putSpr(sys_->vdp, art_.broom, bx, y - 28 + bob + sweep, PAL_YOU, flip);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    ground();

    putSpr(v, art_.sun, 268, 8, PAL_SUN, false);
    putSpr(v, art_.tree, 4, 40, PAL_GND, false);
    putSpr(v, art_.tree, 292, 46, PAL_GND, false);

    for (int i = 0; i < kHeaps; i++) {
        int c = i % kCols;
        int r = i / kCols;
        int x, y;
        cellAt(c, r, x, y);
        if (heap_[i]) putSpr(v, art_.heap, x - 16, y - 8, PAL_HEAP, false);
        else putSpr(v, art_.clean, x - 12, y - 2, PAL_FX, false);
    }
    if (mode_ != Mode::Title) figure();

    hudC(1, "S3 SALLY CLER", PAL_GOLD);
    if (mode_ == Mode::Title) {
        hudC(3, "YOU HAVE THE SALLY", PAL_INK);
        hudC(5, "CLEAR THE GROUND", PAL_INK);
        hudC(6, "BEFORE THE CLOCK DIES", PAL_GOLD);
        hudC(24, "ARROWS MOVE   A SWEEPS", PAL_INK);
        hudC(26, "ANYTHING ELSE IS A LOSS", PAL_BAD);
    } else if (mode_ == Mode::Play) {
        char buf[32];
        int sec = clock_ > 0 ? (clock_ + 59) / 60 : 0;
        std::snprintf(buf, sizeof buf, "CLOCK %d", sec);
        hud(1, 3, buf, sec <= 4 ? PAL_BAD : PAL_GOLD);
        std::snprintf(buf, sizeof buf, "LEFT %d", dirtyCount());
        hud(30, 3, buf, PAL_INK);
        if (work_ > 0) hudC(25, "SWEEPING", PAL_GOOD);
        else hudC(26, "A CLEARS THE GROUND UNDER YOU", PAL_INK);
    } else {
        hudC(3, won_ ? "THE GROUND IS CLEAR" : "LOSS", won_ ? PAL_GOOD : PAL_BAD);
        hudC(5, reason_, won_ ? PAL_GOLD : PAL_BAD);
        if (!bot_) hudC(26, "A AGAIN", PAL_INK);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        age_++;
        bool go = sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C);
        if (bot_ && age_ > 20) go = true;
        draw();
        if (go) begin();
        return;
    }
    if (mode_ == Mode::Play) {
        update();
        draw();
        return;
    }
    draw();
    if (!bot_ && (sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_START))) begin();
}

}  // namespace sally
