#include "game/orchard.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace orchard {

static const float kTrees[][2] = {
    {40, 78},  {120, 74}, {200, 80}, {288, 76}, {80, 132}, {164, 128},
    {248, 134}, {40, 186}, {120, 190}, {204, 184}, {292, 188},
};
static const float kPiles[][3] = {
    {78, 52, 0},  {158, 48, 1}, {236, 54, 2}, {308, 108, 3},
    {18, 108, 1}, {112, 108, 0}, {196, 106, 3}, {268, 108, 2},
    {70, 160, 2}, {146, 164, 0}, {230, 158, 1}, {304, 160, 0},
    {78, 210, 3}, {160, 208, 2}, {246, 210, 0}, {20, 160, 1},
};

int Game::left() const {
    int n = 0;
    for (int i = 0; i < nPiles_; i++)
        if (piles_[i].live) n++;
    return n;
}

int Game::nearest() const {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < nPiles_; i++) {
        if (!piles_[i].live) continue;
        float dx = piles_[i].x - px_, dy = piles_[i].y - py_;
        float d = dx * dx + dy * dy;
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

bool Game::hitsTree(float x, float y) const {
    if (x < 12 || x > 308 || y < 28 || y > 216) return true;
    for (int i = 0; i < nTrees_; i++) {
        float dx = x - trees_[i].x;
        float dy = y - (trees_[i].y - 6);
        if (dx * dx + dy * dy < 13.f * 13.f) return true;
    }
    return false;
}

void Game::spr(const gs::Image& img, float x, float y, int pal, bool flip) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.12f);
    beep_ = 8;
}

void Game::layField() {
    gs::VDP& vdp = sys_->vdp;
    vdp.B.clear();
    vdp.A.clear();
    vdp.A.enabled = false;
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            int t = art_.grass[(x * 3 + y * 5) & 3];
            if (y == 6 || y == 13 || y == 20 || x == 0 || x == 39) t = art_.soil;
            vdp.B.set(x, y, gs::entry(t, PAL_FIELD));
        }
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(2, 7, 3);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::resetRound() {
    nTrees_ = int(sizeof kTrees / sizeof kTrees[0]);
    nPiles_ = int(sizeof kPiles / sizeof kPiles[0]);
    for (int i = 0; i < nTrees_; i++) trees_[i] = {kTrees[i][0], kTrees[i][1]};
    for (int i = 0; i < nPiles_; i++) piles_[i] = {kPiles[i][0], kPiles[i][1], int(kPiles[i][2]), true};
    px_ = 164;
    py_ = 96;
    face_ = 1;
    clock_ = 60 * 70;
    swing_ = 0;
    stuck_ = 0;
    won_ = false;
    over_ = false;
    mode_ = Mode::Play;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layField();
    sys.apu.setMaster(0.4f);
    sys.apu.silence();
    if (bot_) resetRound();
    else mode_ = Mode::Title;
}

void Game::hudRow(int row, const char* s) {
    for (int x = 0; x < 40; x++) sys_->vdp.HUD.set(x, row, 0);
    for (int i = 0; s[i] && i < 40; i++) {
        unsigned c = (unsigned char)s[i];
        if (c < 32 || c > 127) c = '?';
        sys_->vdp.HUD.set(i, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::updatePlay() {
    float mx = 0, my = 0;
    bool rake = false;
    if (bot_) {
        int i = nearest();
        if (i >= 0) {
            float tx = piles_[i].x, ty = piles_[i].y;
            float dx = tx - px_, dy = ty - py_;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d < 16.f) rake = true;
            else {
                if (stuck_ > 12) {
                    float nx = -dy / d, ny = dx / d;
                    float side = (stuck_ / 18) % 2 ? 1.f : -1.f;
                    tx = px_ + nx * side * 22.f;
                    ty = py_ + ny * side * 22.f;
                    dx = tx - px_;
                    dy = ty - py_;
                    d = std::sqrt(dx * dx + dy * dy) + 0.001f;
                }
                mx = dx / d;
                my = dy / d;
            }
        }
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) mx -= 1;
        if (p.down(gs::BTN_RIGHT)) mx += 1;
        if (p.down(gs::BTN_UP)) my -= 1;
        if (p.down(gs::BTN_DOWN)) my += 1;
        float m = std::sqrt(mx * mx + my * my);
        if (m > 0) {
            mx /= m;
            my /= m;
        }
        rake = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B);
    }

    if (swing_ > 0) {
        swing_--;
        mx = my = 0;
    }

    const float spd = 2.35f;
    float ox = px_, oy = py_;
    float nx = px_ + mx * spd;
    float ny = py_ + my * spd;
    if (!hitsTree(nx, py_)) px_ = nx;
    if (!hitsTree(px_, ny)) py_ = ny;
    if (mx > 0.2f) face_ = 1;
    if (mx < -0.2f) face_ = -1;
    if (std::fabs(px_ - ox) + std::fabs(py_ - oy) < 0.4f && (mx != 0 || my != 0)) stuck_++;
    else stuck_ = 0;

    if (rake && swing_ == 0) {
        int i = nearest();
        if (i >= 0) {
            float dx = piles_[i].x - px_, dy = piles_[i].y - py_;
            if (dx * dx + dy * dy < 18.f * 18.f) {
                piles_[i].live = false;
                swing_ = 10;
                aimX_ = piles_[i].x;
                aimY_ = piles_[i].y;
                blip(660.f + float((i & 3) * 40));
            }
        }
    }

    if (left() == 0) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        sys_->apu.tone(0, 523.f, 0.15f);
        sys_->apu.tone(1, 659.f, 0.12f);
        sys_->apu.tone(2, 784.f, 0.1f);
        beep_ = 20;
        return;
    }
    if (clock_ > 0) clock_--;
    if (clock_ <= 0) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        sys_->apu.noiseBurst(0.2f, 800.f, 0.4f);
    }
}

void Game::drawWorld() {
    struct Bit {
        float y;
        int kind;
        int i;
    };
    Bit bits[40];
    int n = 0;
    for (int i = 0; i < nTrees_; i++) bits[n++] = {trees_[i].y, 0, i};
    for (int i = 0; i < nPiles_; i++)
        if (piles_[i].live) bits[n++] = {piles_[i].y, 1, i};
    bits[n++] = {py_, 2, 0};
    for (int a = 1; a < n; a++) {
        Bit key = bits[a];
        int b = a;
        while (b > 0 && bits[b - 1].y < key.y) {
            bits[b] = bits[b - 1];
            b--;
        }
        bits[b] = key;
    }
    for (int i = 0; i < n; i++) {
        const Bit& b = bits[i];
        if (b.kind == 0) {
            const gs::Image& im = art_.tree;
            spr(im, trees_[b.i].x - im.w * 0.5f, trees_[b.i].y - im.h + 4, PAL_TREE);
        } else if (b.kind == 1) {
            const Pile& p = piles_[b.i];
            const gs::Image& im = art_.pile[p.kind];
            spr(im, p.x - im.w * 0.5f, p.y - im.h + 2, PAL_CLUTTER);
        } else {
            const gs::Image& im = art_.hero;
            spr(im, px_ - im.w * 0.5f, py_ - im.h + 2, PAL_HERO, face_ < 0);
            if (swing_ > 0) spr(art_.spark, aimX_ - 7, aimY_ - 12, PAL_BANNER);
        }
    }
}

void Game::drawBanner() {
    char line[48];
    if (mode_ == Mode::Title) {
        std::snprintf(line, sizeof line, "ONE ORCHARD  START TO RAKE");
        hudRow(0, line);
        hudRow(1, "A RAKES  CLEAR IT BEFORE THE CLOCK");
        spr(art_.title, (gs::SCREEN_W - art_.title.w) * 0.5f, 78, PAL_BANNER);
        spr(art_.sub, (gs::SCREEN_W - art_.sub.w) * 0.5f, 112, PAL_BANNER);
    } else if (mode_ == Mode::Play) {
        std::snprintf(line, sizeof line, "LEFT %02d", left());
        hudRow(0, line);
        std::snprintf(line, sizeof line, "CLOCK %02d", clock_ / 60);
        for (int i = 0; line[i]; i++) {
            unsigned c = (unsigned char)line[i];
            sys_->vdp.HUD.set(28 + i, 0, gs::entry(art_.font[c - 32], PAL_HUD));
        }
        hudRow(1, bot_ ? "RAKING THE GROUND" : "RAKE THE FALLEN GROUND");
    } else if (mode_ == Mode::Win) {
        hudRow(0, "DONE");
        hudRow(1, "THE GROUND IS CLEAR");
        spr(art_.win, (gs::SCREEN_W - art_.win.w) * 0.5f, 96, PAL_BANNER);
    } else {
        hudRow(0, "TOO LATE");
        hudRow(1, "THE CLOCK DIED");
        spr(art_.lose, (gs::SCREEN_W - art_.lose.w) * 0.5f, 96, PAL_BANNER);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        sys.apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) resetRound();
    } else if (mode_ == Mode::Play) {
        updatePlay();
    } else if (sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Title;
        over_ = false;
    }

    sys.vdp.clearSprites();
    drawBanner();
    drawWorld();
}

}  // namespace orchard
