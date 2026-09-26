#include "game/dartbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace dartbell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSweet = 0.08f;
constexpr float kMeterRate = 1.12f;
constexpr float kAim = 2.55f;
constexpr float kBot = 5.5f;
constexpr int kFlight = 16;
constexpr float kPi = 3.14159265f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

bool Game::sweet() const { return std::fabs(meter_ - 0.5f) <= kSweet; }

bool Game::audit() const {
    auto check = [&](int seg, float rad, Bed bed, int mul, const char* name) {
        float x, y;
        bedPoint(seg, rad, x, y);
        Mark m = classify(x, y);
        char got[12];
        markName(m, got, int(sizeof got));
        if (m.bed != bed || m.mul != mul || std::strcmp(got, name) != 0) {
            std::fprintf(stderr, "s3dartbell bed %s scored %s +%d at %.2f %.2f\n", name, got, m.mul, x, y);
            return false;
        }
        return true;
    };
    if (classify(kCx, kCy).bed != Bed::Bell) {
        std::fprintf(stderr, "s3dartbell centre is not the bell\n");
        return false;
    }
    for (int s = 0; s < 20; s++) {
        int n = kSeg[s];
        char nm[8];
        std::snprintf(nm, sizeof nm, "S%d", n);
        if (!check(s, (kLipOut + kTripIn) * 0.5f, Bed::Single, 1, nm)) return false;
        if (!check(s, (kTripOut + kDoubIn) * 0.5f, Bed::Single, 1, nm)) return false;
        std::snprintf(nm, sizeof nm, "T%d", n);
        if (!check(s, (kTripIn + kTripOut) * 0.5f, Bed::Triple, 3, nm)) return false;
        std::snprintf(nm, sizeof nm, "D%d", n);
        if (!check(s, (kDoubIn + kDoubOut) * 0.5f, Bed::Double, 2, nm)) return false;
    }
    if (!check(0, (kBellR + kLipOut) * 0.5f, Bed::Mount, 0, "BRASS")) return false;
    if (classify(kCx, kCy - (kDoubOut + 4.f)).bed != Bed::Miss) {
        std::fprintf(stderr, "s3dartbell wood scored\n");
        return false;
    }
    if (classify(kCx, kCy - (kRim + 8.f)).bed != Bed::Miss) return false;
    Mark hot = classify(kCx, kCy - (kLipOut + 2.f));
    if (hot.bed == Bed::Bell || hot.number != 20 || hot.mul != 1) {
        std::fprintf(stderr, "s3dartbell hot still on the bell\n");
        return false;
    }
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3dartbell rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(3, 1, 1));
    sys.apu.setMaster(0.72f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    why_ = Death::None;
    pinN_ = 0;
    last_[0] = 0;
    wasSweet_ = false;
    bellAmp_ = 0.2f;
    bellPh_ = 0;
    clock_ = 0;
    aimX_ = kCx;
    aimY_ = kCy;
    meter_ = 0;
    meterDir_ = 1.f;
    for (Pin& p : pin_) p.on = false;
}

void Game::newGame() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    why_ = Death::None;
    pinN_ = 0;
    last_[0] = 0;
    bellAmp_ = 0.2f;
    for (Pin& p : pin_) p.on = false;
    aimX_ = kCx;
    aimY_ = kCy;
    beginAim();
}

void Game::beginAim() {
    int w = 0;
    for (int i = 0; i < pinN_; i++) {
        if (pin_[i].on && pin_[i].board) pin_[w++] = pin_[i];
    }
    pinN_ = w;
    meter_ = 0;
    meterDir_ = 1.f;
    wasSweet_ = false;
    deadT_ = 0;
    if (bot_) {
        aimX_ = kCx;
        aimY_ = kCy;
    }
    mode_ = Mode::Aim;
}

void Game::remember(float x, float y, bool board) {
    if (pinN_ >= 3) {
        for (int i = 1; i < 3; i++) pin_[i - 1] = pin_[i];
        pinN_ = 2;
    }
    pin_[pinN_].x = x;
    pin_[pinN_].y = y;
    pin_[pinN_].on = true;
    pin_[pinN_].board = board;
    pinN_++;
}

void Game::moveAim(float mx, float my) {
    float m = std::hypot(mx, my);
    if (m > 0.f) {
        float scale = kAim * (m > 1.f ? 1.f / m : 1.f);
        aimX_ += mx * scale;
        aimY_ += my * scale;
    }
    float lim = kRim + 16.f;
    aimX_ = clampf(aimX_, kCx - lim, kCx + lim);
    aimY_ = clampf(aimY_, kCy - lim, kCy + lim);
}

void Game::launch() {
    if (mode_ != Mode::Aim || !rules_) return;
    float off = std::fabs(meter_ - 0.5f);
    fromX_ = kCx + (aimX_ - kCx) * 0.35f;
    fromY_ = float(gs::SCREEN_H) + 18.f;
    if (off <= kSweet) {
        fate_ = Fate::True;
        landX_ = aimX_;
        landY_ = aimY_;
    } else if (meter_ < 0.5f) {
        fate_ = Fate::Short;
        float u = clampf((0.5f - kSweet - meter_) / (0.5f - kSweet), 0.f, 1.f);
        landX_ = kCx + (aimX_ - kCx) * 0.45f;
        landY_ = float(kBoardY + kBmp) + 10.f + u * 6.f;
    } else {
        fate_ = Fate::Hot;
        float u = clampf((meter_ - 0.5f - kSweet) / (0.5f - kSweet), 0.f, 1.f);
        float dx = aimX_ - kCx;
        float dy = aimY_ - kCy;
        float d = std::hypot(dx, dy);
        float dirx = 0.f;
        float diry = -1.f;
        if (d > 0.5f) {
            dirx = dx / d;
            diry = dy / d;
        }
        float push = (kLipOut + 2.f) + u * u * 36.f;
        landX_ = aimX_ + dirx * push;
        landY_ = aimY_ + diry * push;
    }
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.16f, 2100.f, 0.04f);
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Flight) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ringT_ = 0;
    mode_ = Mode::Ring;
    if (!sys_) return;
    sys_->apu.noise(0, 1000);
    sys_->rumble(0.4f, 0.85f, 180);
    sys_->setLight(255, 196, 48);
}

void Game::dieTry(Death why) {
    if (rung_ || mode_ != Mode::Flight) return;
    why_ = why;
    dead_++;
    if (sys_) sys_->apu.noise(0, 1000);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        if (sys_) {
            blip(0, 90.f, 0.1f, 0.4f);
            sys_->apu.tone(1, 64.f, 0.07f);
            tickT_ = 0.4f;
            sys_->setLight(150, 28, 28);
        }
    } else {
        mode_ = Mode::Dead;
        deadT_ = 0;
        if (sys_) {
            blip(0, 140.f, 0.08f, 0.22f);
            sys_->setLight(120, 48, 28);
        }
    }
}

void Game::stick() {
    if (mode_ != Mode::Flight) return;
    float r = std::hypot(landX_ - kCx, landY_ - kCy);
    bool board = fate_ != Fate::Short && r <= kRim;
    remember(landX_, landY_, board);
    Mark m = classify(landX_, landY_);
    if (fate_ == Fate::Short) std::snprintf(last_, sizeof last_, "SHORT");
    else markName(m, last_, int(sizeof last_));
    if (fate_ == Fate::True && rules_ && m.bed == Bed::Bell) {
        ring();
        return;
    }
    Death why = Death::Stuck;
    if (fate_ == Fate::Short) why = Death::Short;
    else if (fate_ == Fate::Hot) why = Death::Hot;
    if (why == Death::Stuck && sys_) sys_->apu.noiseBurst(0.12f, 180.f, 0.06f);
    dieTry(why);
}

void Game::botAim() {
    float dx = kCx - aimX_;
    float dy = kCy - aimY_;
    float d = std::hypot(dx, dy);
    if (d > 0.4f) {
        float step = std::min(kBot, d);
        aimX_ += dx / d * step;
        aimY_ += dy / d * step;
        return;
    }
    aimX_ = kCx;
    aimY_ = kCy;
    if (sweet()) launch();
}

void Game::humanAim(const gs::Pad& p, bool fire) {
    float mx = 0, my = 0;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    if (std::fabs(p.axisX) > 0.2f) mx = p.axisX;
    if (std::fabs(p.axisY) > 0.2f) my = -p.axisY;
    moveAim(mx, my);
    bool sw = sweet();
    if (sw && !wasSweet_) blip(2, 988.f, 0.04f, 0.045f);
    wasSweet_ = sw;
    if (fire) launch();
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) toneT_ = hold;
    else tickT_ = hold;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (chiming) {
        bellTick_ -= dt;
        if (bellTick_ <= 0.f && bellAmp_ > 0.45f) {
            float vol = 0.05f + 0.08f * bellAmp_;
            sys_->apu.tone(0, 784.f, vol);
            sys_->apu.tone(1, 1175.f, vol * 0.7f);
            toneT_ = 0.14f;
            bellTick_ = 0.24f;
        }
    }
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Flight) sys_->apu.noise(0.03f, 1700.f, true);
    else sys_->apu.noise(0, 1000);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 13.5f : 2.3f);
    if (rung_) bellAmp_ = std::max(0.18f, bellAmp_ - kDt * 0.42f);

    Mode before = mode_;
    if (mode_ == Mode::Title || mode_ == Mode::Aim) {
        meter_ += meterDir_ * kMeterRate * kDt;
        if (meter_ >= 1.f) {
            meter_ = 1.f;
            meterDir_ = -1.f;
        } else if (meter_ <= 0.f) {
            meter_ = 0.f;
            meterDir_ = 1.f;
        }
    }

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool back = p.pressed(gs::BTN_MODE);
    bool fire = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        if (bot_ && clock_ > 0.4f) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (start || fire)) newGame();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && mode_ == Mode::Aim && (start || back)) {
        held_ = Mode::Aim;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        if (bot_) botAim();
        else humanAim(p, fire);
    }

    if (mode_ == Mode::Flight && before == Mode::Flight) {
        flightT_++;
        if (flightT_ >= kFlight) stick();
    } else if (mode_ == Mode::Dead && before == Mode::Dead) {
        deadT_ += kDt;
        if (deadT_ > 0.72f) beginAim();
    } else if (mode_ == Mode::Ring && before == Mode::Ring) {
        ringT_ += kDt;
        if (ringT_ > 0.68f) {
            mode_ = Mode::Leave;
            leaveT_ = 0;
        }
    } else if (mode_ == Mode::Leave && before == Mode::Leave) {
        leaveT_ += kDt;
        if (leaveT_ > 0.9f) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    }

    tickAudio(kDt);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::dartAt(float x, float y, float h) {
    float ih = std::max(8.f, h);
    float iw = ih * float(art_.dart.w) / float(std::max(1, int(art_.dart.h)));
    spr(art_.dart.pick(ih), x, y - ih * 0.42f, iw, ih, PAL_DART);
    spr(art_.shadow, x + 1.f, y + 3.f, iw * 0.7f, 4.f, PAL_BOARD, true);
}

void Game::meterRow() {
    for (int i = 0; i < 9; i++) {
        float x = kCx + float(i - 4) * 12.f;
        bool mid = i == 4;
        bool on = std::fabs(meter_ * 8.f - float(i)) <= 0.55f;
        int pal = PAL_WOOD;
        if (on && sweet()) pal = PAL_GREEN;
        else if (on) pal = PAL_ALERT;
        else if (mid) pal = PAL_GOLD;
        float s = mid ? 9.f : 7.f;
        spr(art_.pip, x, 28.f, s, s, pal);
    }
}

void Game::candles() {
    spr(art_.beam, 36.f, 118.f, float(art_.beam.w), float(art_.beam.h), PAL_WOOD);
    for (int i = 0; i < 3; i++) {
        float y = 78.f + float(i) * 32.f;
        bool gone = i < dead_;
        bool live = !gone && !(mode_ == Mode::Over && !won_);
        spr(art_.candle, 36.f, y, float(art_.candle.w), float(art_.candle.h), PAL_WAX);
        if (!live) continue;
        float flick = 9.f + std::sin(clock_ * 16.f + float(i) * 1.7f) * 0.8f;
        if (i == dead_ && !rung_) flick += 1.4f;
        spr(art_.flame, 36.f, y - 12.f, flick, flick + 1.f, PAL_FLAME);
    }
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c;
        if (y < 18) c = gs::rgb4(2, 1, 1);
        else if (y < 190) {
            int shade = 2 + (y - 18) / 48;
            c = gs::rgb4(4 + ((y / 6) & 1), shade, 2);
        } else {
            int band = (y / 4) & 1;
            c = gs::rgb4(3 + band, 2, 1);
        }
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float amp = rung_ ? bellAmp_ : 0.14f;
    float swing = std::sin(bellPh_) * 7.f * amp;
    float bx = kCx + swing;
    float by = kCy;

    if (mode_ == Mode::Title) spr(art_.title, kCx, 16.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_))
        spr(art_.rung, kCx, 16.f, float(art_.rung.w), float(art_.rung.h), PAL_TITLE);
    if (mode_ == Mode::Over && !won_) spr(art_.dead, kCx, 16.f, float(art_.dead.w), float(art_.dead.h), PAL_ALERT);

    if (rung_ && bellAmp_ > 0.4f) {
        for (int i = 0; i < 6; i++) {
            float a = bellPh_ * 1.3f + float(i) * 1.047f;
            float rad = 16.f + (1.f - bellAmp_) * 14.f;
            spr(art_.dot, bx + std::cos(a) * rad, by + std::sin(a) * rad * 0.55f, 3.f, 3.f, PAL_GOLD);
        }
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Pause) {
        meterRow();
        Mark live = classify(aimX_, aimY_);
        int pal = PAL_ALERT;
        if (live.bed == Bed::Bell && sweet()) pal = PAL_GREEN;
        else if (live.bed == Bed::Bell) pal = PAL_GOLD;
        spr(art_.cross, aimX_, aimY_, float(art_.cross.w), float(art_.cross.h), pal);
    } else if (mode_ == Mode::Title) {
        float bob = std::sin(clock_ * 3.f) * 1.2f;
        spr(art_.cross, kCx, kCy + bob, float(art_.cross.w), float(art_.cross.h), PAL_GOLD);
    }

    if (mode_ == Mode::Flight) {
        float u = clampf(float(flightT_) / float(kFlight), 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = fromX_ + (landX_ - fromX_) * e;
        float y = fromY_ + (landY_ - fromY_) * e;
        float lift = std::sin(u * kPi) * (fate_ == Fate::Short ? 52.f : 16.f);
        float h = 28.f + (12.f - 28.f) * e;
        dartAt(x, y - lift, h);
    }
    for (int i = pinN_ - 1; i >= 0; --i) {
        if (!pin_[i].on) continue;
        dartAt(pin_[i].x, pin_[i].y, pin_[i].board ? 13.f : 16.f);
    }

    spr(art_.clapper, bx + swing * 0.8f, by + 5.f, float(art_.clapper.w), float(art_.clapper.h), PAL_BELL);
    float bh = float(art_.bell.h);
    float bw = bh * float(art_.bell.w) / float(std::max(1, int(art_.bell.h)));
    spr(art_.bell.pick(bh), bx, by, bw, bh, PAL_BELL);
    spr(art_.yoke, kCx + swing * 0.25f, by - 24.f, float(art_.yoke.w), float(art_.yoke.h), PAL_WOOD);

    candles();
    spr(art_.flame, 292.f, 36.f, 10.f, 12.f, PAL_FLAME);
    spr(art_.flame, 24.f, 36.f, 8.f, 10.f, PAL_FLAME);
    spr(art_.oche, kCx, 208.f, float(art_.oche.w), float(art_.oche.h), PAL_CHALK);

    gs::Sprite board;
    board.img = art_.board;
    board.x = int16_t(kBoardX);
    board.y = int16_t(kBoardY);
    board.w = int16_t(art_.board.w);
    board.h = int16_t(art_.board.h);
    board.pal = PAL_BOARD;
    v.sprite(board);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(24, "RING THE BELL", PAL_GOLD);
        hudC(25, "BEFORE THE THIRD TRY DIES", PAL_INK);
        hudC(26, "SOFT DIES SHORT   HOT DIES OUT", PAL_INK);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS AIM   Z IN GREEN", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME   ESC TITLE", PAL_INK);
        return;
    }
    if (mode_ == Mode::Over && won_) {
        hudC(24, "BELL", PAL_GOLD);
        hudC(25, "LEFT BEFORE THE THIRD TRY", PAL_INK);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Over) {
        hudC(24, "THIRD TRY DEAD", PAL_ALERT);
        hudC(25, "BELL SILENT", PAL_INK);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Ring) {
        hudC(24, "BELL", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Leave) {
        hudC(24, "LEAVE", PAL_GREEN);
        hudC(25, "THE BELL RANG", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Dead) {
        hudC(11, "TRY DIED", PAL_ALERT);
        hudC(13, dead_ == 1 ? "TWO LEFT" : "ONE LEFT", PAL_GOLD);
        if (why_ == Death::Short) hudC(25, "DIED SHORT", PAL_ALERT);
        else if (why_ == Death::Hot) hudC(25, "DIED HOT", PAL_ALERT);
        else hudC(25, "STUCK", PAL_INK);
        if (last_[0]) hudC(26, last_, PAL_GOLD);
        return;
    }

    std::snprintf(buf, sizeof buf, "TRY %d OF 3", dead_ + 1);
    hud(1, 0, buf, dead_ == 2 ? PAL_ALERT : PAL_GOLD);
    std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
    hud(32, 0, buf, PAL_INK);
    hud(1, 1, "S3 DARTBELL", PAL_GOLD);

    if (mode_ == Mode::Flight) {
        hudC(25, "IN THE AIR", PAL_GOLD);
        return;
    }
    Mark live = classify(aimX_, aimY_);
    char name[12];
    markName(live, name, int(sizeof name));
    int pal = PAL_INK;
    if (live.bed == Bed::Bell) pal = sweet() ? PAL_GREEN : PAL_GOLD;
    else if (live.bed == Bed::Miss || live.bed == Bed::Mount) pal = PAL_ALERT;
    hudC(25, name, pal);
    hudC(26, "FIRM IN THE MOUTH", PAL_GOLD);
    hudC(27, sweet() ? "THROW" : "WAIT FOR GREEN", sweet() ? PAL_GREEN : PAL_AIM);
}

}  // namespace dartbell
