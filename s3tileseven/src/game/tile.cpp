#include "game/tile.h"

#include <cstdio>
#include <cstring>

namespace tileseven {

// Player, rival, player, rival... Player sums to 7 first (3+2+2).
constexpr int DECK[] = {3, 2, 2, 2, 2, 1};
constexpr int DECK_N = 6;

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
}

void Game::open() {
    you_ = 0;
    them_ = 0;
    deck_ = 0;
    over_ = false;
    won_ = false;
    mode_ = Mode::Play;
    yours_ = true;
    deal();
}

void Game::deal() {
    tile_ = DECK[deck_ % DECK_N];
    deck_++;
    life_ = yours_ ? 150 : 28;
    act_ = bot_ ? 6 : 0;
    flash_ = 0;
}

void Game::finishIf() {
    if (you_ >= 7 && you_ > them_) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        sys_->apu.tone(1, 660.f, 0.16f);
        beep_ = 10;
        return;
    }
    if (them_ >= 7) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        sys_->apu.tone(0, 110.f, 0.16f);
        beep_ = 10;
    }
}

void Game::claim(bool yours) {
    if (mode_ != Mode::Play) return;
    if (yours) you_ += tile_;
    else them_ += tile_;
    flash_ = yours ? 8 : 6;
    sys_->apu.tone(0, yours ? 520.f : 180.f, 0.14f);
    beep_ = 5;
    yours_ = !yours_;
    finishIf();
    if (mode_ == Mode::Play) deal();
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 16) open();
        return;
    }
    if (mode_ != Mode::Play || !yours_) return;
    if (act_ > 0) {
        act_--;
        return;
    }
    claim(true);
}

void Game::human() {
    gs::Pad& p = sys_->pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) open();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
        }
        return;
    }
    if (!yours_) return;
    if (p.pressed(gs::BTN_A)) claim(true);
    else if (p.pressed(gs::BTN_B)) claim(false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Play && life_ > 0 && --life_ == 0) {
        if (yours_) claim(false);
        else claim(false);
    }
    if (bot_) botAct();
    else human();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(w < 1 ? 1 : w);
    s.h = int16_t(h < 1 ? 1 : h);
    s.x = int16_t(cx - s.w * 0.5f);
    s.y = int16_t(cy - s.h * 0.5f);
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::pips(float cx, float cy, int n, float h, int pal) {
    if (n <= 0) return;
    if (n > 3) n = 3;
    float dx = h * 1.3f;
    float x0 = cx - (n - 1) * dx * 0.5f;
    for (int i = 0; i < n; i++) spr(art_.pip, x0 + i * dx, cy, h, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    if (mode_ == Mode::Title) {
        hudC(2, "S3 TILE SEVEN", PAL_AMBER);
        hudC(5, "A SHORT TILE", PAL_TEXT);
        hudC(8, "LAY PIPS ON YOUR COURSE", PAL_IVORY);
        hudC(10, "FIRST TO SEVEN", PAL_GREEN);
        hudC(24, "A TAKES   B GIVES IT AWAY", PAL_TEXT);
        spr(art_.tile, 160, 140, 48, PAL_CLAY);
        pips(160, 140, 3, 8, PAL_INK);
        return;
    }

    char buf[32];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(2, 1, buf, PAL_GREEN);
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(30, 1, buf, PAL_RED);
    hudC(1, "FIRST TO SEVEN", PAL_AMBER);
    hud(2, 4, "YOUR COURSE", PAL_GREEN);
    hud(28, 15, "THEIRS", PAL_RED);

    for (int i = 0; i < 7; i++) {
        float x = 42.f + i * 40.f;
        spr(art_.niche, x, 63, 18, (i < you_) ? PAL_GREEN : PAL_IVORY);
        spr(art_.niche, x, 151, 18, (i < them_) ? PAL_RED : PAL_IVORY);
        if (i < you_) spr(art_.tile, x, 63, 16, PAL_CLAY);
        if (i < them_) spr(art_.tile, x, 151, 16, PAL_GLAZE);
    }

    if (mode_ == Mode::Play) {
        int pal = yours_ ? PAL_CLAY : PAL_GLAZE;
        if (flash_ > 0) {
            flash_--;
            pal = PAL_IVORY;
        }
        spr(art_.tile, 160, 108, 40, pal);
        pips(160, 108, tile_, 7, PAL_INK);
        hudC(22, yours_ ? "YOUR TILE" : "THEIR TILE", yours_ ? PAL_GREEN : PAL_RED);
    }
    if (mode_ == Mode::Win) hudC(22, "YOU ARE FIRST TO SEVEN", PAL_GREEN);
    else if (mode_ == Mode::Lose) hudC(22, "THEY REACHED SEVEN", PAL_RED);
}

}  // namespace tileseven
