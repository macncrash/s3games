#include "game/harbor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 132.f;
constexpr float kReach = 22.f;
constexpr float kShove = 0.18f;
constexpr int kClock = 50 * 60;
constexpr float kHorizon = 78.f;
constexpr float kQuay = 152.f;
constexpr float kYardL = 40.f, kYardR = 304.f, kYardT = 168.f, kYardB = 214.f;
constexpr float kBoatX = 70.f, kBoatY = 176.f;
constexpr float kStartX = 236.f, kStartY = 198.f;
constexpr float kBoatFoot = 164.f, kBoatH = 48.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {132.f, 176.f, 0}, {188.f, 172.f, 1}, {248.f, 178.f, 2},
    {286.f, 206.f, 3}, {196.f, 208.f, 0}, {124.f, 204.f, 1},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float haulNeed(int kind) {
    if (kind == 1) return 0.48f;
    if (kind == 2) return 0.36f;
    if (kind == 0) return 0.28f;
    return 0.20f;
}

const char* kindName(int kind) {
    if (kind == 1) return "BARREL";
    if (kind == 2) return "NET";
    if (kind == 3) return "COIL";
    return "CRATE";
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

const gs::Mipped& loadArt(const Art& a, int kind) {
    if (kind == 1) return a.barrel;
    if (kind == 2) return a.net;
    if (kind == 3) return a.coil;
    return a.crate;
}

int loadPal(int kind) {
    if (kind == 1) return PAL_STEEL;
    if (kind == 2) return PAL_NET;
    if (kind == 3) return PAL_WOOD;
    return PAL_WOOD;
}

float loadH(int kind) {
    if (kind == 1) return 32.f;
    if (kind == 3) return 16.f;
    if (kind == 2) return 22.f;
    return 28.f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (stowed_ >= 4) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

void Game::resetQuay() {
    for (int i = 0; i < kLoads; i++) {
        load_[i].x = kLay[i].x;
        load_[i].y = kLay[i].y;
        load_[i].kind = kLay[i].kind;
        load_[i].work = 0.f;
        load_[i].taken = false;
    }
    for (Dust& d : dust_) d.life = 0.f;
    carrying_ = false;
    moving_ = false;
    lifting_ = false;
    shoving_ = false;
    carried_ = 0;
    stowed_ = 0;
    focus_ = -1;
    px_ = kStartX;
    py_ = kStartY;
    face_ = -1.f;
    shove_ = 0.f;
    step_ = 0.f;
    shake_ = 0.f;
    clock_ = kClock;
    rng_ = 7;
}

void Game::begin() {
    resetQuay();
    won_ = false;
    over_ = false;
    reason_ = "";
    fanStep_ = -1;
    mode_ = Mode::Play;
    blip(330.f, 0.12f);
}

void Game::toTitle() {
    if (sys_) {
        sys_->apu.silence();
        sys_->apu.noise(0.f, 400.f);
    }
    fanStep_ = -1;
    blip_ = 0.f;
    tick_ = 0.f;
    lifting_ = false;
    shoving_ = false;
    won_ = false;
    over_ = false;
    reason_ = "";
    resetQuay();
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

int Game::focusLoad() const {
    if (carrying_) return -1;
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kLoads; i++) {
        if (load_[i].taken) continue;
        float d = dist(px_, py_, load_[i].x, load_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

bool Game::atBoat() const { return carrying_ && dist(px_, py_, kBoatX, kBoatY) <= kReach; }

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0.f;
    hold = false;
    float tx = kBoatX, ty = kBoatY;
    if (!carrying_) {
        int id = 0;
        while (id < kLoads && load_[id].taken) id++;
        if (id >= kLoads) return;
        tx = load_[id].x;
        ty = load_[id].y;
    }
    float dx = tx - px_, dy = ty - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    hold = d <= kReach;
    if (d < 1.4f) {
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
        d.x = x + (rnd() - 0.5f) * 8.f;
        d.y = y;
        d.vx = (rnd() - 0.5f) * 18.f;
        d.vy = -8.f - rnd() * 12.f;
        d.life = 0.22f + rnd() * 0.16f;
        return;
    }
}

void Game::updateWork(bool hold) {
    if (carrying_) {
        if (!(hold && atBoat())) return;
        shoving_ = true;
        shove_ += kDt;
        if (shove_ < kShove) return;
        shove_ = 0.f;
        carrying_ = false;
        shoving_ = false;
        stowed_++;
        shake_ = 0.5f;
        sys_->apu.noiseBurst(0.22f, 180.f, 0.1f);
        blip(392.f + float(stowed_) * 24.f, 0.1f);
        sys_->rumble(0.18f, 0.32f, 40);
        if (stowed_ >= kLoads) win();
        return;
    }
    int id = focusLoad();
    focus_ = id;
    if (id < 0 || !hold) return;
    lifting_ = true;
    Load& m = load_[id];
    m.work += kDt / haulNeed(m.kind);
    if ((sys_->frame & 3) == 0) puff(m.x, m.y - 6.f);
    if (m.work < 1.f) return;
    m.work = 1.f;
    m.taken = true;
    carrying_ = true;
    carried_ = m.kind;
    lifting_ = false;
    shove_ = 0.f;
    blip(294.f, 0.08f);
    sys_->apu.noiseBurst(0.14f, 520.f, 0.06f);
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "THE QUAY IS CLEAR";
    lifting_ = false;
    shoving_ = false;
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noise(0.f, 200.f);
    sys_->apu.noiseBurst(0.3f, 140.f, 0.16f);
    sys_->setLight(30, 160, 90);
}

void Game::lose() {
    if (won_ || mode_ != Mode::Play) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Lost;
    reason_ = "THE CLOCK DIED";
    lifting_ = false;
    shoving_ = false;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noise(0.f, 200.f);
    sys_->apu.noiseBurst(0.26f, 700.f, 0.14f);
    sys_->setLight(160, 28, 18);
}

void Game::updatePlay() {
    lifting_ = false;
    shoving_ = false;
    float ix = 0.f, iy = 0.f;
    bool hold = false;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);
    move(ix, iy);
    focus_ = focusLoad();
    updateWork(hold);
    if (moving_ || lifting_) step_ += kDt * (lifting_ ? 3.f : 1.6f);
    if (mode_ != Mode::Play) return;
    int before = (clock_ + 59) / 60;
    if (clock_ > 0) clock_--;
    int after = (clock_ + 59) / 60;
    if (after != before && after > 0 && after <= 10) {
        sys_->apu.tone(1, 620.f, 0.04f);
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
        static const float good[] = {330.f, 392.f, 494.f, 659.f};
        static const float bad[] = {196.f, 165.f, 131.f};
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
    if (lifting_ || shoving_) sys_->apu.noise(0.05f, lifting_ ? 780.f : 220.f, false);
    else sys_->apu.noise(0.f, 500.f);
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
        lifting_ = false;
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
    const float gap = std::max(1.f, scale * 2.f);
    float width = 0.f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) width += 10.f * scale + gap;
        else width += float(art_.glyph[c - 32].w) * scale + gap;
    }
    width -= gap;
    x -= width * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) {
            x += 10.f * scale + gap;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        x += gw + gap;
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

void Game::water() {
    gs::VDP& v = sys_->vdp;
    float warm = mode_ == Mode::Won ? 0.45f : 0.f;
    float hot = mode_ == Mode::Lost ? 0.45f : 0.f;
    if (mode_ == Mode::Play && clock_ < 10 * 60) hot = 0.22f;
    uint16_t top = gs::rgb4(1, 2, 6);
    uint16_t mid = gs::rgb4(2, 5, 9);
    uint16_t low = gs::rgb4(3, 7, 10);
    if (warm > 0.f) {
        top = mix4(top, gs::rgb4(5, 6, 4), warm);
        mid = mix4(mid, gs::rgb4(8, 10, 5), warm);
    }
    if (hot > 0.f) {
        top = mix4(top, gs::rgb4(6, 1, 2), hot);
        mid = mix4(mid, gs::rgb4(8, 2, 3), hot);
        low = mix4(low, gs::rgb4(7, 2, 3), hot);
    }
    float shx = std::sin(t_ * 40.f) * shake_ * 2.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            rd.on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = u < 0.55f ? mix4(top, mid, u / 0.55f) : mix4(mid, low, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            continue;
        }
        rd.on = true;
        rd.cx = 160.f + shx + std::sin(t_ * 0.7f + float(y) * 0.02f) * 6.f;
        rd.hw = 190.f;
        rd.v = float(y) * 16.f + t_ * 40.f;
        rd.band = uint8_t((y / 4 + int(t_ * 6.f)) & 1);
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        if (y < int(kQuay)) {
            rd.pal = uint8_t(PAL_WATER);
            rd.style = 2;
            v.lineFog[y] = uint8_t((1.f - float(y - kHorizon) / (kQuay - kHorizon)) * 4.f);
            v.lineBackdrop[y] = gs::rgb4(1, 4, 7);
        } else {
            rd.pal = uint8_t(PAL_QUAY);
            rd.style = 1;
            rd.v = float(y) * 22.f;
            v.lineFog[y] = 0;
            v.lineBackdrop[y] = gs::rgb4(5, 5, 5);
        }
    }
    v.setFogColor(mode_ == Mode::Lost ? gs::rgb4(6, 2, 2) : gs::rgb4(2, 5, 8));
    v.roadTime = int(t_ * 28.f);
}

void Game::freight() {
    struct Bit {
        float y;
        int kind;
        int id;
    };
    Bit bits[16];
    int n = 0;
    bits[n++] = {py_, 0, 0};
    for (int i = 0; i < kLoads; i++) bits[n++] = {load_[i].y, 1, i};
    bits[n++] = {kBoatFoot, 2, 0};
    for (int k = 0; k < stowed_; k++) bits[n++] = {150.f - float(k / 2) * 6.f, 3, k};
    std::sort(bits, bits + n, [](const Bit& a, const Bit& b) { return a.y > b.y; });

    float shx = std::sin(t_ * 40.f) * shake_ * 2.f;
    for (const Dust& d : dust_) {
        if (d.life <= 0.f) continue;
        spr(art_.dust, d.x + shx, d.y, 5.f + d.life * 10.f, PAL_FX, false, 0, false, false);
    }

    float pulse = 1.f + 0.04f * std::sin(t_ * 8.f);
    bool showMark = carrying_ && (mode_ == Mode::Play || mode_ == Mode::Pause);
    float bobBoat = std::sin(t_ * 1.6f) * 2.f;
    for (int i = 0; i < n; i++) {
        const Bit& b = bits[i];
        if (b.kind == 0) {
            int fr = (moving_ || lifting_) ? (int(step_ * 8.f) & 1) : 0;
            float bob = (moving_ || lifting_) ? 0.f : std::sin(t_ * 2.f) * 1.f;
            bool flip = face_ < 0.f;
            float foot = py_ + bob;
            spr(art_.clerk[fr], px_ + shx, foot, 50.f, PAL_CLERK, flip, 0, true, false);
            float dx = face_ * 14.f;
            if (carrying_) {
                float lean = shoving_ ? std::sin(shove_ * 36.f) * 2.f : 0.f;
                spr(loadArt(art_, carried_), px_ + shx + dx, foot - 20.f + lean, 14.f, loadPal(carried_), flip, 0, true,
                    false);
            }
            spr(art_.barrow, px_ + shx + dx, foot + (shoving_ ? 1.f : 0.f), 24.f, PAL_STEEL, flip, 0, true, false);
            spr(art_.shadow, px_ + shx, foot + 2.f, 8.f, PAL_FX, false, 0, false, true);
            continue;
        }
        if (b.kind == 2) {
            if (showMark) {
                float bob = std::sin(t_ * 7.f) * 3.f;
                spr(art_.mark, kBoatX + shx, kBoatY - 36.f + bob, 11.f, atBoat() ? PAL_GOOD : PAL_GOLD, false, 0,
                    false, false);
            }
            spr(art_.boat, kBoatX + shx, kBoatFoot + bobBoat, kBoatH, PAL_BOAT, false, 0, true, false);
            continue;
        }
        if (b.kind == 3) {
            float ox = (b.id % 2 == 0 ? -10.f : 8.f);
            float oy = float(b.id / 2) * 7.f;
            spr(art_.cargo, kBoatX + ox + shx, 148.f + bobBoat - oy, 12.f, PAL_WOOD, b.id & 1, 0, true, false);
            continue;
        }
        const Load& m = load_[b.id];
        if (m.taken) continue;
        float h = loadH(m.kind) * (1.f - m.work * 0.32f);
        if (b.id == focus_ && mode_ == Mode::Play) h *= pulse;
        spr(loadArt(art_, m.kind), m.x + shx, m.y, std::max(8.f, h), loadPal(m.kind), false, 0, true, false);
        spr(art_.shadow, m.x + shx, m.y + 2.f, 7.f, PAL_FX, false, 0, false, true);
    }
}

void Game::harbor() {
    int lampPal = PAL_GOLD;
    if (mode_ == Mode::Won) lampPal = PAL_GOOD;
    else if (mode_ == Mode::Lost || (mode_ == Mode::Play && clock_ < 10 * 60)) lampPal = PAL_ALERT;
    spr(art_.shed, 168.f, 128.f, 70.f, PAL_SHED, false, 1, true, false);
    spr(art_.crane, 286.f, 118.f, 62.f, PAL_STEEL, false, 1, true, false);
    spr(art_.clock, 160.f, 42.f, 26.f, lampPal, false, 0, false, false);
    spr(art_.lamp, 36.f, 108.f, 28.f, lampPal, false, 0, true, false);
    for (int i = 0; i < 4; i++) {
        float x = 48.f + float(i) * 78.f;
        float bob = std::sin(t_ * 1.8f + float(i)) * 1.5f;
        spr(art_.piling, x, 158.f + bob, 28.f, PAL_WOOD, false, 1, true, false);
    }
    float buoy = std::sin(t_ * 1.4f) * 3.f;
    spr(art_.buoy, 250.f, 118.f + buoy, 18.f, PAL_ALERT, false, 2, true, false);
    for (int i = 0; i < 3; i++) {
        float u = std::fmod(t_ * 0.08f + float(i) * 0.31f, 1.f);
        float gx = 20.f + u * 300.f;
        float gy = 28.f + std::sin(t_ * 1.2f + float(i) * 2.f) * 6.f + float(i) * 8.f;
        spr(art_.gull, gx, gy, 8.f, PAL_FX, (int(t_ * 3.f) + i) & 1, 0, false, false);
    }
    for (int i = 0; i < 5; i++) {
        float u = std::fmod(t_ * 0.15f + float(i) * 0.2f, 1.f);
        spr(art_.foam, 30.f + u * 260.f, 146.f + std::sin(t_ * 2.f + float(i)) * 2.f, 6.f, PAL_FX, i & 1, 2, false,
            false);
    }
    if (mode_ == Mode::Won) sys_->setLight(30, 160, 90);
    else if (mode_ == Mode::Lost) sys_->setLight(160, 28, 18);
    else if (mode_ == Mode::Title) sys_->setLight(30, 50, 120);
    else if (mode_ == Mode::Play && clock_ < 10 * 60) sys_->setLight(170, 36, 20);
    else if (carrying_) sys_->setLight(160, 110, 30);
    else sys_->setLight(40, 80, 140);
}

void Game::messages() {
    if (mode_ == Mode::Title) {
        text("S3 HARBOR CLER", 160.f, 6.f, 0.62f, PAL_GOLD);
        text("CLEAR THE GROUND", 160.f, 24.f, 0.46f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE QUAY IS CLEAR", 160.f, 8.f, 0.46f, PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160.f, 8.f, 0.54f, PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) text("PAUSED", 160.f, 100.f, 0.8f, PAL_GOLD);
    int sec = std::max(0, (clock_ + 59) / 60);
    char clock[12];
    std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
    int pal = (mode_ == Mode::Play && sec <= 10) ? PAL_ALERT : PAL_GOLD;
    text(clock, 160.f, 6.f, 0.9f, pal);
}

void Game::hud() {
    if (mode_ == Mode::Title) {
        if ((sys_->frame / 30) % 2 == 0) tileText(14, 25, "PRESS ENTER", PAL_GOLD);
        tileText(6, 26, "STOW THE LOADS IN THE BOAT", PAL_TEXT);
        return;
    }
    char line[32];
    std::snprintf(line, sizeof line, "LEFT %d", std::max(0, kLoads - stowed_));
    int pal = mode_ == Mode::Won ? PAL_GOOD : (mode_ == Mode::Lost ? PAL_ALERT : PAL_TEXT);
    tileText(32, 0, line, pal);
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return;

    const char* hint = "CLEAR THE GROUND";
    if (carrying_) hint = atBoat() ? "HOLD C TO STOW" : "HAUL IT TO THE BOAT";
    else if (focus_ >= 0) {
        std::snprintf(line, sizeof line, "HOLD C  %s", kindName(load_[focus_].kind));
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
    water();
    messages();
    harbor();
    freight();
    hud();
}

}  // namespace hcler
