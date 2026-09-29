#include "game/mark.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace shelvemark {
namespace {

gs::FMPatch chimePatch(float vol) {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.04f;
    p.op[0] = {1.f, 1.f, 0.008f, 0.22f, 0.28f, 0.35f};
    p.op[1] = {2.f, 0.2f, 0.008f, 0.18f, 0.16f, 0.3f};
    p.op[2] = {3.f, 0.08f, 0.006f, 0.16f, 0.08f, 0.28f};
    p.op[3] = {4.f, 0.05f, 0.01f, 0.2f, 0.1f, 0.26f};
    p.vol = vol;
    p.tone = 2400.f;
    p.echo = 0.2f;
    return p;
}

gs::FMPatch thudPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.12f, 0.f, 0.08f};
    p.op[1] = {0.5f, 0.35f, 0.006f, 0.14f, 0.f, 0.1f};
    p.op[2] = {2.f, 0.12f, 0.004f, 0.1f, 0.f, 0.08f};
    p.op[3] = {1.f, 0.06f, 0.01f, 0.16f, 0.f, 0.1f};
    p.vol = 0.2f;
    p.tone = 640.f;
    p.echo = 0.06f;
    return p;
}

const char* kLetter = "ABCD";
constexpr float kPi = 3.14159265f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setPatch(0, chimePatch(0.2f));
    sys.apu.setPatch(1, chimePatch(0.15f));
    sys.apu.setPatch(2, chimePatch(0.1f));
    sys.apu.setPatch(3, thudPatch());
    sys.apu.setEcho(0.16f, 0.26f, 0.16f);
    sys.apu.setMaster(0.75f);
    rng_ = 0x51E1u;
    if (bot_) begin();
    else toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    anim_ = Anim::None;
    over_ = false;
    won_ = false;
    finished_ = false;
    held_ = false;
    fan_ = -1;
    sys_->apu.silence();
}

void Game::begin() {
    mode_ = Mode::Play;
    anim_ = Anim::None;
    over_ = false;
    won_ = false;
    finished_ = false;
    held_ = false;
    cursor_ = 0;
    shelved_ = 0;
    returns_ = 0;
    hold_ = 0;
    msg_ = 0;
    badRow_ = -1;
    heldUp_ = 0;
    heldDn_ = 0;
    fan_ = -1;
    leaveT_ = 0;
    parked_ = true;
    cartX_ = CART_X;
    cartY_ = rowY(0);
    for (int i = 0; i < kRows; i++) on_[i] = 0;
    markRow_ = int(rnd() % uint32_t(kRows));
    int bag[kBooks];
    for (int i = 0; i < kBooks; i++) bag[i] = i;
    for (int i = kBooks - 1; i > 0; i--) {
        int j = int(rnd() % uint32_t(i + 1));
        std::swap(bag[i], bag[j]);
    }
    cartN_ = kBooks;
    for (int i = 0; i < kBooks; i++) cart_[i] = bag[i];
    sys_->apu.silence();
    chime(markRow_);
}

void Game::enterWin() {
    mode_ = Mode::Win;
    won_ = true;
    finished_ = true;
    over_ = true;
    anim_ = Anim::None;
    fan_ = 0;
}

void Game::enterLose() {
    mode_ = Mode::Lose;
    won_ = false;
    finished_ = false;
    over_ = true;
    anim_ = Anim::None;
    sys_->apu.keyOn(3, 64.f, 0.24f);
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

void Game::popFront() {
    for (int i = 1; i < cartN_; i++) cart_[i - 1] = cart_[i];
    if (cartN_ > 0) cartN_--;
}

void Game::slotXY(int row, int slot, float& x, float& y) const {
    x = SLOT_X0 + float(slot) * SLOT_DX;
    y = rowY(row);
}

void Game::flightXY(float& x, float& y) const {
    float sx, sy;
    slotXY(flightRow_, flightSlot_, sx, sy);
    float t = std::clamp(flightT_, 0.f, 1.f);
    float s = t * t * (3.f - 2.f * t);
    if (anim_ == Anim::Hold) {
        x = sx;
        y = sy + std::sin(hold_ * 1.4f) * 2.f;
        return;
    }
    float hx = HAND_X;
    float hy = cartY_;
    if (anim_ == Anim::Back) {
        x = hx + (sx - hx) * s;
        y = hy + (sy - hy) * s - std::sin(s * kPi) * 14.f;
        return;
    }
    x = flightX0_ + (sx - flightX0_) * s;
    y = flightY0_ + (sy - flightY0_) * s - std::sin(s * kPi) * 8.f;
}

void Game::shelve() {
    if (mode_ != Mode::Play || anim_ != Anim::None || cartN_ <= 0 || held_) return;
    flightBook_ = cart_[0];
    flightRow_ = cursor_;
    flightSlot_ = on_[cursor_];
    flightX0_ = HAND_X;
    flightY0_ = cartY_;
    flightT_ = 0;
    anim_ = Anim::Out;
    sys_->apu.tone(1, 150.f, 0.04f);
    push_ = 4;
}

void Game::leave() {
    if (mode_ != Mode::Play || anim_ != Anim::None) return;
    if (!held_) {
        blip(90.f);
        msg_ = 40;
        return;
    }
    mode_ = Mode::Leave;
    leaveT_ = 0;
    finished_ = true;
    chime(markRow_);
}

void Game::moveCursor(int dir) {
    int n = cursor_ + dir;
    if (n < 0 || n >= kRows) {
        blip(80.f);
        return;
    }
    cursor_ = n;
    blip(420.f + cursor_ * 50.f);
}

void Game::readHuman() {
    gs::Pad& p = sys_->pad;
    auto face = [&]() { return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO); };
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || face()) begin();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (p.pressed(gs::BTN_START) || face()) begin();
        return;
    }
    if (p.pressed(gs::BTN_START)) {
        if (mode_ == Mode::Play) mode_ = Mode::Pause;
        else if (mode_ == Mode::Pause) mode_ = Mode::Play;
        return;
    }
    if (mode_ != Mode::Play) return;
    int dir = 0;
    if (p.pressed(gs::BTN_UP)) {
        heldUp_ = 1;
        dir = -1;
    } else if (p.down(gs::BTN_UP)) {
        if (++heldUp_ > 14 && heldUp_ % 6 == 0) dir = -1;
    } else {
        heldUp_ = 0;
    }
    if (p.pressed(gs::BTN_DOWN)) {
        heldDn_ = 1;
        dir = 1;
    } else if (p.down(gs::BTN_DOWN)) {
        if (++heldDn_ > 14 && heldDn_ % 6 == 0) dir = 1;
    } else {
        heldDn_ = 0;
    }
    if (dir) moveCursor(dir);
    if (face()) shelve();
    if (p.pressed(gs::BTN_B)) leave();
}

void Game::botAct() {
    if (mode_ == Mode::Leave || mode_ == Mode::Win || mode_ == Mode::Lose) return;
    if (mode_ != Mode::Play || anim_ != Anim::None) return;
    if (held_) {
        leave();
        return;
    }
    if (cartN_ <= 0) return;
    int want = cart_[0];
    if (cursor_ != want) {
        moveCursor(want > cursor_ ? 1 : -1);
        return;
    }
    if (std::fabs(cartY_ - rowY(cursor_)) > 0.8f) return;
    shelve();
}

void Game::approach() {
    float goal = rowY(cursor_);
    float d = goal - cartY_;
    if (std::fabs(d) < 0.45f) {
        cartY_ = goal;
        if (!parked_) {
            parked_ = true;
            blip(170.f);
        }
        return;
    }
    parked_ = false;
    cartY_ += d * 0.42f;
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
            chime(flightRow_);
            if (flightRow_ == markRow_ && on_[markRow_] >= 1) held_ = true;
        } else {
            anim_ = Anim::Hold;
            hold_ = 0;
            badRow_ = flightRow_;
            msg_ = 50;
            reject();
        }
        return;
    }
    if (anim_ == Anim::Hold) {
        if (++hold_ >= 14) anim_ = Anim::Back;
        return;
    }
    flightT_ -= 1.f / 14.f;
    if (flightT_ > 0.f) return;
    flightT_ = 0.f;
    anim_ = Anim::None;
    badRow_ = -1;
    returns_++;
    if (returns_ >= kMaxBack) enterLose();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.04f);
    tone_ = 3;
}

void Game::chime(int row) {
    static const float n[4] = {392.f, 440.f, 494.f, 523.25f};
    int i = row & 3;
    sys_->apu.keyOn(0, n[i], 0.18f);
    sys_->apu.keyOn(2, n[i] * 2.f, 0.06f);
}

void Game::reject() {
    sys_->apu.keyOn(3, 88.f, 0.2f);
    sys_->apu.noiseBurst(0.12f, 1000.f, 0.16f);
}

void Game::ticks() {
    if (tone_ > 0 && --tone_ == 0) sys_->apu.tone(0, 0, 0);
    if (push_ > 0 && --push_ == 0) sys_->apu.tone(1, 0, 0);
    if (fan_ < 0) return;
    static const float n[4] = {523.25f, 659.25f, 784.f, 1046.5f};
    if (fan_ < 32 && fan_ % 8 == 0) sys_->apu.keyOn(1, n[fan_ / 8], 0.15f);
    if (++fan_ > 36) fan_ = -1;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Play) approach();
    if (mode_ == Mode::Leave) {
        cartX_ -= 3.2f;
        if (++leaveT_ > 36) enterWin();
    }
    if (bot_) botAct();
    else readHuman();
    if (mode_ == Mode::Play) {
        stepAnim();
        if (msg_ > 0) msg_--;
    }
    ticks();
    draw();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 6;
    if (mode_ == Mode::Lose) return 7;
    if (mode_ == Mode::Leave) return 5;
    if (held_) return 4;
    if (anim_ == Anim::Back || anim_ == Anim::Hold) return 3;
    if (anim_ == Anim::Out) return 2;
    return 1;
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

void Game::panel(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (m.h < 1 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = m.pick(h);
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
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

void Game::wall() {
    bool lose = mode_ == Mode::Lose;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = 5, g = 4, b = 3;
        if (y < 20) {
            r = 3;
            g = 2;
            b = 2;
        } else if (y > 198) {
            r = 3;
            g = 2;
            b = 1;
        } else {
            int band = ((y - 20) / 42) & 1;
            r = band ? 7 : 6;
            g = band ? 5 : 4;
            b = 3;
        }
        if (lose) r = std::min(15, r + 2);
        if (held_ && y > 18 && y < 28) {
            r = 12;
            g = 9;
            b = 2;
        }
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
    }
}

void Game::aisle(bool live) {
    float cy = live ? cartY_ : rowY(markRow_);
    float cx = live ? cartX_ : CART_X;
    int showRow = live ? markRow_ : 1;

    spr(art_.post, 168.f, 112.f, float(art_.post.h), PAL_WOOD, false);
    spr(art_.post, 300.f, 112.f, float(art_.post.h), PAL_WOOD, false);
    for (int r = 0; r < kRows; r++) {
        spr(art_.plate[r], PLATE_X, rowY(r), float(art_.plate[r].h), bookPal(r), false);
        panel(art_.shelf, 234.f, rowY(r) + 16.f, float(art_.shelf.w), float(art_.shelf.h), PAL_WOOD);
        float mx, my;
        slotXY(r, 0, mx, my);
        bool thisMark = r == (live ? markRow_ : showRow);
        if (thisMark && !(live && on_[r] > 0))
            spr(art_.mark, mx, my + 2.f, 16.f, (live && badRow_ == r) ? PAL_ALERT : PAL_MARK, false);
        int filled = live ? on_[r] : 0;
        for (int s = 0; s < filled; s++) {
            float x, y;
            slotXY(r, s, x, y);
            spr(art_.book[r], x, y, BOOK_DH, bookPal(r), false);
        }
    }

    if (live && cartN_ > 0 && anim_ == Anim::None && mode_ != Mode::Leave) {
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
        spr(art_.book[1], 92.f + bob, rowY(1), BOOK_DH, PAL_B, false);
    }

    int inside = live ? std::max(0, cartN_ - (anim_ == Anim::None ? 1 : 0)) : 2;
    for (int i = 0; i < std::min(inside, 3); i++) {
        int book = live ? cart_[(anim_ == Anim::None ? 1 : 0) + i] : (i + 2) % 4;
        if (live && (anim_ == Anim::None ? 1 : 0) + i >= cartN_) break;
        spr(art_.book[book], cx + 8.f - float(i) * 7.f, cy - 2.f, 22.f, bookPal(book), false);
    }
    spr(art_.cart, cx, cy + 4.f, float(art_.cart.h), PAL_WOOD, true);
    spr(art_.cart, cx, cy + 4.f, float(art_.cart.h), PAL_WOOD, false);

    if (live && mode_ == Mode::Play && !held_) spr(art_.lamp, 156.f, 16.f, float(art_.lamp.h), PAL_ROOM, false);
}

void Game::draw() {
    wall();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    if (mode_ == Mode::Title) {
        spr(art_.logo, 160.f, 14.f, float(art_.logo.h), PAL_GOLD, false);
        aisle(false);
        hudC(25, "THE GOLD SLOT IS THE MARK", PAL_INK);
        hudC(26, "A WRONG ROW SENDS THE BOOK BACK", PAL_DIM);
        bool blink = (sys_->frame / 30) % 2 == 0;
        hud(1, 27, "PRESS START", blink ? PAL_GOLD : PAL_DIM);
        char ver[16];
        std::snprintf(ver, sizeof ver, "V%s", S3_VERSION);
        hud(32, 27, ver, PAL_DIM);
        return;
    }
    aisle(true);
    if (msg_ > 0 && !held_ && anim_ != Anim::None)
        spr(art_.backBan, 160.f, 14.f, float(art_.backBan.h), PAL_ALERT, false);
    if (held_ && mode_ == Mode::Play) spr(art_.heldBan, 160.f, 14.f, float(art_.heldBan.h), PAL_GOLD, false);
    if (mode_ == Mode::Win) spr(art_.doneBan, 160.f, 14.f, float(art_.doneBan.h), PAL_OK, false);
    if (mode_ == Mode::Lose) spr(art_.lostBan, 160.f, 14.f, float(art_.lostBan.h), PAL_ALERT, false);

    if (mode_ == Mode::Win) {
        hudC(26, "THE MARK IS FINISHED", PAL_OK);
        hudC(27, "START SHELVES AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(26, "TOO MANY CAME BACK", PAL_ALERT);
        hudC(27, "THE MARK IS STILL OPEN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_GOLD);
        hudC(26, "START RESUMES", PAL_INK);
        return;
    }
    if (mode_ == Mode::Leave) {
        hudC(26, "LEAVING THE AISLE", PAL_OK);
        return;
    }

    char book[20] = "CART EMPTY";
    if (cartN_ > 0) std::snprintf(book, sizeof book, "BOOK %c", kLetter[cart_[0]]);
    hud(1, 0, book, PAL_GOLD);
    char mk[16];
    std::snprintf(mk, sizeof mk, "MARK %c", kLetter[markRow_]);
    hud(16, 0, mk, held_ ? PAL_OK : PAL_GOLD);
    char back[16];
    std::snprintf(back, sizeof back, "BACK %d/%d", returns_, kMaxBack);
    int bn = int(std::strlen(back));
    hud(39 - bn, 0, back, returns_ ? PAL_ALERT : PAL_INK);
    if (held_)
        hudC(26, "B LEAVES AND FINISHES THE MARK", PAL_OK);
    else if (msg_ > 0 && anim_ == Anim::None)
        hudC(26, "HOLD THE MARK BEFORE YOU LEAVE", PAL_ALERT);
    else if (anim_ == Anim::Back || anim_ == Anim::Hold)
        hudC(26, "THAT ROW SENT IT BACK", PAL_ALERT);
    else
        hudC(26, "MATCH THE LETTER, THEN SHELVE", PAL_DIM);
    hudC(27, "A SHELVES   B LEAVES", PAL_INK);
}

}  // namespace shelvemark
