#include "game/tape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace juggletape {
namespace {

constexpr int kFlight = 36;
constexpr int kEvery = 18;
constexpr float kLeftX = 96.f;
constexpr float kRightX = 196.f;
constexpr float kHandY = 152.f;
constexpr float kPi = 3.14159265f;

struct Slip {
    const char* name;
    int ball;
    int score;
};

constexpr Slip kTape[kTapeN] = {{"RED", 0, 3}, {"GOLD", 1, 5}, {"BLUE", 2, 4}};

float handX(int h) { return h == 0 ? kLeftX : kRightX; }

const gs::Image& ballImg(const Art& a, int ball) {
    if (ball == 1) return a.gold;
    if (ball == 2) return a.blue;
    return a.red;
}

int ballPal(int ball) {
    if (ball == 1) return PAL_GOLD;
    if (ball == 2) return PAL_BLUE;
    return PAL_RED;
}

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += kTape[i].score;
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kTape[i].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kTape[i].score;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Drop) return 2;
    if (mode_ == Mode::Leave) return 4;
    if (ready_) return 3;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = won_ = left_ = ready_ = false;
    catches_ = traps_ = 0;
    titleWait_ = 0;
    beep_ = 0;
    reason_ = "OPEN";
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    rules_ = std::strcmp(tapeLabel(0), "RED") == 0 && std::strcmp(tapeLabel(1), "GOLD") == 0 &&
             std::strcmp(tapeLabel(2), "BLUE") == 0 && tapeScore(0) == 3 && tapeScore(1) == 5 && tapeScore(2) == 4 &&
             kTape[0].ball == 0 && kTape[1].ball == 1 && kTape[2].ball == 2;
    if (!rules_) reason_ = "TAPE";
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
}

void Game::begin() {
    if (!rules_) return;
    won_ = over_ = left_ = ready_ = false;
    catches_ = traps_ = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    reason_ = "OPEN";
    pattern();
    blip(392.f);
}

void Game::pattern() {
    mode_ = Mode::Play;
    tick_ = 0;
    wait_ = 0;
    fileLatch_ = 0;
    latch_[0] = latch_[1] = 0;
    hn_[0] = 2;
    hold_[0][0] = 0;
    hold_[0][1] = 2;
    hn_[1] = 1;
    hold_[1][0] = 1;
    hold_[1][1] = 0;
    hold_[1][2] = 0;
    for (auto& a : air_) a = {};
    ready_ = matched();
}

void Game::miss() {
    ready_ = false;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    reason_ = "DROP";
    mode_ = Mode::Drop;
    wait_ = 40;
    for (auto& a : air_) a.on = false;
    blip(90.f);
    if (sys_) sys_->setLight(160, 28, 28);
}

void Game::noteCatch(int ball, bool file) {
    catches_++;
    if (ready_ || !file) {
        blip(440.f);
        return;
    }
    int next = -1;
    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) {
            next = i;
            break;
        }
    }
    if (next < 0) return;
    if (ball != kTape[next].ball) {
        traps_++;
        reason_ = "TRAP";
        blip(180.f);
        return;
    }
    held_[next] = true;
    blip(next == kTapeN - 1 ? 880.f : 620.f);
    if (matched() && traps_ == 0 && drawerScore() == 12) {
        ready_ = true;
        reason_ = "MATCH";
    }
}

void Game::leaveStage() {
    if (!ready_ || !matched() || traps_ != 0 || drawerScore() != 12) return;
    left_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Leave;
    reason_ = "LEFT";
    chord(523.25f, 659.25f, 1046.5f);
    if (sys_) {
        sys_->rumble(0.25f, 0.5f, 120);
        sys_->setLight(80, 220, 140);
    }
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = 0.08f;
}

void Game::chord(float a, float b, float c) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.08f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = 0.36f;
}

void Game::ageTone() {
    if (!sys_) return;
    if (beep_ > 0.f) {
        beep_ -= 1.f / 60.f;
        if (beep_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
}

void Game::stepPlay() {
    if (ready_) {
        bool go = bot_;
        if (!bot_ && sys_) {
            const gs::Pad& p = sys_->pad;
            go = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C);
        }
        if (go) leaveStage();
        return;
    }

    if (bot_) {
        for (const auto& a : air_) {
            if (!a.on) continue;
            if (a.age >= kFlight - 6) latch_[a.to] = 10;
        }
    } else if (sys_) {
        const gs::Pad& p = sys_->pad;
        if (p.pressed(gs::BTN_LEFT) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_X)) latch_[0] = 12;
        if (p.pressed(gs::BTN_RIGHT) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_Y)) latch_[1] = 12;
        if (p.pressed(gs::BTN_DOWN) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Z)) fileLatch_ = 16;
    }

    if (tick_ % kEvery == 0) {
        int h = (tick_ / kEvery) & 1;
        if (hn_[h] <= 0) {
            miss();
            return;
        }
        int b = hold_[h][0];
        hold_[h][0] = hold_[h][1];
        hold_[h][1] = hold_[h][2];
        hn_[h]--;
        bool placed = false;
        for (auto& a : air_) {
            if (a.on) continue;
            a.on = true;
            a.ball = b;
            a.to = 1 - h;
            a.age = 0;
            placed = true;
            break;
        }
        if (!placed) {
            miss();
            return;
        }
    }

    for (auto& a : air_) {
        if (!a.on || a.age != kFlight) continue;
        int h = a.to;
        int ball = a.ball;
        a.on = false;
        if (latch_[h] <= 0 || hn_[h] >= 3) {
            miss();
            return;
        }
        hold_[h][hn_[h]++] = ball;
        latch_[h] = 0;
        int next = -1;
        for (int i = 0; i < kTapeN; i++) {
            if (!held_[i]) {
                next = i;
                break;
            }
        }
        bool file = false;
        if (bot_) file = next >= 0 && ball == kTape[next].ball;
        else file = fileLatch_ > 0;
        noteCatch(ball, file);
        if (file) fileLatch_ = 0;
        if (ready_) return;
    }

    for (auto& a : air_)
        if (a.on && a.age < kFlight) a.age++;
    for (int i = 0; i < 2; i++)
        if (latch_[i] > 0) latch_[i]--;
    if (fileLatch_ > 0) fileLatch_--;
    tick_++;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    ageTone();
    if (mode_ == Mode::Title) {
        titleWait_++;
        bool go = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A);
        if (bot_ && titleWait_ > 20) go = true;
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && !ready_ && sys.pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            titleWait_ = 0;
        } else {
            stepPlay();
        }
    } else if (mode_ == Mode::Drop) {
        if (--wait_ <= 0) pattern();
    } else if (mode_ == Mode::Leave) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) begin();
    }
    if (mode_ == Mode::Play) sys.setLight(ready_ ? 40 : 180, ready_ ? 200 : 80, ready_ ? 90 : 40);
    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(1, 1, 3);
        if (y > 28) c = gs::rgb4(3, 1, 6);
        if (y > 78) c = gs::rgb4(5, 2, 7);
        if (y > 140) c = gs::rgb4(3, 2, 4);
        if (y > 186) c = gs::rgb4(2, 1, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::ballAt(int ball, float x, float y, float scale) {
    const gs::Image& img = ballImg(art_, ball);
    int pal = ballPal(ball);
    float s = 16.f * scale;
    spr(img, x, y + 10.f, s * 0.85f, s * 0.32f, pal, true);
    spr(img, x, y, s, s, pal, false);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    auto glove = [&](int h, float x) {
        bool hot = latch_[h] > 0 && mode_ == Mode::Play && !ready_;
        spr(art_.glove, x, kHandY, hot ? 24.f : 18.f, hot ? 18.f : 14.f, PAL_GLOVE);
    };

    spr(art_.lamp, 28.f, 58.f, float(art_.lamp.w), float(art_.lamp.h), PAL_STAGE);
    spr(art_.reel, 286.f, 58.f, float(art_.reel.w), float(art_.reel.h), PAL_REEL);
    spr(art_.drawer, 276.f, 168.f, float(art_.drawer.w) + 8.f, float(art_.drawer.h) + 6.f, PAL_DRAWER);

    const bool showAir = mode_ == Mode::Play || mode_ == Mode::Drop || mode_ == Mode::Leave;
    if (showAir) {
        glove(0, kLeftX);
        glove(1, kRightX);
        for (const auto& a : air_) {
            if (!a.on) continue;
            float u = float(a.age) / float(kFlight);
            if (u < 0.f) u = 0.f;
            if (u > 1.f) u = 1.f;
            float x0 = handX(1 - a.to);
            float x = x0 + (handX(a.to) - x0) * u;
            float y = kHandY - 6.f - std::sin(u * kPi) * 86.f;
            ballAt(a.ball, x, y, a.ball == 1 ? 1.12f : 1.f);
        }
        for (int h = 0; h < 2; h++) {
            for (int i = 0; i < hn_[h]; i++) ballAt(hold_[h][i], handX(h), kHandY - 12.f - i * 11.f, 1.f);
        }
    } else {
        ballAt(0, kLeftX, kHandY - 20.f, 1.f);
        ballAt(1, 146.f, 78.f, 1.2f);
        ballAt(2, kRightX, kHandY - 20.f, 1.f);
        glove(0, kLeftX);
        glove(1, kRightX);
    }

    spr(art_.juggler, 146.f, 126.f, float(art_.juggler.w), float(art_.juggler.h), PAL_BODY);
    spr(art_.floor, 150.f, 206.f, float(art_.floor.w), float(art_.floor.h), PAL_STAGE);

    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) continue;
        ballAt(kTape[i].ball, 258.f + i * 14.f, 160.f, 0.7f);
    }

    hud(1, 1, "S3 JUGGLE TAPE", PAL_TITLE);
    char buf[48];
    std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
    hud(1, 3, buf, PAL_MARK);
    std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
    hud(30, 1, buf, ready_ ? PAL_WIN : PAL_INK);

    if (mode_ == Mode::Title) {
        hudC(21, "THE DRAWER HAS TO MATCH THE TAPE", PAL_MARK);
        hudC(23, "LEFT AND RIGHT CATCH", PAL_INK);
        hudC(25, "DOWN FILES THE NEXT SLIP", PAL_INK);
        hudC(27, "START  LEAVE WHEN IT MATCHES", PAL_INK);
    } else if (mode_ == Mode::Play && ready_) {
        hudC(24, "THE DRAWER MATCHES THE TAPE", PAL_WIN);
        hudC(26, bot_ ? "LEAVING" : "START TO LEAVE", PAL_MARK);
    } else if (mode_ == Mode::Play) {
        int next = 0;
        while (next < kTapeN && held_[next]) next++;
        if (next < kTapeN) {
            std::snprintf(buf, sizeof buf, "FILE %s", tapeLabel(next));
            hudC(26, buf, PAL_INK);
        }
    } else if (mode_ == Mode::Drop) {
        hudC(25, "DROP  DRAWER OPEN", PAL_ALERT);
    } else if (mode_ == Mode::Leave) {
        hudC(24, "THE DRAWER MATCHES THE TAPE", PAL_WIN);
        hudC(26, "LEFT THE BOOTH", PAL_MARK);
    }
}

}  // namespace juggletape
