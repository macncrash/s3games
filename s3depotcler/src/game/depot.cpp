#include "game/depot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace dcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 126.f;
constexpr float kReach = 20.f;
constexpr float kShove = 0.22f;
constexpr int kClock = 46 * 60;
constexpr float kHorizon = 100.f;
constexpr float kYardL = 36.f, kYardR = 304.f, kYardT = 162.f, kYardB = 214.f;
constexpr float kCarX = 78.f, kCarY = 180.f;
constexpr float kStartX = 210.f, kStartY = 192.f;
constexpr float kCarFoot = 150.f, kCarH = 74.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr int kLayCount = 6;
constexpr Lay kLay[kLayCount] = {
    {128.f, 170.f, 0}, {186.f, 168.f, 1}, {252.f, 172.f, 2},
    {278.f, 206.f, 3}, {188.f, 208.f, 0}, {112.f, 204.f, 1},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float haulNeed(int kind) {
    if (kind == 1) return 0.52f;
    if (kind == 3) return 0.40f;
    if (kind == 0) return 0.30f;
    return 0.22f;
}

const char* kindName(int kind) {
    if (kind == 1) return "DRUM";
    if (kind == 2) return "SACK";
    if (kind == 3) return "BOARDS";
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
    if (kind == 1) return a.drum;
    if (kind == 2) return a.sack;
    if (kind == 3) return a.boards;
    return a.crate;
}

int loadPal(int kind) {
    if (kind == 1) return PAL_STEEL;
    if (kind == 2) return PAL_SACK;
    return PAL_WOOD;
}

float loadH(int kind) {
    if (kind == 1) return 34.f;
    if (kind == 3) return 20.f;
    if (kind == 2) return 30.f;
    return 30.f;
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

void Game::resetApron() {
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
    rng_ = 1;
}

void Game::begin() {
    resetApron();
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
    lifting_ = false;
    shoving_ = false;
    won_ = false;
    over_ = false;
    reason_ = "";
    resetApron();
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

bool Game::atCar() const { return carrying_ && dist(px_, py_, kCarX, kCarY) <= kReach; }

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0.f;
    hold = false;
    float tx = kCarX, ty = kCarY;
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
        d.x = x + (rnd() - 0.5f) * 10.f;
        d.y = y;
        d.vx = (rnd() - 0.5f) * 24.f;
        d.vy = -10.f - rnd() * 16.f;
        d.life = 0.26f + rnd() * 0.2f;
        return;
    }
}

void Game::updateWork(bool hold) {
    if (carrying_) {
        if (!(hold && atCar())) return;
        shoving_ = true;
        shove_ += kDt;
        if (shove_ < kShove) return;
        shove_ = 0.f;
        carrying_ = false;
        shoving_ = false;
        stowed_++;
        shake_ = 0.65f;
        sys_->apu.noiseBurst(0.26f, 240.f, 0.1f);
        blip(480.f + float(stowed_) * 28.f, 0.1f);
        sys_->rumble(0.22f, 0.4f, 48);
        if (stowed_ >= kLoads) win();
        return;
    }
    int id = focusLoad();
    focus_ = id;
    if (id < 0 || !hold) return;
    lifting_ = true;
    Load& m = load_[id];
    m.work += kDt / haulNeed(m.kind);
    if ((sys_->frame & 3) == 0) puff(m.x, m.y - 8.f);
    if (m.work < 1.f) return;
    m.work = 1.f;
    m.taken = true;
    carrying_ = true;
    carried_ = m.kind;
    lifting_ = false;
    shove_ = 0.f;
    blip(330.f, 0.08f);
    sys_->apu.noiseBurst(0.16f, 640.f, 0.07f);
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "THE GROUND IS CLEAR";
    lifting_ = false;
    shoving_ = false;
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
    lifting_ = false;
    shoving_ = false;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->apu.noise(0.f, 200.f);
    sys_->apu.noiseBurst(0.28f, 860.f, 0.15f);
    sys_->setLight(180, 30, 20);
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
    if (moving_ || lifting_) step_ += kDt * (lifting_ ? 3.2f : 1.7f);
    if (mode_ != Mode::Play) return;
    int before = (clock_ + 59) / 60;
    if (clock_ > 0) clock_--;
    int after = (clock_ + 59) / 60;
    if (after != before && after > 0 && after <= 10) {
        sys_->apu.tone(1, 660.f, 0.04f);
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
        static const float good[] = {349.f, 440.f, 523.f, 698.f};
        static const float bad[] = {220.f, 174.f, 131.f};
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
    if (lifting_ || shoving_) sys_->apu.noise(0.06f, lifting_ ? 900.f : 280.f, false);
    else sys_->apu.noise(0.f, 600.f);
}

void Game::fadeDust() {
    for (Dust& d : dust_) {
        if (d.life <= 0.f) continue;
        d.life -= kDt;
        d.x += d.vx * kDt;
        d.y += d.vy * kDt;
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.7f);
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

void Game::apron() {
    gs::VDP& v = sys_->vdp;
    float warm = mode_ == Mode::Won ? 0.6f : 0.f;
    float hot = mode_ == Mode::Lost ? 0.5f : 0.f;
    if (mode_ == Mode::Play && clock_ < 10 * 60) hot = 0.28f;
    uint16_t top = gs::rgb4(1, 2, 5);
    uint16_t mid = gs::rgb4(3, 4, 8);
    uint16_t low = gs::rgb4(8, 5, 3);
    if (warm > 0.f) {
        top = mix4(top, gs::rgb4(6, 5, 3), warm);
        mid = mix4(mid, gs::rgb4(11, 8, 3), warm);
        low = mix4(low, gs::rgb4(14, 10, 4), warm);
    }
    if (hot > 0.f) {
        top = mix4(top, gs::rgb4(6, 1, 2), hot);
        mid = mix4(mid, gs::rgb4(9, 3, 2), hot);
        low = mix4(low, gs::rgb4(8, 3, 2), hot);
    }
    float shx = std::sin(t_ * 48.f) * shake_ * 3.f;
    const float span = float(gs::SCREEN_H) - kHorizon;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = u < 0.62f ? mix4(top, mid, u / 0.62f) : mix4(mid, low, (u - 0.62f) / 0.38f);
            v.lineFog[y] = 0;
            continue;
        }
        float u = (float(y) + 0.5f - kHorizon) / span;
        u = std::clamp(u, 0.f, 1.f);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx;
        rd.hw = 168.f;
        rd.v = float(y) * 18.f;
        rd.pal = uint8_t(PAL_DOCK);
        rd.style = 1;
        rd.band = uint8_t((y / 5) & 1);
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        v.lineFog[y] = uint8_t((1.f - u) * (1.f - u) * 5.f);
        v.lineBackdrop[y] = gs::rgb4(4, 4, 5);
    }
    v.setFogColor(mode_ == Mode::Lost ? gs::rgb4(6, 2, 2) : gs::rgb4(5, 5, 7));
    v.roadTime = int(t_ * 30.f);
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
    bits[n++] = {kCarFoot, 2, 0};
    for (int k = 0; k < stowed_; k++) bits[n++] = {156.f - float(k / 2) * 8.f, 3, k};
    std::sort(bits, bits + n, [](const Bit& a, const Bit& b) { return a.y > b.y; });

    float shx = std::sin(t_ * 48.f) * shake_ * 2.f;
    for (const Dust& d : dust_) {
        if (d.life <= 0.f) continue;
        spr(art_.dust, d.x + shx, d.y, 6.f + d.life * 12.f, PAL_FX, false, 0, false, false);
    }

    float pulse = 1.f + 0.04f * std::sin(t_ * 9.f);
    bool showMark = carrying_ && (mode_ == Mode::Play || mode_ == Mode::Pause);
    for (int i = 0; i < n; i++) {
        const Bit& b = bits[i];
        if (b.kind == 0) {
            int fr = (moving_ || lifting_) ? (int(step_ * 8.f) & 1) : 0;
            float bob = (moving_ || lifting_) ? 0.f : std::sin(t_ * 2.1f) * 1.f;
            bool flip = face_ < 0.f;
            float foot = py_ + bob;
            spr(art_.clerk[fr], px_ + shx, foot, 52.f, PAL_CLERK, flip, 0, true, false);
            float dx = face_ * 16.f;
            if (carrying_) {
                const gs::Mipped& piece = loadArt(art_, carried_);
                float lean = shoving_ ? std::sin(shove_ * 40.f) * 3.f : 0.f;
                spr(piece, px_ + shx + dx, foot - 22.f + lean, 16.f, loadPal(carried_), flip, 0, true, false);
            }
            spr(art_.dolly, px_ + shx + dx, foot + (shoving_ ? 1.f : 0.f), 28.f, PAL_STEEL, flip, 0, true, false);
            spr(art_.shadow, px_ + shx, foot + 2.f, 9.f, PAL_FX, false, 0, false, true);
            continue;
        }
        if (b.kind == 2) {
            if (showMark) {
                float bob = std::sin(t_ * 8.f) * 3.f;
                spr(art_.mark, kCarX + shx, kCarY - 46.f + bob, 12.f, atCar() ? PAL_GOOD : PAL_GOLD, false, 0, false,
                    false);
            }
            spr(art_.car, kCarX + shx, kCarFoot, kCarH, PAL_CAR, false, 0, true, false);
            spr(art_.shadow, kCarX + shx, kCarFoot + 2.f, 12.f, PAL_FX, false, 0, false, true);
            continue;
        }
        if (b.kind == 3) {
            float ox = (b.id % 2 == 0 ? -8.f : 8.f);
            float oy = float(b.id / 2) * 8.f;
            spr(art_.cargo, kCarX + ox + shx, 156.f - oy, 14.f, PAL_WOOD, b.id & 1, 0, true, false);
            continue;
        }
        const Load& m = load_[b.id];
        if (m.taken) continue;
        float h = loadH(m.kind) * (1.f - m.work * 0.35f);
        if (b.id == focus_ && mode_ == Mode::Play) h *= pulse;
        spr(loadArt(art_, m.kind), m.x + shx, m.y, std::max(8.f, h), loadPal(m.kind), false, 0, true, false);
        spr(art_.shadow, m.x + shx, m.y + 2.f, 8.f, PAL_FX, false, 0, false, true);
    }
}

void Game::depot() {
    int lampPal = PAL_GOLD;
    if (mode_ == Mode::Won) lampPal = PAL_GOOD;
    else if (mode_ == Mode::Lost || (mode_ == Mode::Play && clock_ < 10 * 60)) lampPal = PAL_ALERT;
    int clockPal = lampPal;
    float bob = std::sin(t_ * 4.f) * 1.f;
    spr(art_.clock, 168.f, 46.f, 28.f, clockPal, false, 0, false, false);
    spr(art_.lamp, 28.f, 78.f + bob, 26.f, lampPal, false, 0, false, false);
    spr(art_.lamp, 300.f, 80.f + bob, 24.f, lampPal, true, 0, false, false);
    spr(art_.truck, 286.f, 146.f, 32.f, PAL_STEEL, false, 2, true, false);
    spr(art_.stack, 236.f, 142.f, 28.f, PAL_WOOD, false, 2, true, false);
    spr(art_.chimney, 248.f, 78.f, 64.f, PAL_BRICK, false, 1, true, false);
    spr(art_.rails, 160.f, 156.f, 12.f, PAL_STEEL, false, 1, true, false);
    for (int i = 0; i < 3; i++) {
        float u = std::fmod(t_ * 0.22f + float(i) * 0.33f, 1.f);
        spr(art_.steam, 248.f + std::sin(t_ * 1.4f + float(i)) * 6.f, 20.f - u * 16.f, 8.f + u * 10.f, PAL_FX, i & 1,
            int(u * 8.f), false, false);
    }
    spr(art_.wall, 160.f, 128.f, 96.f, PAL_BRICK, false, 0, true, false);

    if (mode_ == Mode::Won) sys_->setLight(40, 180, 70);
    else if (mode_ == Mode::Lost) sys_->setLight(180, 30, 20);
    else if (mode_ == Mode::Title) sys_->setLight(40, 48, 110);
    else if (mode_ == Mode::Play && clock_ < 10 * 60) sys_->setLight(180, 40, 24);
    else if (carrying_) sys_->setLight(180, 120, 30);
    else sys_->setLight(120, 78, 28);
}

void Game::messages() {
    if (mode_ == Mode::Title) {
        text("S3 DEPOT CLER", 160.f, 8.f, 0.72f, PAL_GOLD);
        text("CLEAR THE GROUND", 160.f, 26.f, 0.52f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160.f, 10.f, 0.48f, PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160.f, 10.f, 0.58f, PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) text("PAUSED", 160.f, 108.f, 0.86f, PAL_GOLD);
    int sec = std::max(0, (clock_ + 59) / 60);
    char clock[12];
    std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
    int pal = (mode_ == Mode::Play && sec <= 10) ? PAL_ALERT : PAL_GOLD;
    text(clock, 160.f, 8.f, 0.95f, pal);
}

void Game::hud() {
    if (mode_ == Mode::Title) {
        if ((sys_->frame / 30) % 2 == 0) tileText(14, 25, "PRESS ENTER", PAL_GOLD);
        tileText(7, 26, "HAUL FREIGHT INTO THE CAR", PAL_TEXT);
        return;
    }
    char line[28];
    std::snprintf(line, sizeof line, "LEFT %d", std::max(0, kLoads - stowed_));
    int pal = mode_ == Mode::Won ? PAL_GOOD : (mode_ == Mode::Lost ? PAL_ALERT : PAL_TEXT);
    tileText(32, 0, line, pal);
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return;

    const char* hint = "CLEAR THE GROUND";
    if (carrying_) hint = atCar() ? "HOLD C TO STOW" : "HAUL IT TO THE CAR";
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
    apron();
    messages();
    freight();
    depot();
    hud();
}

}  // namespace dcler
