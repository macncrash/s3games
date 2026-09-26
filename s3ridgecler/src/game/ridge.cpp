#include "game/ridge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 86.f;
constexpr float kZNear = 2.15f;
constexpr float kPpm = 88.f;
constexpr float kRoadHalf = 2.1f;
constexpr float kWalkerH = 1.62f;
constexpr float kSpeed = 4.6f;
constexpr float kReach = 0.75f;
constexpr float kTip = 0.22f;
constexpr int kClock = 60 * 60;
constexpr float kZMin = 2.35f, kZMax = 12.4f;
constexpr float kLatMin = -1.45f, kLatMax = 1.45f;
constexpr float kLipZ = 2.62f, kLipLat = -0.82f;
constexpr float kStartZ = 3.2f, kStartLat = 0.15f;

struct Lay {
    float z, lat;
    int kind;
};

struct Prop {
    float z, lat, h;
    int kind;
};

constexpr Prop kProps[] = {
    {13.6f, -4.5f, 4.6f, 0}, {12.6f, 4.7f, 5.1f, 0}, {11.0f, -4.8f, 3.8f, 0}, {10.2f, 4.3f, 4.3f, 0},
    {8.5f, -4.4f, 3.3f, 0},  {7.2f, 4.6f, 3.6f, 0},  {5.5f, -3.9f, 2.8f, 0},  {4.55f, 4.0f, 3.0f, 0},
    {12.2f, -3.4f, 2.1f, 1}, {9.1f, 3.5f, 1.8f, 1},  {6.6f, -3.5f, 1.6f, 1},  {14.4f, 3.4f, 2.3f, 1},
    {11.5f, 3.15f, 0.8f, 2}, {8.7f, -3.2f, 0.7f, 2}, {6.1f, 3.05f, 0.65f, 2}, {4.7f, -3.15f, 0.6f, 2},
    {13.1f, 2.95f, 0.45f, 3}, {9.5f, -2.9f, 0.42f, 3}, {6.9f, 2.9f, 0.4f, 3}, {5.15f, -2.95f, 0.38f, 3},
    {10.7f, -2.75f, 1.25f, 4}, {7.7f, 2.8f, 1.45f, 4},
};

float dist(float z0, float lat0, float z1, float lat1) {
    float dz = z1 - z0, dl = lat1 - lat0;
    return std::sqrt(dz * dz + dl * dl);
}

float needFor(int kind) {
    if (kind == 2) return 0.58f;
    if (kind == 1) return 0.42f;
    return 0.28f;
}

const char* kindName(int kind) {
    if (kind == 1) return "CRATE";
    if (kind == 2) return "STONE";
    return "BRUSH";
}

const gs::Mipped& pileArt(const Art& a, int kind) {
    if (kind == 1) return a.crate;
    if (kind == 2) return a.stone;
    return a.brush;
}

int pilePal(int kind) {
    if (kind == 1) return PAL_WOOD;
    if (kind == 2) return PAL_STONE;
    return PAL_EARTH;
}

float pileH(int kind) {
    if (kind == 1) return 0.95f;
    if (kind == 2) return 1.05f;
    return 0.82f;
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
    if (dumped_ >= 4) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

float Game::bendAt(float row) const {
    return std::sin(row * 0.016f) * 20.f + std::sin(row * 0.0055f) * 6.f;
}

int Game::fogFor(float z) const {
    return int(std::clamp((z - 5.f) / 12.f, 0.f, 1.f) * 10.f);
}

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > 0.55f)) return p;
    float span = float(gs::SCREEN_H) - kHorizon;
    float t = kZNear / z;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * span;
    p.x = 160.f + bendAt(p.y - kHorizon) + lat * p.ppm;
    p.ok = true;
    return p;
}

void Game::resetField() {
    static constexpr Lay kLay[kPiles] = {
        {4.10f, 0.48f, 0}, {5.45f, -0.62f, 1}, {6.90f, 0.42f, 2},
        {8.35f, -0.36f, 0}, {9.85f, 0.28f, 1}, {11.25f, -0.18f, 2},
    };
    for (int i = 0; i < kPiles; i++) {
        pile_[i].z = kLay[i].z;
        pile_[i].lat = kLay[i].lat;
        pile_[i].kind = kLay[i].kind;
        pile_[i].work = 0.f;
        pile_[i].taken = false;
    }
    for (Dust& d : dust_) d.life = 0.f;
    carrying_ = false;
    moving_ = false;
    raking_ = false;
    tipping_ = false;
    dumped_ = 0;
    focus_ = -1;
    lat_ = kStartLat;
    z_ = kStartZ;
    face_ = -1.f;
    tip_ = 0.f;
    step_ = 0.f;
    shake_ = 0.f;
    clock_ = kClock;
    rng_ = 1;
}

void Game::begin() {
    resetField();
    won_ = false;
    over_ = false;
    reason_ = "";
    fanStep_ = -1;
    mode_ = Mode::Play;
    blip(392.f, 0.12f);
    if (sys_) sys_->setLight(120, 80, 30);
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

bool Game::rakeDown() const {
    const gs::Pad& p = sys_->pad;
    return p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
           p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO) || p.accel > 0.4f;
}

int Game::focusPile() const {
    if (carrying_) return -1;
    int best = -1;
    float bd = kReach + 0.001f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].taken) continue;
        float d = dist(z_, lat_, pile_[i].z, pile_[i].lat);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

bool Game::atLip() const { return carrying_ && dist(z_, lat_, kLipZ, kLipLat) <= kReach; }

void Game::botInput(float& ix, float& iz, bool& hold) {
    ix = iz = 0.f;
    hold = false;
    float tz = kLipZ, tl = kLipLat;
    if (!carrying_) {
        int id = 0;
        while (id < kPiles && pile_[id].taken) id++;
        if (id >= kPiles) return;
        tz = pile_[id].z;
        tl = pile_[id].lat;
    }
    float dz = tz - z_, dl = tl - lat_;
    float d = std::sqrt(dz * dz + dl * dl);
    if (d <= kReach) {
        hold = true;
        if (d < 0.1f) {
            z_ = tz;
            lat_ = tl;
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
    hold = rakeDown();
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

void Game::puff(float x, float y) {
    for (Dust& d : dust_) {
        if (d.life > 0.f) continue;
        d.x = x + (rnd() - 0.5f) * 10.f;
        d.y = y;
        d.vx = (rnd() - 0.5f) * 22.f;
        d.vy = -10.f - rnd() * 16.f;
        d.life = 0.25f + rnd() * 0.2f;
        return;
    }
}

void Game::updateWork(bool hold) {
    if (carrying_) {
        focus_ = -1;
        if (!(hold && atLip())) {
            tipping_ = false;
            if (!atLip()) tip_ = 0.f;
            return;
        }
        tipping_ = true;
        face_ = -1.f;
        tip_ += kDt;
        if ((sys_->frame & 3) == 0) {
            Proj p = project(kLipLat, kLipZ);
            if (p.ok) puff(p.x, p.y - 10.f);
        }
        if (tip_ < kTip) return;
        tip_ = 0.f;
        carrying_ = false;
        tipping_ = false;
        dumped_++;
        shake_ = 0.65f;
        sys_->apu.noiseBurst(0.26f, 280.f, 0.1f);
        blip(480.f + float(dumped_) * 36.f, 0.1f);
        sys_->rumble(0.22f, 0.4f, 46);
        if (dumped_ >= kPiles) win();
        return;
    }
    int id = focusPile();
    focus_ = id;
    if (id < 0 || !hold) return;
    raking_ = true;
    Pile& m = pile_[id];
    face_ = (m.lat < lat_) ? -1.f : 1.f;
    m.work += kDt / needFor(m.kind);
    if ((sys_->frame & 3) == 0) {
        Proj p = project(m.lat, m.z);
        if (p.ok) puff(p.x, p.y - 8.f);
    }
    if (m.work < 1.f) return;
    m.work = 1.f;
    m.taken = true;
    carrying_ = true;
    raking_ = false;
    tip_ = 0.f;
    blip(330.f, 0.08f);
    sys_->apu.noiseBurst(0.16f, 640.f, 0.07f);
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "THE GROUND IS CLEAR";
    raking_ = false;
    tipping_ = false;
    carrying_ = false;
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noise(0.f, 200.f);
    sys_->apu.noiseBurst(0.35f, 160.f, 0.16f);
    sys_->setLight(40, 180, 70);
}

void Game::lose() {
    if (won_ || mode_ != Mode::Play) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Lost;
    reason_ = "THE CLOCK DIED";
    raking_ = false;
    tipping_ = false;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noise(0.f, 200.f);
    sys_->apu.noiseBurst(0.28f, 860.f, 0.14f);
    sys_->setLight(180, 30, 20);
}

void Game::updatePlay() {
    raking_ = false;
    tipping_ = false;
    float ix = 0.f, iz = 0.f;
    bool hold = false;
    if (bot_) botInput(ix, iz, hold);
    else humanInput(ix, iz, hold);
    move(ix, iz);
    updateWork(hold);
    if (moving_ || raking_) {
        step_ += kDt * (raking_ ? 3.4f : 2.4f);
        if (step_ >= 1.f) step_ -= 1.f;
    }
    if (mode_ != Mode::Play) return;
    int before = (clock_ + 59) / 60;
    if (clock_ > 0) clock_--;
    int after = (clock_ + 59) / 60;
    if (after != before && after > 0 && after <= 10) {
        sys_->apu.tone(1, 680.f, 0.04f);
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
    if (raking_ || tipping_) sys_->apu.noise(raking_ ? 0.06f : 0.05f, raking_ ? 1100.f : 420.f, false);
    else sys_->apu.noise(0.f, 500.f);
}

void Game::fadeDust() {
    for (Dust& d : dust_) {
        if (d.life <= 0.f) continue;
        d.life -= kDt;
        d.x += d.vx * kDt;
        d.y += d.vy * kDt;
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.8f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    fadeDust();

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
        raking_ = false;
        tipping_ = false;
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

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    float width = 0.f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) {
            x += 12.f * scale;
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

void Game::drawRoad(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(2, 3, 8);
    uint16_t skyHor = gs::rgb4(13, 8, 5);
    if (mode_ == Mode::Lost) skyHor = gs::rgb4(10, 3, 3);
    else if (mode_ == Mode::Won) skyHor = gs::rgb4(14, 10, 5);
    else if (mode_ == Mode::Play && clock_ <= 10 * 60) skyHor = mixC(skyHor, gs::rgb4(12, 4, 3), 0.45f);
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
        float t = (float(y) + 0.5f - kHorizon) / span;
        t = std::max(t, 0.02f);
        float wz = kZNear / t;
        float row = float(y) - kHorizon;
        rd.on = true;
        rd.cx = 160.f + bendAt(row) + shx;
        rd.hw = std::max(8.f, kRoadHalf * kPpm * t);
        rd.v = wz * 36.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(wz * 0.42f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_DROP;
        rd.right = gs::GROUND_DROP;
        float fogT = std::clamp((wz - 6.f) / 16.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 11.f);
        float dropT = std::clamp(row / span, 0.f, 1.f);
        v.lineBackdrop[y] = mixC(gs::rgb4(5, 4, 3), gs::rgb4(1, 1, 2), dropT);
    }
}

void Game::drawSky(float shx) {
    spr(art_.peak, 70.f + shx, kHorizon + 2.f, 52.f, PAL_SKY, false, 8, true, false);
    spr(art_.peak, 168.f + shx, kHorizon + 6.f, 30.f, PAL_SKY, true, 10, true, false);
    spr(art_.peak, 246.f + shx, kHorizon + 1.f, 46.f, PAL_SKY, true, 8, true, false);
    int sunPal = mode_ == Mode::Lost ? PAL_ALERT : (mode_ == Mode::Won ? PAL_GOOD : PAL_SKY);
    spr(art_.sun, 46.f + shx, 58.f, mode_ == Mode::Won ? 26.f : 20.f, sunPal, false, 1, false, false);
    spr(art_.cloud, 118.f + std::sin(t_ * 0.15f) * 16.f + shx, 24.f, 15.f, PAL_SKY, false, 3, false, false);
    spr(art_.cloud, 214.f + std::sin(t_ * 0.11f + 1.f) * 12.f + shx, 42.f, 11.f, PAL_SKY, true, 4, false, false);
    spr(art_.hawk[int(t_ * 4.f) & 1], 236.f + std::sin(t_ * 0.4f) * 26.f + shx, 30.f + std::sin(t_ * 0.8f) * 4.f, 11.f,
        PAL_FX, std::sin(t_ * 0.4f) > 0.f, 2, false, false);
}

void Game::drawKit(float shx) {
    int cloth = PAL_GOLD;
    if (mode_ == Mode::Won) cloth = PAL_GOOD;
    else if (mode_ == Mode::Lost || (mode_ == Mode::Play && clock_ <= 10 * 60)) cloth = PAL_ALERT;
    float foot = mode_ == Mode::Lost ? 168.f : 118.f;
    int fr = std::sin(t_ * 6.f) > 0.f ? 0 : 1;
    spr(art_.post, 292.f + shx, 214.f, 108.f, PAL_WOOD, false, 0, true, false);
    if (mode_ != Mode::Lost) spr(art_.pennant[fr], 308.f + shx, foot, 16.f, cloth, false, 0, false, false);
    else spr(art_.pennant[0], 304.f + shx, foot, 14.f, cloth, false, 0, false, false);
    float bob = std::sin(t_ * 7.f) * 1.4f;
    spr(art_.flame, 292.f + shx, 112.f + bob, mode_ == Mode::Won ? 16.f : 12.f, cloth, false, 0, false, false);
    spr(art_.crag, 18.f + shx, 214.f, 46.f, PAL_STONE, false, 0, true, false);
}

void Game::drawWorld(float shx) {
    struct Item {
        float z;
        int kind;
        int id;
    };
    Item items[40];
    int n = 0;
    items[n++] = {z_, 0, 0};
    items[n++] = {kLipZ, 2, 0};
    for (int i = 0; i < kPiles; i++) items[n++] = {pile_[i].z, 1, i};
    for (int i = 0; i < int(sizeof kProps / sizeof kProps[0]); i++) items[n++] = {kProps[i].z, 3, i};
    std::sort(items, items + n, [](const Item& a, const Item& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.kind < b.kind;
    });

    for (const Dust& d : dust_) {
        if (d.life <= 0.f) continue;
        spr(art_.dust, d.x + shx, d.y, 5.f + d.life * 12.f, PAL_FX, false, 0, false, false);
    }

    float pulse = 1.f + 0.04f * std::sin(t_ * 9.f);
    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 0) {
            Proj p = project(lat_, z_);
            if (!p.ok) continue;
            int fr = (moving_ || raking_) ? (step_ < 0.5f ? 0 : 1) : 0;
            float bob = (moving_ || raking_) ? 0.f : std::sin(t_ * 2.1f) * 1.2f;
            float h = std::clamp(kWalkerH * p.ppm, 8.f, 150.f);
            float feet = p.y - bob;
            bool flip = face_ < 0.f;
            int fog = fogFor(z_);
            spr(art_.worker[fr], p.x + shx, feet, h, PAL_FIGURE, flip, fog, true, false);
            float bx = p.x + face_ * h * 0.28f + (tipping_ ? face_ * tip_ * 40.f : 0.f);
            float by = feet + (tipping_ ? std::sin(tip_ * 40.f) * 2.f : 0.f);
            spr(art_.barrow, bx + shx, by, std::clamp(h * 0.34f, 6.f, 64.f), PAL_WOOD, flip, fog, true, false);
            if (carrying_)
                spr(art_.load, bx + shx, by - h * 0.16f, std::clamp(h * 0.2f, 5.f, 36.f), PAL_EARTH, false, fog, true,
                    false);
            if (raking_)
                spr(art_.rake, p.x + face_ * h * 0.42f + shx, feet - h * 0.35f, std::clamp(h * 0.5f, 8.f, 80.f),
                    PAL_WOOD, flip, fog, false, false);
            spr(art_.shadow, p.x + shx, p.y + 2.f, std::max(4.f, h * 0.16f), PAL_FX, false, 0, false, true);
            continue;
        }
        if (it.kind == 2) {
            Proj p = project(kLipLat, kLipZ);
            if (!p.ok) continue;
            int fog = fogFor(kLipZ);
            bool show = carrying_ && (mode_ == Mode::Play || mode_ == Mode::Pause);
            if (show) {
                float my = p.y - 1.35f * p.ppm - 8.f + std::sin(t_ * 8.f) * 3.f;
                spr(art_.mark, p.x + shx, my, 12.f, atLip() ? PAL_GOOD : PAL_GOLD, false, 0, false, false);
            }
            for (int k = 0; k < dumped_; k++) {
                float ox = ((k % 3) - 1) * (6.f + p.ppm * 0.04f);
                float oy = (k / 3) * (5.f + p.ppm * 0.03f);
                spr(art_.rubble, p.x + ox + shx, p.y - 8.f - oy, std::clamp(0.32f * p.ppm, 6.f, 22.f), PAL_STONE,
                    k & 1, fog, true, false);
            }
            spr(art_.crib, p.x + shx, p.y, std::clamp(1.15f * p.ppm, 10.f, 120.f), PAL_WOOD, false, fog, true, false);
            spr(art_.shadow, p.x + shx, p.y + 2.f, std::max(4.f, 0.2f * p.ppm), PAL_FX, false, 0, false, true);
            continue;
        }
        if (it.kind == 1) {
            const Pile& m = pile_[it.id];
            Proj p = project(m.lat, m.z);
            if (!p.ok) continue;
            int fog = fogFor(m.z);
            if (m.taken) {
                spr(art_.swept, p.x + shx, p.y, std::clamp(0.35f * p.ppm, 4.f, 28.f), PAL_FX, false, fog, false, false);
                continue;
            }
            float h = pileH(m.kind) * (1.f - m.work * 0.55f) * p.ppm;
            if (it.id == focus_ && mode_ == Mode::Play) h *= pulse;
            spr(pileArt(art_, m.kind), p.x + shx, p.y, std::clamp(h, 6.f, 90.f), pilePal(m.kind), false, fog, true,
                false);
            spr(art_.shadow, p.x + shx, p.y + 1.f, std::max(3.f, h * 0.18f), PAL_FX, false, 0, false, true);
            continue;
        }
        const Prop& pr = kProps[it.id];
        Proj p = project(pr.lat, pr.z);
        if (!p.ok) continue;
        float h = std::clamp(pr.h * p.ppm, 3.f, 140.f);
        int fog = fogFor(pr.z);
        if (pr.kind == 0) spr(art_.pine, p.x + shx, p.y, h, PAL_PINE, pr.lat > 0, fog, true, false);
        else if (pr.kind == 1) spr(art_.crag, p.x + shx, p.y, h, PAL_STONE, pr.lat > 0, fog, true, false);
        else if (pr.kind == 2) spr(art_.bush, p.x + shx, p.y, h, PAL_PINE, pr.lat < 0, fog, true, false);
        else if (pr.kind == 3) spr(art_.grass, p.x + shx, p.y, h, PAL_PINE, false, fog, true, false);
        else spr(art_.cairn, p.x + shx, p.y, h, PAL_STONE, pr.lat > 0, fog, true, false);
    }
}

void Game::messages(float shx) {
    float pipX = 160.f - 2.5f * 18.f;
    for (int i = 0; i < kPiles; i++) {
        int pal = PAL_STONE;
        float s = 8.f;
        if (i < dumped_) {
            pal = mode_ == Mode::Lost ? PAL_ALERT : PAL_GOOD;
            s = 9.f;
        } else if (mode_ == Mode::Lost) pal = PAL_ALERT;
        else if (carrying_ && i == dumped_) pal = PAL_GOLD;
        spr(art_.pip, pipX + float(i) * 18.f + shx, 40.f, s, pal, false, 0, false, false);
    }

    if (mode_ == Mode::Title) {
        text("S3 RIDGE CLER", 160.f + shx, 16.f, 0.62f, PAL_GOLD);
        text("CLEAR THE GROUND", 160.f + shx, 34.f, 0.4f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160.f + shx, 16.f, 0.48f, PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160.f + shx, 16.f, 0.58f, PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 100.f, 0.9f, PAL_GOLD);
    int sec = std::max(0, (clock_ + 59) / 60);
    char clock[12];
    std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
    int pal = (mode_ == Mode::Play && sec <= 10) ? PAL_ALERT : PAL_GOLD;
    float sc = (mode_ == Mode::Play && sec <= 10) ? 1.05f + std::sin(t_ * 10.f) * 0.05f : 1.02f;
    text(clock, 160.f + shx, 16.f, sc, pal);
}

void Game::hud() {
    if (mode_ == Mode::Title) {
        if ((sys_->frame / 30) & 1) tileText(14, 23, "PRESS ENTER", PAL_GOLD);
        tileText(3, 25, "CLEAR THE GROUND BEFORE THE CLOCK", PAL_TEXT);
        tileText(5, 26, "MISS THAT AND THE WATCH IS OVER", PAL_TEXT);
        tileText(6, 27, "ARROWS MOVE  HOLD C TO RAKE", PAL_GOLD);
        if (mode_ == Mode::Title) sys_->setLight(40, 36, 96);
        return;
    }

    char line[40];
    std::snprintf(line, sizeof line, "LEFT %d", std::max(0, kPiles - dumped_));
    int pal = mode_ == Mode::Won ? PAL_GOOD : (mode_ == Mode::Lost ? PAL_ALERT : PAL_TEXT);
    tileText(32, 0, line, pal);

    if (mode_ == Mode::Won) {
        tileText(10, 24, "THE WATCH HOLDS", PAL_GOOD);
        tileText(12, 26, "ENTER RETRIES", PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Lost) {
        tileText(11, 24, "THE WATCH IS OVER", PAL_ALERT);
        tileText(12, 26, "ENTER RETRIES", PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Pause) {
        tileText(12, 25, "ENTER RESUMES", PAL_TEXT);
        return;
    }

    const char* hint = "CLEAR THE GROUND";
    if (carrying_) hint = atLip() ? "HOLD C TO TIP" : "BACK TO THE LIP";
    else if (focus_ >= 0) {
        std::snprintf(line, sizeof line, "HOLD C  %s", kindName(pile_[focus_].kind));
        hint = line;
    }
    int col = 20 - int(std::strlen(hint)) / 2;
    int hpal = clock_ <= 10 * 60 ? PAL_ALERT : PAL_GOLD;
    tileText(col, 26, hint, hpal);

    if (clock_ <= 10 * 60) sys_->setLight(180, 40, 24);
    else if (carrying_) sys_->setLight(180, 120, 36);
    else sys_->setLight(120, 78, 28);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 46.f) * 3.2f * std::min(shake_, 1.f);
    drawRoad(shx);
    messages(shx);
    drawKit(shx);
    drawWorld(shx);
    drawSky(shx);
    hud();
}

}  // namespace rcler
