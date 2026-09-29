#include "game/boardseven.h"

#include <cstdio>
#include <cstring>

namespace boardseven {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
}

void Game::open() {
    you_ = 0;
    them_ = 0;
    cursor_ = 0;
    over_ = false;
    won_ = false;
    mode_ = Mode::Play;
    nextCall();
}

void Game::nextCall() {
    rng_ = rng_ * 1664525u + 1013904223u;
    mark_ = int((rng_ >> 16) % JACKS);
    life_ = 150;
    flash_ = 0;
    act_ = bot_ ? 8 : 0;
}

void Game::plug(int jack) {
    if (mode_ != Mode::Play) return;
    bool good = jack == mark_;
    if (good) you_++;
    else them_++;
    flash_ = good ? 10 : 8;
    sys_->apu.tone(0, good ? 880.f : 140.f, 0.18f);
    beep_ = 6;
    if (you_ >= 7 && you_ > them_) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        sys_->apu.tone(1, 660.f, 0.16f);
        return;
    }
    if (them_ >= 7) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        return;
    }
    nextCall();
    flash_ = good ? 10 : 8;
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 18) open();
        return;
    }
    if (mode_ != Mode::Play) return;
    if (act_ > 0) {
        act_--;
        return;
    }
    cursor_ = mark_;
    plug(mark_);
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
    if (p.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + JACKS - 1) % JACKS;
    if (p.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % JACKS;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B)) plug(cursor_);
}

float Game::jackX(int i) const { return 52.f + i * 60.f; }

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Play && life_ > 0 && --life_ == 0) {
        them_++;
        sys.apu.tone(0, 90.f, 0.16f);
        beep_ = 8;
        if (them_ >= 7) {
            mode_ = Mode::Lose;
            over_ = true;
            won_ = false;
        } else {
            nextCall();
        }
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

void Game::box(float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1) return;
    gs::Sprite s{};
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.img = art_.solid.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    if (mode_ == Mode::Title) {
        hudC(3, "S3 BOARD SEVEN", PAL_AMBER);
        hudC(6, "A SHORT BOARD", PAL_TEXT);
        hudC(10, "MATCH THE LAMP", PAL_IVORY);
        hudC(12, "FIRST TO SEVEN", PAL_GREEN);
        hudC(24, "A PLUGS   ARROWS MOVE", PAL_TEXT);
        spr(art_.lamp, 160, 150, 36, PAL_LAMP);
        return;
    }

    char buf[32];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(2, 1, buf, PAL_GREEN);
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(30, 1, buf, PAL_RED);
    hudC(1, "FIRST TO SEVEN", PAL_AMBER);

    float lx = 160.f;
    float ly = 78.f;
    int lampPal = (flash_ > 0) ? PAL_IVORY : PAL_LAMP;
    if (flash_ > 0) flash_--;
    spr(art_.lamp, lx, ly, 32, lampPal);
    if (mode_ == Mode::Play) spr(art_.mark[mark_], lx, ly - 2, 14, PAL_MARK);

    float px = jackX(cursor_);
    box(lx - 1.5f, ly + 10, 3, 42, PAL_CORD);
    float cordY = ly + 50;
    float x0 = lx < px ? lx : px;
    float x1 = lx < px ? px : lx;
    box(x0, cordY, x1 - x0 + 3, 3, PAL_CORD);
    box(px - 1, cordY, 3, 18, PAL_CORD);

    for (int i = 0; i < JACKS; i++) {
        float x = jackX(i);
        spr(art_.jack, x, 142, 26, PAL_BRASS);
        spr(art_.mark[i], x, 142, 12, PAL_MARK);
        if (i == cursor_) spr(art_.plug, x, 128, 16, PAL_IVORY);
    }

    if (mode_ == Mode::Win) hudC(24, "YOU ARE FIRST TO SEVEN", PAL_GREEN);
    else if (mode_ == Mode::Lose) hudC(24, "THEY REACHED SEVEN", PAL_RED);
    else hudC(24, "PLUG THE LIT MARK", PAL_TEXT);
}

}  // namespace boardseven
