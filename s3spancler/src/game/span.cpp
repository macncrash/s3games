#include "game/span.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace spancler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 74.f;
constexpr float kZNear = 2.2f;
constexpr float kPpm = 96.f;
constexpr float kDeckHalf = 2.05f;
constexpr float kWalkerH = 1.7f;
constexpr float kSpeed = 6.4f;
constexpr float kReach = 0.72f;
constexpr int kClock = 50 * 60;
constexpr float kZMin = 2.3f, kZMax = 13.2f;
constexpr float kLatMin = -1.35f, kLatMax = 1.35f;
constexpr float kStartZ = 2.55f, kStartLat = 0.f;

struct Lay {
    float z, lat;
    int kind;
};

struct Prop {
    float z, lat, h;
    int kind;
};

constexpr Prop kProps[] = {
    {13.6f, -2.15f, 6.4f, 0}, {13.6f, 2.15f, 6.4f, 0}, {8.2f, -2.2f, 5.2f, 0}, {8.2f, 2.2f, 5.2f, 0},
    {4.4f, -2.15f, 4.4f, 0},  {4.4f, 2.15f, 4.4f, 0},  {13.6f, -2.15f, 1.1f, 1}, {13.6f, 2.15f, 1.1f, 1},
    {8.2f, -2.2f, 0.95f, 1},  {8.2f, 2.2f, 0.95f, 1},  {11.4f, 0.f, 0.55f, 2},    {6.1f, 0.f, 0.5f, 2},
    {12.4f, -3.4f, 0.4f, 3},  {9.6f, 3.5f, 0.35f, 3},  {5.2f, -3.6f, 0.32f, 3},
};

float dist(float z0, float lat0, float z1, float lat1) {
    float dz = z1 - z0, dl = lat1 - lat0;
    return std::sqrt(dz * dz + dl * dl);
}

float needFor(int kind) {
    if (kind == 2) return 0.46f;
    if (kind == 1) return 0.36f;
    return 0.28f;
}

const char* kindName(int kind) {
    if (kind == 1) return "CRATE";
    if (kind == 2) return "COIL";
    return "BARREL";
}

const gs::Mipped& pileArt(const Art& a, int kind) {
    if (kind == 1) return a.crate;
    if (kind == 2) return a.coil;
    return a.barrel;
}

int pilePal(int kind) { return kind == 2 ? PAL_STEEL : PAL_WOOD; }

float pileH(int kind) {
    if (kind == 1) return 0.9f;
    if (kind == 2) return 0.55f;
    return 1.0f;
}

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [](int c0, int c1, float u) { return int(std::lround(c0 + (c1 - c0) * u)); };
    return gs::rgb4(ch(ar, br, t), ch(ag, bg, t), ch(ab, bb, t));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (cleared_ >= 3) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

float Game::sagAt(float row) const {
    float sag = std::sin(row * 0.012f) * 14.f;
    return sag + std::sin(t_ * 0.7f + row * 0.02f) * 1.4f;
}

int Game::fogFor(float z) const { return int(std::clamp((z - 5.f) / 14.f, 0.f, 1.f) * 10.f); }

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > 0.5f)) return p;
    float span = float(gs::SCREEN_H) - kHorizon;
    float t = kZNear / z;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * span;
    p.x = 160.f + sagAt(p.y - kHorizon) + lat * p.ppm;
    p.ok = true;
    return p;
}

void Game::resetField() {
    static constexpr Lay kLay[kPiles] = {
        {3.7f, 0.42f, 0}, {5.35f, -0.55f, 1}, {7.15f, 0.38f, 2}, {9.05f, -0.32f, 0}, {11.1f, 0.22f, 1},
    };
    for (int i = 0; i < kPiles; i++) {
        pile_[i].z = kLay[i].z;
        pile_[i].lat = kLay[i].lat;
        pile_[i].kind = kLay[i].kind;
        pile_[i].work = 0.f;
        pile_[i].gone = false;
    }
    for (Splash& s : splash_) s.life = 0.f;
    moving_ = false;
    sweeping_ = false;
    cleared_ = 0;
    focus_ = -1;
    lat_ = kStartLat;
    z_ = kStartZ;
    face_ = 1.f;
    step_ = 0.f;
    shake_ = 0.f;
    clock_ = kClock;
    rng_ = 7;
}

void Game::begin() {
    resetField();
    won_ = false;
    over_ = false;
    reason_ = "";
    fanStep_ = -1;
    mode_ = Mode::Play;
    blip(440.f, 0.12f);
    if (sys_) sys_->setLight(40, 90, 140);
}

void Game::toTitle() {
    if (sys_) {
        sys_->apu.silence();
        sys_->apu.noise(0.f, 400.f);
    }
    fanStep_ = -1;
    blip_ = 0.f;
    tick_ = 0.f;
    won_ = false;
    over_ = false;
    reason_ = "";
    resetField();
    mode_ = Mode::Title;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    if (bot_) begin();
    else toTitle();
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_TURBO);
}

bool Game::sweepDown() const {
    const gs::Pad& p = sys_->pad;
    return p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
           p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO) || p.accel > 0.4f;
}

int Game::focusPile() const {
    int best = -1;
    float bd = kReach + 0.001f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float d = dist(z_, lat_, pile_[i].z, pile_[i].lat);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, float& iz, bool& hold) {
    ix = iz = 0.f;
    hold = false;
    int id = 0;
    while (id < kPiles && pile_[id].gone) id++;
    if (id >= kPiles) return;
    float dz = pile_[id].z - z_, dl = pile_[id].lat - lat_;
    float d = std::sqrt(dz * dz + dl * dl);
    if (d <= kReach) {
        hold = true;
        if (d < 0.12f) {
            z_ = pile_[id].z;
            lat_ = pile_[id].lat;
        }
        return;
    }
    iz = dz / d;
    ix = dl / d;
}

void Game::humanInput(float& ix, float& iz, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = iz = 0.f;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (p.down(gs::BTN_UP)) iz += 1.f;
    if (p.down(gs::BTN_DOWN)) iz -= 1.f;
    if (std::fabs(p.axisX) > 0.2f || std::fabs(p.axisY) > 0.2f) {
        ix = p.axisX;
        iz = p.axisY;
    }
    hold = sweepDown();
}

void Game::move(float ix, float iz) {
    float mag = std::sqrt(ix * ix + iz * iz);
    if (mag < 0.05f) {
        moving_ = false;
        return;
    }
    if (mag > 1.f) {
        ix /= mag;
        iz /= mag;
    }
    lat_ = std::clamp(lat_ + ix * kSpeed * kDt, kLatMin, kLatMax);
    z_ = std::clamp(z_ + iz * kSpeed * kDt, kZMin, kZMax);
    if (ix < -0.15f) face_ = -1.f;
    else if (ix > 0.15f) face_ = 1.f;
    moving_ = true;
}

void Game::splashAt(float x, float y) {
    for (Splash& s : splash_) {
        if (s.life > 0.f) continue;
        s.x = x;
        s.y = y;
        s.life = 0.35f;
        return;
    }
}

void Game::sweep(bool hold) {
    int id = focusPile();
    focus_ = id;
    if (id < 0 || !hold) return;
    sweeping_ = true;
    Pile& m = pile_[id];
    face_ = (m.lat < lat_) ? -1.f : 1.f;
    m.work += kDt / needFor(m.kind);
    if ((sys_->frame & 4) == 0) {
        Proj p = project(m.lat, m.z);
        if (p.ok) splashAt(p.x + face_ * 18.f, p.y - 6.f);
    }
    if (m.work < 1.f) return;
    m.gone = true;
    sweeping_ = false;
    cleared_++;
    shake_ = 0.4f;
    sys_->apu.noiseBurst(0.22f, 240.f, 0.12f);
    blip(520.f + float(cleared_) * 40.f, 0.08f);
    sys_->rumble(0.15f, 0.3f, 40);
    if (cleared_ >= kPiles) win();
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "THE GROUND IS CLEAR";
    sweeping_ = false;
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noise(0.f, 200.f);
    sys_->setLight(30, 160, 90);
}

void Game::lose() {
    if (won_ || mode_ != Mode::Play) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Lost;
    reason_ = "THE CLOCK DIED";
    sweeping_ = false;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noise(0.f, 200.f);
    sys_->apu.noiseBurst(0.28f, 860.f, 0.14f);
    sys_->setLight(160, 30, 24);
}

void Game::updatePlay() {
    sweeping_ = false;
    float ix = 0.f, iz = 0.f;
    bool hold = false;
    if (bot_) botInput(ix, iz, hold);
    else humanInput(ix, iz, hold);
    move(ix, iz);
    sweep(hold);
    if (moving_ || sweeping_) {
        step_ += kDt * (sweeping_ ? 4.f : 2.6f);
        if (step_ >= 1.f) step_ -= 1.f;
    }
    if (mode_ != Mode::Play) return;
    int before = (clock_ + 59) / 60;
    if (clock_ > 0) clock_--;
    int after = (clock_ + 59) / 60;
    if (after != before && after > 0 && after <= 10) {
        sys_->apu.tone(1, 720.f, 0.04f);
        tick_ = 0.05f;
    }
    if (clock_ <= 0) lose();
}

void Game::blip(float freq, float hold) {
    if (!sys_ || fanStep_ >= 0) return;
    sys_->apu.tone(0, freq, 0.06f);
    blip_ = hold;
}

void Game::serviceAudio() {
    if (tick_ > 0.f) {
        tick_ -= kDt;
        if (tick_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ < 0.14f) return;
        fanT_ = 0.f;
        static const float good[] = {392.f, 523.f, 659.f, 784.f};
        static const float bad[] = {196.f, 146.f, 110.f};
        const float* notes = fanGood_ ? good : bad;
        int n = fanGood_ ? 4 : 3;
        if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
        else sys_->apu.tone(0, 0.f, 0.f);
        if (++fanStep_ > n + 2) fanStep_ = -1;
        return;
    }
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (sweeping_) sys_->apu.noise(0.05f, 900.f, false);
    else sys_->apu.noise(0.f, 500.f);
}

void Game::fadeFx() {
    for (Splash& s : splash_)
        if (s.life > 0.f) s.life -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 2.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    fadeFx();
    if (mode_ == Mode::Title) {
        if (startPressed()) begin();
        else if (sys.pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        serviceAudio();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (startPressed()) mode_ = Mode::Play;
        else if (sys.pad.pressed(gs::BTN_MODE)) toTitle();
        serviceAudio();
        draw();
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (startPressed()) begin();
        else if (sys.pad.pressed(gs::BTN_MODE)) toTitle();
        serviceAudio();
        draw();
        return;
    }
    if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        sweeping_ = false;
        mode_ = Mode::Pause;
        serviceAudio();
        draw();
        return;
    }
    if (!bot_ && sys.pad.pressed(gs::BTN_MODE) && sys.hasHome()) {
        sys.eject();
        return;
    }
    updatePlay();
    serviceAudio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -500, 500));
    s.y = int16_t(std::clamp(int(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -500, 500));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float cx, float y, float scale, int pal) {
    if (!s) return;
    float w = 0.f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) w += 14.f * scale;
        else w += float(art_.glyph[c - 32].w) * scale;
    }
    float x = cx - w * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) {
            x += 14.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        x += gw;
    }
}

void Game::tileText(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::drawDeck(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(3, 6, 12);
    uint16_t skyHor = gs::rgb4(10, 13, 15);
    if (mode_ == Mode::Lost) skyHor = gs::rgb4(10, 4, 4);
    else if (mode_ == Mode::Won) skyHor = gs::rgb4(12, 14, 10);
    else if (mode_ == Mode::Play && clock_ <= 10 * 60) skyHor = mixC(skyHor, gs::rgb4(12, 5, 4), 0.4f);
    v.setFogColor(skyHor);
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            rd.on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = mixC(skyTop, skyHor, u * u);
            v.lineFog[y] = 0;
            continue;
        }
        float t = std::max((float(y) + 0.5f - kHorizon) / span, 0.02f);
        float wz = kZNear / t;
        float row = float(y) - kHorizon;
        rd.on = true;
        rd.cx = 160.f + sagAt(row) + shx;
        rd.hw = std::max(10.f, kDeckHalf * kPpm * t);
        rd.v = wz * 28.f + t_ * 8.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.55f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
        v.lineFog[y] = uint8_t(std::clamp((wz - 7.f) / 16.f, 0.f, 1.f) * 9.f);
        v.lineBackdrop[y] = mixC(gs::rgb4(4, 8, 12), gs::rgb4(1, 3, 7), std::clamp(row / span, 0.f, 1.f));
    }
    v.roadTime = int(t_ * 18.f);
}

void Game::drawSky(float shx) {
    int sunPal = mode_ == Mode::Lost ? PAL_ALERT : (mode_ == Mode::Won ? PAL_GOOD : PAL_SKY);
    spr(art_.sun, 52.f + shx, 28.f, 18.f, sunPal, false, 1, false, false);
    spr(art_.cloud, 140.f + std::sin(t_ * 0.12f) * 10.f + shx, 22.f, 14.f, PAL_SKY, false, 2, false, false);
    spr(art_.cloud, 230.f + std::sin(t_ * 0.09f) * 8.f + shx, 38.f, 11.f, PAL_SKY, true, 3, false, false);
    float gx = 80.f + std::fmod(t_ * 18.f, 280.f);
    spr(art_.gull[int(t_ * 5.f) & 1], gx + shx, 46.f + std::sin(t_ * 1.4f) * 3.f, 9.f, PAL_FX, false, 1, false, false);
}

void Game::drawWorld(float shx) {
    struct Item {
        float z;
        int kind;
        int id;
    };
    Item items[32];
    int n = 0;
    items[n++] = {z_, 0, 0};
    for (int i = 0; i < kPiles; i++) items[n++] = {pile_[i].z, 1, i};
    for (int i = 0; i < int(sizeof kProps / sizeof kProps[0]); i++) items[n++] = {kProps[i].z, 2, i};
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Splash& s : splash_) {
        if (s.life <= 0.f) continue;
        spr(art_.splash, s.x + shx, s.y, 6.f + s.life * 18.f, PAL_WATER, false, 0, false, false);
    }

    float pulse = 1.f + 0.05f * std::sin(t_ * 9.f);
    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 0) {
            Proj p = project(lat_, z_);
            if (!p.ok) continue;
            int fr = (moving_ || sweeping_) ? (step_ < 0.5f ? 0 : 1) : 0;
            float h = std::clamp(kWalkerH * p.ppm, 8.f, 150.f);
            float feet = p.y;
            int fog = fogFor(z_);
            bool flip = face_ < 0.f;
            spr(art_.hand[fr], p.x + shx, feet, h, PAL_FIGURE, flip, fog, true, false);
            if (sweeping_)
                spr(art_.broom, p.x + face_ * h * 0.45f + shx, feet - h * 0.28f, std::clamp(h * 0.55f, 8.f, 80.f),
                    PAL_WOOD, flip, fog, false, false);
            spr(art_.shadow, p.x + shx, p.y + 2.f, std::max(4.f, h * 0.14f), PAL_FX, false, 0, false, true);
            continue;
        }
        if (it.kind == 1) {
            const Pile& m = pile_[it.id];
            Proj p = project(m.lat, m.z);
            if (!p.ok) continue;
            int fog = fogFor(m.z);
            if (m.gone) {
                spr(art_.patch, p.x + shx, p.y, std::clamp(0.4f * p.ppm, 4.f, 28.f), PAL_DECK, false, fog, false,
                    false);
                continue;
            }
            float h = pileH(m.kind) * (1.f - m.work * 0.6f) * p.ppm;
            if (it.id == focus_ && mode_ == Mode::Play) h *= pulse;
            spr(pileArt(art_, m.kind), p.x + shx, p.y, std::clamp(h, 5.f, 90.f), pilePal(m.kind), false, fog, true,
                false);
            spr(art_.shadow, p.x + shx, p.y + 1.f, std::max(3.f, h * 0.16f), PAL_FX, false, 0, false, true);
            continue;
        }
        const Prop& pr = kProps[it.id];
        Proj p = project(pr.lat, pr.z);
        if (!p.ok) continue;
        float h = std::clamp(pr.h * p.ppm, 3.f, 160.f);
        int fog = fogFor(pr.z);
        if (pr.kind == 0) spr(art_.tower, p.x + shx, p.y, h, PAL_STEEL, pr.lat > 0, fog, true, false);
        else if (pr.kind == 1)
            spr(art_.lamp, p.x + shx, p.y - h, std::clamp(h * 0.35f, 6.f, 28.f),
                mode_ == Mode::Won ? PAL_GOOD : PAL_GOLD, false, fog, false, false);
        else if (pr.kind == 2)
            spr(art_.cable, p.x + shx, p.y - 18.f, h * 8.f, PAL_STEEL, false, fog, false, false);
        else
            spr(art_.splash, p.x + std::sin(t_ * 2.f + pr.z) * 6.f + shx, p.y, h * 6.f, PAL_WATER, false, fog + 2,
                false, false);
    }
}

void Game::messages(float shx) {
    float pipX = 160.f - 2.f * 16.f;
    for (int i = 0; i < kPiles; i++) {
        int pal = i < cleared_ ? (mode_ == Mode::Lost ? PAL_ALERT : PAL_GOOD) : PAL_STEEL;
        spr(art_.pip, pipX + float(i) * 16.f + shx, 42.f, i < cleared_ ? 9.f : 7.f, pal, false, 0, false, false);
    }
    if (mode_ == Mode::Title) {
        text("S3 SPAN CLER", 160.f + shx, 14.f, 0.55f, PAL_GOLD);
        text("CLEAR THE GROUND", 160.f + shx, 32.f, 0.36f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160.f + shx, 14.f, 0.42f, PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160.f + shx, 14.f, 0.5f, PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 100.f, 0.8f, PAL_GOLD);
    int sec = std::max(0, (clock_ + 59) / 60);
    char clock[12];
    std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
    int pal = (mode_ == Mode::Play && sec <= 10) ? PAL_ALERT : PAL_GOLD;
    text(clock, 160.f + shx, 14.f, 0.9f, pal);
}

void Game::hud() {
    if (mode_ == Mode::Title) {
        if ((sys_->frame / 30) & 1) tileText(14, 23, "PRESS ENTER", PAL_GOLD);
        tileText(3, 25, "CLEAR THE GROUND BEFORE THE CLOCK", PAL_TEXT);
        tileText(5, 26, "MISS THAT AND THE WATCH IS OVER", PAL_TEXT);
        tileText(7, 27, "ARROWS MOVE  HOLD C TO SWEEP", PAL_GOLD);
        sys_->setLight(30, 60, 120);
        return;
    }
    char line[40];
    std::snprintf(line, sizeof line, "LEFT %d", std::max(0, kPiles - cleared_));
    int pal = mode_ == Mode::Won ? PAL_GOOD : (mode_ == Mode::Lost ? PAL_ALERT : PAL_TEXT);
    tileText(32, 0, line, pal);
    if (mode_ == Mode::Won) {
        tileText(12, 24, "THE WATCH HOLDS", PAL_GOOD);
        tileText(13, 26, "ENTER RETRIES", PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Lost) {
        tileText(11, 24, "THE WATCH IS OVER", PAL_ALERT);
        tileText(13, 26, "ENTER RETRIES", PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Pause) {
        tileText(13, 25, "ENTER RESUMES", PAL_TEXT);
        return;
    }
    const char* hint = "CLEAR THE SPAN";
    if (focus_ >= 0) {
        std::snprintf(line, sizeof line, "HOLD C  %s", kindName(pile_[focus_].kind));
        hint = line;
    }
    int col = 20 - int(std::strlen(hint)) / 2;
    tileText(col, 26, hint, clock_ <= 10 * 60 ? PAL_ALERT : PAL_GOLD);
    if (clock_ <= 10 * 60) sys_->setLight(170, 40, 28);
    else if (sweeping_) sys_->setLight(40, 140, 160);
    else sys_->setLight(36, 80, 130);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = shake_ > 0.f ? std::sin(t_ * 40.f) * 2.4f * std::min(shake_, 1.f) : 0.f;
    drawDeck(shx);
    drawSky(shx);
    drawWorld(shx);
    messages(shx);
    hud();
}

}  // namespace spancler
