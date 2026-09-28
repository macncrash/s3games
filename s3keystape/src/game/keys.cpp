#include "game/keys.h"

#include <cstdio>
#include <string>

namespace keys {

namespace {
const int kTape[NKEYS][NWARDS] = {
    {1, 2, 3}, {3, 1, 2}, {2, 3, 1}, {1, 1, 3}, {3, 2, 1},
};
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.5f);
    deal();
    mode_ = bot_ ? Mode::Play : Mode::Title;
}

void Game::deal() {
    for (int k = 0; k < NKEYS; k++)
        for (int w = 0; w < NWARDS; w++) {
            tape_[k][w] = kTape[k][w];
            have_[k][w] = 1;
        }
    sel_ = 0;
    ward_ = 0;
    t_ = 0;
    shake_ = 0;
    cool_ = 0;
    leaveT_ = 0;
    door_ = 0;
    over_ = false;
    won_ = false;
}

int Game::cut(int key, int ward) const {
    if (key < 0 || key >= NKEYS || ward < 0 || ward >= NWARDS) return 0;
    return have_[key][ward];
}

int Game::tapeCut(int key, int ward) const {
    if (key < 0 || key >= NKEYS || ward < 0 || ward >= NWARDS) return 0;
    return tape_[key][ward];
}

bool Game::matched() const {
    for (int k = 0; k < NKEYS; k++)
        for (int w = 0; w < NWARDS; w++)
            if (have_[k][w] != tape_[k][w]) return false;
    return true;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    cool_ = 4;
}

void Game::raise(int dir) {
    int& c = have_[sel_][ward_];
    c += dir;
    if (c < 1) c = 3;
    if (c > 3) c = 1;
    blip(180.f + float(c) * 90.f + float(ward_) * 40.f);
}

void Game::tryLeave() {
    if (matched()) {
        mode_ = Mode::Leave;
        leaveT_ = 0;
        blip(520.f);
        sys_->apu.tone(1, 780.f, 0.04f);
    } else {
        shake_ = 18;
        blip(90.f);
    }
}

void Game::botAct() {
    if (cool_ > 0) return;
    for (int k = 0; k < NKEYS; k++) {
        for (int w = 0; w < NWARDS; w++) {
            if (have_[k][w] != tape_[k][w]) {
                sel_ = k;
                ward_ = w;
                raise(1);
                return;
            }
        }
    }
    tryLeave();
}

void Game::human() {
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) {
        sel_ = (sel_ + NKEYS - 1) % NKEYS;
        blip(220.f);
    }
    if (p.pressed(gs::BTN_RIGHT)) {
        sel_ = (sel_ + 1) % NKEYS;
        blip(260.f);
    }
    if (p.pressed(gs::BTN_UP)) {
        ward_ = (ward_ + NWARDS - 1) % NWARDS;
        blip(300.f);
    }
    if (p.pressed(gs::BTN_DOWN)) {
        ward_ = (ward_ + 1) % NWARDS;
        blip(240.f);
    }
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z)) raise(1);
    if (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X)) raise(-1);
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_START)) tryLeave();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (cool_ > 0) {
        cool_--;
        if (cool_ == 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (shake_ > 0) shake_--;
    t_++;

    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Play;
            blip(440.f);
        }
    } else if (mode_ == Mode::Play) {
        if (bot_) botAct();
        else human();
    } else if (mode_ == Mode::Leave) {
        leaveT_++;
        if (door_ < 70) door_ += 2;
        if (leaveT_ > 48) {
            mode_ = Mode::Victory;
            won_ = true;
            over_ = true;
        }
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 2 + y / 40;
        if (g > 5) g = 5;
        sys.vdp.lineBackdrop[y] = gs::rgb4(g, g + 1, g + 3);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    sys.vdp.clearSprites();
    sys.vdp.HUD.clear();
    draw();
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

void Game::spr(const gs::Mipped& m, float x, float y, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    int jx = (shake_ % 4) ? (shake_ & 1 ? 2 : -2) : 0;
    spr(art_.door, 262.f + float(door_), 78.f, 120.f, PAL_DOOR);
    spr(art_.lamp, 24.f, 8.f, 16.f, PAL_DOOR);
    spr(art_.desk, 0, 140.f, 84.f, PAL_DESK);
    spr(art_.tape, 16.f, 22.f, 62.f, PAL_TAPE);
    spr(art_.drawer, 8.f + float(jx), 132.f, 78.f, PAL_DESK);

    for (int k = 0; k < NKEYS; k++) {
        float x = 28.f + float(k) * 40.f;
        for (int w = 0; w < NWARDS; w++) {
            if (w < tape_[k][w]) spr(art_.pip, x + 6.f, 32.f + float(w) * 14.f, 8.f, PAL_TAPE);
        }
        float bx = x + float(jx);
        float by = 168.f;
        spr(art_.bow, bx, by, 18.f, PAL_KEY);
        spr(art_.blade, bx + 7.f, by - 34.f, 36.f, PAL_KEY);
        for (int w = 0; w < NWARDS; w++) {
            int d = have_[k][w];
            if (d < 1) d = 1;
            if (d > 3) d = 3;
            spr(art_.tooth[d - 1], bx + 12.f, by - 32.f + float(w) * 10.f, 16.f, PAL_KEY);
        }
        if (mode_ == Mode::Play && k == sel_) {
            spr(art_.cursor, bx - 2.f, by + 18.f, 8.f, PAL_DOOR);
        }
    }

    if (mode_ == Mode::Title) {
        hudC(2, "S3 KEYSTAPE", PAL_HUD);
        hudC(4, "PLAY THE KEYS", PAL_HUD);
        hudC(6, "DRAWER MUST MATCH THE TAPE", PAL_HUD);
        hudC(24, "START TO BEGIN", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        hud(1, 1, "TAPE", PAL_HUD);
        hud(1, 26, "A CUT   ARROWS PICK   C LEAVE", PAL_HUD);
        if (shake_ > 0) hudC(12, "DOES NOT MATCH", PAL_HUD);
        else if (matched()) hudC(12, "MATCH  LEAVE", PAL_HUD);
        char mark[8];
        std::snprintf(mark, sizeof mark, "W%d", ward_ + 1);
        hud(34, 16, mark, PAL_HUD);
    } else {
        hudC(8, "THE DRAWER MATCHES", PAL_HUD);
        hudC(10, "YOU LEAVE", PAL_HUD);
    }
}

}  // namespace keys
