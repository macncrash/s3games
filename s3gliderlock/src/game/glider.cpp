#include "glider.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace gliderlock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LO_X = 86.f;
constexpr float HI_X = 196.f;
constexpr float END_X = 308.f;
constexpr float CLOCK = 56.f;
constexpr float NOSE = 4.9f;
constexpr float TAIL = 4.05f;
constexpr float TOP = 3.45f;
constexpr float BELLY = 1.12f;
constexpr float SLAB = 0.7f;

constexpr float LO_SILL = 2.55f;
constexpr float LO_SHUT = 4.4f;
constexpr float LO_OPEN = 16.8f;
constexpr float LO_LINTEL = 23.5f;
constexpr float HI_SILL = 9.70f;
constexpr float HI_SHUT = 12.1f;
constexpr float HI_OPEN = 23.2f;
constexpr float HI_LINTEL = 28.5f;
constexpr float LO_RATE = 0.38f;
constexpr float HI_RATE = 0.36f;
constexpr float LO_TRIG = 42.f;

constexpr float END_LO = 12.4f;
constexpr float END_HI = 17.6f;
constexpr float BANK_X = 292.f;
constexpr float BANK_H = 10.35f;
constexpr float UP_WATER = 8.55f;

constexpr float TRIM = 1.08f;
constexpr float NOSE_G = 5.5f;
constexpr float SPOIL_SINK = 3.8f;
constexpr float STALL_V = 11.2f;
constexpr float STALL_K = 0.48f;

constexpr float T_LO = 8.40f;
constexpr float T_HI = 14.70f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float liftOf(float v) { return clampf((v - 7.f) / 10.f, 0.30f, 1.05f); }

float stallOf(float v) { return std::max(0.f, STALL_V - v) * STALL_K; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.16f;
    p.op[0] = {1.f, 1.f, 0.008f, 0.18f, 0.6f, 0.22f, 0.f};
    p.op[1] = {2.f, 0.32f, 0.012f, 0.24f, 0.35f, 0.28f, 0.f};
    p.op[2] = {3.2f, 0.14f, 0.01f, 0.2f, 0.22f, 0.3f, 0.f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f, 0.f};
    p.vol = 0.22f;
    p.tone = 2100.f;
    p.echo = 0.22f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 5;
    if (hi_.passed) return 4;
    if (lo_.passed) return 3;
    if (x_ > LO_X - 18.f) return 2;
    return 1;
}

float Game::leafBottom(float shut, float openH, float open) const {
    return shut + (openH - shut) * clampf(open, 0.f, 1.f);
}

float Game::sx(float wx) const { return anchor_ + (wx - camX_) * zoom_ + shx_; }

float Game::sy(float wy) const { return 136.f - (wy - camH_) * zoom_ + shy_; }

int Game::wingFrame() const {
    if (att_ > 0.22f) return 0;
    if (att_ > 0.07f) return 1;
    if (att_ < -0.22f) return 4;
    if (att_ < -0.07f) return 3;
    return 2;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    endDone_ = false;
    why_ = "";
    note_ = "";
    noteT_ = 0;
    chime_ = -1;
    lo_ = {};
    hi_ = {};
    x_ = 52.f;
    h_ = 9.2f;
    v_ = 16.f;
    vy_ = -0.2f;
    att_ = 0.04f;
    nose_ = 0;
    spoil_ = 0;
    openLo_ = 0.72f;
    openHi_ = 0.15f;
    legT_ = 0;
    shake_ = 0;
    snapCam_ = true;
    for (Puff& p : puffs_) p = {};
}

void Game::startRun() {
    mode_ = Mode::Fly;
    over_ = false;
    won_ = false;
    endDone_ = false;
    why_ = "";
    note_ = "HOLD THE LOWER SLOT";
    noteT_ = 2.4f;
    chime_ = -1;
    lo_ = {};
    hi_ = {};
    x_ = 18.f;
    h_ = T_LO;
    v_ = 16.2f;
    vy_ = 0.f;
    att_ = 0.02f;
    nose_ = 0;
    spoil_ = 0;
    openLo_ = 0;
    openHi_ = 0;
    t_ = 0;
    legT_ = 0;
    shake_ = 0;
    puffN_ = 0;
    snapCam_ = true;
    for (Puff& p : puffs_) p = {};
    blip(640.f);
}

void Game::win() {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "clear";
    note_ = "LEG CLEAR";
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.25f, 0.1f, 140);
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    sys_->rumble(0.65f, 0.35f, 180);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.5f, 480.f, 0.3f);
    sys_->apu.tone(2, 0.f, 0.f);
}

void Game::pilot(float& nose, float& spoil) const {
    float target = T_LO;
    if (!lo_.passed) {
        target = T_LO;
    } else if (!hi_.passed) {
        float u = clampf((x_ - (LO_X + 8.f)) / 52.f, 0.f, 1.f);
        u = u * u * (3.f - 2.f * u);
        target = T_LO + (T_HI - T_LO) * u;
    } else {
        target = T_HI;
    }

    float err = target - h_;
    float vyWant = clampf(err * 0.95f, -3.0f, 3.1f);
    spoil = 0.f;
    auto bleed = [&](float gateX, float open, float rate, float needOpen) {
        if (open >= needOpen || x_ > gateX - NOSE) return;
        float needT = (needOpen - open) / rate;
        float eta = (gateX - SLAB - (x_ + NOSE)) / std::max(v_, 1.f);
        if (eta < needT + 0.45f) spoil = std::max(spoil, 0.72f);
    };
    if (openLo_ < 1.f) bleed(LO_X, openLo_, LO_RATE, 0.70f);
    if (x_ > LO_TRIG) bleed(HI_X, openHi_, HI_RATE, 0.78f);
    if (v_ > 21.f) spoil = std::max(spoil, 0.28f);
    if (h_ < target - 1.6f) spoil *= 0.15f;

    float lift = liftOf(v_);
    float stall = stallOf(v_);
    nose = (vyWant + spoil * SPOIL_SINK + stall + TRIM * lift) / (NOSE_G * std::max(lift, 0.3f));
    nose = clampf(nose, -1.f, 1.f);
}

void Game::moveGates() {
    if (x_ >= LO_TRIG) openLo_ = std::min(1.f, openLo_ + LO_RATE * DT);
    if (x_ >= LO_X - 6.f) openHi_ = std::min(1.f, openHi_ + HI_RATE * DT);
}

void Game::throat(Throat& th, float gx, float sill, float leaf, float lintel, const char* scrape) {
    if (th.done || mode_ != Mode::Fly) return;
    const float x0 = x_ - TAIL;
    const float x1 = x_ + NOSE;
    const bool over = x1 > gx - SLAB && x0 < gx + SLAB;
    const float belly = h_ - BELLY;
    const float crown = h_ + TOP;
    if (over) {
        const bool hitSill = belly < sill;
        const bool hitLeaf = crown > leaf && belly < lintel;
        if (hitSill || hitLeaf) {
            fail(scrape);
            return;
        }
        if (belly > sill + 0.12f && crown < leaf - 0.12f) th.saw = true;
    }
    if (x0 > gx + SLAB) {
        th.done = true;
        if (th.saw) {
            th.passed = true;
            note_ = (&th == &lo_) ? "LOWER CLEAR" : "UPPER CLEAR";
            noteT_ = 1.6f;
            blip((&th == &lo_) ? 520.f : 690.f);
        } else {
            fail("missed the end");
        }
    }
}

void Game::floorHit() {
    if (mode_ != Mode::Fly) return;
    float bed = 0.f;
    if (x_ >= BANK_X) bed = BANK_H;
    else if (x_ >= HI_X + 1.8f) bed = UP_WATER;
    if (h_ - BELLY <= bed + 0.06f) fail("missed the end");
}

void Game::finishLine() {
    if (mode_ != Mode::Fly || endDone_) return;
    if (x_ + NOSE < END_X) return;
    endDone_ = true;
    if (!lo_.passed || !hi_.passed || h_ < END_LO || h_ > END_HI) fail("missed the end");
    else win();
}

void Game::physics(float noseCmd, float spoilCmd) {
    if (bot_) {
        nose_ = noseCmd;
        spoil_ = spoilCmd;
    } else {
        nose_ += (noseCmd - nose_) * 0.45f;
        spoil_ += (spoilCmd - spoil_) * 0.35f;
    }
    nose_ = clampf(nose_, -1.f, 1.f);
    spoil_ = clampf(spoil_, 0.f, 1.f);
    legT_ += DT;
    t_ += DT;

    float lift = liftOf(v_);
    float stall = stallOf(v_);
    float vyCmd = (nose_ * NOSE_G - TRIM) * lift - spoil_ * SPOIL_SINK - stall;
    vy_ += (vyCmd - vy_) * std::min(1.f, 4.4f * DT);
    v_ += (-0.06f + 0.40f * std::max(-vy_, 0.f) - 0.10f * std::max(vy_, 0.f) - spoil_ * 2.15f) * DT;
    v_ = clampf(v_, 0.f, 24.f);
    x_ += v_ * DT;
    h_ += vy_ * DT;
    if (!std::isfinite(x_) || !std::isfinite(h_) || !std::isfinite(v_)) {
        fail("missed the end");
        return;
    }

    moveGates();
    float loLeaf = leafBottom(LO_SHUT, LO_OPEN, openLo_);
    float hiLeaf = leafBottom(HI_SHUT, HI_OPEN, openHi_);
    throat(lo_, LO_X, LO_SILL, loLeaf, LO_LINTEL, "scraped the lower gate");
    throat(hi_, HI_X, HI_SILL, hiLeaf, HI_LINTEL, "scraped the upper gate");
    if (mode_ != Mode::Fly) return;
    floorHit();
    if (mode_ != Mode::Fly) return;
    finishLine();
    if (mode_ == Mode::Fly && legT_ >= CLOCK) fail("missed the end");
    if (mode_ == Mode::Fly && x_ > END_X + 30.f) fail("missed the end");
}

void Game::audio() {
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chime_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        chimeT_ += DT;
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys_->apu.keyOn(0, notes[chime_], 0.2f);
            else sys_->apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }
    if (mode_ != Mode::Fly) {
        if (mode_ != Mode::Win) sys_->apu.tone(2, 0.f, 0.f);
        sys_->apu.noise(0.f, 800.f, false);
        return;
    }
    float wind = clampf((v_ - 8.f) / 16.f, 0.f, 1.f) * (spoil_ > 0.4f ? 0.07f : 0.04f);
    sys_->apu.noise(wind, 700.f + v_ * 36.f, false);
    bool gateMove = (openLo_ > 0.02f && openLo_ < 0.98f) || (openHi_ > 0.02f && openHi_ < 0.98f && x_ > LO_X - 8.f);
    if (gateMove && beep_ <= 0.f) sys_->apu.tone(1, 78.f + openLo_ * 30.f, 0.03f);
    varioT_ -= DT;
    if (varioT_ <= 0.f) {
        varioT_ = (vy_ > 0.45f) ? 0.16f : (vy_ < -0.8f) ? 0.09f : 0.28f;
        if (v_ < STALL_V + 0.4f) sys_->apu.tone(2, 180.f, 0.04f);
        else if (vy_ > 0.35f) sys_->apu.tone(2, 620.f + vy_ * 28.f, 0.025f);
        else if (vy_ < -0.7f) sys_->apu.tone(2, 340.f, 0.02f);
        else sys_->apu.tone(2, 0.f, 0.f);
    }
}

void Game::camera() {
    float wantZ = 7.15f, wantA = 76.f, wantX = x_ + 4.f, wantH = h_ * 0.78f + 1.7f;
    if (mode_ == Mode::Title) {
        wantZ = 4.05f;
        wantA = 168.f;
        wantX = 74.f;
        wantH = 9.4f;
    } else if (mode_ == Mode::Win) {
        wantZ = 8.1f;
        wantX = x_ - 1.5f;
        wantH = h_ * 0.7f + 2.2f;
    } else if (mode_ == Mode::Fail) {
        wantX = x_ + 1.f;
    }
    if (snapCam_) {
        camX_ = wantX;
        camH_ = wantH;
        zoom_ = wantZ;
        anchor_ = wantA;
        snapCam_ = false;
    } else {
        float k = 1.f - std::exp(-7.5f * DT);
        camX_ += (wantX - camX_) * k;
        camH_ += (wantH - camH_) * k;
        zoom_ += (wantZ - zoom_) * k;
        anchor_ += (wantA - anchor_) * k;
    }
    if (shake_ > 0.f) {
        shake_ = std::max(0.f, shake_ - DT * 1.8f);
        shx_ = std::sin(t_ * 47.f) * shake_ * 4.f;
        shy_ = std::cos(t_ * 39.f) * shake_ * 3.f;
    } else {
        shx_ = shy_ = 0.f;
    }
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 5, 11);
    uint16_t mid = gs::rgb4(7, 10, 14);
    uint16_t hor = gs::rgb4(15, 11, 7);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 5, 4), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(12, 14, 9), 0.35f);
    float horY = sy(mode_ == Mode::Title ? 6.f : std::max(0.f, camH_ - 6.f));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, hor, (t - 0.55f) / 0.45f);
        float df = std::fabs(float(y) - horY);
        sys_->vdp.lineFog[y] = uint8_t(clampf(5.f - df / 18.f, 0.f, 4.f));
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.setFogColor(gs::rgb4(12, 10, 8));
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.4f || m.h < 1) return;
    float w = ht * float(m.w) / float(std::max(1, m.h));
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -4000L, 4000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprAnchor(const gs::Mipped& m, float ax, float ay, float cx, float cy, float destH, int pal) {
    if (destH < 1.5f || m.h < 1) return;
    float sc = destH / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(destH)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - ax * sc)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - ay * sc)), -4000L, 4000L));
    if (s.x > gs::SCREEN_W + 12 || s.x + s.w < -12 || s.y > gs::SCREEN_H + 12 || s.y + s.h < -12) return;
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal) {
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -4000L, 4000L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H + 4 || s.y + s.h < -4) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldBand(float x0, float x1, float hTop, float hBot, const gs::Mipped& m, int pal) {
    float s0 = sx(x0), s1 = sx(x1);
    float y0 = sy(hTop), y1 = sy(hBot);
    float top = std::min(y0, y1);
    float bot = std::max(y0, y1);
    sprBox(m, (s0 + s1) * 0.5f, top, std::fabs(s1 - s0) + 1.f, std::max(1.5f, bot - top), pal);
}

void Game::prop(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip, int fog) {
    spr(m, sx(wx), sy(wy), worldH * zoom_, pal, flip, fog);
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
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    const float adv = 18.f * scale;
    float left = x - float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + g.w * scale * 0.5f, y, std::max(8.f, g.h * scale), pal, false);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    camera();
    sky();

    const char* banner = nullptr;
    int bannerPal = PAL_HUD;
    if (mode_ == Mode::Title) {
        banner = "GLIDER LOCK";
    } else if (mode_ == Mode::Pause) {
        banner = "PAUSED";
        bannerPal = PAL_AMBER;
    } else if (mode_ == Mode::Fail) {
        banner = (why_ && std::strstr(why_, "scraped")) ? "SCRAPED" : "MISSED";
        bannerPal = PAL_BAD;
    } else if (mode_ == Mode::Win) {
        banner = "CLEAR";
        bannerPal = PAL_GOOD;
    }
    if (banner) text(banner, 160.f, mode_ == Mode::Title ? 22.f : 26.f, mode_ == Mode::Title ? 1.15f : 1.2f, bannerPal);
    if (mode_ == Mode::Title) text("PASS THE LOCK", 160.f, 48.f, 0.62f, PAL_AMBER);

    const Wing& wing = art_.wing[wingFrame()];
    float destH = float(wing.img.h) * (zoom_ / wing.ppm);
    sprAnchor(wing.img, wing.ax, wing.ay, sx(x_), sy(h_), destH, PAL_SHIP);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        prop(art_.puff, p.x, p.y, 0.7f + (1.f - p.life) * 0.8f, PAL_FX, false, int((1.f - p.life) * 6));
    }

    auto gate = [&](float gx, float sill, float leaf, float lintel, float open) {
        float lampH = lintel + 1.5f;
        prop(art_.lamp, gx - 3.1f, lampH, 1.15f, open > 0.82f ? PAL_GOOD : PAL_BAD);
        worldBand(gx - 4.6f, gx + 3.4f, lintel + 1.15f, lintel - 0.35f, art_.lintel, PAL_STONE);
        worldBand(gx - 3.6f, gx + 3.2f, sill, std::max(0.f, sill - 2.4f), art_.sill, PAL_STONE);
        if (leaf < lintel - 0.3f) worldBand(gx - 1.15f, gx + 1.15f, lintel - 0.2f, leaf, art_.leaf, PAL_GATE);
        worldBand(gx - 5.4f, gx - 2.5f, lintel + 0.4f, 0.f, art_.pier, PAL_STONE);
    };
    float loLeaf = leafBottom(LO_SHUT, LO_OPEN, openLo_);
    float hiLeaf = leafBottom(HI_SHUT, HI_OPEN, openHi_);
    gate(LO_X, LO_SILL, loLeaf, LO_LINTEL, openLo_);
    gate(HI_X, HI_SILL, hiLeaf, HI_LINTEL, openHi_);

    float bed = (x_ >= HI_X) ? UP_WATER : 0.f;
    float shade = clampf(38.f - (h_ - bed) * 1.3f, 12.f, 40.f);
    spr(art_.shadow, sx(x_), sy(bed) + 3.f, shade * 0.22f, PAL_FX);

    // End tape. The opening between the ribbons is the end of the leg.
    worldBand(END_X - 0.35f, END_X + 0.35f, 21.5f, BANK_H, art_.post, PAL_TAPE);
    worldBand(END_X - 7.f, END_X + 7.f, 20.6f, 19.5f, art_.bunting, PAL_TAPE);
    worldBand(END_X - 7.f, END_X + 7.f, 11.15f, 10.45f, art_.bunting, PAL_TAPE);

    float left = camX_ - (anchor_ + 40.f) / zoom_;
    float right = camX_ + (gs::SCREEN_W - anchor_ + 60.f) / zoom_;

    auto repeat = [&](float x0, float x1, float step, float base, float top, const gs::Mipped& m, int pal) {
        float a = std::max(x0, std::floor(left / step) * step);
        float b = std::min(x1, right + step);
        for (float x = a; x < b; x += step) worldBand(x, x + step + 0.4f, top, base, m, pal);
    };
    repeat(-20.f, LO_X - 6.f, 7.f, -0.15f, 1.15f, art_.grass, PAL_BANK);
    repeat(LO_X + 6.f, HI_X - 6.f, 8.f, 0.f, 3.15f, art_.stone, PAL_STONE);
    repeat(HI_X + 6.f, BANK_X - 2.f, 8.f, UP_WATER - 0.2f, UP_WATER + 1.05f, art_.grass, PAL_BANK);
    repeat(BANK_X, 380.f, 8.f, BANK_H - 0.3f, BANK_H + 1.35f, art_.grass, PAL_BANK);

    const float reeds[] = {28.f, 46.f, 68.f, 210.f, 236.f, 268.f};
    for (float rx : reeds) {
        if (rx < left - 4.f || rx > right + 4.f) continue;
        float ground = rx > HI_X ? UP_WATER : 0.f;
        prop(art_.reed, rx, ground + 1.3f, 2.8f, PAL_BANK, rx > 100.f);
    }
    if (30.f > left - 6.f && 30.f < right + 6.f) prop(art_.heron, 30.f, 1.5f, 2.4f, PAL_BIRD);
    prop(art_.sock[int(t_ * 3.f) % 3], 24.f, 3.6f, 3.3f, PAL_TAPE);
    prop(art_.cottage, 132.f, 5.0f, 4.6f, PAL_HOUSE);
    prop(art_.willow, 248.f, UP_WATER + 3.4f, 6.2f, PAL_BANK, false, 1);
    prop(art_.willow, 336.f, BANK_H + 3.6f, 6.8f, PAL_BANK, true, 2);

    worldBand(-30.f, HI_X + 1.f, 0.55f, -0.55f, art_.water, PAL_WATER);
    worldBand(-30.f, HI_X + 1.f, -0.45f, -14.f, art_.deep, PAL_WATER);
    worldBand(HI_X - 1.f, BANK_X + 2.f, UP_WATER + 0.45f, UP_WATER - 0.45f, art_.water, PAL_WATER);
    worldBand(HI_X - 1.f, BANK_X + 2.f, UP_WATER - 0.4f, UP_WATER - 12.f, art_.deep, PAL_WATER);

    int gull = int(t_ * 4.f) & 1;
    prop(art_.gull[gull], 40.f + std::fmod(t_ * 7.f, 90.f), 20.f + std::sin(t_ * 0.8f) * 1.2f, 1.3f, PAL_BIRD, false, 3);
    prop(art_.gull[1 - gull], 150.f + std::sin(t_ * 0.35f) * 18.f, 24.f, 1.15f, PAL_BIRD, true, 5);

    for (int i = 0; i < 4; i++) {
        float hx = std::fmod(20.f + float(i) * 78.f - camX_ * 0.35f, 340.f);
        if (hx < -40.f) hx += 340.f;
        spr(art_.hill, hx, 168.f, 36.f + float(i % 2) * 8.f, PAL_SKY, i & 1, 7);
    }
    spr(art_.sun, 284.f, 28.f, 22.f, PAL_SKY);
    for (int i = 0; i < 4; i++) {
        float cx = std::fmod(30.f + float(i) * 110.f - camX_ * 0.12f + t_ * 8.f, 480.f);
        if (cx < -30.f) cx += 480.f;
        spr(art_.cloud, cx, 26.f + float(i % 3) * 16.f, 16.f + float(i % 2) * 6.f, PAL_SKY, i & 1, 2);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(16, "UP CLIMB    DOWN DIVE", PAL_HUD);
        hudC(18, "Z  C  SPACE  SPOILER", PAL_HUD);
        hudC(20, "THREAD BOTH GATES", PAL_HUD);
        hudC(21, "A SCRAPE FAILS THE PASS", PAL_BAD);
        hudC(23, "MISSING THE END FAILS THE LEG", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START FLIES    ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(23, why_ ? why_ : "", PAL_BAD);
        hudC(25, "START TRIES THE LEG AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(22, "PASSED THE LOCK", PAL_GOOD);
        hudC(23, "THE LEG IS COMPLETE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "%.1f S", legT_);
        hudC(25, buf, PAL_AMBER);
    } else {
        std::snprintf(buf, sizeof buf, "ALT %4.1f", h_);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(12, 1, buf, v_ < STALL_V ? PAL_BAD : PAL_HUD);
        int leftSec = std::max(0, int(std::ceil(CLOCK - legT_ - 1e-3f)));
        std::snprintf(buf, sizeof buf, "LEG %2d", leftSec);
        hud(24, 1, buf, leftSec <= 10 ? PAL_BAD : PAL_HUD);
        if (spoil_ > 0.45f) hud(33, 1, "SPOILER", PAL_AMBER);

        const char* line = "TO THE LOWER GATE";
        int pal = PAL_HUD;
        if (!lo_.passed) {
            float dist = LO_X - (x_ + NOSE);
            if (dist > 8.f) {
                std::snprintf(buf, sizeof buf, "LOWER  %3.0f M", std::max(0.f, dist));
                line = buf;
            } else if (openLo_ < 0.62f) {
                line = "WAIT FOR THE GAP";
                pal = PAL_AMBER;
            } else {
                line = "LOWER SLOT  5 TO 12";
                pal = PAL_GOOD;
            }
        } else if (!hi_.passed) {
            float dist = HI_X - (x_ + NOSE);
            if (h_ < 11.4f) {
                line = "CLIMB FOR THE UPPER GATE";
                pal = PAL_AMBER;
            } else if (dist > 6.f) {
                std::snprintf(buf, sizeof buf, "UPPER  %3.0f M", std::max(0.f, dist));
                line = buf;
                pal = PAL_HUD;
            } else {
                line = "UPPER SLOT  12 TO 18";
                pal = PAL_GOOD;
            }
        } else {
            float dist = END_X - (x_ + NOSE);
            if (dist > 4.f) {
                std::snprintf(buf, sizeof buf, "END  %3.0f M", std::max(0.f, dist));
                line = buf;
                pal = PAL_AMBER;
            } else {
                line = "THROUGH THE TAPE";
                pal = PAL_GOOD;
            }
        }
        hudC(2, line, pal);
        if (noteT_ > 0.f && note_ && note_[0]) hudC(3, note_, PAL_GOOD);
        hudC(26, "UP DOWN FLY    Z SPOILER", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(12, 10, 8));
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.16f, 0.22f, 0.14f);
    sys.apu.setPatch(0, chimePatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) t_ += DT;
    if (noteT_ > 0.f) noteT_ = std::max(0.f, noteT_ - DT);
    for (Puff& p : puffs_)
        if (p.life > 0.f) p.life = std::max(0.f, p.life - DT);
    audio();

    if (!bot_ && mode_ == Mode::Title) {
        h_ = 9.2f + std::sin(t_ * 1.3f) * 0.28f;
        att_ = 0.05f + std::sin(t_ * 0.7f) * 0.06f;
        x_ = 52.f + std::sin(t_ * 0.4f) * 1.4f;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) {
            blip(600.f);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (mode_ == Mode::Win) t_ += DT;
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    float nose = 0.f, spoil = 0.f;
    if (bot_) {
        pilot(nose, spoil);
    } else {
        if (pad.down(gs::BTN_UP)) nose += 1.f;
        if (pad.down(gs::BTN_DOWN)) nose -= 1.f;
        if (std::fabs(pad.axisY) > 0.18f) nose = pad.axisY;
        nose = clampf(nose, -1.f, 1.f);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_X) ||
            pad.down(gs::BTN_Z) || pad.down(gs::BTN_TURBO))
            spoil = 1.f;
        if (pad.accel > 0.08f) spoil = std::max(spoil, pad.accel);
        if (pad.brake > 0.08f) spoil = std::max(spoil, pad.brake);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
            draw();
            return;
        }
    }

    bool wasFly = mode_ == Mode::Fly;
    physics(nose, spoil);
    if (wasFly && (mode_ == Mode::Fly || mode_ == Mode::Win)) {
        float path = std::atan2(vy_, std::max(8.f, v_));
        float want = path * 0.75f + nose_ * 0.16f;
        att_ += (want - att_) * 0.3f;
        att_ = clampf(att_, -0.55f, 0.55f);
    }
    if (mode_ == Mode::Fly && spoil_ > 0.55f && (sys.frame % 6) == 0) {
        puffs_[puffN_ % 8] = {x_ - TAIL * 0.4f, h_ - 0.3f, 0.45f};
        puffN_++;
    }
    if (mode_ == Mode::Fly) {
        if (h_ < 4.f || vy_ < -3.4f) sys.setLight(170, 50, 28);
        else if (lo_.passed && hi_.passed) sys.setLight(40, 150, 90);
        else sys.setLight(50, 90, 160);
    }
    draw();
}

}  // namespace gliderlock
