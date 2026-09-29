#include "game/drawermark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace drawermark {
namespace {

int centsOf(Kind k) { return k == Kind::Gold ? kMarkCents : k == Kind::One ? 100 : 0; }

const char* kindName(Kind k) {
    if (k == Kind::Gold) return "GOLD";
    if (k == Kind::One) return "ONE";
    return "BUTTON";
}

int palOf(Kind k) {
    if (k == Kind::Gold) return PAL_GOLD;
    if (k == Kind::One) return PAL_BILL;
    return PAL_JUNK;
}

}  // namespace

std::string Game::money(int cents) const {
    if (cents < 0) cents = -cents;
    char b[16];
    std::snprintf(b, sizeof b, "%d.%02d", cents / 100, cents % 100);
    return b;
}

int Game::drawerCents() const {
    int s = 0;
    for (int i = 0; i < n_; i++)
        if (pieces_[i].inDrawer) s += centsOf(pieces_[i].kind);
    return s;
}

bool Game::goldIn() const {
    for (int i = 0; i < n_; i++)
        if (pieces_[i].kind == Kind::Gold && pieces_[i].inDrawer) return true;
    return false;
}

bool Game::otherIn() const {
    for (int i = 0; i < n_; i++)
        if (pieces_[i].kind != Kind::Gold && pieces_[i].inDrawer) return true;
    return false;
}

bool Game::junkIn() const {
    for (int i = 0; i < n_; i++)
        if (pieces_[i].inDrawer && centsOf(pieces_[i].kind) == 0) return true;
    return false;
}

bool Game::seated() const { return goldIn() && !otherIn() && drawerCents() == kMarkCents; }

void Game::layout(bool snap) {
    int na = 0;
    for (int i = 0; i < n_; i++)
        if (!pieces_[i].inDrawer) na++;
    float pitch = na > 1 ? std::min(40.f, 240.f / float(na - 1)) : 40.f;
    float x0 = 160.f - pitch * float(std::max(0, na - 1)) * 0.5f;
    int ai = 0;
    int side = 0;
    for (int i = 0; i < n_; i++) {
        Piece& p = pieces_[i];
        float tx, ty;
        if (!p.inDrawer) {
            tx = x0 + ai * pitch;
            ty = kAsideY;
            ai++;
        } else if (p.kind == Kind::Gold) {
            tx = kWellX;
            ty = kWellY;
        } else {
            tx = 48.f + (side % 4) * 28.f;
            ty = 168.f;
            side++;
        }
        if (snap) {
            p.x = tx;
            p.y = ty;
        } else {
            p.x += (tx - p.x) * 0.5f;
            p.y += (ty - p.y) * 0.5f;
        }
    }
}

void Game::deal() {
    n_ = 0;
    auto add = [&](Kind k) {
        if (n_ >= 8) return;
        pieces_[n_].kind = k;
        pieces_[n_].inDrawer = false;
        pieces_[n_].x = 40.f + n_ * 30.f;
        pieces_[n_].y = kAsideY;
        n_++;
    };
    add(Kind::One);
    add(Kind::One);
    add(Kind::Button);
    add(Kind::Gold);
    add(Kind::One);
    add(Kind::One);
    add(Kind::One);
    sel_ = 0;
    tries_ = 3;
    wrong_ = 0;
    shake_ = 0;
    shutT_ = 0;
    cents_ = 0;
    onMark_ = false;
    reason_.clear();
    cool_ = bot_ ? 18 : 0;
    layout(true);
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 5;
}

void Game::nudge(int d) {
    if (n_ <= 0) return;
    sel_ = (sel_ + d + n_) % n_;
    blip(480.f + (sel_ % 5) * 40.f, 0.03f);
}

void Game::toggle() {
    if (n_ <= 0 || mode_ != Mode::Play) return;
    Piece& p = pieces_[sel_];
    p.inDrawer = !p.inDrawer;
    blip(p.inDrawer ? (p.kind == Kind::Gold ? 988.f : 740.f) : 330.f);
    onMark_ = seated();
    cents_ = drawerCents();
    if (onMark_) sys_->setLight(220, 160, 40);
}

void Game::tryShut() {
    if (mode_ != Mode::Play) return;
    cents_ = drawerCents();
    onMark_ = seated();
    if (!onMark_) {
        shake_ = 12;
        wrong_ = 50;
        tries_--;
        if (cents_ == kMarkCents && !goldIn()) reason_ = "SAME SUM IS NOT THE MARK";
        else if (goldIn() && otherIn()) reason_ = "ONLY THE GOLD";
        else if (junkIn()) reason_ = "JUNK IN THE TILL";
        else reason_ = "NOT ON THE MARK";
        sys_->apu.noiseBurst(0.35f, 240, 12);
        sys_->rumble(0.4f, 0.1f, 80);
        sys_->setLight(180, 30, 20);
        if (tries_ <= 0) {
            for (int i = 0; i < n_; i++) pieces_[i].inDrawer = false;
            tries_ = 3;
            reason_ = "COUNT IT AGAIN";
            wrong_ = 64;
            onMark_ = false;
            cents_ = 0;
            layout(true);
        }
        return;
    }
    mode_ = Mode::Shut;
    shutT_ = 0;
    sys_->apu.noiseBurst(0.2f, 160, 8);
    sys_->rumble(0.12f, 0.3f, 160);
    sys_->setLight(255, 190, 50);
}

void Game::botAct() {
    if (cool_ > 0) {
        cool_--;
        return;
    }
    for (int i = 0; i < n_; i++) {
        bool want = pieces_[i].kind == Kind::Gold;
        if (pieces_[i].inDrawer == want) continue;
        sel_ = i;
        toggle();
        cool_ = 8;
        return;
    }
    if (seated()) tryShut();
    else cool_ = 12;
}

void Game::human() {
    auto tap = [&](gs::Button b, int slot, auto&& fn) {
        int& h = hold_[slot];
        if (sys_->pad.pressed(b)) {
            h = 0;
            fn();
        } else if (sys_->pad.down(b)) {
            if (++h >= 14 && (h % 4) == 0) fn();
        } else h = 0;
    };
    tap(gs::BTN_LEFT, 0, [&] { nudge(-1); });
    tap(gs::BTN_RIGHT, 1, [&] { nudge(1); });
    tap(gs::BTN_UP, 2, [&] { nudge(-1); });
    tap(gs::BTN_DOWN, 3, [&] { nudge(1); });
    if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C)) toggle();
    if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_B)) tryShut();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    deal();
    mode_ = bot_ ? Mode::Play : Mode::Title;
    won_ = false;
    finished_ = false;
    shut_ = false;
    over_ = false;
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.1f, 0.18f, 0.08f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (shake_ > 0) shake_--;
    if (wrong_ > 0) wrong_--;
    cents_ = drawerCents();
    if (mode_ == Mode::Play) onMark_ = seated();

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            mode_ = Mode::Play;
            blip(523.f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (bot_) botAct();
        else human();
        if (pad.pressed(gs::BTN_MODE) && !bot_) mode_ = Mode::Title;
        layout(false);
    } else if (mode_ == Mode::Shut) {
        shutT_++;
        if (shutT_ == 1) sys.apu.tone(1, 523, 0.05f);
        else if (shutT_ == 8) sys.apu.tone(1, 659, 0.05f);
        else if (shutT_ == 16) sys.apu.tone(1, 784, 0.05f);
        else if (shutT_ == 24) sys.apu.tone(1, 1046, 0.06f);
        if (shutT_ >= 42) {
            sys.apu.tone(1, 0, 0);
            mode_ = Mode::Win;
            won_ = true;
            finished_ = true;
            shut_ = true;
            onMark_ = true;
            if (bot_) over_ = true;
            sys.setLight(255, 200, 80);
        }
    } else if (mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            won_ = false;
            finished_ = false;
            shut_ = false;
            deal();
            mode_ = Mode::Title;
        }
    }
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::box(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    gs::Sprite s{};
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float jx = (shake_ > 0) ? ((shake_ & 1) ? 2.f : -2.f) : 0.f;

    if (mode_ == Mode::Title) {
        box(art_.solid, 16, 70, 288, 120, PAL_SHADE);
        hudC(10, "S3 DRAWERMARK", PAL_AMBER);
        hudC(12, "THE GOLD COIN IS THE MARK", PAL_TEXT);
        hudC(14, "SEAT IT ALONE AND SHUT", PAL_AMBER);
        hudC(16, "FIVE ONES SUM THE SAME", PAL_TEXT);
        hudC(17, "AND STILL LEAVE IT OPEN", PAL_TEXT);
        hudC(19, "ARROWS PICK   A OR C MOVES", PAL_TEXT);
        hudC(21, "START SHUTS THE DRAWER", PAL_AMBER);
        if ((t_ / 30) % 2 == 0) hudC(24, "PRESS START", PAL_GREEN);
        return;
    }

    hud(1, 1, "MARK", PAL_INK);
    hud(8, 1, money(kMarkCents), PAL_AMBER);
    int dpal = onMark_ ? PAL_GREEN : PAL_AMBER;
    hud(22, 1, "DRAWER", PAL_TEXT);
    hud(30, 1, money(cents_), dpal);

    if (mode_ == Mode::Play && n_ > 0) {
        const Piece& sel = pieces_[sel_];
        std::string label = kindName(sel.kind);
        label += sel.kind == Kind::Gold ? "  THE MARK" : (centsOf(sel.kind) ? "  SILVER" : "  NOT CASH");
        label += sel.inDrawer ? "  IN" : "  OUT";
        hud(1, 3, label, sel.kind == Kind::Gold ? PAL_GOLD : PAL_TEXT);
        const gs::Mipped& pm = art_.piece[int(sel.kind)];
        int cpal = onMark_ ? PAL_GREEN : (sel.kind == Kind::Gold ? PAL_GOLD : PAL_AMBER);
        spr(art_.caret, sel.x + jx, sel.y - 8.f - pm.h * 0.55f, float(art_.caret.h), cpal);
        spr(pm, sel.x + jx, sel.y - 8.f, float(pm.h), palOf(sel.kind));
    }

    bool cover = mode_ == Mode::Shut || mode_ == Mode::Win;
    if (cover) {
        float top = kCoverTop;
        if (mode_ == Mode::Shut) {
            float u = std::min(1.f, shutT_ / 36.f);
            u = u * u * (3.f - 2.f * u);
            top = 224.f - u * (224.f - kCoverTop);
        }
        spr(art_.cover, 160.f + jx, top + art_.cover.h * 0.5f, float(art_.cover.h), PAL_WOOD);
    }

    for (int i = n_ - 1; i >= 0; i--) {
        if (mode_ == Mode::Play && i == sel_) continue;
        const Piece& p = pieces_[i];
        const gs::Mipped& m = art_.piece[int(p.kind)];
        spr(m, p.x + jx, p.y, float(m.h), palOf(p.kind));
    }

    if (mode_ == Mode::Play) {
        std::string st = reason_.empty() || wrong_ == 0 ? (onMark_ ? "GOLD IS ON THE MARK" : "SEAT THE GOLD ALONE") : reason_;
        hudC(26, st, (wrong_ > 0) ? PAL_RED : (onMark_ ? PAL_GREEN : PAL_AMBER));
        char tries[16];
        std::snprintf(tries, sizeof tries, "TRIES %d", tries_);
        hud(31, 26, tries, PAL_TEXT);
    } else if (mode_ == Mode::Shut) {
        hudC(26, "SHUTTING THE DRAWER", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        box(art_.solid, 36, 150, 248, 40, PAL_SHADE);
        hudC(20, "FINISHED MARK", PAL_GREEN);
        hudC(22, "THE GOLD SHUT THE DRAWER", PAL_AMBER);
    }
}

}  // namespace drawermark
