#include "game/beds.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace beds {
namespace {

constexpr int N = 6;
constexpr float DT = 1.f / 60.f;
constexpr float SUN_SECS = 18.f;
constexpr float SPEED = 102.f;
constexpr float POUR = 0.92f;
constexpr float WALL_X = 256.f;
constexpr float LOX = 8.f, HIX = 246.f, LOY = 48.f, HIY = 216.f;

// Zones sit in the aisle against each bed. Feet pin on the bed edge and stay inside the zone.
struct Bed {
    float x, y, w, h, zx, zy, zw, zh;
    int crop;
};

constexpr Bed kBeds[N] = {
    {16, 58, 60, 44, 16, 106, 60, 18, 0},  {94, 58, 60, 44, 94, 106, 60, 18, 1},
    {172, 58, 60, 44, 172, 106, 60, 18, 2}, {16, 150, 60, 44, 16, 132, 60, 18, 3},
    {94, 150, 60, 44, 94, 132, 60, 18, 4},  {172, 150, 60, 44, 172, 132, 60, 18, 5},
};

constexpr int kRoute[N] = {0, 1, 2, 5, 4, 3};

struct Rgb {
    int r, g, b;
};

constexpr Rgb kBrickShade[9] = {
    {0, 0, 0}, {8, 7, 6}, {12, 4, 3}, {8, 2, 2}, {14, 7, 4}, {12, 11, 9}, {9, 8, 7}, {4, 7, 3}, {5, 3, 2},
};
constexpr Rgb kBrickHot[9] = {
    {0, 0, 0}, {12, 8, 6}, {15, 8, 3}, {12, 4, 2}, {15, 11, 5}, {15, 13, 8}, {13, 10, 7}, {8, 10, 3}, {10, 5, 2},
};

bool overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

gs::FMPatch stingPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.vol = 0.2f;
    p.echo = 0.35f;
    p.op[0] = {1, 0.9f, 0.004f, 0.18f, 0.35f, 0.22f};
    p.op[1] = {2, 0.35f, 0.006f, 0.2f, 0.15f, 0.24f};
    p.op[2] = {4, 0.18f, 0.008f, 0.16f, 0.0f, 0.2f};
    p.op[3] = {1, 0.12f, 0.01f, 0.3f, 0.0f, 0.28f};
    return p;
}

gs::FMPatch chordPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.vol = 0.16f;
    p.echo = 0.4f;
    p.op[0] = {1, 0.85f, 0.01f, 0.4f, 0.45f, 0.5f};
    p.op[1] = {2, 0.28f, 0.01f, 0.35f, 0.2f, 0.45f};
    p.op[2] = {3, 0.14f, 0.02f, 0.3f, 0.0f, 0.4f};
    p.op[3] = {1, 0.1f, 0.02f, 0.5f, 0.0f, 0.55f};
    return p;
}

float noteHz(int i) {
    const float n[6] = {392.f, 440.f, 494.f, 523.f, 587.f, 659.f};
    return n[std::clamp(i, 0, 5)];
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 2;
    if (mode_ == Mode::Lose) return 3;
    return 1;
}

int Game::wetCount() const {
    int n = 0;
    for (int i = 0; i < N; i++)
        if (wet_[i] >= 1.f) n++;
    return n;
}

bool Game::allWet() const { return wetCount() == N; }

int Game::nextDry() const {
    for (int k = 0; k < N; k++)
        if (wet_[kRoute[k]] < 1.f) return kRoute[k];
    return -1;
}

int Game::atBed() const {
    for (int i = 0; i < N; i++) {
        const Bed& b = kBeds[i];
        if (px_ >= b.zx && px_ < b.zx + b.zw && py_ >= b.zy && py_ < b.zy + b.zh) return i;
    }
    return -1;
}

float Game::sunU() const {
    if (mode_ == Mode::Title) return 0.16f + 0.025f * std::sin(t_ * 0.7f);
    float u = sunT_ / SUN_SECS;
    return std::clamp(u, 0.f, 1.f);
}

float Game::lightP() const { return std::pow(sunU(), 1.45f); }

bool Game::blocked(float x, float y) const {
    if (x < LOX || x > HIX || y < LOY || y > HIY) return true;
    const float fx = x - 5.f, fy = y - 4.f, fw = 10.f, fh = 4.f;
    for (int i = 0; i < N; i++) {
        const Bed& b = kBeds[i];
        if (overlap(fx, fy, fw, fh, b.x, b.y, b.w, b.h)) return true;
    }
    return false;
}

void Game::move(float ax, float ay, float dt) {
    bool moving = ax != 0.f || ay != 0.f;
    if (moving) {
        if (ax < -0.15f) face_ = -1;
        else if (ax > 0.15f) face_ = 1;
        walk_ += dt;
    } else {
        walk_ = 0;
    }
    float step = SPEED * dt;
    float nx = px_ + ax * step;
    float ny = py_ + ay * step;
    if (!blocked(nx, py_)) px_ = nx;
    if (!blocked(px_, ny)) py_ = ny;
}

void Game::startRound() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    sunT_ = 0;
    walk_ = 0;
    melody_ = 0;
    stuck_ = 0;
    pourHeard_ = false;
    beep_ = 0;
    tick_ = 0.5f;
    for (int i = 0; i < N; i++) wet_[i] = 0;
    px_ = 28.f;
    py_ = 128.f;
    lastX_ = px_;
    lastY_ = py_;
    face_ = 1;
    bits_.clear();
    if (sys_) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::note(int i) {
    if (!sys_) return;
    sys_->apu.keyOn(0, noteHz(i), 0.22f);
    sys_->rumble(0.15f, 0.35f, 50);
}

void Game::chirp() {
    if (!sys_) return;
    if (birdN_ >= 3) {
        sys_->apu.tone(2, 0, 0);
        birdN_ = 0;
        bird_ = 2.2f + float(rng_ & 255) / 180.f;
        return;
    }
    float f = birdN_ == 0 ? 1480.f : birdN_ == 1 ? 1760.f : 1320.f;
    sys_->apu.tone(2, f, 0.03f);
    birdN_++;
    bird_ = birdN_ >= 3 ? 0.12f : 0.08f;
}

void Game::win() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.noise(0, 2000, false);
    gs::FMPatch chord = chordPatch();
    const float hz[3] = {523.f, 659.f, 784.f};
    for (int i = 0; i < 3; i++) {
        sys_->apu.setPatch(1 + i, chord);
        sys_->apu.setPan(1 + i, (i - 1) * 0.45f);
        sys_->apu.keyOn(1 + i, hz[i], 0.2f);
    }
    sys_->rumble(0.35f, 0.55f, 180);
    sys_->setLight(40, 140, 70);
}

void Game::lose() {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    sunT_ = SUN_SECS;
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.noise(0, 2000, false);
    sys_->apu.keyOn(4, 110.f, 0.28f);
    sys_->rumble(0.7f, 0.15f, 220);
    sys_->setLight(180, 40, 20);
}

void Game::put(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
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

void Game::sky(float p) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 48) {
            float k = y / 47.f;
            float r0 = 2.f + p * 6.f, g0 = 4.f + p * 2.f, b0 = 12.f - p * 3.f;
            float r1 = 14.f, g1 = 10.f - p * 5.f, b1 = 8.f - p * 4.f;
            int r = int(std::lround(r0 + (r1 - r0) * k));
            int g = int(std::lround(g0 + (g1 - g0) * k));
            int b = int(std::lround(b0 + (b1 - b0) * k));
            auto c = [](int q) { return std::clamp(q, 0, 15); };
            v.lineBackdrop[y] = gs::rgb4(c(r), c(g), c(b));
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 4, 2);
        }
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::tintBrick(float p) {
    float k = 0;
    if (mode_ == Mode::Lose) k = 1.f;
    else if (p > 0.58f) k = std::clamp((p - 0.58f) / 0.42f, 0.f, 1.f);
    for (int i = 1; i <= 8; i++) {
        float r = kBrickShade[i].r + (kBrickHot[i].r - kBrickShade[i].r) * k;
        float g = kBrickShade[i].g + (kBrickHot[i].g - kBrickShade[i].g) * k;
        float b = kBrickShade[i].b + (kBrickHot[i].b - kBrickShade[i].b) * k;
        sys_->vdp.setColor(PAL_BRICK * 16 + i, gs::rgb4(int(std::lround(r)), int(std::lround(g)), int(std::lround(b))));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float p = lightP();
    sky(p);
    tintBrick(p);

    float edge = std::min(WALL_X, p * WALL_X);
    if (mode_ == Mode::Lose) edge = WALL_X + 6.f;

    auto putC = [&](const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false) {
        put(m, cx - w * 0.5f, cy - h * 0.5f, w, h, pal, flip, false);
    };

    if (mode_ == Mode::Title) {
        putC(art_.logo, 118, 22, float(art_.logo.w), float(art_.logo.h), PAL_GOLD);
        putC(art_.sub, 118, 44, float(art_.sub.w), float(art_.sub.h), PAL_HUD);
    } else if (mode_ == Mode::Win) {
        putC(art_.watered, 118, 126, float(art_.watered.w), float(art_.watered.h), PAL_GOOD);
    } else if (mode_ == Mode::Lose) {
        putC(art_.late, 118, 126, float(art_.late.w), float(art_.late.h), PAL_WARN);
    }

    for (const Bit& d : bits_) {
        if (d.kind == 0) putC(art_.drop, d.x, d.y, 6, 8, PAL_WATER);
        else putC(art_.spark, d.x, d.y, 7, 7, PAL_GOLD);
    }

    int here = (mode_ == Mode::Play) ? atBed() : -1;
    if (here >= 0 && wet_[here] < 1.f) {
        const Bed& b = kBeds[here];
        float bob = std::sin(t_ * 7.f) * 2.5f;
        putC(art_.drop, b.x + b.w * 0.5f, b.y - 4 + bob, 7, 9, PAL_WATER);
    }

    struct Item {
        float key;
        int kind;
        int i;
    };
    Item items[7];
    int nitem = 0;
    float manY = py_;
    if (mode_ == Mode::Title) manY = 128.f + std::sin(t_ * 2.1f) * 1.4f;
    items[nitem++] = {manY, 0, 0};
    for (int i = 0; i < N; i++) items[nitem++] = {kBeds[i].y + kBeds[i].h, 1, i};
    std::sort(items, items + nitem, [](const Item& a, const Item& b) { return a.key > b.key; });

    bool moving = walk_ > 0.05f && mode_ == Mode::Play;
    int pose = 0;
    if (mode_ == Mode::Play && here >= 0 && wet_[here] < 1.f && pourHeard_) pose = here < 3 ? 2 : 3;
    else if (moving) pose = (int(walk_ * 8.f) & 1) ? 1 : 0;
    bool flip = face_ < 0;

    for (int n = 0; n < nitem; n++) {
        if (items[n].kind == 0) {
            float x = (mode_ == Mode::Title) ? 30.f : px_;
            float y = manY;
            put(art_.shadow, x - 10, y - 3, 20, 7, PAL_MAN, false, true);
            put(art_.man[pose], x - 13, y - 26, 26, 26, PAL_MAN, flip, false);
        } else {
            int i = items[n].i;
            const Bed& b = kBeds[i];
            float m = wet_[i];
            int stage = 0;
            if (mode_ == Mode::Lose && m < 1.f) stage = 3;
            else if (m >= 1.f) stage = 2;
            else if (m > 0.42f) stage = 1;
            float ph = stage == 2 ? 32.f : stage == 1 ? 24.f : stage == 3 ? 18.f : 16.f;
            float pw = ph * (44.f / 34.f);
            float lift = (b.crop == 1 || b.crop == 4) ? 8.f : 2.f;
            if (stage == 0) lift = 0;
            put(art_.barBg, b.x + 10, b.y + b.h - 8, 40, 4, PAL_WATER);
            if (m > 0.02f) put(art_.barFg, b.x + 10, b.y + b.h - 8, std::max(2.f, 40.f * m), 4, PAL_WATER);
            put(art_.plant[b.crop][stage], b.x + (b.w - pw) * 0.5f, b.y - lift + 4.f, pw, ph, PAL_PLANT);
            int soilPal = m >= 0.5f ? PAL_WET : PAL_DRY;
            put(art_.soil, b.x + 7, b.y + 7, 46, 20, soilPal);
            put(art_.frame, b.x, b.y, b.w, b.h, PAL_WOOD);
        }
    }

    put(art_.vine, 286, 78, 16, 64, PAL_PLANT);
    put(art_.vine, 300, 150, 14, 52, PAL_PLANT);
    put(art_.barrel, 262, 176, 22, 26, PAL_WOOD);

    float bx = std::fmod(t_ * 22.f, 380.f) - 30.f;
    int flap = int(t_ * 8.f) & 1;
    put(art_.bird[flap], bx, 16 + std::sin(t_ * 1.7f) * 3.f, 14, 8, PAL_SUN);

    float sx = 26.f + p * 214.f;
    float sy = 30.f - std::sin(std::clamp(p, 0.f, 1.f) * 3.14159f) * 12.f;
    int sunFrame = int(t_ * 3.f) & 3;
    putC(art_.sun[sunFrame], sx, sy, 28, 28, PAL_SUN);
    float c0 = std::fmod(t_ * 6.f, 400.f) - 50.f;
    putC(art_.cloud, c0, 14, 40, 16, PAL_SUN);
    putC(art_.cloud, std::fmod(c0 + 180.f, 400.f) - 50.f, 26, 28, 12, PAL_SUN);

    if (edge > 2.f) put(art_.light, edge - 320.f, 48.f, 320, 176, PAL_LIGHT);

    int wet = wetCount();
    if (mode_ == Mode::Title) {
        hudC(25, "WATER THEM BEFORE THE SUN HITS THE WALL", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "ENTER", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(25, "THE SUN HAS NOT HIT THE WALL", PAL_GOOD);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) hudC(26, "ENTER", PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        hudC(25, "THE SUN HIT THE WALL", PAL_WARN);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) hudC(26, "ENTER", PAL_HUD);
    } else {
        char buf[16];
        std::snprintf(buf, sizeof buf, "%d/6", wet);
        hud(1, 0, buf, wet == 6 ? PAL_GOOD : PAL_HUD);
        int cells = 14;
        int filled = std::clamp(int(std::lround(p * cells)), 0, cells);
        std::string bar = "SUN ";
        for (int i = 0; i < cells; i++) bar.push_back(i < filled ? '|' : '.');
        hud(22, 0, bar, p > 0.72f ? PAL_WARN : PAL_GOLD);
        if (here >= 0 && wet_[here] < 1.f) hudC(26, "HOLD Z TO POUR", PAL_GOOD);
        else if (here >= 0) hudC(26, "THIS BED IS WET", PAL_GOLD);
        else if (p > 0.72f) hudC(26, "THE SUN IS CLOSE", (int(t_ * 4.f) & 1) ? PAL_WARN : PAL_GOLD);
        else hudC(26, "WATER THE SIX BEDS", PAL_HUD);
    }

    int r = int(30 + p * 200);
    int g = int(90 - p * 50);
    int bcol = int(150 - p * 120);
    if (mode_ != Mode::Win && mode_ != Mode::Lose)
        sys_->setLight(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(bcol, 0, 255));
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.18f, 0.28f, 0.22f);
    sys.apu.setPatch(0, stingPatch());
    sys.apu.setPatch(4, stingPatch());
    sys.setLight(40, 90, 150);
    t_ = 0;
    if (bot_) startRound();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    rng_ = rng_ * 1664525u + 1013904223u;

    float ax = 0, ay = 0;
    bool pour = false;
    bool start = false;
    bool back = false;

    if (bot_) {
        if (mode_ == Mode::Play) {
            int i = nextDry();
            if (i >= 0) {
                const Bed& b = kBeds[i];
                float tx = b.zx + b.zw * 0.5f;
                float ty = b.zy + b.zh * 0.5f;
                float dx = tx - px_;
                float dy = ty - py_;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist > 3.f) {
                    ax = dx / dist;
                    ay = dy / dist;
                    float moved = std::fabs(px_ - lastX_) + std::fabs(py_ - lastY_);
                    if (moved < 0.15f) stuck_ += DT;
                    else stuck_ = 0;
                    if (stuck_ > 0.45f) {
                        px_ += ax * 8.f;
                        py_ += ay * 8.f;
                        stuck_ = 0;
                    }
                } else {
                    px_ = tx;
                    py_ = ty;
                    pour = true;
                    stuck_ = 0;
                    if (tx < px_) face_ = -1;
                }
            }
            lastX_ = px_;
            lastY_ = py_;
        }
    } else {
        const gs::Pad& pad = sys.pad;
        back = pad.pressed(gs::BTN_MODE);
        start = pad.pressed(gs::BTN_START);
        ax = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        ay = float(pad.down(gs::BTN_DOWN)) - float(pad.down(gs::BTN_UP));
        if (std::fabs(pad.axisX) > 0.25f && ax == 0.f) ax = pad.axisX;
        float mag = std::sqrt(ax * ax + ay * ay);
        if (mag > 1.f) {
            ax /= mag;
            ay /= mag;
        }
        pour = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
    }

    if (back) {
        if (mode_ == Mode::Title) sys.quit();
        else {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            sunT_ = 0;
            bits_.clear();
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.noise(0, 2000, false);
        }
    } else if (mode_ == Mode::Title) {
        if (start) startRound();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (start && !bot_) startRound();
    } else if (mode_ == Mode::Play) {
        move(ax, ay, DT);
        int at = atBed();
        bool watering = pour && at >= 0 && wet_[at] < 1.f;
        if (watering) {
            float before = wet_[at];
            wet_[at] = std::min(1.f, wet_[at] + POUR * DT);
            if (before < 1.f && wet_[at] >= 1.f) {
                note(melody_++);
                const Bed& b = kBeds[at];
                for (int k = 0; k < 8; k++) {
                    float a = (k / 8.f) * 6.28318f;
                    bits_.push_back({b.x + b.w * 0.5f, b.y + 16.f, std::cos(a) * 28.f, std::sin(a) * 18.f - 16.f, 0.45f, 1});
                }
            }
            if ((rng_ & 3) == 0 && bits_.size() < 40) {
                float sx = px_ + (face_ < 0 ? -10.f : 10.f);
                float sy = py_ - (at < 3 ? 20.f : 12.f);
                float tx = kBeds[at].x + kBeds[at].w * 0.5f;
                float ty = kBeds[at].y + 18.f;
                bits_.push_back({sx, sy, (tx - sx) / 0.36f, (ty - sy) / 0.36f, 0.4f, 0});
            }
            float m = wet_[at];
            sys.apu.tone(0, 210.f + m * 220.f, 0.028f);
            sys.apu.noise(0.04f, 1600.f + m * 2200.f, false);
            pourHeard_ = true;
            if (face_ == 0) face_ = 1;
            if (kBeds[at].x + kBeds[at].w * 0.5f < px_ - 4.f) face_ = -1;
            else if (kBeds[at].x + kBeds[at].w * 0.5f > px_ + 4.f) face_ = 1;
        } else {
            if (pourHeard_) {
                sys.apu.tone(0, 0, 0);
                sys.apu.noise(0, 2000, false);
            }
            pourHeard_ = false;
            if (ax != 0.f || ay != 0.f) {
                int stepNow = int(walk_ * 8.f);
                int stepPrev = int((walk_ - DT) * 8.f);
                if (stepNow != stepPrev) sys.apu.noiseBurst(0.04f, 700.f, 0.05f);
            }
        }

        if (allWet()) win();
        else {
            sunT_ += DT;
            if (sunT_ >= SUN_SECS) lose();
        }
    }

    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(1, 0, 0);
    }
    float p = lightP();
    if (mode_ == Mode::Play && p > 0.66f) {
        tick_ -= DT;
        if (tick_ <= 0.f) {
            sys.apu.tone(1, 820.f + p * 200.f, 0.04f);
            beep_ = 0.05f;
            tick_ = 0.34f - p * 0.2f;
        }
    }
    bird_ -= DT;
    if (bird_ <= 0.f && mode_ != Mode::Lose) chirp();

    for (int i = int(bits_.size()) - 1; i >= 0; i--) {
        Bit& d = bits_[size_t(i)];
        d.life -= DT;
        if (d.kind == 0) d.vy += 70.f * DT;
        d.x += d.vx * DT;
        d.y += d.vy * DT;
        if (d.life <= 0.f) bits_.erase(bits_.begin() + i);
    }

    draw();
}

}  // namespace beds
