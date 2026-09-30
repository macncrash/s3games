#include "game/foundry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace foundrydawn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kNight = 46.f;
constexpr float kWarn = 2.35f;
constexpr float kSpeed = 195.f;
constexpr float kReach = 20.f;
constexpr float kFeet = 176.f;
constexpr float kX[Game::kFlares] = {48.f, 116.f, 196.f, 272.f};

struct Beat {
    float t;
    int flare;
    int kind;
};

constexpr Beat kBeats[] = {
    {5.5f, 0, 0},  {11.5f, 3, 1}, {17.5f, 1, 0}, {23.5f, 2, 1},
    {29.5f, 3, 0}, {35.5f, 0, 1}, {41.0f, 2, 0},
};

uint16_t mix4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    int r = int(std::lround(ar + (br - ar) * t));
    int g = int(std::lround(ag + (bg - ag) * t));
    int bl = int(std::lround(ab + (bb - ab) * t));
    return gs::rgb4(r, g, bl);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.45f);
    sys.apu.silence();
    bootTitle();
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    hold_ = 0;
    dead_ = -1;
    beat_ = 0;
    fed_ = hauled_ = braced_ = 0;
    px_ = 160.f;
    threat_.live = false;
    for (int i = 0; i < kFlares; i++) {
        fuel_[i] = 78.f;
        pop_[i] = hurt_[i] = 0;
    }
    motes_.clear();
}

void Game::beginWatch() {
    bootTitle();
    mode_ = Mode::Watch;
    t_ = 0;
    blip(180.f, 0.12f, 8);
}

void Game::beginWin() {
    mode_ = Mode::Won;
    won_ = true;
    hold_ = 0;
    threat_.live = false;
    burst(160.f, 70.f, 16, 40.f, PAL_GOLD);
    blip(523.f, 0.16f, 18);
}

void Game::beginLoss(int flare) {
    mode_ = Mode::Lost;
    won_ = false;
    dead_ = flare;
    hold_ = 0;
    fuel_[flare] = 0;
    threat_.live = false;
    burst(flareX(flare), kFeet - 24.f, 12, 36.f, PAL_ALERT);
    sys_->apu.noiseBurst(0.35f, 1800.f, 0.25f);
}

int Game::lit() const {
    int n = 0;
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] > 0.5f) n++;
    return n;
}

int Game::marker() const {
    if (mode_ == Mode::Watch) return 1;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 0;
}

float Game::flareX(int i) const { return kX[i]; }

float Game::dawnEase() const {
    if (mode_ == Mode::Won) return 1.f;
    if (mode_ != Mode::Watch) return 0.f;
    float u = std::clamp(t_ / kNight, 0.f, 1.f);
    return u * u;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (toneT_ > 0 && --toneT_ == 0) sys.apu.tone(0, 0, 0);
    if (mode_ == Mode::Title) updateTitle();
    else if (mode_ == Mode::Watch) updateWatch(kDt);
    else updateEnd();
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt);
    stepMotes(kDt);
    draw();
}

void Game::updateTitle() {
    t_ += kDt;
    bool go = bot_ && t_ > 0.45f;
    if (!go && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) go = true;
    if (go) beginWatch();
}

void Game::readPad(float& dir, bool& feed, bool& brace, bool& haul) const {
    dir = 0;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(sys_->pad.axisX) > 0.3f) dir = sys_->pad.axisX;
    dir = std::clamp(dir, -1.f, 1.f);
    feed = sys_->pad.down(gs::BTN_A);
    brace = sys_->pad.down(gs::BTN_B);
    haul = sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_X);
}

void Game::think(float& dir, bool& feed, bool& brace, bool& haul) {
    dir = 0;
    feed = brace = haul = false;
    int goal = lowest();
    bool urgent = threat_.live && threat_.eta < 2.05f;
    if (urgent) goal = threat_.flare;
    float dx = flareX(goal) - px_;
    if (std::fabs(dx) > 6.f) dir = dx > 0 ? 1.f : -1.f;
    bool near = std::fabs(dx) <= kReach;
    if (urgent && near) {
        if (threat_.kind == Kind::Slag) brace = true;
        else haul = true;
    } else if (!urgent && near && fuel_[goal] < 86.f) {
        feed = true;
    }
}

int Game::lowest() const {
    int best = 0;
    for (int i = 1; i < kFlares; i++)
        if (fuel_[i] < fuel_[best] - 0.01f) best = i;
    return best;
}

void Game::spawnDue() {
    if (threat_.live) return;
    if (beat_ >= int(sizeof(kBeats) / sizeof(kBeats[0]))) return;
    if (t_ < kBeats[beat_].t) return;
    threat_.live = true;
    threat_.flare = kBeats[beat_].flare;
    threat_.kind = kBeats[beat_].kind ? Kind::Quench : Kind::Slag;
    threat_.eta = kWarn;
    beat_++;
    blip(threat_.kind == Kind::Slag ? 90.f : 320.f, 0.1f, 6);
}

void Game::resolve(Threat& th) {
    float dx = std::fabs(px_ - flareX(th.flare));
    bool near = dx <= kReach + 6.f;
    bool held = th.kind == Kind::Slag ? sys_->pad.down(gs::BTN_B) : (sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_X));
    if (bot_) {
        near = dx <= kReach + 10.f;
        held = near;
    }
    if (near && held) {
        if (th.kind == Kind::Slag) braced_++;
        else hauled_++;
        pop_[th.flare] = 1.f;
        burst(flareX(th.flare), kFeet - 30.f, 8, 28.f, th.kind == Kind::Slag ? PAL_SLAG : PAL_QUENCH);
        blip(240.f, 0.12f, 6);
    } else {
        fuel_[th.flare] -= th.kind == Kind::Slag ? 36.f : 44.f;
        hurt_[th.flare] = 0.45f;
        shake_ = 0.35f;
        burst(flareX(th.flare), kFeet - 18.f, 10, 32.f, PAL_ALERT);
        sys_->apu.noiseBurst(0.22f, 900.f, 0.12f);
    }
    th.live = false;
}

void Game::updateWatch(float dt) {
    t_ += dt;
    if (feedCd_ > 0.f) feedCd_ -= dt;
    float dir = 0;
    bool feed = false, brace = false, haul = false;
    if (bot_) think(dir, feed, brace, haul);
    else readPad(dir, feed, brace, haul);
    if (bot_) {
        sys_->pad.cur[gs::BTN_B] = brace;
        sys_->pad.cur[gs::BTN_C] = haul;
    }
    move_ = dir;
    if (dir < -0.1f) face_ = -1;
    if (dir > 0.1f) face_ = 1;
    px_ = std::clamp(px_ + dir * kSpeed * dt, 18.f, 302.f);

    spawnDue();
    if (threat_.live) {
        threat_.eta -= dt;
        if (threat_.eta <= 0.f) resolve(threat_);
    }

    int nearI = -1;
    float best = kReach;
    for (int i = 0; i < kFlares; i++) {
        float d = std::fabs(px_ - flareX(i));
        if (d <= best) {
            best = d;
            nearI = i;
        }
    }
    if (feed && nearI >= 0 && feedCd_ <= 0.f && fuel_[nearI] < 96.f) {
        fuel_[nearI] = std::min(100.f, fuel_[nearI] + 28.f);
        feedCd_ = 0.22f;
        fed_++;
        pop_[nearI] = 1.f;
        burst(flareX(nearI), kFeet - 28.f, 5, 18.f, PAL_FIRE);
        blip(196.f, 0.08f, 4);
    }

    for (int i = 0; i < kFlares; i++) {
        float drain = 4.15f;
        if (fuel_[i] > 88.f) drain += 0.6f;
        fuel_[i] -= drain * dt;
        if (pop_[i] > 0.f) pop_[i] = std::max(0.f, pop_[i] - dt * 2.4f);
        if (hurt_[i] > 0.f) hurt_[i] = std::max(0.f, hurt_[i] - dt);
        if ((sys_->frame + i * 7) % 11 == 0) {
            motes_.push_back({flareX(i) + std::sin(t_ * 3.f + i) * 3.f, kFeet - 36.f, 0.f, -14.f, 0.55f, PAL_SMOKE});
        }
        if (fuel_[i] <= 0.f) {
            beginLoss(i);
            return;
        }
    }
    if (t_ >= kNight) beginWin();
}

void Game::updateEnd() {
    hold_++;
    if (hold_ == 70) over_ = true;
}

void Game::stepMotes(float dt) {
    for (Mote& m : motes_) {
        m.x += m.vx * dt;
        m.y += m.vy * dt;
        m.vy += 18.f * dt;
        m.life -= dt;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.life <= 0.f; }), motes_.end());
    if (motes_.size() > 40) motes_.erase(motes_.begin(), motes_.begin() + int(motes_.size() - 40));
}

void Game::burst(float x, float y, int n, float speed, int pal) {
    for (int i = 0; i < n; i++) {
        float a = (i + 0.3f) / float(std::max(n, 1)) * 6.28318f;
        motes_.push_back({x, y, std::cos(a) * speed, std::sin(a) * speed - 8.f, 0.36f, pal});
    }
}

void Game::blip(float freq, float vol, int frames) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = frames;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(std::max(1.f, w)));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = 0;
    s.hflip = flip;
    s.shadow = false;
    s.clipY = gs::SCREEN_H;
    sys_->vdp.sprite(s);
}

void Game::sky() {
    float e = dawnEase();
    const uint16_t n0 = gs::rgb4(1, 1, 2);
    const uint16_t n1 = gs::rgb4(3, 2, 2);
    const uint16_t n2 = gs::rgb4(4, 2, 1);
    const uint16_t d0 = gs::rgb4(6, 5, 8);
    const uint16_t d1 = gs::rgb4(14, 8, 4);
    const uint16_t d2 = gs::rgb4(15, 11, 6);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t night, dawn;
        if (y < 80) {
            float u = y / 80.f;
            night = mix4(n0, n1, u);
            dawn = mix4(d0, d1, u);
        } else {
            float u = std::min(1.f, (y - 80) / 144.f);
            night = mix4(n1, n2, u);
            dawn = mix4(d1, d2, u);
        }
        uint16_t c = mix4(night, dawn, e);
        if (mode_ == Mode::Lost) c = mix4(c, gs::rgb4(6, 1, 0), hold_ < 16 ? 0.5f : 0.18f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(255, 150, 48);
    else if (mode_ == Mode::Lost) sys_->setLight(80, 8, 0);
    else if (mode_ == Mode::Title) sys_->setLight(40, 18, 8);
    else {
        float f = 0;
        for (int i = 0; i < kFlares; i++) f += fuel_[i];
        f = std::clamp(f / 400.f, 0.f, 1.f);
        float e = dawnEase();
        sys_->setLight(std::clamp(int(50 + 90 * f + 100 * e), 0, 255), std::clamp(int(16 + 30 * f + 50 * e), 0, 255),
                       std::clamp(int(8 + 10 * (1.f - e)), 0, 255));
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
    if (threat_.live && threat_.eta < 1.6f) {
        if (threat_.kind == Kind::Slag) return "SLAG - STAND AND BRACE";
        return "QUENCH - STAND AND HAUL";
    }
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] < 28.f) return "A FLARE IS DYING";
    return "A FEED   B BRACE   C HAUL";
}

void Game::drawWorld() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    for (int y = 3; y < 16; y++) {
        for (int x = 0; x < 40; x++) {
            int tile = (x % 9 == 0) ? art_.beam : art_.soot;
            v.A.set(x, y, gs::entry(tile, PAL_SOOT));
        }
    }
    for (int x = 0; x < 40; x++) {
        v.A.set(x, 16, gs::entry(art_.grate, PAL_IRON));
        v.A.set(x, 17, gs::entry(art_.dark, PAL_IRON));
    }
    for (int y = 22; y < 28; y++)
        for (int x = 0; x < 40; x++) v.A.set(x, y, gs::entry(y == 22 ? art_.grate : art_.plate, PAL_FLOOR));

    const uint64_t fr = sys_->frame;
    float jx = shake_ > 0.f ? std::sin(float(fr) * 1.8f) * shake_ * 7.f : 0.f;

    if (mode_ == Mode::Title) {
        word("S3 FOUNDRY DAWN", 160.f, 28.f, 12.f, PAL_GOLD);
        word("KEEP THE FLARES LIT", 160.f, 46.f, 9.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        word("DAWN", 160.f, 26.f, 18.f, PAL_GOLD);
    } else if (mode_ == Mode::Lost) {
        word("DARK", 160.f, 26.f, 18.f, PAL_ALERT);
    }

    int step = (std::fabs(move_) > 0.1f && (fr / 6) & 1) ? 1 : 0;
    spr(art_.man[step], px_ + jx, kFeet, 48.f, PAL_MAN, face_ < 0, true);
    if (feedCd_ > 0.12f) spr(art_.coal, px_ + face_ * 10.f, kFeet - 22.f, 8.f, PAL_COAL);

    for (int i = 0; i < kFlares; i++) {
        bool out = fuel_[i] <= 0.5f || (dead_ == i && mode_ == Mode::Lost);
        if (!out) {
            int flick = int(fr / 4 + i) & 1;
            float h = 14.f + (fuel_[i] / 100.f) * 10.f + pop_[i] * 8.f;
            int pal = hurt_[i] > 0.f ? PAL_ALERT : (fuel_[i] < 30.f ? PAL_EMBER : PAL_FIRE);
            spr(art_.flame[flick], kX[i] + jx, kFeet - 22.f, h, pal, false, true);
        }
    }

    if (threat_.live) {
        float u = 1.f - std::clamp(threat_.eta / kWarn, 0.f, 1.f);
        float x = flareX(threat_.flare) + jx;
        float y = 40.f + u * (kFeet - 70.f);
        if (threat_.kind == Kind::Slag) spr(art_.slag, x, y, 16.f + u * 8.f, PAL_SLAG);
        else spr(art_.quench, x, y, 18.f, PAL_QUENCH);
    }
    for (const Mote& m : motes_) spr(art_.spark, m.x, m.y, 3.f + m.life * 4.f, m.pal);

    for (int i = 0; i < kFlares; i++)
        spr(art_.crucible, kX[i] + jx, kFeet - 6.f, 22.f, PAL_IRON, false, true);

    if (mode_ == Mode::Watch || mode_ == Mode::Title)
        spr(art_.moon, 250.f, 22.f, 14.f, dawnEase() > 0.55f ? PAL_SUN : PAL_MOON);
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        hudC(24, "START  KEEP THEM LIT", PAL_HUD);
        hudC(26, "A FEED  B BRACE SLAG  C HAUL QUENCH", PAL_PIP);
        return;
    }
    if (mode_ == Mode::Won) {
        hudC(24, "THE FLARES HELD UNTIL DAWN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Lost) {
        hudC(24, "THE FOUNDRY GOES DARK", PAL_ALERT);
        return;
    }
    int left = int(std::ceil(std::max(0.f, kNight - t_)));
    char line[48];
    std::snprintf(line, sizeof(line), "LIT %d   DAWN %02d", lit(), left);
    hud(1, 1, line, PAL_HUD);
    for (int i = 0; i < kFlares; i++) {
        int bars = std::clamp(int(fuel_[i] / 12.5f), 0, 8);
        char pip[10];
        for (int b = 0; b < 8; b++) pip[b] = b < bars ? '#' : '.';
        pip[8] = 0;
        int col = 4 + i * 9;
        hud(col, 3, pip, fuel_[i] < 28.f ? PAL_ALERT : PAL_GOLD);
    }
    hudC(26, hint(), PAL_GOLD);
}

void Game::draw() {
    sky();
    lamp();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sys_->vdp.hudEnabled = true;
    sys_->vdp.A.enabled = true;
    sys_->vdp.B.enabled = false;
    drawWorld();
    drawHud();
}

}  // namespace foundrydawn
