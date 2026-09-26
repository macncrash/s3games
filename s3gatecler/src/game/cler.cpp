#include "game/cler.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace cler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 104.f;
constexpr float kReach = 18.f;
constexpr float kTip = 0.30f;
constexpr int kClock = 45 * 60;
constexpr int kMounds = 6;
constexpr float kHorizon = 102.f;
constexpr float kYardL = 34.f, kYardR = 302.f, kYardT = 160.f, kYardB = 214.f;
constexpr float kSpoilX = 58.f, kSpoilY = 198.f;
constexpr float kStartX = 188.f, kStartY = 188.f;
constexpr float kPierL = 54.f, kPierR = 266.f, kPierFoot = 150.f, kPierH = 118.f;
constexpr float kDoorFoot = 152.f, kDoorH = 96.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[kMounds] = {
    {120.f, 168.f, 0}, {188.f, 166.f, 1}, {256.f, 170.f, 2},
    {258.f, 208.f, 0}, {186.f, 208.f, 1}, {116.f, 206.f, 2},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float rakeNeed(int kind) {
    if (kind == 1) return 0.68f;
    if (kind == 2) return 0.46f;
    return 0.34f;
}

const char* kindName(int kind) {
    if (kind == 1) return "STONE";
    if (kind == 2) return "CRATE";
    return "BRUSH";
}

uint16_t mix4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ch[3];
    for (int i = 0; i < 3; i++) {
        int s = 8 - 4 * i;
        int ca = (a >> s) & 15;
        int cb = (b >> s) & 15;
        ch[i] = int(std::lround(ca + (cb - ca) * t));
    }
    return gs::rgb4(ch[0], ch[1], ch[2]);
}

const gs::Mipped& pileArt(const Art& a, int kind) {
    if (kind == 1) return a.stone;
    if (kind == 2) return a.crate;
    return a.brush;
}

int pilePal(int kind) {
    if (kind == 1) return PAL_STONE;
    if (kind == 2) return PAL_CRATE;
    return PAL_EARTH;
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

void Game::resetYard() {
    for (int i = 0; i < kMounds; i++) {
        mound_[i].x = kLay[i].x;
        mound_[i].y = kLay[i].y;
        mound_[i].kind = kLay[i].kind;
        mound_[i].work = 0.f;
        mound_[i].taken = false;
    }
    for (Dust& d : dust_) d.life = 0.f;
    carrying_ = false;
    moving_ = false;
    raking_ = false;
    tipping_ = false;
    dumped_ = 0;
    focus_ = -1;
    px_ = kStartX;
    py_ = kStartY;
    face_ = -1.f;
    tip_ = 0.f;
    step_ = 0.f;
    shake_ = 0.f;
    clock_ = kClock;
    rng_ = 1;
}

void Game::begin() {
    resetYard();
    won_ = false;
    over_ = false;
    reason_ = "";
    fanStep_ = -1;
    mode_ = Mode::Play;
    blip(392.f, 0.12f);
}

void Game::toTitle() {
    if (sys_) {
        sys_->apu.silence();
        sys_->apu.noise(0.f, 400.f);
    }
    fanStep_ = -1;
    blip_ = 0.f;
    tick_ = 0.f;
    raking_ = false;
    tipping_ = false;
    won_ = false;
    over_ = false;
    reason_ = "";
    resetYard();
    mode_ = Mode::Title;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    toTitle();
    if (bot_) begin();
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

bool Game::modePressed() const { return sys_->pad.pressed(gs::BTN_MODE); }

int Game::focusMound() const {
    if (carrying_) return -1;
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kMounds; i++) {
        if (mound_[i].taken) continue;
        float d = dist(px_, py_, mound_[i].x, mound_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

bool Game::atCrib() const { return carrying_ && dist(px_, py_, kSpoilX, kSpoilY) <= kReach; }

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0.f;
    hold = false;
    float tx = kSpoilX, ty = kSpoilY;
    if (!carrying_) {
        int id = 0;
        while (id < kMounds && mound_[id].taken) id++;
        if (id >= kMounds) return;
        tx = mound_[id].x;
        ty = mound_[id].y;
    }
    float dx = tx - px_, dy = ty - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    hold = d <= kReach;
    if (d < 1.2f) {
        px_ = std::clamp(tx, kYardL, kYardR);
        py_ = std::clamp(ty, kYardT, kYardB);
        return;
    }
    ix = dx / d;
    iy = dy / d;
}

void Game::humanInput(float& ix, float& iy, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = iy = 0.f;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (p.down(gs::BTN_UP)) iy -= 1.f;
    if (p.down(gs::BTN_DOWN)) iy += 1.f;
    if (std::fabs(p.axisX) > 0.2f || std::fabs(p.axisY) > 0.2f) {
        ix = p.axisX;
        iy = -p.axisY;
    }
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
           p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO) || p.accel > 0.45f;
}

void Game::move(float ix, float iy) {
    float mag = std::sqrt(ix * ix + iy * iy);
    if (mag < 0.05f) {
        moving_ = false;
        return;
    }
    if (mag > 1.f) {
        ix /= mag;
        iy /= mag;
    }
    px_ = std::clamp(px_ + ix * kSpeed * kDt, kYardL, kYardR);
    py_ = std::clamp(py_ + iy * kSpeed * kDt, kYardT, kYardB);
    if (ix < -0.15f) face_ = -1.f;
    else if (ix > 0.15f) face_ = 1.f;
    moving_ = true;
}

void Game::puff(float x, float y) {
    for (Dust& d : dust_) {
        if (d.life > 0.f) continue;
        d.x = x + (rnd() - 0.5f) * 12.f;
        d.y = y;
        d.vx = (rnd() - 0.5f) * 28.f;
        d.vy = -12.f - rnd() * 18.f;
        d.life = 0.28f + rnd() * 0.22f;
        return;
    }
}

void Game::updateWork(bool hold) {
    if (carrying_) {
        if (!(hold && atCrib())) return;
        tipping_ = true;
        tip_ += kDt;
        if (tip_ < kTip) return;
        tip_ = 0.f;
        carrying_ = false;
        tipping_ = false;
        dumped_++;
        shake_ = 0.7f;
        sys_->apu.noiseBurst(0.28f, 320.f, 0.1f);
        blip(520.f + float(dumped_) * 30.f, 0.1f);
        sys_->rumble(0.25f, 0.45f, 50);
        if (dumped_ >= kMounds) win();
        return;
    }
    int id = focusMound();
    focus_ = id;
    if (id < 0 || !hold) return;
    raking_ = true;
    Mound& m = mound_[id];
    m.work += kDt / rakeNeed(m.kind);
    if ((sys_->frame & 3) == 0) puff(m.x, m.y - 6.f);
    if (m.work < 1.f) return;
    m.work = 1.f;
    m.taken = true;
    carrying_ = true;
    raking_ = false;
    tip_ = 0.f;
    blip(440.f, 0.08f);
    sys_->apu.noiseBurst(0.18f, 700.f, 0.08f);
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "THE GROUND IS CLEAR";
    raking_ = false;
    tipping_ = false;
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noise(0.f, 200.f);
    sys_->apu.noiseBurst(0.4f, 180.f, 0.18f);
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
    sys_->apu.noiseBurst(0.3f, 900.f, 0.16f);
    sys_->setLight(180, 30, 20);
}

void Game::updatePlay() {
    raking_ = false;
    tipping_ = false;
    float ix = 0.f, iy = 0.f;
    bool hold = false;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);
    move(ix, iy);
    focus_ = focusMound();
    updateWork(hold);
    if (moving_ || raking_) step_ += kDt * (raking_ ? 3.f : 1.6f);
    if (mode_ != Mode::Play) return;
    int before = (clock_ + 59) / 60;
    if (clock_ > 0) clock_--;
    int after = (clock_ + 59) / 60;
    if (after != before && after > 0 && after <= 10) {
        sys_->apu.tone(1, 700.f, 0.035f);
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
        if (fanT_ < 0.15f) return;
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
    if (raking_) sys_->apu.noise(0.07f, 980.f, false);
    else sys_->apu.noise(0.f, 700.f);
}

void Game::fadeDust() {
    for (Dust& d : dust_) {
        if (d.life <= 0.f) continue;
        d.life -= kDt;
        d.x += d.vx * kDt;
        d.y += d.vy * kDt;
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    fadeDust();

    if (mode_ == Mode::Title) {
        if (startPressed()) begin();
        else if (modePressed()) sys.quit();
        serviceAudio();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (startPressed()) mode_ = Mode::Play;
        else if (modePressed()) toTitle();
        serviceAudio();
        draw();
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (startPressed() || modePressed()) toTitle();
        serviceAudio();
        draw();
        return;
    }

    if (!bot_ && (startPressed() || modePressed())) {
        raking_ = false;
        mode_ = Mode::Pause;
        serviceAudio();
        draw();
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
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    float warm = mode_ == Mode::Won ? 0.55f : 0.f;
    float hot = mode_ == Mode::Lost ? 0.55f : 0.f;
    if (mode_ == Mode::Play && clock_ < 10 * 60) hot = 0.22f;
    uint16_t top = gs::rgb4(2, 2, 6);
    uint16_t mid = gs::rgb4(7, 4, 7);
    uint16_t low = gs::rgb4(12, 7, 4);
    if (warm > 0.f) {
        top = mix4(top, gs::rgb4(6, 5, 3), warm);
        mid = mix4(mid, gs::rgb4(12, 8, 3), warm);
        low = mix4(low, gs::rgb4(14, 10, 4), warm);
    }
    if (hot > 0.f) {
        top = mix4(top, gs::rgb4(6, 1, 2), hot);
        mid = mix4(mid, gs::rgb4(10, 3, 2), hot);
        low = mix4(low, gs::rgb4(8, 2, 1), hot);
    }
    float shx = std::sin(t_ * 46.f) * shake_ * 3.f;
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = u < 0.55f ? mix4(top, mid, u / 0.55f) : mix4(mid, low, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - kHorizon) / span;
        t = std::clamp(t, 0.f, 1.f);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx;
        rd.hw = 132.f + t * 26.f;
        rd.v = float(y) * 17.f;
        rd.pal = uint8_t(PAL_YARD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = uint8_t((y / 4) & 1);
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        v.lineFog[y] = uint8_t((1.f - t) * (1.f - t) * 7.f);
        v.lineBackdrop[y] = gs::rgb4(2, 3, 1);
    }
    v.setFogColor(mode_ == Mode::Lost ? gs::rgb4(6, 2, 2) : gs::rgb4(6, 4, 5));
}

void Game::actors() {
    struct Bit {
        float y;
        int kind;
        int id;
    };
    Bit bits[16];
    int n = 0;
    bits[n++] = {py_, 0, 0};
    for (int i = 0; i < kMounds; i++) bits[n++] = {mound_[i].y, 1, i};
    bits[n++] = {kSpoilY, 2, 0};
    std::sort(bits, bits + n, [](const Bit& a, const Bit& b) { return a.y > b.y; });

    for (const Dust& d : dust_) {
        if (d.life <= 0.f) continue;
        spr(art_.dust, d.x, d.y, 6.f + d.life * 14.f, PAL_FX, false, 0, false, false);
    }

    float pulse = 1.f + 0.045f * std::sin(t_ * 10.f);
    bool showMark = carrying_ && (mode_ == Mode::Play || mode_ == Mode::Pause);
    for (int i = 0; i < n; i++) {
        const Bit& b = bits[i];
        if (b.kind == 0) {
            int fr = (moving_ || raking_) ? (int(step_ * 8.f) & 1) : 0;
            float bob = (moving_ || raking_) ? 0.f : std::sin(t_ * 2.2f) * 1.1f;
            bool flip = face_ < 0.f;
            float foot = py_ + bob;
            spr(art_.worker[fr], px_, foot, 54.f, PAL_FIGURE, flip, 0, true, false);
            if (carrying_) spr(art_.load, px_ + face_ * 16.f, foot - 8.f, 14.f, PAL_EARTH, false, 0, true, false);
            float tipBob = tipping_ ? std::sin(tip_ * 48.f) * 2.2f : 0.f;
            spr(art_.barrow, px_ + face_ * 16.f, foot + tipBob, 18.f, PAL_CRATE, flip, 0, true, false);
            spr(art_.shadow, px_, foot + 2.f, 10.f, PAL_FX, false, 0, false, true);
            continue;
        }
        if (b.kind == 2) {
            if (showMark) {
                float bob = std::sin(t_ * 8.f) * 3.f;
                spr(art_.mark, kSpoilX, kSpoilY - 36.f + bob, 12.f, atCrib() ? PAL_GOOD : PAL_GOLD, false, 0, false, false);
            }
            for (int k = 0; k < dumped_; k++) {
                float ox = ((k % 3) - 1) * 10.f;
                float oy = (k / 3) * 7.f;
                spr(art_.rubble, kSpoilX + ox, kSpoilY - 8.f - oy, 12.f, PAL_EARTH, k & 1, 0, true, false);
            }
            float ch = (showMark ? 40.f * pulse : 40.f);
            spr(art_.crib, kSpoilX, kSpoilY, ch, PAL_CRATE, false, 0, true, false);
            spr(art_.shadow, kSpoilX, kSpoilY + 2.f, 12.f, PAL_FX, false, 0, false, true);
            continue;
        }
        const Mound& m = mound_[b.id];
        if (m.taken) {
            spr(art_.swept, m.x, m.y + 1.f, 12.f, PAL_FX, false, 0, false, false);
            continue;
        }
        float h = (m.kind == 2 ? 32.f : 30.f) * (1.f - m.work * 0.55f);
        if (b.id == focus_ && mode_ == Mode::Play) h *= pulse;
        int pal = pilePal(m.kind);
        spr(pileArt(art_, m.kind), m.x, m.y, std::max(8.f, h), pal, false, 0, true, false);
        spr(art_.shadow, m.x, m.y + 2.f, 9.f, PAL_FX, false, 0, false, true);
        spr(art_.dirt, m.x, m.y + 1.f, 11.f, PAL_EARTH, false, 0, false, false);
    }
}

void Game::gate() {
    int lampPal = PAL_GOLD;
    if (mode_ == Mode::Won) lampPal = PAL_GOOD;
    else if (mode_ == Mode::Lost || (mode_ == Mode::Play && clock_ < 10 * 60)) lampPal = PAL_ALERT;
    float bob = std::sin(t_ * 5.f) * 1.2f;
    spr(art_.lamp, kPierL, 58.f + bob, 18.f, lampPal, false, 0, false, false);
    spr(art_.lamp, kPierR, 58.f + bob, 18.f, lampPal, true, 0, false, false);
    spr(art_.keystone, 160.f, 46.f, 28.f, PAL_STONE, false, 0, false, false);
    spr(art_.lintel, 160.f, 50.f, 24.f, PAL_STONE, false, 0, false, false);
    spr(art_.pier, kPierL, kPierFoot, kPierH, PAL_STONE, false, 0, true, false);
    spr(art_.pier, kPierR, kPierFoot, kPierH, PAL_STONE, true, 0, true, false);
    const gs::Mipped& leaf = mode_ == Mode::Won ? art_.dawn : art_.door;
    spr(leaf, 160.f, kDoorFoot, kDoorH, PAL_NIGHT, false, 0, true, false);
    spr(art_.tree, 16.f, 168.f, 62.f, PAL_TREE, false, 1, true, false);
    spr(art_.tree, 306.f, 172.f, 58.f, PAL_TREE, true, 1, true, false);
    float c0 = std::fmod(20.f + t_ * 7.f, 400.f) - 40.f;
    float c1 = std::fmod(180.f + t_ * 4.f, 420.f) - 50.f;
    spr(art_.cloud, c0, 18.f, 16.f, PAL_NIGHT, false, 0, false, false);
    spr(art_.cloud, c1, 30.f, 13.f, PAL_NIGHT, true, 0, false, false);
    int sunPal = mode_ == Mode::Lost ? PAL_ALERT : PAL_GOLD;
    spr(art_.sun, 292.f, 20.f, mode_ == Mode::Won ? 20.f : 16.f, sunPal, false, 0, false, false);

    if (mode_ == Mode::Won) sys_->setLight(40, 180, 70);
    else if (mode_ == Mode::Lost) sys_->setLight(180, 30, 20);
    else if (mode_ == Mode::Title) sys_->setLight(40, 32, 96);
    else if (mode_ == Mode::Play && clock_ < 10 * 60) sys_->setLight(180, 40, 24);
    else if (carrying_) sys_->setLight(180, 110, 30);
    else sys_->setLight(96, 64, 28);
}

void Game::messages() {
    if (mode_ == Mode::Title) {
        text("S3 GATE CLER", 160.f, 12.f, 0.78f, PAL_GOLD);
        text("CLEAR THE GROUND", 160.f, 28.f, 0.46f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160.f, 14.f, 0.52f, PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160.f, 14.f, 0.62f, PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) text("PAUSED", 160.f, 108.f, 0.9f, PAL_GOLD);
    int sec = std::max(0, (clock_ + 59) / 60);
    char clock[12];
    std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
    int pal = (mode_ == Mode::Play && sec <= 10) ? PAL_ALERT : PAL_GOLD;
    text(clock, 160.f, 13.f, 1.05f, pal);
}

void Game::hud() {
    if (mode_ == Mode::Title) {
        if ((sys_->frame / 30) % 2 == 0) tileText(14, 24, "PRESS ENTER", PAL_GOLD);
        tileText(6, 26, "RAKE A PILE INTO THE CRIB", PAL_TEXT);
        return;
    }
    char line[24];
    std::snprintf(line, sizeof line, "LEFT %d", std::max(0, kMounds - dumped_));
    int pal = mode_ == Mode::Won ? PAL_GOOD : (mode_ == Mode::Lost ? PAL_ALERT : PAL_TEXT);
    tileText(32, 0, line, pal);
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return;

    const char* hint = "CLEAR THE GROUND";
    if (carrying_) hint = atCrib() ? "HOLD C TO TIP" : "TIP IT IN THE CRIB";
    else if (focus_ >= 0) {
        std::snprintf(line, sizeof line, "HOLD C  %s", kindName(mound_[focus_].kind));
        hint = line;
    }
    int col = 20 - int(std::strlen(hint)) / 2;
    int hpal = (mode_ == Mode::Play && clock_ <= 10 * 60) ? PAL_ALERT : PAL_GOLD;
    tileText(col, 26, hint, hpal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    messages();
    actors();
    gate();
    hud();
}

}  // namespace cler
