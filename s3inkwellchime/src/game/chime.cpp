#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace inkwellchime {
namespace {
constexpr float kPi = 3.14159265f;
constexpr float kDt = 1.f / 60.f;
constexpr float kWellSpeed = 220.f;
constexpr float kLip = 22.f;
}  // namespace

void Game::tone(float freq) {
    gs::FMPatch p;
    p.alg = 4;
    p.vol = 0.34f;
    p.op[0].mul = 1.f;
    p.op[0].level = 1.f;
    p.op[0].ar = 0.01f;
    p.op[0].dr = 0.45f;
    p.op[0].sl = 0.15f;
    p.op[0].rr = 0.7f;
    sys_->apu.setPatch(0, p);
    sys_->apu.keyOn(0, freq, 0.42f);
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hand(float cx, float cy, float ang, float len, int pal) {
    int n = std::max(1, int(len / 3.f));
    for (int i = 1; i <= n; i++) {
        float t = len * (i / float(n));
        spr(art_.dot, cx + std::sin(ang) * t, cy - std::cos(ang) * t, 4.f, 4.f, pal);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

float Game::needle() const {
    int t = phase_ % kPeriod;
    float u = t / float(kPeriod);
    return u < 0.5f ? u * 2.f : (1.f - u) * 2.f;
}

float Game::dropX() const { return 56.f + needle() * 168.f; }

bool Game::hot() const {
    float n = needle();
    return n > 0.40f && n < 0.60f;
}

int Game::clockSec() const { return kHourSec - kLeadSec + playFrames_ / kFpc; }

void Game::face(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
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

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::begin() {
    dips_ = 0;
    dead_ = 0;
    tryNo_ = 1;
    playFrames_ = 0;
    phase_ = 0;
    anim_ = 0;
    armed_ = false;
    wellX_ = 140.f;
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    tone(392.f);
}

void Game::dip() {
    if (full()) return;
    dips_++;
    tone(480.f + dips_ * 40.f);
    if (full()) {
        mode_ = Mode::Hold;
        anim_ = 0;
        tone(660.f);
    }
}

void Game::dieTry() {
    dead_++;
    tryNo_ = dead_ + 1;
    anim_ = 0;
    sys_->apu.noiseBurst(0.28f, 620.f, 0.22f);
    if (dead_ >= kTries) beginFail("TRIES");
    else mode_ = Mode::Miss;
}

void Game::beginChime() {
    mode_ = Mode::Chime;
    anim_ = 0;
    reason_ = "CHIME";
    won_ = true;
    tone(880.f);
    sys_->apu.noiseBurst(0.12f, 380.f, 0.16f);
}

void Game::beginFail(const char* why) {
    mode_ = Mode::Fail;
    anim_ = 0;
    reason_ = why;
    won_ = false;
    over_ = true;
    tone(140.f);
}

void Game::tickClock() {
    if (mode_ == Mode::Play || mode_ == Mode::Hold || mode_ == Mode::Miss) {
        playFrames_++;
        if (mode_ != Mode::Hold) phase_++;
    }
}

void Game::botAct(bool& press) {
    press = false;
    if (mode_ != Mode::Play || full()) return;
    float dx = dropX() - wellX_;
    float cap = kWellSpeed * kDt;
    if (dx > cap) dx = cap;
    if (dx < -cap) dx = -cap;
    wellX_ += dx;
    if (!hot()) armed_ = false;
    else if (!armed_ && std::fabs(dropX() - wellX_) < kLip - 4.f) {
        press = true;
        armed_ = true;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t mid = gs::rgb4(4, 3, 5);
    uint16_t bot = gs::rgb4(3, 2, 1);
    if (mode_ == Mode::Chime || (over_ && won_)) mid = gs::rgb4(10, 8, 3);
    if (mode_ == Mode::Fail) mid = gs::rgb4(6, 2, 2);
    if (mode_ == Mode::Hold) mid = gs::rgb4(5, 4, 6);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    spr(art_.page, 92.f, 112.f, float(art_.page.w), float(art_.page.h), PAL_PAGE);
    spr(art_.desk, 160.f, 200.f, float(art_.desk.w), float(art_.desk.h), PAL_DESK);

    for (int i = 0; i < dips_ && i < kDips; i++) {
        float y = 86.f + i * 14.f;
        spr(art_.drop, 70.f, y, 8.f, 10.f, PAL_INK);
        spr(art_.dot, 86.f, y, 28.f, 3.f, PAL_INK);
    }

    const float wellY = 176.f;
    int wellPal = hot() && mode_ == Mode::Play ? PAL_GOLD : PAL_INK;
    if (mode_ == Mode::Hold || mode_ == Mode::Chime) wellPal = PAL_GOLD;
    spr(art_.well, wellX_, wellY, 62.f, 38.f, wellPal);
    float qbob = (mode_ == Mode::Title) ? float((sys_->frame / 8) % 3) : (hot() ? 2.f : 0.f);
    spr(art_.quill, wellX_ + 16.f, wellY - 36.f - qbob, 14.f, 56.f, PAL_QUILL);

    if (mode_ == Mode::Play || mode_ == Mode::Title || mode_ == Mode::Miss)
        spr(art_.drop, dropX(), 148.f, 12.f, 16.f, PAL_INK);

    spr(art_.face, 262.f, 64.f, 52.f, 52.f, PAL_CLOCK);
    int h, m, s;
    face(h, m, s);
    float hang = (h % 12) * (kPi / 6.f) + m * (kPi / 360.f);
    float mang = m * (kPi / 30.f) + s * (kPi / 1800.f);
    float sang = s * (kPi / 30.f);
    hand(262.f, 64.f, hang, 10.f, PAL_CLOCK);
    hand(262.f, 64.f, mang, 16.f, PAL_BRASS);
    hand(262.f, 64.f, sang, 18.f, PAL_GOLD);

    for (int i = 0; i < kTries; i++) {
        int pal = i < (kTries - dead_) ? PAL_BRASS : PAL_DIM;
        spr(art_.quill, 18.f + i * 12.f, 28.f, 8.f, 22.f, pal);
    }

    char buf[80];
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 INKWELL CHIME", PAL_BRASS);
        hudC(16, "THE HOUR HAS TO CHIME", PAL_HUD);
        hudC(18, "FOUR DIPS IN THE OPEN MOUTH", PAL_BRASS);
        hudC(20, "THEN WAIT FOR THE STRIKE", PAL_HUD);
        hudC(22, "ARROWS SET THE WELL   A DIPS", PAL_DIM);
        if ((sys_->frame & 16) == 0) hudC(25, "PRESS START", PAL_BRASS);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Chime || (over_ && won_)) {
        hudC(2, "THE HOUR CHIMES", PAL_BRASS);
        hudC(4, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "DIPS %d  TRY %d", dips_, tryNo_);
        hudC(20, buf, PAL_BRASS);
        hudC(22, "THE WELL WAS FULL", PAL_HUD);
        hudC(24, "LEAVE", PAL_BRASS);
    } else if (mode_ == Mode::Fail) {
        hudC(2, "THE HOUR IS GONE", PAL_BAD);
        hudC(4, reason_, PAL_BAD);
        std::snprintf(buf, sizeof buf, "DIPS %d  TRY %d", dips_, std::min(tryNo_, kTries));
        hudC(20, buf, PAL_BAD);
        if ((sys_->frame & 16) == 0) hudC(25, "START", PAL_BRASS);
    } else if (mode_ == Mode::Hold) {
        hudC(2, onHour() ? "THE HOUR CHIMES" : "HOLD FOR THE HOUR", PAL_BRASS);
        hud(16, 1, buf, onHour() ? PAL_GOLD : PAL_HUD);
        hudC(26, "THE WELL IS FULL", PAL_DIM);
    } else if (mode_ == Mode::Miss) {
        hudC(2, "SPILL", PAL_BAD);
        std::snprintf(buf, sizeof buf, "TRY %d", tryNo_);
        hudC(22, buf, PAL_BAD);
    } else {
        hud(1, 1, "DIPS", PAL_INK);
        std::snprintf(buf, sizeof buf, "%d/%d", dips_, kDips);
        hud(6, 1, buf, PAL_BRASS);
        int hh, mm, ss;
        face(hh, mm, ss);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hh, mm, ss);
        hud(16, 1, buf, onHour() ? PAL_GOLD : PAL_HUD);
        hudC(26, hot() ? "DIP" : "WAIT FOR THE MOUTH", hot() ? PAL_BRASS : PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.55f);
    rules_ = kDips == 4 && kTries == 3 && kGraceSec > 0 && kLeadSec > kGraceSec && kPeriod > 8 && kFpc > 0;
    mode_ = Mode::Title;
    phase_ = 0;
    wellX_ = 150.f;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        phase_++;
        wellX_ = 120.f + std::sin(sys.frame * 0.05f) * 36.f;
        titleWait_++;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || (bot_ && titleWait_ > 8)) begin();
    } else if (mode_ == Mode::Fail) {
        anim_++;
        if (pad.pressed(gs::BTN_START) && !bot_) begin();
        if (anim_ > 30 && bot_) {
            /* stay failed */
        }
    } else if (mode_ == Mode::Chime) {
        anim_++;
        phase_ += 2;
        if (anim_ == 8) tone(988.f);
        if (anim_ == 20) tone(1174.f);
        if (anim_ > 36) {
            over_ = true;
            won_ = true;
            sys.quit();
        }
    } else if (mode_ == Mode::Miss) {
        tickClock();
        anim_++;
        if (pastHour() && !full()) beginFail("DRY");
        else if (anim_ > 24) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play || mode_ == Mode::Hold) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            /* no pause stack — start does not spill */
        }
        bool press = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B);
        if (bot_) botAct(press);
        else {
            float axis = std::fabs(pad.axisX) > 0.12f ? pad.axisX
                                                       : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
            wellX_ = std::clamp(wellX_ + axis * kWellSpeed * kDt, 48.f, 230.f);
        }
        if (mode_ == Mode::Play && press) {
            bool under = std::fabs(dropX() - wellX_) < kLip;
            if (hot() && under) dip();
            else dieTry();
        }
        tickClock();
        if (mode_ == Mode::Hold && onHour()) beginChime();
        else if ((mode_ == Mode::Play || mode_ == Mode::Hold) && pastHour() && !full()) beginFail("DRY");
        else if (mode_ == Mode::Play && onHour() && !full()) beginFail("DRY");
    }

    if (mode_ == Mode::Play) wellX_ = std::clamp(wellX_, 48.f, 230.f);
    draw();
}

}  // namespace inkwellchime
