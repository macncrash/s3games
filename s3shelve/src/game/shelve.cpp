#include "game/shelve.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace shelve {
namespace {

gs::FMPatch chimePatch(float vol) {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.04f;
    p.op[0] = {1.f, 1.f, 0.008f, 0.22f, 0.28f, 0.35f};
    p.op[1] = {2.f, 0.22f, 0.008f, 0.18f, 0.16f, 0.3f};
    p.op[2] = {3.01f, 0.1f, 0.006f, 0.16f, 0.08f, 0.28f};
    p.op[3] = {4.02f, 0.06f, 0.01f, 0.2f, 0.1f, 0.26f};
    p.vol = vol;
    p.tone = 2600.f;
    p.echo = 0.22f;
    return p;
}

gs::FMPatch thudPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.2f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.12f, 0.f, 0.08f};
    p.op[1] = {0.5f, 0.4f, 0.006f, 0.14f, 0.f, 0.1f};
    p.op[2] = {2.f, 0.15f, 0.004f, 0.1f, 0.f, 0.08f};
    p.op[3] = {1.f, 0.08f, 0.01f, 0.16f, 0.f, 0.1f};
    p.vol = 0.22f;
    p.tone = 700.f;
    p.echo = 0.08f;
    return p;
}

const char* kLetter = "ABCD";
constexpr float kPi = 3.14159265f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setPatch(0, chimePatch(0.2f));
    sys.apu.setPatch(1, chimePatch(0.16f));
    sys.apu.setPatch(2, chimePatch(0.1f));
    sys.apu.setPatch(3, thudPatch());
    sys.apu.setEcho(0.18f, 0.28f, 0.18f);
    sys.apu.setMaster(0.75f);
    rng_ = 0x5E17u;
    if (bot_) begin();
    else toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    anim_ = Anim::None;
    over_ = false;
    won_ = false;
    msg_ = 0;
    fan_ = -1;
    sys_->apu.silence();
}

void Game::begin() {
    mode_ = Mode::Play;
    anim_ = Anim::None;
    over_ = false;
    won_ = false;
    cursor_ = 0;
    shelved_ = 0;
    returns_ = 0;
    hold_ = 0;
    msg_ = 0;
    badRow_ = -1;
    heldUp_ = 0;
    heldDn_ = 0;
    fan_ = -1;
    parked_ = true;
    cartY_ = rowY(0);
    for (int i = 0; i < kRows; i++) on_[i] = 0;
    int bag[kBooks];
    for (int i = 0; i < kBooks; i++) bag[i] = i % kRows;
    for (int i = kBooks - 1; i > 0; i--) {
        int j = int(rnd() % uint32_t(i + 1));
        std::swap(bag[i], bag[j]);
    }
    cartN_ = kBooks;
    for (int i = 0; i < kBooks; i++) cart_[i] = bag[i];
    sys_->apu.silence();
    chime(0);
}

void Game::enterWin() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    anim_ = Anim::None;
    fan_ = 0;
}

void Game::enterLose() {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    anim_ = Anim::None;
    sys_->apu.keyOn(3, 70.f, 0.24f);
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
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
    y = rowY(row);
}

void Game::flightXY(float& x, float& y) const {
    float hx, hy, sx, sy;
    handXY(hx, hy);
    slotXY(flightRow_, flightSlot_, sx, sy);
    float t = std::clamp(flightT_, 0.f, 1.f);
    float s = t * t * (3.f - 2.f * t);
    if (anim_ == Anim::Hold) {
        x = sx + std::sin(hold_ * 1.6f) * 2.2f;
        y = sy;
        return;
    }
    if (anim_ == Anim::Back) {
        x = hx + (sx - hx) * s;
        y = hy + (sy - hy) * s - std::sin(s * kPi) * 16.f;
        return;
    }
    x = flightX0_ + (sx - flightX0_) * s;
    y = flightY0_ + (sy - flightY0_) * s - std::sin(s * kPi) * 8.f;
}

void Game::shelve() {
    if (mode_ != Mode::Play || anim_ != Anim::None || cartN_ <= 0) return;
    flightBook_ = cart_[0];
    flightRow_ = cursor_;
    flightSlot_ = on_[cursor_];
    handXY(flightX0_, flightY0_);
    flightT_ = 0;
    anim_ = Anim::Out;
    push();
}

void Game::moveCursor(int dir) {
    int n = cursor_ + dir;
    if (n < 0 || n >= kRows) {
        blip(80.f);
        return;
    }
    cursor_ = n;
    blip(480.f + cursor_ * 40.f);
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
}

void Game::botAct() {
    if (mode_ != Mode::Play || anim_ != Anim::None || cartN_ <= 0) return;
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
            blip(180.f);
        }
        return;
    }
    parked_ = false;
    cartY_ += d * 0.42f;
}

void Game::stepAnim() {
    if (anim_ == Anim::None) return;
    if (anim_ == Anim::Out) {
        flightT_ += 1.f / 16.f;
        if (flightT_ < 1.f) return;
        flightT_ = 1.f;
        if (flightBook_ == flightRow_) {
            on_[flightRow_]++;
            popFront();
            shelved_++;
            anim_ = Anim::None;
            chime(flightRow_);
            if (cartN_ == 0) enterWin();
        } else {
            anim_ = Anim::Hold;
            hold_ = 0;
            badRow_ = flightRow_;
            msg_ = 70;
            reject();
        }
        return;
    }
    if (anim_ == Anim::Hold) {
        if (++hold_ >= 16) anim_ = Anim::Back;
        return;
    }
    flightT_ -= 1.f / 16.f;
    if (flightT_ > 0.f) return;
    flightT_ = 0.f;
    anim_ = Anim::None;
    badRow_ = -1;
    returns_++;
    if (returns_ >= kMaxBack) enterLose();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.045f);
    tone_ = 3;
}

void Game::push() {
    sys_->apu.tone(1, 160.f, 0.04f);
    push_ = 4;
}

void Game::chime(int row) {
    static const float n[4] = {392.f, 440.f, 494.f, 523.25f};
    int i = row & 3;
    sys_->apu.keyOn(0, n[i], 0.2f);
    sys_->apu.keyOn(2, n[i] * 2.f, 0.07f);
}

void Game::reject() {
    sys_->apu.keyOn(3, 92.f, 0.22f);
    sys_->apu.noiseBurst(0.14f, 1100.f, 0.18f);
}

void Game::ticks() {
    if (tone_ > 0 && --tone_ == 0) sys_->apu.tone(0, 0, 0);
    if (push_ > 0 && --push_ == 0) sys_->apu.tone(1, 0, 0);
    if (fan_ < 0) return;
    static const float n[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
    if (fan_ < 32 && fan_ % 8 == 0) {
        int i = fan_ / 8;
        sys_->apu.keyOn(1, n[i], 0.16f);
        if (i == 3) sys_->apu.keyOn(2, 1318.5f, 0.1f);
    }
    if (++fan_ > 40) fan_ = -1;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Play) approach();
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
    if (mode_ == Mode::Win) return 4;
    if (mode_ == Mode::Lose) return 5;
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
        int r, g, b;
        if (y < 22) {
            r = 4;
            g = 3;
            b = 2;
        } else if (y < 200) {
            int band = ((y - 22) / 44) & 1;
            r = band ? 8 : 7;
            g = band ? 6 : 5;
            b = 3;
            if (((y - 22) % 44) < 2) {
                r = 4;
                g = 3;
                b = 2;
            }
        } else if (y < 206) {
            r = 4;
            g = 3;
            b = 2;
        } else {
            r = 3;
            g = 2;
            b = 1;
        }
        if (y > 6 && y < 28) {
            r = std::min(15, r + 1);
            g = std::min(15, g + 1);
        }
        if (lose) r = std::min(15, r + 2);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
    }
}

void Game::drawAisle(bool live) {
    // Title freezes the cart on B, with book C coming back off that row.
    float cartCy = live ? cartY_ : rowY(1);
    int showFrom = 1;
    int showN = 0;
    if (live) showN = std::max(0, cartN_ - 1);
    else showN = 3;

    // Front-most first. The cart lip is in front of the books standing in it.
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
        float bob = std::sin(float(sys_->frame) * 0.08f) * 6.f;
        spr(art_.book[2], 102.f + bob, rowY(1) - 14.f, BOOK_DH, PAL_C, true);
        spr(art_.book[2], 102.f + bob, rowY(1) - 14.f, BOOK_DH, PAL_C, false);
    }

    int bracketRow = live ? cursor_ : 1;
    int bracketPal = PAL_MARK;
    if (live && bracketRow == badRow_) bracketPal = PAL_ALERT;
    if (!live) bracketPal = PAL_ALERT;
    panel(art_.bracket, PLANK_CX, rowY(bracketRow), float(art_.bracket.w), float(art_.bracket.h), bracketPal);

    spr(art_.cart, CART_X, cartCy + 2.f, float(art_.cart.h), PAL_WOOD, true);
    spr(art_.cart, CART_X, cartCy + 2.f, float(art_.cart.h), PAL_WOOD, false);

    int nInside = std::min(showN, 4);
    for (int i = 0; i < nInside; i++) {
        int book = live ? cart_[showFrom + i] : (i % 4);
        float x = CART_X + 10.f - float(i) * 8.f;
        spr(art_.book[book], x, cartCy - 1.f, 24.f, bookPal(book), false);
    }

    for (int r = 0; r < kRows; r++) {
        float ph = float(art_.plate[r].h);
        if (live && r == cursor_) ph += 2.f;
        spr(art_.plate[r], PLATE_X, rowY(r), ph, bookPal(r), false);
        panel(art_.rail, PLANK_CX, rowY(r) + 16.f, float(art_.rail.w), float(art_.rail.h), bookPal(r));
        panel(art_.plank, PLANK_CX, rowY(r) + 18.f, float(art_.plank.w), float(art_.plank.h), PAL_WOOD);
        if (live) {
            for (int s = 0; s < on_[r]; s++) {
                float x, y;
                slotXY(r, s, x, y);
                spr(art_.book[r], x, y, BOOK_DH, bookPal(r), false);
            }
        }
        spr(art_.residents, RES_X, rowY(r) + 1.f, float(art_.residents.h), PAL_WOOD, false);
    }
    panel(art_.caseBack, CASE_CX, CASE_CY, float(art_.caseBack.w), float(art_.caseBack.h), PAL_WOOD);
    if (live && msg_ == 0 && mode_ == Mode::Play) spr(art_.lamp, 164.f, 14.f, float(art_.lamp.h), PAL_ROOM, false);
}

void Game::drawTitle() {
    spr(art_.logo, 160.f, 12.f, float(art_.logo.h), PAL_GOLD, false);
    drawAisle(false);
    hudC(26, "A BOOK IN THE WRONG ROW COMES BACK", PAL_INK);
    bool blink = (sys_->frame / 30) % 2 == 0;
    hud(1, 27, "PRESS START", blink ? PAL_GOLD : PAL_DIM);
    char ver[12];
    std::snprintf(ver, sizeof ver, "V%s", S3_VERSION);
    hud(33, 27, ver, PAL_DIM);
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
    if (msg_ > 0) spr(art_.come, 160.f, 12.f, float(art_.come.h), PAL_ALERT, false);
    if (mode_ == Mode::Win) spr(art_.clear, 160.f, 12.f, float(art_.clear.h), PAL_OK, false);
    if (mode_ == Mode::Lose) spr(art_.goes, 160.f, 12.f, float(art_.goes.h), PAL_ALERT, false);

    if (mode_ == Mode::Win) {
        hudC(26, "THE CART IS CLEAR", PAL_OK);
        hudC(27, "START SHELVES ANOTHER", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(26, "TOO MANY CAME BACK", PAL_ALERT);
        hudC(27, "START TRIES THE CART AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_GOLD);
        hudC(26, "START RESUMES", PAL_INK);
        return;
    }

    char book[16] = "BOOK";
    if (cartN_ > 0) std::snprintf(book, sizeof book, "BOOK %c", kLetter[cart_[0]]);
    hud(1, 0, book, PAL_GOLD);
    char back[16];
    std::snprintf(back, sizeof back, "BACK %d/%d", returns_, kMaxBack);
    int bn = int(std::strlen(back));
    hud(39 - bn, 0, back, returns_ ? PAL_ALERT : PAL_INK);
    if (msg_ > 0) {
        hudC(26, "THAT ROW SENT IT BACK", PAL_ALERT);
    } else if (shelved_ == 0 && returns_ == 0) {
        hudC(26, "MATCH THE LETTER, THEN SHELVE", PAL_DIM);
    } else {
        hudC(26, "ARROWS ALIGN THE CART", PAL_DIM);
    }
    hudC(27, "A SHELVES", PAL_INK);
}

}  // namespace shelve
