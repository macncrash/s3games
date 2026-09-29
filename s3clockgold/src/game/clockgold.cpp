#include "game/clockgold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace clockgold {
namespace {
constexpr float kTau = 6.2831853f;
static_assert(kGoldNeed * kGoldFace * 2 >= kLine, "doubled gold clears the line");
static_assert(kGoldNeed * kGoldFace < kLine, "bare gold faces stay short");
static_assert((kGoldNeed - 1) * kGoldFace * 2 < kLine, "leave on the last gold");
static_assert(kCreamFace < kGoldFace * 2, "cream is not a double");
}  // namespace

bool Game::open() const {
    return finisher_ && score_ >= kLine && bare_ < kLine && golds_ == kGoldNeed && cream_ == 0;
}

int Game::nextGold() const {
    for (int h = 0; h < kHours; h++)
        if (goldHour(h) && !struck_[h]) return h;
    return -1;
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.2f); }

void Game::chime(bool gold) {
    if (gold) {
        sys_->apu.keyOn(0, 392.f, 0.32f);
        sys_->apu.keyOn(1, 523.25f, 0.26f);
        sys_->apu.keyOn(2, 659.25f, 0.2f);
    } else {
        sys_->apu.keyOn(0, 196.f, 0.22f);
        sys_->apu.noiseBurst(0.16f, 160.f, 0.1f);
    }
}

void Game::begin() {
    hour_ = 11;
    score_ = 0;
    bare_ = 0;
    golds_ = 0;
    cream_ = 0;
    strikes_ = 0;
    rep_ = 0;
    anim_ = 0;
    sec_ = 0;
    secAcc_ = 0;
    over_ = false;
    won_ = false;
    finisher_ = false;
    for (int i = 0; i < kHours; i++) struck_[i] = false;
    mode_ = Mode::Play;
    sys_->apu.silence();
}

void Game::turn(int dir) {
    if (dir == 0 || mode_ != Mode::Play) return;
    hour_ = (hour_ + dir + kHours) % kHours;
    blip(goldHour(hour_) ? 620.f : (creamHour(hour_) ? 280.f : 360.f));
}

void Game::strike() {
    if (mode_ != Mode::Play) return;
    if (struck_[hour_]) {
        blip(140.f);
        return;
    }
    struck_[hour_] = true;
    strikes_++;
    if (goldHour(hour_)) {
        bare_ += kGoldFace;
        score_ += kGoldFace * 2;
        golds_++;
        if (score_ >= kLine && bare_ < kLine && cream_ == 0 && golds_ == kGoldNeed) finisher_ = true;
        chime(true);
    } else if (creamHour(hour_)) {
        bare_ += kCreamFace;
        score_ += kCreamFace;
        cream_++;
        finisher_ = false;
        chime(false);
    } else {
        blip(220.f);
    }
    mode_ = Mode::Strike;
    anim_ = 0;
}

void Game::leave() {
    if (mode_ != Mode::Play && mode_ != Mode::Strike) return;
    won_ = open();
    over_ = true;
    mode_ = Mode::Over;
    if (won_) {
        sys_->apu.keyOn(0, 440.f, 0.3f);
        sys_->apu.keyOn(1, 554.f, 0.26f);
        sys_->apu.keyOn(2, 659.f, 0.22f);
    } else {
        sys_->apu.noiseBurst(0.34f, 90.f, 0.28f);
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 6);
    uint16_t mid = gs::rgb4(7, 4, 3);
    uint16_t bot = gs::rgb4(2, 1, 3);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(12, 8, 2);
    if (mode_ == Mode::Over && !won_) {
        top = gs::rgb4(2, 0, 1);
        mid = gs::rgb4(5, 1, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.5f ? mix(top, mid, u / 0.5f) : mix(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    int shown = hour_;
    if (mode_ == Mode::Title) shown = int(sys_->frame / 16) % kHours;
    bool hot = goldHour(shown) && (mode_ != Mode::Title);
    spr(art_.bell, float(kCx), 40.f + (mode_ == Mode::Strike ? float(anim_ < 6 ? -anim_ : anim_ - 12) : 0.f), PAL_BELL);
    spr(art_.rope, float(kCx + 52), 78.f + (mode_ == Mode::Strike ? 6.f : 0.f), PAL_BELL);
    spr(art_.dial, float(kCx), float(kCy), PAL_DIAL);

    for (int h = 0; h < kHours; h++) {
        if (!goldHour(h) && !creamHour(h)) continue;
        float a = h * (kTau / 12.f);
        float px = kCx + std::sin(a) * 30.f;
        float py = kCy - std::cos(a) * 30.f;
        int pal = goldHour(h) ? PAL_GOLD : PAL_CREAM;
        if (struck_[h]) pal = PAL_DIM;
        spr(goldHour(h) ? art_.pipGold : art_.pipCream, px, py, pal);
    }

    spr(art_.minute, float(kCx), float(kCy), PAL_HAND);
    spr(art_.hour[shown], float(kCx), float(kCy), hot ? PAL_LIT : PAL_HAND);
    spr(art_.second[sec_ % kHours], float(kCx), float(kCy), PAL_SEC);

    char buf[64];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 CLOCK GOLD", PAL_GOLD);
        hudC(23, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(24, "CREAM DOES NOT BUY THE LINE", PAL_HUD);
        hudC(25, "LEAVE WHEN THAT IS TRUE", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "LEFT ON THE DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(24, buf, PAL_GOLD);
        hudC(25, "ONLY THE GOLD COUNTED DOUBLE", PAL_HUD);
        hudC(26, "THE DOOR IS OPEN", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL INSIDE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d  CREAM %d", score_, bare_, cream_);
        hudC(24, buf, PAL_BAD);
        hudC(25, "THE DOUBLE WAS NOT THE LEAVE", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BARE %d", bare_);
        hud(12, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "LINE %d", kLine);
        hud(30, 1, buf, PAL_DIM);
        int face = shown == 0 ? 12 : shown;
        std::snprintf(buf, sizeof buf, "HOUR %d", face);
        hudC(24, buf, hot ? PAL_GOLD : (creamHour(shown) ? PAL_CREAM : PAL_HUD));
        std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
        hudC(25, buf, cream_ ? PAL_BAD : PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_GOLD);
        else if (open()) hudC(26, "LEAVE  THE GOLD IS DOUBLE", PAL_GOLD);
        else if (goldHour(shown) && !struck_[shown]) hudC(26, "STRIKE THE GOLD", PAL_GOLD);
        else if (creamHour(shown)) hudC(26, "CREAM IS NOT A DOUBLE", PAL_CREAM);
        else hudC(26, "FIND THE GOLD HOURS", PAL_HUD);
        hudC(27, "LR TURN  C STRIKE  A LEAVE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (++secAcc_ >= 10) {
        secAcc_ = 0;
        sec_ = (sec_ + 1) % kHours;
    }

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else if (bot_) {
            int want = nextGold();
            if (want < 0) {
                if (open()) leave();
            } else if (hour_ != want) {
                int cw = (want - hour_ + kHours) % kHours;
                int ccw = (hour_ - want + kHours) % kHours;
                turn(cw <= ccw ? 1 : -1);
            } else {
                strike();
            }
        } else {
            int dir = 0;
            if (pad.down(gs::BTN_RIGHT)) dir += 1;
            if (pad.down(gs::BTN_LEFT)) dir -= 1;
            if (dir == 0) rep_ = 0;
            else if (pad.pressed(gs::BTN_RIGHT) || pad.pressed(gs::BTN_LEFT)) {
                rep_ = 0;
                turn(dir);
            } else if (++rep_ > 8 && (rep_ % 3) == 0) {
                turn(dir);
            }
            if (pad.pressed(gs::BTN_C)) strike();
            if (pad.pressed(gs::BTN_A)) leave();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Strike) {
        anim_++;
        if (anim_ >= 12) {
            mode_ = Mode::Play;
            if (bot_ && open()) leave();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (!bot_ && mode_ != Mode::Title && pad.pressed(gs::BTN_MODE)) {
        mode_ = Mode::Title;
        sys.apu.silence();
    }
    draw();
}

}  // namespace clockgold
