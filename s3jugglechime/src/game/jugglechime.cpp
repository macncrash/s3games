#include "game/jugglechime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace jugglechime {
namespace {

constexpr int kFlight = 30;
constexpr int kEvery = 15;
constexpr float kLeftX = 112.f;
constexpr float kRightX = 208.f;
constexpr float kHandY = 158.f;
constexpr float kPi = 3.14159265f;

float handX(int h) { return h == 0 ? kLeftX : kRightX; }
bool isGold(int ball) { return ball == 1; }

}  // namespace

int Game::secAt(int frames) const {
    if (frames < 0) frames = 0;
    return kStartSec + frames / kFpc;
}

int Game::clockSec() const { return secAt(playFrames_); }

bool Game::onHourAt(int frames) const {
    int s = secAt(frames);
    return s >= kHourSec && s < kHourSec + kGraceSec;
}

bool Game::onHour() const { return onHourAt(playFrames_); }

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    h = (t / 3600) % 24;
    m = (t / 60) % 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    split(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    split(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    split(h, m, s);
    return s;
}

bool Game::audit() const {
    auto bad = [](const char* w) {
        std::fprintf(stderr, "s3jugglechime %s\n", w);
        return false;
    };
    if (secAt(0) != kStartSec) return bad("clock does not open at 11:58");
    if (secAt(120 * kFpc) != kHourSec) return bad("frame 720 is not noon");
    if (onHourAt(120 * kFpc - 1)) return bad("11:59 counted as the hour");
    if (!onHourAt(120 * kFpc)) return bad("noon missed");
    if (!onHourAt(120 * kFpc + kGraceSec * kFpc - 1)) return bad("grace ended early");
    if (onHourAt(120 * kFpc + kGraceSec * kFpc)) return bad("grace ran long");
    if (secAt(120 * kFpc) / 3600 != 12) return bad("hour is not twelve");
    if ((secAt(120 * kFpc) / 60) % 60 != 0) return bad("noon is not zero minutes");
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3jugglechime rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 3));
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    sys.apu.setMaster(0.7f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = won_ = clockOn_ = live_ = false;
    reason_ = "";
    catches_ = playFrames_ = titleWait_ = tick_ = wait_ = 0;
    chimeFrames_ = strikes_ = leaveT_ = 0;
    beep_ = bellPh_ = 0;
    for (auto& a : air_) a = {};
    hn_[0] = hn_[1] = 0;
    latch_[0] = latch_[1] = 0;
}

void Game::begin() {
    if (!rules_) return;
    won_ = over_ = live_ = false;
    reason_ = "";
    catches_ = 0;
    playFrames_ = 0;
    clockOn_ = true;
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
    for (auto& a : air_) a = {};
}

void Game::miss() {
    mode_ = Mode::Drop;
    wait_ = 36;
    for (auto& a : air_) a.on = false;
    blip(96.f);
    if (sys_) sys_->setLight(150, 30, 30);
}

void Game::beginChime() {
    clockOn_ = false;
    live_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    chord(523.25f, 659.25f, 784.f);
    if (sys_) {
        sys_->rumble(0.3f, 0.5f, 80);
        sys_->setLight(255, 210, 80);
    }
}

void Game::beginFail(const char* why) {
    clockOn_ = false;
    live_ = false;
    won_ = false;
    over_ = true;
    reason_ = why;
    mode_ = Mode::Fail;
    blip(70.f);
}

void Game::stepPlay() {
    if (bot_) {
        for (const auto& a : air_) {
            if (!a.on) continue;
            if (a.age >= kFlight - 5) latch_[a.to] = 8;
        }
    } else if (sys_) {
        const gs::Pad& p = sys_->pad;
        if (p.pressed(gs::BTN_LEFT) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_X)) latch_[0] = 10;
        if (p.pressed(gs::BTN_RIGHT) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_Y)) latch_[1] = 10;
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
        catches_++;
        blip(isGold(a.ball) ? 660.f : 440.f);
    }

    for (auto& a : air_)
        if (a.on && a.age < kFlight) a.age++;
    for (int i = 0; i < 2; i++)
        if (latch_[i] > 0) latch_[i]--;
    tick_++;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = 0.07f;
}

void Game::chord(float a, float b, float c) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.09f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = 0.28f;
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

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    ageTone();
    bellPh_ += 0.08f;

    if (mode_ == Mode::Title) {
        titleWait_++;
        bool go = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A);
        if (bot_ && titleWait_ > 18) go = true;
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        stepPlay();
        if (mode_ == Mode::Play && clockOn_) {
            playFrames_++;
            if (onHour()) beginChime();
            else if (pastHour()) beginFail("LATE");
        }
    } else if (mode_ == Mode::Drop) {
        if (clockOn_) {
            playFrames_++;
            if (pastHour()) beginFail("LATE");
        }
        if (mode_ == Mode::Drop && --wait_ <= 0) {
            if (pastHour() || !clockOn_) beginFail("LATE");
            else pattern();
        }
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (strikes_ < 12 && (chimeFrames_ % 6) == 0) {
            strikes_++;
            float f = 392.f * (1.f + 0.02f * float(strikes_));
            chord(f, f * 1.5f, f * 2.f);
            bellPh_ = 0.f;
        }
        if (chimeFrames_ >= 12 * 6 + 24) {
            won_ = live_ && std::strcmp(reason_, "CHIME") == 0 && onHour() && catches_ > 0;
            over_ = true;
            mode_ = Mode::Leave;
            leaveT_ = 0;
        }
    } else if (mode_ == Mode::Leave || mode_ == Mode::Fail) {
        leaveT_++;
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) toTitle();
    }

    if (mode_ == Mode::Play && clockOn_) sys.setLight(40, 80, 160);
    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(1, 1, 4);
        if (y > 28) c = gs::rgb4(2, 2, 6);
        if (y > 80) c = gs::rgb4(3, 2, 7);
        if (y > 140) c = gs::rgb4(2, 1, 4);
        if (y > 188) c = gs::rgb4(1, 1, 2);
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
    float s = 16.f * scale;
    spr(img, x, y + 10.f, s * 0.85f, s * 0.32f, pal, true);
    spr(img, x, y, s, s, pal, false);
}

void Game::clockAt(float cx, float cy) {
    spr(art_.face, cx, cy, float(art_.face.w), float(art_.face.h), PAL_CLOCK);
    int h, m, s;
    split(h, m, s);
    float ha = (float(h % 12) + float(m) / 60.f) * kPi / 6.f - kPi / 2.f;
    float ma = float(m) * kPi / 30.f - kPi / 2.f;
    auto hand = [&](float ang, float len, float w, int pal) {
        for (int i = 1; i <= 7; i++) {
            float u = float(i) / 7.f;
            spr(art_.pip, cx + std::cos(ang) * len * u, cy + std::sin(ang) * len * u, w, w, pal);
        }
    };
    hand(ha, 12.f, 4.f, PAL_CLOCK);
    hand(ma, 16.f, 3.f, PAL_INK);
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

    const bool showAir = mode_ != Mode::Title;
    auto glove = [&](int h, float x) {
        bool hot = latch_[h] > 0 && mode_ == Mode::Play;
        spr(art_.glove, x, kHandY, hot ? 22.f : 16.f, hot ? 16.f : 12.f, PAL_GLOVE);
    };

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
            float y = kHandY - 8.f - std::sin(u * kPi) * 86.f;
            ballAt(a.ball, x, y, isGold(a.ball) ? 1.15f : 1.f);
        }
        for (int hnd = 0; hnd < 2; hnd++) {
            for (int i = 0; i < hn_[hnd]; i++) ballAt(hold_[hnd][i], handX(hnd), kHandY - 12.f - i * 11.f, 1.f);
        }
    } else {
        ballAt(0, kLeftX, kHandY - 24.f, 1.f);
        ballAt(1, 160.f, 78.f, 1.2f);
        ballAt(2, kRightX, kHandY - 24.f, 1.f);
        glove(0, kLeftX);
        glove(1, kRightX);
    }

    float swing = std::sin(bellPh_) * (mode_ == Mode::Chime || mode_ == Mode::Leave ? 10.f : 2.f);
    spr(art_.bell, 46.f + swing, 58.f, float(art_.bell.w), float(art_.bell.h), PAL_BELL);
    clockAt(274.f, 58.f);
    spr(art_.juggler, 160.f, 132.f, float(art_.juggler.w), float(art_.juggler.h), PAL_BODY);
    spr(art_.floor, 160.f, 206.f, 240.f, float(art_.floor.h), PAL_STAGE);

    hud(1, 1, "S3 JUGGLECHIME", PAL_TITLE);
    char buf[48];
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
    hud(30, 1, buf, onHour() ? PAL_WIN : PAL_INK);
    std::snprintf(buf, sizeof buf, "CATCH %d", catches_);
    hud(1, 3, buf, PAL_INK);

    if (mode_ == Mode::Title) {
        hudC(20, "PLAY UNTIL THE HOUR CHIMES", PAL_WIN);
        hudC(22, "LEFT AND RIGHT CATCH", PAL_INK);
        hudC(24, "A CATCH BEFORE NOON IS NOT IT", PAL_INK);
        hudC(26, "START", PAL_TITLE);
    } else if (mode_ == Mode::Play) {
        hudC(26, onHour() ? "THE HOUR" : "KEEP THE CASCADE", PAL_INK);
    } else if (mode_ == Mode::Drop) {
        hudC(25, "DROP", PAL_ALERT);
    } else if (mode_ == Mode::Chime) {
        hudC(24, "THE HOUR CHIMES", PAL_WIN);
        std::snprintf(buf, sizeof buf, "STRIKE %d", strikes_);
        hudC(26, buf, PAL_TITLE);
    } else if (mode_ == Mode::Leave) {
        hudC(24, "THE HOUR CHIMES", PAL_WIN);
        hudC(26, "LEFT THE STAGE", PAL_TITLE);
    } else if (mode_ == Mode::Fail) {
        hudC(24, "THE HOUR PASSED", PAL_ALERT);
        hudC(26, "START", PAL_INK);
    }
}

}  // namespace jugglechime
