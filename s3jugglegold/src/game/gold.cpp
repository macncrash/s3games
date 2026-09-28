#include "game/gold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace jugglegold {
namespace {

constexpr int kFlight = 36;
constexpr int kEvery = 18;
constexpr float kLeftX = 108.f;
constexpr float kRightX = 212.f;
constexpr float kHandY = 156.f;
constexpr float kPi = 3.14159265f;

float handX(int h) { return h == 0 ? kLeftX : kRightX; }

bool isGold(int ball) { return ball == 1; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = won_ = left_ = rules_ = finisherGold_ = ready_ = false;
    gold_ = cream_ = catches_ = 0;
    titleWait_ = 0;
    beep_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
}

void Game::begin() {
    rules_ = true;
    won_ = over_ = left_ = finisherGold_ = ready_ = false;
    gold_ = cream_ = catches_ = 0;
    pattern();
    blip(392.f);
}

void Game::pattern() {
    mode_ = Mode::Play;
    tick_ = 0;
    wait_ = 0;
    latch_[0] = latch_[1] = 0;
    hn_[0] = 2;
    hold_[0][0] = 0;
    hold_[0][1] = 2;
    hn_[1] = 1;
    hold_[1][0] = 1;
    hold_[1][1] = 0;
    hold_[1][2] = 0;
    for (auto& a : air_) a = {};
}

void Game::miss() {
    ready_ = false;
    finisherGold_ = false;
    gold_ = 0;
    cream_ = 0;
    mode_ = Mode::Drop;
    wait_ = 40;
    for (auto& a : air_) a.on = false;
    blip(90.f);
    if (sys_) sys_->setLight(160, 28, 28);
}

bool Game::canLeave() const {
    return ready_ && finisherGold_ && score() >= kLine && bare() < kLine && gold_ > 0 && cream_ < kLine;
}

void Game::noteCatch(int ball) {
    catches_++;
    if (ready_) return;
    if (isGold(ball)) {
        gold_++;
        finisherGold_ = score() >= kLine && bare() < kLine;
        ready_ = finisherGold_;
        blip(740.f);
        return;
    }
    // Cream counts its face. It cannot meet the line and it cannot fill the pile.
    if (score() + 1 >= kLine || bare() + 1 >= kLine) {
        blip(330.f);
        return;
    }
    cream_++;
    finisherGold_ = false;
    blip(480.f);
}

void Game::leaveStage() {
    if (!canLeave()) return;
    left_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Leave;
    chord(523.25f, 659.25f, 1046.5f);
    if (sys_) {
        sys_->rumble(0.25f, 0.55f, 120);
        sys_->setLight(255, 200, 60);
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
        a.on = false;
        if (latch_[h] <= 0 || hn_[h] >= 3) {
            miss();
            return;
        }
        hold_[h][hn_[h]++] = a.ball;
        latch_[h] = 0;
        noteCatch(a.ball);
        if (ready_) return;
    }

    for (auto& a : air_)
        if (a.on && a.age < kFlight) a.age++;
    for (int i = 0; i < 2; i++)
        if (latch_[i] > 0) latch_[i]--;
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
    if (mode_ == Mode::Play) sys.setLight(220, 160, 40);
    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(2, 1, 4);
        if (y > 36) c = gs::rgb4(4, 1, 7);
        if (y > 90) c = gs::rgb4(6, 2, 8);
        if (y > 150) c = gs::rgb4(3, 2, 4);
        if (y > 190) c = gs::rgb4(2, 1, 2);
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
    const gs::Image& img = isGold(ball) ? art_.gold : art_.cream;
    int pal = isGold(ball) ? PAL_GOLD : PAL_CREAM;
    float s = (isGold(ball) ? 18.f : 16.f) * scale;
    spr(img, x, y + 12.f, s * 0.9f, s * 0.35f, pal, true);
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
            float y = kHandY - 6.f - std::sin(u * kPi) * 92.f;
            ballAt(a.ball, x, y, isGold(a.ball) ? 1.12f : 1.f);
        }
        for (int h = 0; h < 2; h++) {
            for (int i = 0; i < hn_[h]; i++) ballAt(hold_[h][i], handX(h), kHandY - 10.f - i * 12.f, 1.f);
        }
    } else {
        ballAt(0, kLeftX, kHandY - 22.f, 1.f);
        ballAt(1, 160.f, 86.f, 1.25f);
        ballAt(2, kRightX, kHandY - 22.f, 1.f);
        glove(0, kLeftX);
        glove(1, kRightX);
    }

    spr(art_.juggler, 160.f, 128.f, float(art_.juggler.w), float(art_.juggler.h), PAL_BODY);
    spr(art_.lamp, 36.f, 70.f, float(art_.lamp.w), float(art_.lamp.h), PAL_LAMP);
    spr(art_.lamp, 284.f, 70.f, float(art_.lamp.w), float(art_.lamp.h), PAL_LAMP);
    spr(art_.drape, 22.f, 100.f, float(art_.drape.w), float(art_.drape.h), PAL_LAMP);
    spr(art_.drape, 298.f, 100.f, float(art_.drape.w), float(art_.drape.h), PAL_LAMP, false);
    spr(art_.floor, 160.f, 204.f, float(art_.floor.w) + 40.f, float(art_.floor.h), PAL_STAGE);

    hud(1, 1, "S3 JUGGLE GOLD", PAL_TITLE);
    char buf[48];
    std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d", gold_, cream_);
    hud(1, 3, buf, PAL_MARK);
    std::snprintf(buf, sizeof buf, "SCORE %d/%d", score(), kLine);
    hud(28, 1, buf, ready_ ? PAL_WIN : PAL_INK);

    if (mode_ == Mode::Title) {
        hudC(22, "ONLY THE GOLD COUNTS DOUBLE", PAL_MARK);
        hudC(24, "LEFT AND RIGHT CATCH", PAL_INK);
        hudC(26, "START  LEAVE ON THE DOUBLE", PAL_INK);
    } else if (mode_ == Mode::Play && ready_) {
        hudC(24, "GOLD DOUBLED THE LINE", PAL_WIN);
        hudC(26, bot_ ? "LEAVING" : "START TO LEAVE", PAL_MARK);
    } else if (mode_ == Mode::Play) {
        hudC(26, "CREAM CANNOT BUY THE LINE", PAL_INK);
    } else if (mode_ == Mode::Drop) {
        hudC(25, "DROP  TALLY CLEARED", PAL_ALERT);
    } else if (mode_ == Mode::Leave) {
        hudC(24, "ONLY THE GOLD COUNTS DOUBLE", PAL_WIN);
        hudC(26, "LEFT THE STAGE", PAL_MARK);
    }
}

}  // namespace jugglegold
