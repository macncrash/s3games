#include "game/shelvechime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace shelvechime {
namespace {
constexpr float kPi = 3.14159265f;
constexpr char kLetter[4] = {'A', 'B', 'C', 'D'};
constexpr int kOrder[4] = {1, 3, 0, 2};
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    anim_ = Anim::None;
    over_ = false;
    won_ = false;
    reason_ = "";
    cursor_ = 0;
    cartN_ = 0;
    shelved_ = 0;
    returns_ = 0;
    playFrames_ = 0;
    msg_ = 0;
    badRow_ = -1;
    fan_ = -1;
    strike_ = 0;
    endHold_ = 0;
    flightOnHour_ = false;
    cartY_ = rowY(1);
    for (int i = 0; i < kRows; i++) on_[i] = 0;
}

void Game::begin() {
    toTitle();
    mode_ = Mode::Play;
    cartN_ = kBooks;
    for (int i = 0; i < kBooks; i++) cart_[i] = kOrder[i];
    cursor_ = 0;
    cartY_ = rowY(0);
    parked_ = false;
    playFrames_ = 0;
}

void Game::enterWin() {
    mode_ = Mode::Win;
    won_ = true;
    reason_ = "CHIME";
    anim_ = Anim::None;
    endHold_ = 48;
    strike_ = 0;
    hourStrike();
}

void Game::enterLose(const char* why) {
    mode_ = Mode::Lose;
    won_ = false;
    reason_ = why;
    anim_ = Anim::None;
    endHold_ = 36;
    reject();
}

void Game::popFront() {
    for (int i = 1; i < cartN_; i++) cart_[i - 1] = cart_[i];
    if (cartN_ > 0) cartN_--;
}

void Game::handXY(float& x, float& y) const {
    x = HAND_X;
    y = cartY_;
}

void Game::slotXY(int row, int slot, float& x, float& y) const {
    x = SLOT_X0 + float(slot) * SLOT_DX;
    y = rowY(row) - 2.f;
}

void Game::flightXY(float& x, float& y) const {
    float sx, sy;
    slotXY(flightRow_, flightSlot_, sx, sy);
    float s = std::clamp(flightT_, 0.f, 1.f);
    float ease = s * s * (3.f - 2.f * s);
    x = flightX0_ + (sx - flightX0_) * ease;
    y = flightY0_ + (sy - flightY0_) * ease - std::sin(s * kPi) * 10.f;
}

void Game::shelve() {
    if (mode_ != Mode::Play || anim_ != Anim::None || cartN_ <= 0) return;
    flightBook_ = cart_[0];
    flightRow_ = cursor_;
    flightSlot_ = on_[cursor_];
    handXY(flightX0_, flightY0_);
    flightT_ = 0;
    flightOnHour_ = onHour();
    anim_ = Anim::Out;
    push();
}

void Game::moveCursor(int dir) {
    int n = cursor_ + dir;
    if (n < 0 || n >= kRows) {
        blip(90.f);
        return;
    }
    cursor_ = n;
    blip(420.f + cursor_ * 50.f);
}

void Game::readHuman() {
    gs::Pad& p = sys_->pad;
    auto face = [&]() {
        return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    };
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || face()) begin();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (p.pressed(gs::BTN_START) || face()) begin();
        return;
    }
    if (p.pressed(gs::BTN_START)) {
        mode_ = mode_ == Mode::Pause ? Mode::Play : Mode::Pause;
        return;
    }
    if (mode_ != Mode::Play) return;
    int dir = 0;
    if (p.pressed(gs::BTN_UP)) {
        heldUp_ = 1;
        dir = -1;
    } else if (p.down(gs::BTN_UP)) {
        if (++heldUp_ > 12 && heldUp_ % 5 == 0) dir = -1;
    } else {
        heldUp_ = 0;
    }
    if (p.pressed(gs::BTN_DOWN)) {
        heldDn_ = 1;
        dir = 1;
    } else if (p.down(gs::BTN_DOWN)) {
        if (++heldDn_ > 12 && heldDn_ % 5 == 0) dir = 1;
    } else {
        heldDn_ = 0;
    }
    if (dir) moveCursor(dir);
    if (face()) shelve();
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        begin();
        return;
    }
    if (mode_ != Mode::Play || anim_ != Anim::None || cartN_ <= 0) return;
    int want = cart_[0];
    if (cursor_ != want) {
        moveCursor(want > cursor_ ? 1 : -1);
        return;
    }
    if (!parked_) return;
    if (!onHour()) return;
    shelve();
}

void Game::approach() {
    float goal = rowY(cursor_);
    float d = goal - cartY_;
    if (std::fabs(d) < 0.5f) {
        cartY_ = goal;
        parked_ = true;
        return;
    }
    parked_ = false;
    cartY_ += d * 0.45f;
}

void Game::stepAnim() {
    if (anim_ == Anim::None) return;
    if (anim_ == Anim::Out) {
        flightT_ += 1.f / 14.f;
        if (flightT_ < 1.f) return;
        flightT_ = 1.f;
        if (flightBook_ == flightRow_) {
            on_[flightRow_]++;
            popFront();
            shelved_++;
            anim_ = Anim::None;
            note(flightRow_);
            if (flightOnHour_ || onHour()) {
                enterWin();
            } else if (cartN_ == 0) {
                enterLose("EARLY");
            } else {
                msg_ = 50;
            }
        } else {
            anim_ = Anim::Hold;
            hold_ = 0;
            badRow_ = flightRow_;
            msg_ = 60;
            reject();
        }
        return;
    }
    if (anim_ == Anim::Hold) {
        if (++hold_ >= 14) anim_ = Anim::Back;
        return;
    }
    flightT_ -= 1.f / 12.f;
    if (flightT_ > 0.f) return;
    flightT_ = 0.f;
    anim_ = Anim::None;
    badRow_ = -1;
    returns_++;
    if (returns_ >= kMaxBack) enterLose("BACK");
}

void Game::clockWatch() {
    if (mode_ != Mode::Play || anim_ != Anim::None) return;
    if (pastHour()) enterLose("LATE");
}

int Game::clockSec() const {
    int extra = mode_ == Mode::Title ? 0 : playFrames_ / kTick;
    return kStartSec + extra;
}

void Game::face(int& h, int& m, int& s) const {
    int t = clockSec() % (12 * 3600);
    h = t / 3600;
    if (h == 0) h = 12;
    m = (t % 3600) / 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    face(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    face(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    face(h, m, s);
    return s;
}

bool Game::onHour() const {
    int h, m, s;
    face(h, m, s);
    return clockSec() >= 12 * 3600 && h == 12 && m == 0 && s < kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= 12 * 3600 + kGraceSec; }

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.04f);
    tone_ = 3;
}

void Game::push() { sys_->apu.tone(1, 140.f, 0.04f); push_ = 4; }

void Game::note(int row) {
    static const float n[4] = {392.f, 440.f, 494.f, 523.25f};
    sys_->apu.keyOn(0, n[row & 3], 0.16f);
}

void Game::hourStrike() {
    fan_ = 0;
    sys_->apu.keyOn(1, 523.25f, 0.28f);
    sys_->apu.keyOn(2, 784.f, 0.18f);
    sys_->apu.keyOn(3, 1046.5f, 0.12f);
}

void Game::reject() {
    sys_->apu.keyOn(3, 90.f, 0.2f);
    sys_->apu.noiseBurst(0.12f, 900.f, 0.16f);
}

void Game::ticks() {
    if (tone_ > 0 && --tone_ == 0) sys_->apu.tone(0, 0, 0);
    if (push_ > 0 && --push_ == 0) sys_->apu.tone(1, 0, 0);
    if (fan_ >= 0) {
        static const float n[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
        if (fan_ < 32 && fan_ % 8 == 0) {
            int i = fan_ / 8;
            sys_->apu.keyOn(1, n[i], 0.2f);
            sys_->apu.keyOn(2, n[i] * 2.f, 0.08f);
            strike_ = 8;
        }
        if (++fan_ > 48) fan_ = -1;
    }
    if (strike_ > 0) strike_--;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Play) {
        playFrames_++;
        approach();
    }
    if (bot_) botAct();
    else readHuman();
    if (mode_ == Mode::Play) {
        stepAnim();
        clockWatch();
        if (msg_ > 0) msg_--;
    }
    if ((mode_ == Mode::Win || mode_ == Mode::Lose) && endHold_ > 0) {
        if (--endHold_ == 0) over_ = true;
    }
    ticks();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * (float(m.w) / float(m.h));
    gs::Sprite s;
    s.h = int16_t(std::lround(h));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal & 15);
    s.shadow = shadow;
    if (shadow) {
        s.x = int16_t(s.x + 3);
        s.y = int16_t(s.y + 3);
    }
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
    hud(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::wall() {
    bool lose = mode_ == Mode::Lose;
    bool win = mode_ == Mode::Win;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r, g, b;
        if (y < 20) {
            r = 3;
            g = 3;
            b = 5;
        } else if (y < 198) {
            int band = ((y - 20) / 42) & 1;
            r = band ? 7 : 6;
            g = band ? 5 : 4;
            b = 3;
            if (((y - 20) % 42) < 2) {
                r = 4;
                g = 3;
                b = 2;
            }
        } else {
            r = 3;
            g = 2;
            b = 1;
        }
        if (win && y < 36) {
            g = std::min(15, g + 2);
            r = std::min(15, r + 1);
        }
        if (lose) r = std::min(15, r + 2);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = 0;
    }
}

void Game::drawClock(float cx, float cy, float swing) {
    spr(art_.clock, cx, cy, 34.f, PAL_ROOM, true);
    spr(art_.clock, cx, cy, 34.f, PAL_ROOM, false);
    int h, m, s;
    face(h, m, s);
    float ha = ((float(h % 12) + float(m) / 60.f) / 12.f) * 2.f * kPi;
    float ma = (float(m) + float(s) / 60.f) / 60.f * 2.f * kPi;
    float sa = float(s) / 60.f * 2.f * kPi;
    auto hand = [&](float ang, float len, int pal) {
        int n = std::max(2, int(len / 3.5f));
        for (int i = 1; i <= n; i++) {
            float t = float(i) / float(n);
            spr(art_.dot, cx + std::sin(ang) * len * t, cy - std::cos(ang) * len * t, 3.5f, pal, false);
        }
    };
    hand(ha, 8.f, PAL_INK);
    hand(ma, 11.f, PAL_GOLD);
    hand(sa, 12.f, PAL_ALERT);
    float bx = cx + 22.f + std::sin(swing) * 4.f;
    spr(art_.bell, bx, cy - 2.f, 16.f, onHour() || mode_ == Mode::Win ? PAL_GOLD : PAL_DIM, false);
}

void Game::drawAisle(bool live) {
    float cartCy = live ? cartY_ : rowY(1);
    int showFrom = 1;
    int showN = live ? std::max(0, cartN_ - 1) : 3;

    if (live && cartN_ > 0 && anim_ == Anim::None) {
        spr(art_.book[cart_[0]], HAND_X, cartY_, BOOK_DH, bookPal(cart_[0]), true);
        spr(art_.book[cart_[0]], HAND_X, cartY_, BOOK_DH, bookPal(cart_[0]), false);
    }
    if (live && anim_ != Anim::None) {
        float x, y;
        flightXY(x, y);
        spr(art_.book[flightBook_], x, y, BOOK_DH, bookPal(flightBook_), true);
        spr(art_.book[flightBook_], x, y, BOOK_DH, bookPal(flightBook_), false);
    }
    if (!live) {
        float bob = std::sin(float(sys_->frame) * 0.08f) * 5.f;
        spr(art_.book[1], 104.f + bob, rowY(1) - 10.f, BOOK_DH, PAL_B, false);
    }

    int bracketRow = live ? cursor_ : 1;
    int bracketPal = (live && bracketRow == badRow_) ? PAL_ALERT : PAL_MARK;
    spr(art_.bracket, PLANK_CX, rowY(bracketRow), float(art_.bracket.h), bracketPal, false);

    spr(art_.cart, CART_X, cartCy + 4.f, float(art_.cart.h), PAL_WOOD, true);
    spr(art_.cart, CART_X, cartCy + 4.f, float(art_.cart.h), PAL_WOOD, false);

    int nInside = std::min(showN, 3);
    for (int i = 0; i < nInside; i++) {
        int book = live ? cart_[showFrom + i] : kOrder[i];
        float x = CART_X + 8.f - float(i) * 7.f;
        spr(art_.book[book], x, cartCy - 2.f, 22.f, bookPal(book), false);
    }

    spr(art_.caseBack, CASE_CX, CASE_CY, 150.f, PAL_WOOD, false);
    for (int r = 0; r < kRows; r++) {
        float ph = float(art_.plate[r].h);
        if (live && r == cursor_) ph += 2.f;
        spr(art_.plate[r], PLATE_X, rowY(r), ph, bookPal(r), false);
        spr(art_.rail, PLANK_CX, rowY(r) + 14.f, 4.f, bookPal(r), false);
        spr(art_.plank, PLANK_CX, rowY(r) + 16.f, 6.f, PAL_WOOD, false);
        spr(art_.residents, RES_X, rowY(r), 26.f, PAL_WOOD, false);
        int placed = live ? on_[r] : (r == 0 ? 1 : 0);
        for (int s = 0; s < placed; s++) {
            float x, y;
            slotXY(r, s, x, y);
            spr(art_.book[r], x, y, BOOK_DH, bookPal(r), false);
        }
    }

    float swing = 0.f;
    if (strike_ > 0) swing = std::sin(float(strike_) * 1.4f) * 0.6f;
    else if (onHour()) swing = std::sin(float(sys_->frame) * 0.5f) * 0.25f;
    drawClock(28.f, 28.f, swing);
}

void Game::drawTitle() {
    drawAisle(false);
    spr(art_.banner, 160.f, 14.f, 12.f, PAL_GOLD, false);
    hudC(0, "SHELVE", PAL_GOLD);
    hudC(25, "THE HOUR HAS TO CHIME", PAL_INK);
    bool blink = (sys_->frame / 30) % 2 == 0;
    hud(1, 27, "PRESS START", blink ? PAL_GOLD : PAL_DIM);
    hud(32, 27, "V1.0.0", PAL_DIM);
}

void Game::draw() {
    wall();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    drawAisle(true);
    char clk[16];
    std::snprintf(clk, sizeof clk, "%d:%02d:%02d", hour(), minute(), second());
    hud(1, 0, clk, onHour() || mode_ == Mode::Win ? PAL_GOLD : PAL_INK);
    char back[16];
    std::snprintf(back, sizeof back, "BACK %d", returns_);
    hud(32, 0, back, returns_ ? PAL_ALERT : PAL_DIM);

    if (mode_ == Mode::Win) {
        hudC(26, "THE HOUR CHIMES", PAL_OK);
        hudC(27, "START SHELVES ANOTHER", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Lose) {
        const char* line = "THE HOUR PASSED";
        if (std::strcmp(reason_, "BACK") == 0) line = "TOO MANY CAME BACK";
        if (std::strcmp(reason_, "EARLY") == 0) line = "THE CART EMPTIED EARLY";
        hudC(26, line, PAL_ALERT);
        hudC(27, "START TRIES THE CART AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(26, "PAUSED", PAL_GOLD);
        return;
    }
    if (cartN_ > 0) {
        char book[12];
        std::snprintf(book, sizeof book, "BOOK %c", kLetter[cart_[0]]);
        hud(16, 0, book, bookPal(cart_[0]));
    }
    if (badRow_ >= 0 && anim_ != Anim::None) {
        hudC(26, "THAT ROW SENT IT BACK", PAL_ALERT);
    } else if (msg_ > 0 && !onHour()) {
        hudC(26, "EARLY. THE HOUR HAS TO CHIME", PAL_GOLD);
    } else if (onHour()) {
        hudC(26, "TWELVE. SHELVE THE MATCH", PAL_OK);
    } else {
        hudC(26, "WAIT FOR THE HOUR", PAL_DIM);
    }
    hudC(27, "UP DOWN  A SHELVES", PAL_INK);
}

}  // namespace shelvechime
