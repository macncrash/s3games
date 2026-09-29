#include "game/drawerseven.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace drawerseven {
namespace {

constexpr int kTape = 7;
constexpr int kGoal = 7;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::drawerCents() const {
    int c = 0;
    for (int i = 0; i < n_; i++)
        if (pieces_[i].inDrawer) {
            if (pieces_[i].kind == Kind::Penny) c += 1;
            else if (pieces_[i].kind == Kind::Nickel) c += 5;
        }
    return c;
}

bool Game::junkIn() const {
    for (int i = 0; i < n_; i++)
        if (pieces_[i].inDrawer && pieces_[i].kind == Kind::Button) return true;
    return false;
}

bool Game::matched() const { return drawerCents() == kTape && !junkIn(); }

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.04f); }

void Game::deal() {
    // Short till: nickel and two pennies make seven. The spare penny and the button stay out.
    const Kind kinds[6] = {Kind::Penny, Kind::Penny, Kind::Nickel, Kind::Penny, Kind::Button, Kind::Penny};
    const bool belong[6] = {true, true, true, false, false, false};
    n_ = 6;
    for (int i = 0; i < n_; i++) {
        pieces_[i].kind = kinds[i];
        pieces_[i].belongs = belong[i];
        pieces_[i].inDrawer = false;
    }
    sel_ = 0;
    shake_ = 0;
    cool_ = 2;
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    yours_ = true;
    over_ = false;
    won_ = false;
    shutT_ = 0;
    deal();
    mode_ = Mode::Play;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

void Game::nudge(int d) {
    if (n_ <= 0 || mode_ != Mode::Play) return;
    sel_ = (sel_ + d + n_) % n_;
    blip(480.f + float(sel_) * 40.f);
}

void Game::toggle() {
    if (mode_ != Mode::Play || n_ <= 0) return;
    pieces_[sel_].inDrawer = !pieces_[sel_].inDrawer;
    blip(pieces_[sel_].inDrawer ? 880.f : 330.f);
}

void Game::tryClose() {
    if (mode_ != Mode::Play) return;
    if (!matched()) {
        shake_ = 10;
        sys_->apu.noiseBurst(0.28f, 220.f, 0.12f);
        return;
    }
    mode_ = Mode::Shut;
    shutT_ = 0;
    sys_->apu.tone(1, 523.f, 0.06f);
    sys_->apu.noiseBurst(0.16f, 140.f, 0.08f);
}

void Game::afterShut() {
    if (yours_) you_++;
    else them_++;
    if (you_ >= kGoal && them_ < kGoal) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        sys_->apu.tone(0, 523.f, 0.08f);
        sys_->apu.tone(1, 659.f, 0.07f);
        sys_->apu.tone(2, 784.f, 0.06f);
        return;
    }
    if (them_ >= kGoal && you_ < kGoal) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        sys_->apu.noiseBurst(0.3f, 90.f, 0.2f);
        return;
    }
    yours_ = !yours_;
    deal();
    mode_ = Mode::Play;
    sys_->apu.tone(1, 0, 0);
}

void Game::botAct() {
    if (cool_ > 0) {
        cool_--;
        return;
    }
    for (int i = 0; i < n_; i++) {
        if (pieces_[i].inDrawer == pieces_[i].belongs) continue;
        sel_ = i;
        toggle();
        cool_ = 2;
        return;
    }
    if (matched()) tryClose();
}

void Game::human() {
    auto tap = [&](gs::Button b, int slot, auto&& fn) {
        int& h = hold_[slot];
        if (sys_->pad.pressed(b)) {
            h = 0;
            fn();
        } else if (sys_->pad.down(b)) {
            if (++h >= 12 && (h % 3) == 0) fn();
        } else h = 0;
    };
    tap(gs::BTN_LEFT, 0, [&] { nudge(-1); });
    tap(gs::BTN_RIGHT, 1, [&] { nudge(1); });
    tap(gs::BTN_UP, 2, [&] { nudge(-1); });
    tap(gs::BTN_DOWN, 3, [&] { nudge(1); });
    if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C)) toggle();
    if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_B)) tryClose();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    you_ = 0;
    them_ = 0;
    yours_ = true;
    over_ = false;
    won_ = false;
    deal();
    mode_ = bot_ ? Mode::Play : Mode::Title;
    sys.apu.setMaster(0.8f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (shake_ > 0) shake_--;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            blip(523.f);
        }
    } else if (mode_ == Mode::Play) {
        const bool autoClerk = bot_ || !yours_;
        if (autoClerk) botAct();
        else human();
    } else if (mode_ == Mode::Shut) {
        if (++shutT_ > 14) afterShut();
    } else if ((mode_ == Mode::Win || mode_ == Mode::Lose) && !bot_) {
        if (pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 1, 3);
    uint16_t mid = gs::rgb4(6, 3, 2);
    uint16_t bot = gs::rgb4(2, 1, 1);
    if (mode_ == Mode::Win || mode_ == Mode::Shut) mid = gs::rgb4(10, 7, 2);
    if (mode_ == Mode::Lose) {
        top = gs::rgb4(3, 0, 1);
        mid = gs::rgb4(6, 1, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        v.lineBackdrop[y] = u < 0.5f ? lerpC(top, mid, u / 0.5f) : lerpC(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
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

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    sky();

    float jx = (shake_ > 0) ? float((shake_ & 1) ? 2 : -2) : 0.f;
    spr(art_.desk, 160.f + jx, 128.f, PAL_WOOD);
    float dy = 176.f;
    if (mode_ == Mode::Shut) dy = 176.f + float(shutT_) * 1.4f;
    if (mode_ == Mode::Win || mode_ == Mode::Lose) dy = 198.f;
    spr(art_.drawer, 160.f + jx, dy, PAL_WOOD);
    spr(art_.lamp, 42.f, 58.f, PAL_LAMP);

    for (int i = 0; i < n_; i++) {
        float x = 48.f + float(i) * 44.f + jx;
        float y = pieces_[i].inDrawer ? 168.f : 96.f;
        if (mode_ == Mode::Shut && pieces_[i].inDrawer) y += float(shutT_) * 1.2f;
        const gs::Image* img = &art_.penny;
        int pal = PAL_COIN;
        if (pieces_[i].kind == Kind::Nickel) {
            img = &art_.nickel;
            pal = PAL_NICK;
        } else if (pieces_[i].kind == Kind::Button) {
            img = &art_.button;
            pal = PAL_JUNK;
        }
        spr(*img, x, y, pal);
        if (i == sel_ && mode_ == Mode::Play) spr(art_.caret, x, y - 22.f, PAL_GOLD);
    }

    char buf[64];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 DRAWER SEVEN", PAL_GOLD);
        hudC(21, "A SHORT DRAWER", PAL_INK);
        hudC(22, "NICKEL AND TWO CENTS MAKE SEVEN", PAL_HUD);
        hudC(23, "FIRST TRUE CLOSE TO SEVEN", PAL_GOLD);
        hudC(24, "LEFT RIGHT  A FILES  START SHUTS", PAL_DIM);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Win) {
        hudC(1, "FIRST TO SEVEN", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(22, buf, PAL_GOLD);
        hudC(24, "THE SHORT DRAWER IS CLOSED", PAL_HUD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "THEY GOT THERE FIRST", PAL_BAD);
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(22, buf, PAL_BAD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 1, buf, yours_ ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(28, 1, buf, yours_ ? PAL_HUD : PAL_BAD);
        hudC(22, yours_ ? "YOUR COUNT" : "THEIR COUNT", yours_ ? PAL_GOLD : PAL_BAD);
        std::snprintf(buf, sizeof buf, "DRAWER %d CENTS   TAPE %d", drawerCents(), kTape);
        hudC(23, buf, matched() ? PAL_GOLD : PAL_INK);
        if (mode_ == Mode::Shut) hudC(25, "SHUT", PAL_GOLD);
        else if (!matched()) hudC(25, "FILE TO SEVEN  LEAVE THE BUTTON", PAL_DIM);
    }
}

}  // namespace drawerseven
