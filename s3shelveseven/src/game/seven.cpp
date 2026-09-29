#include "game/seven.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace shelveseven {
namespace {
const char* kLetter = "ABCD";
constexpr float kPi = 3.14159265f;
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
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
    sys_->apu.silence();
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

void Game::pushBook() {
    if (qn_ >= kQueue) return;
    queue_[qn_++] = int(rnd() % uint32_t(kRows));
}

void Game::begin() {
    mode_ = Mode::Play;
    anim_ = Anim::None;
    over_ = false;
    won_ = false;
    cursor_ = 0;
    you_ = 0;
    them_ = 0;
    returns_ = 0;
    themWait_ = 0;
    hold_ = 0;
    msg_ = 0;
    badRow_ = -1;
    heldUp_ = 0;
    heldDn_ = 0;
    qn_ = 0;
    cartY_ = rowY(0);
    for (int i = 0; i < kQueue; i++) pushBook();
    sys_->apu.silence();
    chime(0);
}

void Game::enterWin() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    anim_ = Anim::None;
    chime(3);
}

void Game::enterLose() {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    anim_ = Anim::None;
    sys_->apu.tone(2, 70.f, 0.2f);
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

void Game::shelve() {
    if (mode_ != Mode::Play || anim_ != Anim::None || qn_ <= 0) return;
    flightBook_ = queue_[0];
    flightRow_ = cursor_;
    flightT_ = 0;
    anim_ = Anim::Out;
    sys_->apu.tone(1, 150.f, 0.04f);
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
    if (mode_ != Mode::Play || anim_ != Anim::None || qn_ <= 0) return;
    int want = queue_[0];
    if (cursor_ != want) {
        moveCursor(want > cursor_ ? 1 : -1);
        return;
    }
    if (std::fabs(cartY_ - rowY(cursor_)) > 1.2f) return;
    shelve();
}

void Game::approach() {
    float goal = rowY(cursor_);
    float d = goal - cartY_;
    if (std::fabs(d) < 0.5f) {
        cartY_ = goal;
        return;
    }
    cartY_ += d * 0.5f;
}

void Game::stepAnim() {
    if (anim_ == Anim::None) return;
    if (anim_ == Anim::Out) {
        flightT_ += 1.f / 12.f;
        if (flightT_ < 1.f) return;
        flightT_ = 1.f;
        if (flightBook_ == flightRow_) {
            for (int i = 1; i < qn_; i++) queue_[i - 1] = queue_[i];
            qn_--;
            pushBook();
            you_++;
            anim_ = Anim::None;
            chime(flightRow_);
            if (you_ >= kGoal && them_ < kGoal) enterWin();
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
        if (++hold_ >= 10) anim_ = Anim::Back;
        return;
    }
    flightT_ -= 1.f / 12.f;
    if (flightT_ > 0.f) return;
    flightT_ = 0.f;
    anim_ = Anim::None;
    badRow_ = -1;
    returns_++;
}

void Game::rival() {
    if (mode_ != Mode::Play) return;
    if (++themWait_ < kThemEvery) return;
    themWait_ = 0;
    if (them_ >= kGoal) return;
    them_++;
    sys_->apu.tone(3, 220.f, 0.05f);
    tone_ = 5;
    if (them_ >= kGoal && you_ < kGoal) enterLose();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.04f);
    tone_ = 4;
}

void Game::chime(int row) {
    static const float n[4] = {392.f, 440.f, 494.f, 523.25f};
    sys_->apu.tone(0, n[row & 3], 0.08f);
    tone_ = 8;
}

void Game::reject() {
    sys_->apu.tone(2, 90.f, 0.12f);
    tone_ = 8;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Play) approach();
    if (bot_) botAct();
    else readHuman();
    if (mode_ == Mode::Play) {
        stepAnim();
        if (!over_) rival();
        if (msg_ > 0) msg_--;
    }
    if (tone_ > 0 && --tone_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        sys.apu.tone(2, 0, 0);
        sys.apu.tone(3, 0, 0);
    }
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
        s.x = int16_t(s.x + 2);
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
    bool win = mode_ == Mode::Win;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = 6, g = 5, b = 3;
        if (y < 20) {
            r = 4;
            g = 3;
            b = 2;
        } else if (y > 198) {
            r = 3;
            g = 2;
            b = 1;
        } else if (((y - 20) / 40) & 1) {
            r = 8;
            g = 6;
            b = 3;
        }
        if (win && y > 20 && y < 198) g = std::min(15, g + 1);
        if (lose) r = std::min(15, r + 2);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.road[y].on = false;
    }
}

void Game::aisle(bool live) {
    float cy = live ? cartY_ : rowY(1);
    int front = live && qn_ > 0 ? queue_[0] : 1;
    bool flying = live && anim_ != Anim::None;

    spr(art_.seven, 300.f, 112.f, 36.f, mode_ == Mode::Win ? PAL_OK : PAL_GOLD, false);

    for (int r = 0; r < kRows; r++) {
        panel(art_.plank, 210.f, rowY(r) + 14.f, 150.f, 6.f, PAL_WOOD);
        float ph = 20.f;
        int pp = bookPal(r);
        if (live && r == cursor_) ph = 24.f;
        if (live && r == badRow_) pp = PAL_ALERT;
        spr(art_.plate, PLATE_X, rowY(r), ph, pp, false);
        int nYou = 0;
        if (live) {
            int base = you_ / kRows;
            int extra = you_ % kRows;
            nYou = base + (r < extra ? 1 : 0);
        } else if (r < 2) {
            nYou = 1;
        }
        for (int s = 0; s < nYou && s < 6; s++) {
            float x = SLOT_X0 + float(s) * SLOT_DX;
            spr(art_.book[r], x, rowY(r), BOOK_DH - 4.f, bookPal(r), false);
        }
        int nThem = 0;
        if (live) nThem = (them_ > r) ? 1 : 0;
        if (nThem) spr(art_.book[(r + 1) & 3], 286.f, rowY(r), 18.f, PAL_DIM, false);
    }

    if (!flying && (live ? qn_ > 0 : true)) {
        float hx = HAND_X;
        float hy = live ? cy : rowY(1);
        if (!live) hx += std::sin(float(sys_->frame) * 0.09f) * 5.f;
        spr(art_.book[front], hx, hy, BOOK_DH, bookPal(front), true);
        spr(art_.book[front], hx, hy, BOOK_DH, bookPal(front), false);
    }
    if (flying) {
        float sx = SLOT_X0;
        float sy = rowY(flightRow_);
        float hx = HAND_X;
        float hy = cartY_;
        float t = std::clamp(flightT_, 0.f, 1.f);
        float s = t * t * (3.f - 2.f * t);
        float x, y;
        if (anim_ == Anim::Hold) {
            x = sx;
            y = sy + std::sin(hold_ * 1.4f) * 2.f;
        } else if (anim_ == Anim::Back) {
            x = hx + (sx - hx) * s;
            y = hy + (sy - hy) * s - std::sin(s * kPi) * 14.f;
        } else {
            x = hx + (sx - hx) * s;
            y = hy + (sy - hy) * s - std::sin(s * kPi) * 8.f;
        }
        spr(art_.book[flightBook_], x, y, BOOK_DH, bookPal(flightBook_), true);
        spr(art_.book[flightBook_], x, y, BOOK_DH, bookPal(flightBook_), false);
    }

    spr(art_.cart, CART_X, cy + 4.f, 30.f, PAL_WOOD, true);
    spr(art_.cart, CART_X, cy + 4.f, 30.f, PAL_WOOD, false);
    int inside = live ? std::max(0, qn_ - 1) : 3;
    for (int i = 0; i < inside && i < 3; i++) {
        int book = live ? queue_[i + 1] : (i + 2) & 3;
        spr(art_.book[book], CART_X + 6.f - float(i) * 6.f, cy - 2.f, 18.f, bookPal(book), false);
    }
}

void Game::draw() {
    wall();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;

    if (mode_ == Mode::Title) {
        aisle(false);
        hudC(0, "S3 SHELVE SEVEN", PAL_GOLD);
        hudC(25, "A SHORT SHELVE", PAL_INK);
        hudC(26, "FIRST TO SEVEN", PAL_GOLD);
        bool blink = (sys_->frame / 30) & 1;
        hud(1, 27, "PRESS START", blink ? PAL_GOLD : PAL_DIM);
        char ver[16];
        std::snprintf(ver, sizeof ver, "V%s", S3_VERSION);
        hud(33, 27, ver, PAL_DIM);
        return;
    }

    aisle(true);
    char score[40];
    std::snprintf(score, sizeof score, "YOU %d", you_);
    hud(1, 0, score, you_ >= them_ ? PAL_OK : PAL_GOLD);
    std::snprintf(score, sizeof score, "THEM %d", them_);
    int n = int(std::strlen(score));
    hud(39 - n, 0, score, them_ + 1 >= you_ ? PAL_ALERT : PAL_INK);

    if (mode_ == Mode::Win) {
        hudC(26, "FIRST TO SEVEN", PAL_OK);
        hudC(27, "START SHELVES AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(26, "THEY GOT THERE FIRST", PAL_ALERT);
        hudC(27, "START TRIES THE SHELF AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(26, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUMES", PAL_INK);
        return;
    }
    if (qn_ > 0) {
        char book[16];
        std::snprintf(book, sizeof book, "BOOK %c", kLetter[queue_[0]]);
        hud(16, 0, book, bookPal(queue_[0]));
    }
    if (msg_ > 0) hudC(26, "THAT ROW SENT IT BACK", PAL_ALERT);
    else hudC(26, "MATCH THE LETTER", PAL_DIM);
    hudC(27, "ARROWS MOVE   A SHELVES", PAL_INK);
}

}  // namespace shelveseven
