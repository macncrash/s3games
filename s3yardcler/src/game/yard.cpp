#include "game/yard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace yardcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 170.f;
constexpr float kReach = 24.f;
constexpr float kTip = 0.18f;
constexpr int kClock = 42 * 60;
constexpr float kYardL = 42.f, kYardR = 286.f, kYardT = 148.f, kYardB = 204.f;
constexpr float kBinX = 54.f, kBinY = 170.f;
constexpr float kStartX = 154.f, kStartY = 178.f;
constexpr float kHouseX = 158.f, kHouseY = 124.f, kHouseH = 76.f;
constexpr float kTreeX = 304.f, kTreeY = 156.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {116.f, 164.f, 0}, {186.f, 156.f, 1}, {240.f, 178.f, 2},
    {130.f, 196.f, 3}, {208.f, 198.f, 4}, {80.f, 186.f, 5},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float rakeNeed(int kind) {
    if (kind == 0) return 0.40f;
    if (kind == 1) return 0.26f;
    if (kind == 2) return 0.48f;
    if (kind == 3) return 0.22f;
    if (kind == 4) return 0.32f;
    return 0.36f;
}

const char* kindName(int kind) {
    if (kind == 1) return "STICKS";
    if (kind == 2) return "BRANCH";
    if (kind == 3) return "TOYS";
    if (kind == 4) return "CLIPPINGS";
    if (kind == 5) return "STONES";
    return "LEAVES";
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
    if (kind == 1) return a.sticks;
    if (kind == 2) return a.branch;
    if (kind == 3) return a.toys;
    if (kind == 4) return a.clippings;
    if (kind == 5) return a.stones;
    return a.leaves;
}

int pilePal(int kind) {
    if (kind == 1 || kind == 2) return PAL_WOOD;
    if (kind == 3) return PAL_TOY;
    if (kind == 4) return PAL_TREE;
    if (kind == 5) return PAL_STONE;
    return PAL_LEAF;
}

float pileH(int kind) {
    if (kind == 1) return 16.f;
    if (kind == 2) return 18.f;
    if (kind == 3) return 24.f;
    if (kind == 4) return 20.f;
    if (kind == 5) return 14.f;
    return 20.f;
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

void Game::resetYard() {
    for (int i = 0; i < kPiles; i++) {
        pile_[i].x = kLay[i].x;
        pile_[i].y = kLay[i].y;
        pile_[i].kind = kLay[i].kind;
        pile_[i].work = 0.f;
        pile_[i].taken = false;
    }
    for (Dust& d : dust_) d.life = 0.f;
    carrying_ = false;
    moving_ = false;
    raking_ = false;
    tipping_ = false;
    carried_ = 0;
    stowed_ = 0;
    focus_ = -1;
    px_ = kStartX;
    py_ = kStartY;
    face_ = 1.f;
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

bool Game::startPressed() const { return sys_->pad.pressed(gs::BTN_START); }

bool Game::confirmPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) ||
           p.pressed(gs::BTN_TURBO);
}

bool Game::modePressed() const { return sys_->pad.pressed(gs::BTN_MODE); }

int Game::focusPile() const {
    if (carrying_) return -1;
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].taken) continue;
        float d = dist(px_, py_, pile_[i].x, pile_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

bool Game::atBin() const { return carrying_ && dist(px_, py_, kBinX, kBinY) <= kReach; }

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0.f;
    hold = false;
    float tx = kBinX, ty = kBinY;
    if (!carrying_) {
        int id = 0;
        while (id < kPiles && pile_[id].taken) id++;
        if (id >= kPiles) return;
        tx = pile_[id].x;
        ty = pile_[id].y;
    }
    float dx = tx - px_, dy = ty - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    hold = d <= kReach;
    if (d < 1.6f) {
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
           p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO) || p.accel > 0.4f || p.brake > 0.4f;
}

void Game::move(float ix, float iy, bool hold) {
    bool planted = hold && (carrying_ ? atBin() : focusPile() >= 0);
    if (planted) {
        float tx = carrying_ ? kBinX : pile_[focusPile()].x;
        if (tx < px_ - 1.f) face_ = -1.f;
        else if (tx > px_ + 1.f) face_ = 1.f;
        moving_ = false;
        return;
    }
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
        d.vx = (rnd() - 0.5f) * 28.f;
        d.vy = -12.f - rnd() * 18.f;
        d.life = 0.28f + rnd() * 0.18f;
        return;
    }
}

void Game::updateWork(bool hold) {
    if (carrying_) {
        if (!(hold && atBin())) {
            tip_ = 0.f;
            return;
        }
        tipping_ = true;
        tip_ += kDt;
        if ((sys_->frame & 3) == 0) puff(kBinX, kBinY - 20.f);
        if (tip_ < kTip) return;
        tip_ = 0.f;
        carrying_ = false;
        tipping_ = false;
        stowed_++;
        shake_ = 0.55f;
        sys_->apu.noiseBurst(0.22f, 180.f, 0.1f);
        blip(440.f + float(stowed_) * 32.f, 0.1f);
        sys_->rumble(0.18f, 0.32f, 40);
        if (stowed_ >= kPiles) win();
        return;
    }
    int id = focusPile();
    focus_ = id;
    if (id < 0 || !hold) {
        if (id >= 0) pile_[id].work = std::max(0.f, pile_[id].work - kDt * 0.45f);
        return;
    }
    raking_ = true;
    Pile& m = pile_[id];
    m.work += kDt / rakeNeed(m.kind);
    if ((sys_->frame & 3) == 0) puff(m.x, m.y - 6.f);
    if (m.work < 1.f) return;
    m.work = 1.f;
    m.taken = true;
    carrying_ = true;
    carried_ = m.kind;
    raking_ = false;
    tip_ = 0.f;
    blip(294.f, 0.08f);
    sys_->apu.noiseBurst(0.14f, 520.f, 0.06f);
    sys_->rumble(0.1f, 0.16f, 28);
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
    sys_->apu.noiseBurst(0.3f, 140.f, 0.16f);
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
    sys_->apu.noiseBurst(0.26f, 700.f, 0.14f);
    sys_->setLight(180, 30, 20);
}

void Game::updatePlay() {
    raking_ = false;
    tipping_ = false;
    float ix = 0.f, iy = 0.f;
    bool hold = false;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);
    move(ix, iy, hold);
    focus_ = focusPile();
    updateWork(hold);
    if (moving_ || raking_) step_ += kDt * (raking_ ? 4.f : 2.f);
    if (mode_ != Mode::Play) return;
    int before = (clock_ + 59) / 60;
    if (clock_ > 0) clock_--;
    int after = (clock_ + 59) / 60;
    if (after != before && after > 0) {
        bool hot = after <= 10;
        sys_->apu.tone(1, hot ? 784.f : 392.f, hot ? 0.05f : 0.018f);
        tick_ = hot ? 0.07f : 0.03f;
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
        if (fanT_ < 0.13f) return;
        fanT_ = 0.f;
        static const float good[] = {392.f, 494.f, 587.f, 784.f};
        static const float bad[] = {311.f, 247.f, 196.f};
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
    if (mode_ == Mode::Play && (raking_ || tipping_))
        sys_->apu.noise(raking_ ? 0.05f : 0.07f, raking_ ? 740.f : 160.f, false);
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
        if (confirmPressed()) begin();
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
        tipping_ = false;
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    float warm = mode_ == Mode::Won ? 0.65f : 0.f;
    float hot = mode_ == Mode::Lost ? 0.55f : 0.f;
    if (mode_ == Mode::Play && clock_ <= 10 * 60) hot = 0.35f;
    uint16_t top = gs::rgb4(5, 8, 14);
    uint16_t mid = gs::rgb4(9, 13, 15);
    uint16_t low = gs::rgb4(14, 12, 8);
    if (warm > 0.f) {
        top = mix4(top, gs::rgb4(12, 8, 3), warm);
        mid = mix4(mid, gs::rgb4(15, 12, 6), warm);
        low = mix4(low, gs::rgb4(14, 10, 4), warm);
    }
    if (hot > 0.f) {
        top = mix4(top, gs::rgb4(7, 2, 3), hot);
        mid = mix4(mid, gs::rgb4(11, 4, 3), hot);
        low = mix4(low, gs::rgb4(8, 3, 2), hot);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = float(y) / 96.f;
        if (u < 1.f) {
            v.lineBackdrop[y] = u < 0.55f ? mix4(top, mid, u / 0.55f) : mix4(mid, low, (u - 0.55f) / 0.45f);
        } else {
            v.lineBackdrop[y] = gs::rgb4(4, 8, 2);
        }
    }
    v.A.enabled = false;
    v.B.enabled = true;
    v.setFogColor(hot > 0.f ? gs::rgb4(6, 2, 2) : gs::rgb4(8, 10, 12));
}

void Game::messages() {
    if (mode_ == Mode::Title) {
        text("S3 YARD CLER", 160.f, 11.f, 0.62f, PAL_GOLD);
        text("CLEAR THE GROUND", 160.f, 28.f, 0.42f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160.f, 10.f, 0.40f, PAL_GOOD);
        text("THEN IT IS DONE", 160.f, 24.f, 0.36f, PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160.f, 12.f, 0.52f, PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) text("PAUSED", 160.f, 86.f, 0.7f, PAL_GOLD);
    int sec = std::max(0, (clock_ + 59) / 60);
    char clock[12];
    std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
    int pal = (mode_ == Mode::Play && sec <= 10) ? PAL_ALERT : PAL_GOLD;
    float sc = 0.72f;
    if (mode_ == Mode::Play && sec <= 10) sc += 0.04f * std::sin(t_ * 9.f);
    text(clock, 176.f, 12.f, sc, pal);
}

void Game::world() {
    struct Bit {
        float key;
        int kind;
        int id;
    };
    Bit bits[72];
    int n = 0;
    auto add = [&](float key, int kind, int id) {
        if (n < 72) bits[n++] = Bit{key, kind, id};
    };

    add(18.f, 12, 0);
    add(30.f, 12, 1);
    add(22.f, 12, 2);
    add(36.f, 12, 3);
    add(kHouseY, 7, 0);
    add(kTreeY, 8, 0);
    add(132.f, 9, 0);
    add(168.f, 9, 1);
    add(204.f, 9, 2);
    add(132.f, 9, 3);
    add(168.f, 9, 4);
    add(204.f, 9, 5);
    add(128.f, 9, 6);
    add(128.f, 9, 7);
    add(136.f, 10, 0);
    add(134.f, 10, 1);
    add(138.f, 10, 2);
    add(140.f, 11, 0);
    add(138.f, 11, 1);
    add(112.f, 11, 2);
    add(kBinY, 4, 0);
    add(py_, 0, 0);
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].taken) add(pile_[i].y - 2.f, 3, i);
        else {
            add(pile_[i].y - 1.f, 2, i);
            add(pile_[i].y, 1, i);
        }
    }
    for (int k = 0; k < stowed_; k++) add(kBinY + 1.f, 5, k);
    bool showAim = mode_ == Mode::Play || mode_ == Mode::Pause;
    if (showAim && carrying_) add(kBinY + 8.f, 6, 1);
    if (showAim && focus_ >= 0) add(pile_[focus_].y + 6.f, 6, 0);
    for (int i = 0; i < 3; i++) {
        float u = std::fmod(t_ * 0.07f + float(i) * 0.33f, 1.f);
        add(146.f + float(i) * 12.f, 13, i);
        (void)u;
    }

    std::stable_sort(bits, bits + n, [](const Bit& a, const Bit& b) { return a.key > b.key; });
    float shx = std::sin(t_ * 46.f) * shake_ * 2.4f;
    float pulse = 1.f + 0.05f * std::sin(t_ * 8.f);

    for (const Dust& d : dust_) {
        if (d.life <= 0.f) continue;
        spr(art_.dust, d.x + shx, d.y, 6.f + d.life * 10.f, PAL_LEAF, false, 0, false, false);
    }

    for (int i = 0; i < n; i++) {
        const Bit& b = bits[i];
        if (b.kind == 12) {
            if (b.id == 0) {
                spr(art_.sun, 292.f, 20.f, 26.f, PAL_GOLD, false, 0, false, false);
            } else if (b.id == 1) {
                float x = std::fmod(30.f + t_ * 6.f, 400.f) - 40.f;
                spr(art_.cloud, x, 16.f, 16.f, PAL_TEXT, false, 0, false, false);
            } else if (b.id == 2) {
                float x = std::fmod(180.f + t_ * 4.f, 420.f) - 50.f;
                spr(art_.cloud, x, 34.f, 12.f, PAL_TEXT, true, 2, false, false);
            } else {
                float x = std::fmod(t_ * 28.f + 40.f, 380.f) - 30.f;
                int flap = int(t_ * 8.f) & 1;
                spr(art_.bird[flap], x, 14.f + std::sin(t_ * 2.f) * 2.f, 12.f, PAL_BIRD, false, 0, false, false);
            }
            continue;
        }
        if (b.kind == 13) {
            float u = std::fmod(t_ * 0.07f + float(b.id) * 0.33f, 1.f);
            float x = -16.f + u * 350.f;
            float y = 150.f + std::sin(t_ * 1.4f + float(b.id)) * 10.f;
            spr(art_.dust, x, y, 7.f, PAL_LEAF, b.id & 1, 0, false, false);
            continue;
        }
        if (b.kind == 7) {
            spr(art_.house, kHouseX + shx, kHouseY, kHouseH, PAL_HOUSE, false, 0, true, false);
            continue;
        }
        if (b.kind == 8) {
            spr(art_.tree, kTreeX + shx, kTreeY, 102.f, PAL_TREE, false, 0, true, false);
            spr(art_.shadow, kTreeX + shx, kTreeY + 2.f, 14.f, PAL_FX, false, 0, false, true);
            continue;
        }
        if (b.kind == 9) {
            static const float fx[8] = {16.f, 16.f, 16.f, 306.f, 306.f, 306.f, 46.f, 274.f};
            static const float fy[8] = {150.f, 186.f, 214.f, 150.f, 186.f, 214.f, 128.f, 128.f};
            spr(art_.fence, fx[b.id] + shx, fy[b.id], 36.f, PAL_HOUSE, false, 0, true, false);
            continue;
        }
        if (b.kind == 10) {
            static const float x[3] = {102.f, 168.f, 214.f};
            float bob = std::sin(t_ * 1.7f + float(b.id)) * 1.f;
            spr(art_.flower, x[b.id] + shx, 136.f + bob, 18.f, PAL_BLOOM, false, 0, true, false);
            continue;
        }
        if (b.kind == 11) {
            if (b.id == 0) spr(art_.hose, 198.f + shx, 142.f, 16.f, PAL_TREE, false, 0, true, false);
            else if (b.id == 1) spr(art_.can, 126.f + shx, 140.f, 18.f, PAL_STONE, false, 0, true, false);
            else spr(art_.line, 214.f + shx, 108.f, 18.f, PAL_KEEPER, false, 0, false, false);
            continue;
        }
        if (b.kind == 4) {
            spr(art_.bin, kBinX + shx, kBinY, 46.f, PAL_WOOD, false, 0, true, false);
            spr(art_.shadow, kBinX + shx, kBinY + 2.f, 10.f, PAL_FX, false, 0, false, true);
            continue;
        }
        if (b.kind == 5) {
            float ox = (b.id % 3 - 1) * 6.f;
            float oy = float(b.id / 3) * 4.f;
            spr(art_.leaves, kBinX + ox + shx, kBinY - 26.f - oy, 10.f, PAL_LEAF, b.id & 1, 0, true, false);
            continue;
        }
        if (b.kind == 6) {
            float bob = std::sin(t_ * 7.f) * 3.f;
            if (b.id == 0 && focus_ >= 0) {
                spr(art_.aim, pile_[focus_].x + shx, pile_[focus_].y - pileH(pile_[focus_].kind) - 8.f + bob, 10.f,
                    PAL_GOOD, false, 0, false, false);
            } else if (b.id == 1) {
                spr(art_.aim, kBinX + shx, kBinY - 52.f + bob, 11.f, atBin() ? PAL_GOOD : PAL_GOLD, false, 0, false,
                    false);
            }
            continue;
        }
        if (b.kind == 3) {
            const Pile& m = pile_[b.id];
            spr(art_.raked, m.x + shx, m.y - 2.f, 14.f, PAL_TREE, false, 0, false, false);
            continue;
        }
        if (b.kind == 1) {
            const Pile& m = pile_[b.id];
            float h = pileH(m.kind) * (1.f - m.work * 0.4f);
            if (b.id == focus_ && mode_ == Mode::Play) h *= pulse;
            spr(pileArt(art_, m.kind), m.x + shx, m.y, std::max(8.f, h), pilePal(m.kind), false, 0, true, false);
            continue;
        }
        if (b.kind == 2) {
            const Pile& m = pile_[b.id];
            spr(art_.shadow, m.x + shx, m.y + 1.f, 7.f, PAL_FX, false, 0, false, true);
            continue;
        }
        if (b.kind == 0) {
            int fr = raking_ ? 2 : ((moving_ ? int(step_ * 6.f) : 0) & 1);
            float bob = moving_ ? 0.f : std::sin(t_ * 2.f) * 1.f;
            bool flip = face_ < 0.f;
            float foot = py_ + bob;
            float side = face_ * 16.f;
            if (carrying_) {
                float lean = tipping_ ? std::sin(tip_ * 28.f) * 3.f : 0.f;
                spr(pileArt(art_, carried_), px_ + shx + side, foot - 16.f + lean, 12.f, pilePal(carried_), flip, 0,
                    true, false);
            }
            spr(art_.keeper[fr], px_ + shx, foot, 56.f, PAL_KEEPER, flip, 0, true, false);
            float tipy = tipping_ ? -3.f : 0.f;
            spr(art_.barrow, px_ + shx + side, foot + tipy, 26.f, PAL_WOOD, flip, 0, true, false);
            spr(art_.shadow, px_ + shx, foot + 2.f, 8.f, PAL_FX, false, 0, false, true);
            if (mode_ == Mode::Play || mode_ == Mode::Pause)
                spr(art_.clock, 128.f, 14.f, 16.f, PAL_GOLD, false, 0, false, false);
        }
    }
}

void Game::hud() {
    if (mode_ == Mode::Title) {
        if ((sys_->frame / 30) & 1) tileText(13, 25, "PRESS ENTER", PAL_GOLD);
        tileText(6, 26, "RAKE THE YARD INTO THE BIN", PAL_TEXT);
        tileText(1, 27, "ONE YARD", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        int col = 39 - int(std::strlen(ver));
        if (col < 1) col = 1;
        tileText(col, 27, ver, PAL_TEXT);
        return;
    }
    char line[32];
    std::snprintf(line, sizeof line, "LEFT %d", std::max(0, kPiles - stowed_));
    int pal = mode_ == Mode::Won ? PAL_GOOD : (mode_ == Mode::Lost ? PAL_ALERT : PAL_TEXT);
    tileText(1, 0, line, pal);
    if (mode_ == Mode::Won) {
        tileText(13, 26, "THEN IT IS DONE", PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Lost) {
        tileText(10, 26, "THE YARD IS NOT DONE", PAL_ALERT);
        return;
    }
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return;

    const char* hint = "CLEAR THE GROUND";
    if (carrying_) hint = atBin() ? "HOLD C TO TIP" : "WHEEL IT TO THE BIN";
    else if (focus_ >= 0) {
        std::snprintf(line, sizeof line, "HOLD C  %s", kindName(pile_[focus_].kind));
        hint = line;
    }
    int col = 20 - int(std::strlen(hint)) / 2;
    int hpal = (mode_ == Mode::Play && clock_ <= 10 * 60) ? PAL_ALERT : PAL_GOLD;
    tileText(col, 26, hint, hpal);
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(40, 180, 70);
    else if (mode_ == Mode::Lost) sys_->setLight(180, 30, 20);
    else if (mode_ == Mode::Title) sys_->setLight(40, 110, 36);
    else if (mode_ == Mode::Play && clock_ <= 10 * 60) sys_->setLight(180, 40, 24);
    else if (carrying_) sys_->setLight(170, 120, 28);
    else sys_->setLight(70, 140, 36);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    messages();
    world();
    hud();
    lamp();
}

}  // namespace yardcler
