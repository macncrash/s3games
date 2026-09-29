#include "game/dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace viaduct {
namespace {

constexpr float kX[Game::kFlares] = {78.f, 160.f, 242.f};
constexpr float kCask = 22.f;
constexpr float kNight = 32.f;
constexpr float kSpeed = 240.f;
constexpr float kReach = 22.f;
constexpr float kDrain = 2.15f;
constexpr float kGust = 12.f;
constexpr float kGustLen = 1.35f;
constexpr float kFeed = 46.f;
constexpr float kDeck = 150.f;
const float kFan[] = {262.f, 330.f, 392.f, 523.f};

struct Ev {
    float t;
    int flare;
};
const Ev kGusts[] = {{4.2f, 1}, {9.0f, 0}, {13.8f, 2}, {18.6f, 1}, {23.4f, 0}, {27.6f, 2}};

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto L = [&](int x, int y) { return int(std::lround(x + (y - x) * t)); };
    return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
}

float smooth(float a, float b, float x) {
    float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 1;
}

int Game::lit() const {
    int n = 0;
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] > 0.5f) n++;
    return n;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.16f, 0.22f, 0.12f);
    buildArt(sys.vdp, art_);
    bootTitle();
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    fed_ = 0;
    cupped_ = 0;
    dead_ = -1;
    hold_ = 0;
    toneT_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    gustIx_ = 0;
    gustFlare_ = -1;
    face_ = 1;
    pitch_ = 2;
    t_ = 0;
    px_ = 40.f;
    move_ = 0;
    feedCd_ = 0;
    caskCd_ = 0;
    shake_ = 0;
    gustLeft_ = 0;
    gustHeld_ = 0;
    motes_.clear();
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] = 76.f;
        pop_[i] = 0;
        lowPing_[i] = false;
    }
}

void Game::beginWatch() {
    bootTitle();
    mode_ = Mode::Watch;
    blip(196.f, 0.05f, 8);
}

void Game::beginWin() {
    mode_ = Mode::Won;
    won_ = true;
    hold_ = 0;
    fanI_ = 0;
    fanT_ = 0;
    move_ = 0;
    sys_->setLight(255, 160, 70);
}

void Game::beginLoss(int flare) {
    mode_ = Mode::Lost;
    won_ = false;
    dead_ = flare;
    hold_ = 0;
    move_ = 0;
    fuel_[flare] = 0;
    sys_->rumble(0.7f, 0.25f, 180);
    sys_->setLight(70, 8, 0);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    switch (mode_) {
        case Mode::Title: updateTitle(); break;
        case Mode::Watch: updateWatch(dt); break;
        case Mode::Pause: updatePause(); break;
        case Mode::Won: updateEnd(true); break;
        case Mode::Lost: updateEnd(false); break;
    }
    if (mode_ != Mode::Title) stepMotes(dt);
    if (toneT_ > 0 && --toneT_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt);
    for (int i = 0; i < kFlares; i++)
        if (pop_[i] > 0.f) pop_[i] = std::max(0.f, pop_[i] - dt * 2.4f);
    draw();
}

void Game::updateTitle() {
    if (bot_) {
        if (sys_->frame >= 8) beginWatch();
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_MODE)) {
        if (sys_->hasHome()) sys_->eject();
        else sys_->quit();
        return;
    }
    bool go = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) ||
              p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_TURBO);
    if (go) beginWatch();
}

void Game::updatePause() {
    if (sys_->pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
    else if (sys_->pad.pressed(gs::BTN_MODE)) bootTitle();
}

void Game::updateEnd(bool dawn) {
    hold_++;
    if (dawn) {
        if (fanT_ > 0) fanT_--;
        if (fanT_ == 0 && fanI_ < 4) {
            float f = kFan[fanI_++];
            sys_->apu.tone(0, f, 0.07f);
            sys_->apu.tone(1, f * 0.5f, 0.03f);
            toneT_ = 12;
            fanT_ = 12;
        }
    } else if (hold_ == 1) {
        sys_->apu.tone(0, 70.f, 0.07f);
        sys_->apu.tone(1, 40.f, 0.04f);
        toneT_ = 28;
        sys_->apu.noiseBurst(0.22f, 360.f, 0.3f);
    }
    if (hold_ >= 72) over_ = true;
    if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_MODE)))
        bootTitle();
}

void Game::readPad(float& dir, bool& feed, bool& cup) const {
    const gs::Pad& p = sys_->pad;
    dir = 0;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (dir == 0.f && std::fabs(p.axisX) > 0.28f) dir = p.axisX > 0 ? 1.f : -1.f;
    feed = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
    cup = p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.down(gs::BTN_Y);
}

int Game::nearest() const {
    int best = 0;
    float d = 1e9f;
    for (int i = 0; i < kFlares; i++) {
        float a = std::fabs(px_ - kX[i]);
        if (a < d) {
            d = a;
            best = i;
        }
    }
    return best;
}

void Game::think(float& dir, bool& feed, bool& cup) {
    dir = 0;
    feed = false;
    cup = false;
    int low = 0;
    for (int i = 1; i < kFlares; i++)
        if (fuel_[i] < fuel_[low]) low = i;

    auto go = [&](float x) {
        float d = x - px_;
        if (std::fabs(d) < 3.f) return;
        dir = d > 0 ? 1.f : -1.f;
    };

    if (gustFlare_ >= 0) {
        go(kX[gustFlare_]);
        if (std::fabs(px_ - kX[gustFlare_]) <= kReach) {
            dir = 0;
            cup = true;
            if (pitch_ > 0 && fuel_[gustFlare_] < 50.f) feed = true;
        }
        return;
    }

    bool hungry = fuel_[low] < 56.f;
    if (pitch_ == 0 || (!hungry && pitch_ < 3 && fuel_[low] > 62.f)) {
        go(kCask);
        if (std::fabs(px_ - kCask) < 16.f) {
            dir = 0;
            feed = true;
        }
        if (pitch_ == 0) return;
        if (!hungry) return;
    }
    if (hungry || fuel_[low] < 68.f) {
        go(kX[low]);
        if (std::fabs(px_ - kX[low]) <= kReach && pitch_ > 0 && fuel_[low] < 70.f) {
            dir = 0;
            feed = true;
        }
    }
}

void Game::updateWatch(float dt) {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    float dir = 0;
    bool feed = false, cup = false;
    if (bot_) think(dir, feed, cup);
    else readPad(dir, feed, cup);

    if (dir != 0.f) face_ = dir > 0 ? 1 : -1;
    px_ = std::clamp(px_ + dir * kSpeed * dt, 14.f, 306.f);
    move_ = dir;

    const int nGust = int(sizeof(kGusts) / sizeof(kGusts[0]));
    if (gustFlare_ < 0 && gustIx_ < nGust && t_ + dt >= kGusts[gustIx_].t) {
        gustFlare_ = kGusts[gustIx_].flare;
        gustLeft_ = kGustLen;
        gustHeld_ = 0;
        gustIx_++;
        shake_ = 0.15f;
        blip(110.f, 0.045f, 7);
    }

    bool cuppedNow = false;
    if (gustFlare_ >= 0 && cup && std::fabs(px_ - kX[gustFlare_]) <= kReach + 2.f) {
        cuppedNow = true;
        gustHeld_ += dt;
    }

    feedCd_ = std::max(0.f, feedCd_ - dt);
    caskCd_ = std::max(0.f, caskCd_ - dt);
    if (feed && feedCd_ <= 0.f) {
        int i = nearest();
        if (std::fabs(px_ - kX[i]) <= kReach && pitch_ > 0 && fuel_[i] < 96.f) {
            fuel_[i] = std::min(100.f, fuel_[i] + kFeed);
            pitch_--;
            fed_++;
            pop_[i] = 1.f;
            feedCd_ = 0.28f;
            burst(kX[i], kDeck - 36.f, 7, PAL_FIRE);
            blip(480.f, 0.05f, 5);
        }
    }
    if (feed && caskCd_ <= 0.f && pitch_ < 3 && std::fabs(px_ - kCask) < 18.f) {
        pitch_++;
        caskCd_ = 0.34f;
        blip(160.f, 0.04f, 5);
    }

    for (int i = 0; i < kFlares; i++) {
        float loss = kDrain * dt;
        if (gustFlare_ == i && !cuppedNow) loss += kGust * dt;
        fuel_[i] -= loss;
        if (fuel_[i] < 24.f && !lowPing_[i]) {
            lowPing_[i] = true;
            blip(84.f, 0.05f, 8);
        }
        if (fuel_[i] > 42.f) lowPing_[i] = false;
        if (fuel_[i] <= 0.f) {
            beginLoss(i);
            return;
        }
    }

    if (gustFlare_ >= 0) {
        gustLeft_ -= dt;
        if (gustLeft_ <= 0.f) {
            if (gustHeld_ >= 0.4f) cupped_++;
            gustFlare_ = -1;
            gustLeft_ = 0;
        }
    }

    t_ += dt;
    if (t_ >= kNight) beginWin();
}

void Game::stepMotes(float dt) {
    for (size_t i = 0; i < motes_.size();) {
        Mote& m = motes_[i];
        m.x += m.vx * dt;
        m.y += m.vy * dt;
        m.vy -= 18.f * dt;
        m.life -= dt;
        if (m.life <= 0.f) motes_.erase(motes_.begin() + int(i));
        else i++;
    }
}

void Game::burst(float x, float y, int n, int pal) {
    for (int i = 0; i < n; i++) {
        float a = float(i) / float(n) * 6.2f;
        motes_.push_back({x, y, std::cos(a) * 28.f, -20.f - float(i % 3) * 8.f, 0.45f, pal});
    }
}

void Game::blip(float freq, float vol, int frames) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = std::max(toneT_, frames);
}

void Game::sky() {
    float e = 0.f;
    if (mode_ == Mode::Won) e = 1.f;
    else if (mode_ == Mode::Watch || mode_ == Mode::Pause) e = smooth(0.45f, 1.f, t_ / kNight);
    const uint16_t n0 = gs::rgb4(1, 1, 4);
    const uint16_t n1 = gs::rgb4(1, 2, 5);
    const uint16_t n2 = gs::rgb4(1, 2, 3);
    const uint16_t d0 = gs::rgb4(4, 5, 10);
    const uint16_t d1 = gs::rgb4(14, 8, 4);
    const uint16_t d2 = gs::rgb4(15, 12, 7);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t night, dawn;
        if (y < 90) {
            float u = y / 90.f;
            night = mix(n0, n1, u);
            dawn = mix(d0, d1, u);
        } else {
            float u = std::min(1.f, (y - 90) / 134.f);
            night = mix(n1, n2, u);
            dawn = mix(d1, d2, u);
        }
        uint16_t c = mix(night, dawn, e);
        if (mode_ == Mode::Lost) c = mix(c, gs::rgb4(3, 0, 0), 0.35f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::gorge() {
    if (mode_ == Mode::Title) return;
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(sys_->frame);
    float e = mode_ == Mode::Won ? 1.f : std::clamp(t_ / kNight, 0.f, 1.f);
    for (int y = 168; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f;
        r.hw = 420.f;
        r.v = float(y) * 3.f + t_ * 40.f;
        r.pal = PAL_WATER;
        r.band = ((y + int(t_ * 8.f)) / 6) & 1;
        r.style = 2;
        r.left = 0;
        r.right = 0;
        (void)e;
    }
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(255, 168, 72);
    else if (mode_ == Mode::Lost) sys_->setLight(64, 6, 0);
    else if (mode_ == Mode::Title) sys_->setLight(24, 28, 70);
    else {
        float f = 0;
        for (int i = 0; i < kFlares; i++) f += fuel_[i];
        f = std::clamp(f / 300.f, 0.f, 1.f);
        float e = smooth(0.5f, 1.f, t_ / kNight);
        sys_->setLight(int(36 + 80 * f + 110 * e), int(18 + 24 * f + 40 * e), int(28 + 8 * (1.f - e)));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(std::max(1.f, w)));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = false;
    s.clipY = gs::SCREEN_H;
    sys_->vdp.sprite(s);
}

float Game::lineWidth(const std::string& s, float h) const {
    float w = 0;
    for (char ch : s) {
        if (ch == ' ') {
            w += h * 0.45f;
            continue;
        }
        char c = ch;
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c < 32 || c >= 127) continue;
        const gs::Mipped& g = art_.glyph[int(c) - 32];
        if (g.h < 1) continue;
        w += h * float(g.w) / float(g.h) + 1.f;
    }
    return w;
}

void Game::word(const std::string& s, float cx, float y, float h, int pal) {
    float pen = cx - lineWidth(s, h) * 0.5f;
    for (char ch : s) {
        if (ch == ' ') {
            pen += h * 0.45f;
            continue;
        }
        char c = ch;
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c < 32 || c >= 127) continue;
        const gs::Mipped& g = art_.glyph[int(c) - 32];
        if (g.h < 1) continue;
        float w = h * float(g.w) / float(g.h);
        spr(g, pen + w * 0.5f, y, h, pal);
        pen += w + 1.f;
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

const char* Game::hint() const {
    if (gustFlare_ >= 0) return "GORGE WIND  CUP THE FLARE WITH B";
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] < 28.f) return "A PIER FLARE IS LOW  FEED IT";
    if (pitch_ == 0) return "PITCH CASK ON THE WEST BANK";
    return "A FEEDS PITCH   B CUPS THE WIND";
}

void Game::drawWorld() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.B.clear();
    int row0 = 16;
    for (int x = 0; x < 40; x++) {
        v.A.set(x, row0, gs::entry(x % 5 == 0 ? art_.joint : art_.deck, PAL_STONE));
        v.A.set(x, row0 + 1, gs::entry(art_.deck, PAL_STONE));
        v.A.set(x, row0 + 2, gs::entry(art_.deck, PAL_STONE, 0, 1));
    }

    const uint64_t fr = sys_->frame;
    if (mode_ == Mode::Won) word("DAWN", 160.f, 18.f, 18.f, PAL_GOLD);
    else if (mode_ == Mode::Title) {
        word("S3 VIADUCT DAWN", 160.f, 36.f, 14.f, PAL_GOLD);
        word("KEEP THE FLARES LIT", 160.f, 56.f, 10.f, PAL_HUD);
    } else if (mode_ == Mode::Lost)
        word("DARK", 160.f, 22.f, 18.f, PAL_ALERT);

    for (const Mote& m : motes_) spr(art_.spark, m.x, m.y, 3.f + m.life * 5.f, m.pal);

    int step = (std::fabs(move_) > 0.1f && (fr / 7) & 1) ? 1 : 0;
    if (mode_ != Mode::Title) spr(art_.man[step], px_, kDeck, 40.f, PAL_MAN, face_ < 0, 0, true);

    const float spanX[2] = {119.f, 201.f};
    for (int i = 0; i < kFlares; i++) {
        bool out = fuel_[i] <= 0.5f || (dead_ == i && mode_ == Mode::Lost);
        if (!out && mode_ != Mode::Title) {
            int flick = int(fr / 4 + i) & 1;
            float h = 16.f + pop_[i] * 9.f + std::sin(float(fr) * 0.33f + i) * 1.1f;
            if (fuel_[i] < 26.f) h *= 0.62f;
            int pal = fuel_[i] < 30.f ? PAL_EMBER : PAL_FIRE;
            if (gustFlare_ == i) pal = PAL_ALERT;
            spr(art_.flame[flick], kX[i], kDeck - 10.f, h, pal, false, 0, true);
        }
        if (mode_ != Mode::Title) spr(art_.basket, kX[i], kDeck + 2.f, 14.f, PAL_IRON, false, 0, true);
    }
    if (mode_ != Mode::Title) {
        spr(art_.cask, kCask, kDeck + 2.f, 26.f, PAL_CASK, false, 0, true);
        for (float x : spanX) spr(art_.span, x, kDeck + 28.f, 30.f, PAL_STONE, false, 0, true);
        for (float x : kX) spr(art_.pier, x, kDeck + 62.f, 58.f, PAL_PIER, false, 0, true);
    }
    if (mode_ != Mode::Won) spr(art_.moon, 268.f, 28.f, 16.f, PAL_MOON);
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        hudC(23, "START  WATCH THE VIADUCT", PAL_HUD);
        hudC(25, "A FEEDS A FLARE   B CUPS WIND", PAL_HINT);
        hudC(26, "PITCH LIVES IN THE WEST CASK", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_GOLD);
        hudC(26, "START RESUME   MODE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Won) {
        hudC(24, "THE FLARES HELD UNTIL DAWN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Lost) {
        hudC(24, "A FLARE WENT DARK", PAL_ALERT);
        hudC(26, "THE WATCH IS OVER", PAL_HUD);
        return;
    }
    int left = int(std::ceil(std::max(0.f, kNight - t_)));
    char line[48];
    std::snprintf(line, sizeof(line), "LIT %d  PITCH %d  DAWN %02d", lit(), pitch_, left);
    hud(1, 1, line, PAL_HUD);
    hudC(26, hint(), PAL_GOLD);
}

void Game::draw() {
    sky();
    gorge();
    lamp();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    drawWorld();
    drawHud();
}

}  // namespace viaduct
