#include "game/loomchime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace loomchime {
namespace {

constexpr float kPi = 3.14159265f;

}  // namespace

int Game::clockSec() const {
    int t = (kHourSec - kLeadSec) + playFrames_ / kFpc;
    if (t < 0) t = 0;
    return t;
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::face(int& h, int& m, int& s) const {
    int t = clockSec();
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
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

void Game::begin() {
    spent_ = dead_ = early_ = laid_ = age_ = anim_ = playFrames_ = 0;
    dir_ = 1;
    over_ = won_ = false;
    reason_ = "";
    mode_ = Mode::Pass;
    sys_->apu.silence();
}

void Game::nextPass() {
    if (spent_ >= kShuttles) {
        fail("SHUTTLES");
        return;
    }
    age_ = 0;
    anim_ = 0;
    dir_ = -dir_;
    mode_ = Mode::Pass;
}

void Game::fail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    anim_ = 0;
    if (sys_) sys_->apu.tone(0, 90.f, 0.07f);
}

void Game::chime() {
    laid_++;
    spent_++;
    won_ = true;
    reason_ = "CHIME";
    anim_ = 0;
    mode_ = Mode::Chime;
    sys_->apu.keyOn(0, 523.f, 0.42f);
    sys_->apu.keyOn(1, 784.f, 0.28f);
    sys_->rumble(0.25f, 0.45f, 90);
}

void Game::killPass() {
    dead_++;
    spent_++;
    anim_ = 0;
    mode_ = Mode::Gap;
    sys_->apu.noiseBurst(0.18f, 70.f, 0.12f);
    if (spent_ >= kShuttles) reason_ = pastHour() ? "HOUR" : "SHUTTLES";
}

void Game::throwPick(bool sweet) {
    if (!sweet) {
        killPass();
        return;
    }
    if (onHour()) {
        chime();
        return;
    }
    early_++;
    spent_++;
    anim_ = 0;
    mode_ = Mode::Early;
    reason_ = "EARLY";
    sys_->apu.tone(0, 196.f, 0.06f);
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, bool hflip) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    sys_->vdp.sprite(s);
}

void Game::hand(float cx, float cy, float ang, float len, int pal) {
    for (float t = 3.f; t <= len; t += 3.5f) spr(art_.dot, cx + std::sin(ang) * t, cy - std::cos(ang) * t, pal);
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
    uint16_t top = gs::rgb4(1, 2, 4);
    uint16_t mid = gs::rgb4(4, 4, 6);
    uint16_t bot = gs::rgb4(2, 2, 3);
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) mid = gs::rgb4(8, 6, 3);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 2, 2);
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

    const float loomX = 132.f;
    const float loomY = 132.f;
    float swing = 0.f;
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) swing = std::sin(anim_ * 0.45f) * 8.f;

    spr(art_.frame, loomX, loomY, PAL_WOOD);
    spr(art_.beam, loomX, 58.f, PAL_WOOD);
    spr(art_.bell, loomX + swing, 34.f, PAL_BELL);
    spr(art_.clapper, loomX + swing * 1.5f, 42.f, PAL_BELL);

    const float warp0 = 90.f;
    const float gap = 14.f;
    float shed = 0.f;
    if (mode_ == Mode::Pass) shed = (age_ & 16) ? 5.f : -5.f;
    for (int c = 0; c < kWarps; c++) {
        float x = warp0 + c * gap;
        spr(art_.heddle, x, 112.f + ((c & 1) ? shed : -shed), PAL_WOOD);
        spr(art_.warp, x, 122.f, PAL_WARP);
    }

    int shown = (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) ? (anim_ / 4) : 0;
    if (shown > kPicks) shown = kPicks;
    for (int r = 0; r < shown; r++) {
        float y = 176.f - r * 7.f;
        for (int c = 0; c < kWarps - 1; c++) spr(art_.pick, warp0 + gap * 0.5f + c * gap, y, PAL_CLOTH);
    }

    float reedY = 156.f;
    if (mode_ == Mode::Pass) reedY = 146.f + (age_ / float(kDieAt)) * 16.f;
    if (mode_ == Mode::Chime) reedY = 168.f - (anim_ < 10 ? anim_ : 10);
    spr(art_.reed, loomX, reedY, PAL_WOOD);

    float t = 0.f;
    if (mode_ == Mode::Pass) t = age_ / float(kDieAt);
    else if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) t = 0.5f;
    else if (mode_ == Mode::Early) t = 0.5f - anim_ / 56.f;
    if (t < 0.f) t = 0.f;
    if (dir_ < 0) t = 1.f - t;
    float sx = 78.f + t * 108.f;
    bool sweet = mode_ == Mode::Pass && age_ >= kSweetLo && age_ <= kSweetHi;
    spr(art_.shuttle, sx, 136.f, sweet ? PAL_CLOTH : PAL_SHUTTLE, dir_ < 0);

    for (int i = 0; i < kShuttles; i++) {
        int pal = PAL_LAMP;
        if (i < dead_) pal = PAL_WOOD;
        else if (i < spent_) pal = PAL_BELL;
        spr(art_.lamp, 16.f + i * 12.f, 20.f, pal);
    }

    int h, m, s;
    face(h, m, s);
    float hang = ((float(h % 12) + float(m) / 60.f) / 12.f) * 2.f * kPi;
    float mang = ((float(m) + float(s) / 60.f) / 60.f) * 2.f * kPi;
    spr(art_.face, 276.f, 52.f, PAL_CLOCK);
    hand(276.f, 52.f, hang, 10.f, PAL_HAND);
    hand(276.f, 52.f, mang, 15.f, PAL_HAND);

    char buf[80];
    int f = int(sys_->frame);
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 LOOMCHIME", PAL_GOLD);
        hudC(18, "PLAY LOOM UNTIL", PAL_INK);
        hudC(19, "THE HOUR HAS TO CHIME", PAL_GOLD);
        hudC(21, "THROW IN THE SHED", PAL_INK);
        hudC(22, "ONLY THE HOUR STAYS", PAL_INK);
        hud(31, 3, buf, PAL_GOLD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE HOUR CHIMES", PAL_GOLD);
        hudC(21, "LEAVE THE LOOM", PAL_INK);
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(2, "THE HOUR IS GONE", PAL_BAD);
        hudC(21, reason_, PAL_INK);
        hudC(23, buf, PAL_DIM);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Chime) {
        hudC(2, "CHIME", PAL_GOLD);
        hudC(24, buf, PAL_GOLD);
    } else {
        hud(1, 1, buf, onHour() ? PAL_GOLD : PAL_INK);
        std::snprintf(buf, sizeof buf, "SHUTTLE %d/%d", spent_ + 1 > kShuttles ? kShuttles : spent_ + 1, kShuttles);
        hud(26, 1, buf, PAL_DIM);
        if (mode_ == Mode::Pass) {
            int mark = age_ * 20 / kDieAt;
            if (mark > 19) mark = 19;
            char bar[24];
            for (int i = 0; i < 20; i++) {
                bool band = (i * kDieAt / 20) >= kSweetLo && (i * kDieAt / 20) <= kSweetHi;
                bar[i] = (i == mark) ? '|' : (band ? '=' : '-');
            }
            bar[20] = 0;
            hudC(24, bar, sweet && onHour() ? PAL_GOLD : PAL_INK);
            if (onHour()) hudC(26, "THE HOUR — THROW", PAL_GOLD);
            else if (pastHour()) hudC(26, "TOO LATE", PAL_BAD);
            else {
                int left = (kHourSec - clockSec());
                if (left < 0) left = 0;
                std::snprintf(buf, sizeof buf, "HOUR IN %d", left);
                hudC(26, buf, PAL_DIM);
            }
        } else if (mode_ == Mode::Early) {
            hudC(24, "PULLED — BEFORE THE HOUR", PAL_BAD);
        } else {
            hudC(24, "THAT PASS DIED", PAL_BAD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
    over_ = won_ = false;
    reason_ = "";
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Pass) {
        if (pastHour()) {
            fail("HOUR");
        } else {
            bool tap = false;
            if (bot_) tap = onHour() && age_ == (kSweetLo + kSweetHi) / 2;
            else tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
            if (tap) {
                throwPick(age_ >= kSweetLo && age_ <= kSweetHi);
            } else if (++age_ >= kDieAt) {
                killPass();
                if (spent_ >= kShuttles && mode_ == Mode::Gap) fail(pastHour() ? "HOUR" : "SHUTTLES");
            }
        }
    } else if (mode_ == Mode::Early) {
        anim_++;
        if (pastHour()) fail("HOUR");
        else if (anim_ >= 22) nextPass();
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (pastHour() && !won_) fail("HOUR");
        else if (anim_ >= 24) nextPass();
    } else if (mode_ == Mode::Chime) {
        anim_++;
        if (anim_ == 12 || anim_ == 24 || anim_ == 36) sys_->apu.keyOn(1, 880.f, 0.22f);
        if (anim_ >= 48) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        anim_++;
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (mode_ == Mode::Pass || mode_ == Mode::Early || mode_ == Mode::Gap) playFrames_++;

    draw();
}

}  // namespace loomchime
