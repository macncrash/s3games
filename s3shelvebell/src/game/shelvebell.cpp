#include "game/shelvebell.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace shelvebell {
namespace {

const char* kLetter = "ABCD";
constexpr float kPi = 3.14159265f;
constexpr float kDt = 1.f / 60.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setEcho(0.2f, 0.3f, 0.16f);
    sys.apu.setMaster(0.75f);
    rng_ = 0xB311u;
    rules_ = (kMaxDead == 3 && kBooks >= 1 && kRows == 4);
    why_ = "";
    if (bot_) begin();
    else toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    anim_ = Anim::None;
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    msg_ = 0;
    fan_ = -1;
    bellAmp_ = 0.12f;
    why_ = "";
    sys_->apu.silence();
}

void Game::begin() {
    mode_ = Mode::Play;
    anim_ = Anim::None;
    over_ = false;
    won_ = false;
    rung_ = false;
    cursor_ = 0;
    shelved_ = 0;
    dead_ = 0;
    tryNo_ = 0;
    hold_ = 0;
    msg_ = 0;
    badRow_ = -1;
    heldUp_ = 0;
    heldDn_ = 0;
    fan_ = -1;
    parked_ = true;
    bellAmp_ = 0.12f;
    bellPh_ = 0;
    ringT_ = 0;
    leaveT_ = 0;
    why_ = "";
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
    note(0);
}

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Play: return "play";
    case Mode::Pause: return "pause";
    case Mode::Ring: return "ring";
    case Mode::Leave: return "leave";
    case Mode::Over: return won_ ? "leave" : "dead";
    }
    return "?";
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
        x = sx + std::sin(hold_ * 1.7f) * 2.4f;
        y = sy;
        return;
    }
    if (anim_ == Anim::Back) {
        x = hx + (sx - hx) * s;
        y = hy + (sy - hy) * s - std::sin(s * kPi) * 18.f;
        return;
    }
    x = flightX0_ + (sx - flightX0_) * s;
    y = flightY0_ + (sy - flightY0_) * s - std::sin(s * kPi) * 10.f;
}

void Game::shelve() {
    if (mode_ != Mode::Play || anim_ != Anim::None || cartN_ <= 0 || rung_) return;
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
        blip(90.f);
        return;
    }
    cursor_ = n;
    blip(420.f + cursor_ * 48.f);
}

void Game::readHuman() {
    gs::Pad& p = sys_->pad;
    auto face = [&]() {
        return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    };
    if (mode_ == Mode::Title || mode_ == Mode::Over) {
        if (p.pressed(gs::BTN_START) || face()) begin();
        return;
    }
    if (mode_ == Mode::Ring || mode_ == Mode::Leave) return;
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
    if (mode_ != Mode::Play || anim_ != Anim::None || cartN_ <= 0 || rung_) return;
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
    cartY_ += d * 0.45f;
}

void Game::ring() {
    if (rung_ || dead_ >= kMaxDead || cartN_ != 0) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    why_ = "BELL";
    bellAmp_ = 1.f;
    ringT_ = 0;
    mode_ = Mode::Ring;
    fan_ = 0;
    if (!sys_) return;
    sys_->apu.keyOn(0, 523.25f, 0.28f);
    sys_->apu.keyOn(1, 659.25f, 0.2f);
    sys_->apu.keyOn(2, 783.99f, 0.16f);
    sys_->apu.keyOn(3, 1046.5f, 0.12f);
    sys_->rumble(0.35f, 0.7f, 160);
    sys_->setLight(255, 196, 64);
}

void Game::dieTry() {
    if (rung_) return;
    dead_++;
    why_ = "BACK";
    if (dead_ >= kMaxDead) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        why_ = "THIRD";
        bellAmp_ = 0.02f;
        if (sys_) {
            sys_->apu.keyOn(3, 70.f, 0.26f);
            sys_->setLight(150, 28, 28);
        }
        return;
    }
    if (sys_) sys_->setLight(120, 48, 28);
}

void Game::stepAnim() {
    if (anim_ == Anim::None || mode_ != Mode::Play) return;
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
            if (cartN_ == 0) ring();
        } else {
            anim_ = Anim::Hold;
            hold_ = 0;
            badRow_ = flightRow_;
            msg_ = 48;
            reject();
        }
        return;
    }
    if (anim_ == Anim::Hold) {
        if (++hold_ >= 12) anim_ = Anim::Back;
        return;
    }
    flightT_ -= 1.f / 14.f;
    if (flightT_ > 0.f) return;
    flightT_ = 0.f;
    anim_ = Anim::None;
    badRow_ = -1;
    dieTry();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.04f);
    tone_ = 3;
}

void Game::push() {
    sys_->apu.tone(1, 150.f, 0.04f);
    push_ = 4;
}

void Game::note(int row) {
    static const float n[4] = {392.f, 440.f, 494.f, 523.25f};
    sys_->apu.tone(2, n[row & 3], 0.06f);
    tone_ = 6;
}

void Game::reject() {
    sys_->apu.tone(3, 96.f, 0.07f);
    sys_->apu.noiseBurst(0.12f, 900.f, 0.16f);
    tone_ = 8;
}

void Game::ticks() {
    if (tone_ > 0 && --tone_ == 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(2, 0, 0);
        sys_->apu.tone(3, 0, 0);
    }
    if (push_ > 0 && --push_ == 0) sys_->apu.tone(1, 0, 0);
    bellPh_ += (rung_ ? 0.55f : 0.08f);
    if (bellAmp_ > 0.08f) bellAmp_ *= rung_ ? 0.985f : 0.99f;
    if (fan_ < 0) return;
    static const float n[5] = {523.25f, 659.25f, 783.99f, 1046.5f, 1318.5f};
    if (fan_ < 40 && fan_ % 8 == 0) {
        int i = fan_ / 8;
        sys_->apu.keyOn(1, n[i], 0.14f);
    }
    if (++fan_ > 48) fan_ = -1;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    Mode before = mode_;
    if (mode_ == Mode::Play) approach();
    if (bot_) botAct();
    else readHuman();
    if (mode_ == Mode::Play) {
        stepAnim();
        if (msg_ > 0) msg_--;
    }
    if (mode_ == Mode::Ring && before == Mode::Ring) {
        ringT_ += kDt;
        if (ringT_ > 0.7f) {
            mode_ = Mode::Leave;
            leaveT_ = 0;
        }
    } else if (mode_ == Mode::Leave && before == Mode::Leave) {
        leaveT_ += kDt;
        if (leaveT_ > 0.9f) {
            over_ = true;
            why_ = "BELL";
        }
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
    bool dead = mode_ == Mode::Over && !won_;
    bool gold = rung_;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r, g, b;
        if (y < 36) {
            r = 5;
            g = 4;
            b = 2;
        } else if (y < 196) {
            int band = ((y - 36) / 40) & 1;
            r = band ? 8 : 6;
            g = band ? 6 : 5;
            b = 3;
        } else {
            r = 3;
            g = 2;
            b = 1;
        }
        if (gold && y < 40) {
            r = std::min(15, r + 3);
            g = std::min(15, g + 2);
        }
        if (dead) r = std::min(15, r + 2);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::drawAisle(bool live) {
    float swing = std::sin(bellPh_) * bellAmp_ * 10.f;
    float bellY = BELL_Y + (rung_ ? std::sin(ringT_ * 18.f) * 1.5f : 0.f);
    spr(art_.bell, BELL_X + swing, bellY, float(art_.bell.h), PAL_BELL, false);
    spr(art_.clapper, BELL_X + swing * 1.35f, bellY + 10.f, float(art_.clapper.h), PAL_BELL, false);

    float cartCy = live ? cartY_ : rowY(1);
    if (live && cartN_ > 0 && anim_ == Anim::None) {
        spr(art_.book[cart_[0]], HAND_X, cartY_, BOOK_DH, bookPal(cart_[0]), false);
        spr(art_.book[cart_[0]], HAND_X, cartY_, BOOK_DH, bookPal(cart_[0]), true);
    }
    if (live && anim_ != Anim::None) {
        float x, y;
        flightXY(x, y);
        spr(art_.book[flightBook_], x, y, BOOK_DH, bookPal(flightBook_), false);
        spr(art_.book[flightBook_], x, y, BOOK_DH, bookPal(flightBook_), true);
    }
    if (!live) {
        float bob = std::sin(float(sys_->frame) * 0.07f) * 5.f;
        spr(art_.book[2], 108.f + bob, rowY(1) - 8.f, BOOK_DH, PAL_C, false);
    }

    int bracketRow = live ? cursor_ : 1;
    int bracketPal = PAL_MARK;
    if (live && bracketRow == badRow_) bracketPal = PAL_ALERT;
    panel(art_.bracket, PLANK_CX, rowY(bracketRow), float(art_.bracket.w), float(art_.bracket.h), bracketPal);

    spr(art_.cart, CART_X, cartCy + 4.f, float(art_.cart.h), PAL_WOOD, false);
    spr(art_.cart, CART_X, cartCy + 4.f, float(art_.cart.h), PAL_WOOD, true);

    int showFrom = 1;
    int showN = live ? std::max(0, cartN_ - (anim_ == Anim::None ? 1 : 0)) : 3;
    if (!live) showFrom = 0;
    int nInside = std::min(showN, 3);
    for (int i = 0; i < nInside; i++) {
        int book = live ? cart_[std::min(cartN_ - 1, showFrom + i)] : (i % 4);
        float x = CART_X + 8.f - float(i) * 7.f;
        spr(art_.book[book], x, cartCy, 22.f, bookPal(book), false);
    }

    for (int r = 0; r < kRows; r++) {
        float ph = float(art_.plate[r].h);
        if (live && r == cursor_) ph += 2.f;
        spr(art_.plate[r], PLATE_X, rowY(r) - 2.f, ph, bookPal(r), false);
        panel(art_.rail, PLANK_CX, rowY(r) + 14.f, float(art_.rail.w), float(art_.rail.h), bookPal(r));
        panel(art_.plank, PLANK_CX, rowY(r) + 16.f, float(art_.plank.w), float(art_.plank.h), PAL_WOOD);
        if (live) {
            for (int s = 0; s < on_[r]; s++) {
                float x, y;
                slotXY(r, s, x, y);
                spr(art_.book[r], x, y, BOOK_DH, bookPal(r), false);
            }
        }
    }
    panel(art_.caseBack, CASE_CX, CASE_CY, float(art_.caseBack.w), float(art_.caseBack.h), PAL_WOOD);
}

void Game::drawTitle() {
    drawAisle(false);
    hudC(0, "S3 SHELVEBELL", PAL_GOLD);
    hudC(25, "SHELVE UNTIL THE BELL RINGS", PAL_INK);
    hudC(26, "BEFORE THE THIRD TRY DIES", PAL_INK);
    bool blink = (sys_->frame / 30) % 2 == 0;
    hud(1, 27, "PRESS START", blink ? PAL_GOLD : PAL_DIM);
    char ver[16];
    std::snprintf(ver, sizeof ver, "V%s", S3_VERSION);
    hud(33, 27, ver, PAL_DIM);
}

void Game::draw() {
    wall();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sys_->vdp.A.clear();
    sys_->vdp.B.clear();
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    drawAisle(true);

    if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(0, "BELL", PAL_GOLD);
        hudC(26, "THE BELL RANG", PAL_OK);
        hudC(27, mode_ == Mode::Leave || (mode_ == Mode::Over && won_) ? "LEAVE" : "CLEAR CART", PAL_INK);
        return;
    }
    if (mode_ == Mode::Over && !won_) {
        hudC(0, "SILENT", PAL_ALERT);
        hudC(26, "THE THIRD TRY DIED", PAL_ALERT);
        hudC(27, "START SHELVES AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_GOLD);
        hudC(26, "START RESUMES", PAL_INK);
        return;
    }

    char book[16] = "CART EMPTY";
    if (cartN_ > 0) std::snprintf(book, sizeof book, "BOOK %c", kLetter[cart_[0]]);
    hud(1, 0, book, PAL_GOLD);
    char back[20];
    std::snprintf(back, sizeof back, "TRY %d/3", dead_ + (rung_ ? 0 : 1));
    int bn = int(std::strlen(back));
    hud(39 - bn, 0, back, dead_ ? PAL_ALERT : PAL_INK);
    if (msg_ > 0) hudC(26, "THAT ROW KILLED THE TRY", PAL_ALERT);
    else if (shelved_ == 0 && dead_ == 0) hudC(26, "MATCH THE LETTER THEN SHELVE", PAL_DIM);
    else hudC(26, "ARROWS MOVE THE CART", PAL_DIM);
    hudC(27, "A SHELVES", PAL_INK);
}

}  // namespace shelvebell
