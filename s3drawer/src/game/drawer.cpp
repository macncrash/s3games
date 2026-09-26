#include "game/drawer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace drawer {
namespace {

struct Line {
    const char* name;
    int cents;
};

struct Spec {
    Kind kind;
    bool belongs;
    bool inDrawer;
};

struct NightDef {
    const char* name;
    const char* tag;
    Line lines[4];
    int nLines;
    Spec specs[16];
    int nSpecs;
};

// Three nights. The lines are the tape. Pieces that belong are the take.
// Anything else in the till (junk, or cash the tape never saw) keeps it from matching.
const NightDef kNights[3] = {
    {"TUESDAY", "TUE", {{"SODA", 110}, {"BREAD", 500}, {"PAPER", 150}}, 3,
     {{Kind::Bill5, true, true},
      {Kind::Bill1, true, true},
      {Kind::Bill1, true, false},
      {Kind::Quarter, true, true},
      {Kind::Quarter, true, true},
      {Kind::Dime, true, true},
      {Kind::Nickel, false, true},
      {Kind::Button, false, true}},
     8},
    {"WEDNESDAY", "WED", {{"MILK", 325}, {"SOAP", 406}, {"TIN", 1000}}, 3,
     {{Kind::Bill10, true, true},
      {Kind::Bill5, true, false},
      {Kind::Bill1, true, true},
      {Kind::Bill1, true, true},
      {Kind::Quarter, true, true},
      {Kind::Nickel, true, true},
      {Kind::Penny, true, false},
      {Kind::Iou, false, true},
      {Kind::Dime, false, true}},
     9},
    {"THURSDAY", "THU", {{"FLOUR", 899}, {"LAMP", 1200}, {"SOAP", 678}, {"TIN", 1000}}, 4,
     {{Kind::Bill20, true, true},
      {Kind::Bill10, true, false},
      {Kind::Bill5, true, true},
      {Kind::Bill1, true, true},
      {Kind::Bill1, true, false},
      {Kind::Quarter, true, true},
      {Kind::Quarter, true, true},
      {Kind::Dime, true, true},
      {Kind::Dime, true, true},
      {Kind::Nickel, true, true},
      {Kind::Penny, true, true},
      {Kind::Penny, true, true},
      {Kind::Token, false, true},
      {Kind::Bill1, false, true},
      {Kind::Quarter, false, false},
      {Kind::Clip, false, true}},
     16},
};

const NightDef& def(int i) { return kNights[i < 0 ? 0 : (i > 2 ? 2 : i)]; }

int centsOf(Kind k) {
    switch (k) {
    case Kind::Bill20: return 2000;
    case Kind::Bill10: return 1000;
    case Kind::Bill5: return 500;
    case Kind::Bill1: return 100;
    case Kind::Quarter: return 25;
    case Kind::Dime: return 10;
    case Kind::Nickel: return 5;
    case Kind::Penny: return 1;
    default: return 0;
    }
}

const char* kindName(Kind k) {
    switch (k) {
    case Kind::Bill20: return "TWENTY";
    case Kind::Bill10: return "TEN";
    case Kind::Bill5: return "FIVE";
    case Kind::Bill1: return "ONE";
    case Kind::Quarter: return "QUARTER";
    case Kind::Dime: return "DIME";
    case Kind::Nickel: return "NICKEL";
    case Kind::Penny: return "PENNY";
    case Kind::Button: return "BUTTON";
    case Kind::Iou: return "IOU";
    case Kind::Token: return "TOKEN";
    case Kind::Clip: return "CLIP";
    case Kind::Count: break;
    }
    return "?";
}

int slotOf(Kind k) {
    switch (k) {
    case Kind::Bill20: return 0;
    case Kind::Bill10: return 1;
    case Kind::Bill5: return 2;
    case Kind::Bill1: return 3;
    case Kind::Quarter: return 4;
    case Kind::Dime: return 5;
    case Kind::Nickel: return 6;
    case Kind::Penny: return 7;
    default: return -1;
    }
}

int palOf(Kind k) {
    switch (k) {
    case Kind::Bill20:
    case Kind::Bill10:
    case Kind::Bill5:
    case Kind::Bill1: return PAL_BILL;
    case Kind::Quarter:
    case Kind::Dime:
    case Kind::Nickel:
    case Kind::Penny: return PAL_COIN;
    default: return PAL_JUNK;
    }
}

std::string cols(const char* left, const std::string& right) {
    std::string s(12, ' ');
    for (int i = 0; left[i] && i < 12; i++) s[i] = left[i];
    int n = int(right.size());
    if (n > 12) n = 12;
    for (int i = 0; i < n; i++) s[12 - n + i] = right[i];
    return s;
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

bool Game::junkInside() const {
    for (int i = 0; i < n_; i++)
        if (pieces_[i].inDrawer && centsOf(pieces_[i].kind) == 0) return true;
    return false;
}

bool Game::matched() const { return drawerCents() == tapeCents_ && !junkInside(); }

std::string Game::statusText() const {
    if (wrong_ > 0 && !reason_.empty()) return reason_;
    int d = drawerCents() - tapeCents_;
    if (d == 0 && junkInside()) return "JUNK IN THE TILL";
    if (d == 0) return "MATCHES THE TAPE";
    return std::string(d > 0 ? "OVER " : "SHORT ") + money(d);
}

int Game::statusPal() const {
    if (wrong_ > 0) return PAL_RED;
    int d = drawerCents() - tapeCents_;
    if (d == 0 && junkInside()) return PAL_RED;
    if (d == 0) return PAL_GREEN;
    return PAL_AMBER;
}

void Game::cacheTapes() {
    for (int i = 0; i < 3; i++) {
        int lines = 0, belong = 0;
        const NightDef& n = def(i);
        for (int L = 0; L < n.nLines; L++) lines += n.lines[L].cents;
        for (int s = 0; s < n.nSpecs; s++)
            if (n.specs[s].belongs) belong += centsOf(n.specs[s].kind);
        if (lines != belong) std::fprintf(stderr, "s3drawer night %d tape %d pieces %d\n", i, lines, belong);
        tape_[i] = lines;
    }
}

void Game::layout(bool snap) {
    int stack[8] = {};
    int mess = 0;
    int na = 0;
    for (int i = 0; i < n_; i++)
        if (!pieces_[i].inDrawer) na++;

    float pitch = 34.f;
    if (na > 1) pitch = std::min(36.f, 220.f / float(na - 1));
    float x0 = 196.f - pitch * float(na - 1) * 0.5f;

    int ai = 0;
    for (int i = 0; i < n_; i++) {
        Piece& p = pieces_[i];
        float tx, ty;
        if (!p.inDrawer) {
            tx = x0 + ai * pitch;
            ty = kAsideY;
            ai++;
        } else {
            int s = slotOf(p.kind);
            if (s < 0) {
                tx = 56.f + (mess % 6) * 40.f;
                ty = kMessY;
                mess++;
            } else {
                int k = stack[s]++;
                tx = kSlotX0 + s * kSlotPitch;
                ty = kDrawerY - k * 5.f;
            }
        }
        if (snap) {
            p.x = tx;
            p.y = ty;
        } else {
            if (std::fabs(tx - p.x) < 0.6f) p.x = tx;
            else p.x += (tx - p.x) * 0.5f;
            if (std::fabs(ty - p.y) < 0.6f) p.y = ty;
            else p.y += (ty - p.y) * 0.5f;
        }
    }
}

void Game::deal() {
    const NightDef& n = def(night_);
    tapeCents_ = tape_[night_];
    n_ = n.nSpecs;
    if (n_ > 16) n_ = 16;
    for (int i = 0; i < n_; i++) {
        pieces_[i].kind = n.specs[i].kind;
        pieces_[i].belongs = n.specs[i].belongs;
        pieces_[i].inDrawer = n.specs[i].inDrawer;
        pieces_[i].startIn = n.specs[i].inDrawer;
        pieces_[i].x = 0;
        pieces_[i].y = 0;
    }
    sel_ = 0;
    tries_ = 3;
    wrong_ = 0;
    shake_ = 0;
    shutT_ = 0;
    reason_.clear();
    cool_ = bot_ ? 14 : 0;
    layout(true);
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 5;
}

void Game::nudge(int d) {
    if (n_ <= 0) return;
    sel_ = (sel_ + d + n_) % n_;
    blip(520.f + (sel_ % 5) * 36.f, 0.03f);
}

void Game::toggle() {
    if (n_ <= 0 || mode_ != Mode::Play) return;
    Piece& p = pieces_[sel_];
    bool was = matched();
    p.inDrawer = !p.inDrawer;
    blip(p.inDrawer ? 880.f : 392.f);
    if (!was && matched()) {
        blip(1174.f, 0.05f);
        sys_->setLight(80, 180, 60);
    }
}

void Game::tryClose() {
    if (mode_ != Mode::Play) return;
    if (!matched()) {
        shake_ = 14;
        wrong_ = 54;
        tries_--;
        int d = drawerCents() - tapeCents_;
        if (d == 0 && junkInside()) reason_ = "JUNK IN THE TILL";
        else reason_ = "DOES NOT MATCH";
        sys_->apu.noiseBurst(0.4f, 280, 14);
        sys_->apu.tone(2, 70, 0.08f);
        beep_ = 12;
        sys_->rumble(0.5f, 0.1f, 90);
        sys_->setLight(180, 30, 20);
        if (tries_ <= 0) {
            for (int i = 0; i < n_; i++) pieces_[i].inDrawer = pieces_[i].startIn;
            tries_ = 3;
            reason_ = "COUNT IT AGAIN";
            wrong_ = 70;
            layout(true);
        }
        return;
    }
    mode_ = Mode::Shut;
    shutT_ = 0;
    sys_->apu.noiseBurst(0.22f, 180, 10);
    sys_->rumble(0.15f, 0.35f, 180);
    sys_->setLight(255, 190, 60);
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
        cool_ = 5;
        return;
    }
    if (matched()) tryClose();
    else cool_ = 20;
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
    if (sys_->pad.pressed(gs::BTN_START)) tryClose();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    cacheTapes();
    night_ = 0;
    deal();
    mode_ = bot_ ? Mode::Play : Mode::Title;
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.2f, 0.1f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(2, 0, 0);
    }
    if (shake_ > 0) shake_--;
    if (wrong_ > 0) wrong_--;

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
        if (shutT_ >= 40) {
            sys.apu.tone(1, 0, 0);
            if (night_ >= 2) {
                mode_ = Mode::Victory;
                won_ = true;
                if (bot_) over_ = true;
                sys.setLight(255, 210, 120);
            } else {
                mode_ = Mode::Between;
                betweenT_ = 0;
            }
        }
    } else if (mode_ == Mode::Between) {
        betweenT_++;
        if (betweenT_ >= 48) {
            night_++;
            deal();
            mode_ = Mode::Play;
            blip(659.f);
        }
    } else if (mode_ == Mode::Victory) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
            won_ = false;
            night_ = 0;
            deal();
            mode_ = Mode::Title;
            blip(523.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            night_ = 0;
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
    s.w = int16_t(std::clamp(std::lround(w), 1L, 2000L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::box(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1) return;
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
    bool cover = mode_ == Mode::Shut || mode_ == Mode::Between || mode_ == Mode::Victory;

    if (mode_ == Mode::Title) {
        box(art_.solid, 10, 112, 300, 104, PAL_SHADE);
        box(art_.solid, 6, 108, 308, 112, PAL_AMBER);
        hudC(14, "S3 DRAWER", PAL_AMBER);
        hudC(16, "CLOSE THE SHOP", PAL_TEXT);
        hudC(18, "THE DRAWER MATCHES THE TAPE", PAL_AMBER);
        hudC(20, "TILL COUNTS. JUNK STAYS OUT.", PAL_TEXT);
        hudC(22, "ARROWS PICK. Z OR C MOVES.", PAL_TEXT);
        hudC(24, "ENTER SHUTS THE DRAWER", PAL_AMBER);
        if ((t_ / 30) % 2 == 0) hudC(26, "PRESS START", PAL_GREEN);
        return;
    }

    // Receipt, then the live count in the register window.
    const NightDef& n = def(night_);
    hud(1, 1, n.name, PAL_INK);
    hud(1, 2, "------------", PAL_INK);
    for (int i = 0; i < n.nLines; i++) hud(1, 3 + i, cols(n.lines[i].name, money(n.lines[i].cents)), PAL_INK);
    int row = 3 + n.nLines;
    hud(1, row, "------------", PAL_INK);
    hud(1, row + 1, cols("TOTAL", money(tapeCents_)), PAL_INK);

    char nightBuf[20];
    std::snprintf(nightBuf, sizeof nightBuf, "NIGHT %d OF 3", night_ + 1);
    hud(18, 4, nightBuf, PAL_AMBER);

    int dpal = PAL_AMBER;
    if (drawerCents() == tapeCents_) dpal = junkInside() ? PAL_RED : PAL_GREEN;
    hud(17, 7, "DRAWER", PAL_TEXT);
    hud(17, 8, money(drawerCents()), dpal);
    hud(17, 10, "TAPE", PAL_TEXT);
    hud(17, 11, money(tapeCents_), PAL_TEXT);

    if (mode_ == Mode::Play && n_ > 0) {
        const Piece& sel = pieces_[sel_];
        std::string label = kindName(sel.kind);
        if (centsOf(sel.kind) == 0) label += "  NOT CASH";
        else label += std::string("  ") + money(centsOf(sel.kind));
        label += sel.inDrawer ? "  IN" : "  OUT";
        hud(17, 12, label, centsOf(sel.kind) == 0 ? PAL_RED : PAL_AMBER);
    }

    // Caret and the closed sign sit above the till. The cover hides what was counted.
    // Earlier sprites are drawn on top. Sign and banners, then the sliding front, then the caret,
    // then the cash (the picked bill above the rest of the stack).
    if (mode_ == Mode::Victory) {
        spr(art_.sign, kSignX + kSignW * 0.5f, kSignY + kSignH * 0.5f, float(kSignH), PAL_SIGN);
        box(art_.solid, 28, 156, 264, 52, PAL_SHADE);
        hudC(20, "SHOP CLOSED", PAL_GREEN);
        hudC(22, "THE DRAWER MATCHES THE TAPE", PAL_AMBER);
        std::string days = std::string(def(0).tag) + " " + money(tape_[0]) + "  " + def(1).tag + " " + money(tape_[1]) +
                           "  " + def(2).tag + " " + money(tape_[2]);
        hudC(24, days, PAL_TEXT);
        hudC(27, "START OPENS TOMORROW", PAL_AMBER);
    } else if (mode_ == Mode::Between) {
        box(art_.solid, 48, 150, 224, 36, PAL_SHADE);
        hudC(19, "THE DRAWER MATCHES", PAL_GREEN);
        hudC(21, n.name, PAL_AMBER);
    }

    if (cover) {
        float top = kCoverTop;
        if (mode_ == Mode::Shut) {
            float u = std::min(1.f, shutT_ / 36.f);
            u = u * u * (3.f - 2.f * u);
            top = 224.f - u * (224.f - kCoverTop);
        }
        spr(art_.cover, 160.f + jx, top + art_.cover.h * 0.5f, float(art_.cover.h), PAL_WOOD);
    }

    auto drawPiece = [&](int i, float lift) {
        const Piece& p = pieces_[i];
        const gs::Mipped& m = art_.piece[int(p.kind)];
        spr(m, p.x + jx, p.y + lift, float(m.h), palOf(p.kind));
    };
    if (mode_ == Mode::Play && n_ > 0) {
        const Piece& sel = pieces_[sel_];
        const gs::Mipped& pm = art_.piece[int(sel.kind)];
        float lift = -7.f;
        int cpal = matched() ? PAL_GREEN : (centsOf(sel.kind) == 0 ? PAL_RED : PAL_AMBER);
        spr(art_.caret, sel.x + jx, sel.y + lift - pm.h * 0.5f - 4.f, float(art_.caret.h), cpal);
        drawPiece(sel_, lift);
    }
    int order[16];
    int no = 0;
    for (int i = 0; i < n_; i++) {
        if (mode_ == Mode::Play && i == sel_) continue;
        order[no++] = i;
    }
    std::sort(order, order + no, [&](int a, int b) { return pieces_[a].y > pieces_[b].y; });
    for (int k = 0; k < no; k++) drawPiece(order[k], 0);

    if (mode_ == Mode::Play) {
        hudC(26, statusText(), statusPal());
        char tries[12];
        std::snprintf(tries, sizeof tries, "TRIES %d", tries_);
        hud(32, 26, tries, PAL_TEXT);
        if (!matched() || ((t_ / 16) % 2) == 0) hudC(27, "Z OR C MOVES   ENTER SHUTS", PAL_TEXT);
    } else if (mode_ == Mode::Shut) {
        hudC(26, "SHUTTING THE DRAWER", PAL_AMBER);
    }
}

}  // namespace drawer
