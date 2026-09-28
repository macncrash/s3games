#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mill {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 150.f;
constexpr float kCarry = 112.f;
constexpr float kReach = 24.f;
constexpr int kClock = 36 * 60;
constexpr float kL = 108.f, kR = 304.f, kT = 96.f, kB = 206.f;
constexpr float kDoorL = 72.f, kDoorR = 112.f, kDoorT = 148.f, kDoorB = 188.f;
constexpr float kHorizon = 78.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[5] = {
    {168.f, 176.f, 0}, {214.f, 132.f, 1}, {248.f, 188.f, 2}, {286.f, 146.f, 3}, {196.f, 108.f, 0},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int c0, int c1) { return int(std::lround(c0 + (c1 - c0) * t)); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

const gs::Mipped& pileArt(const Art& a, int kind) {
    if (kind == 1) return a.stone;
    if (kind == 2) return a.reed;
    if (kind == 3) return a.chaff;
    return a.sack;
}

int pilePal(int kind) {
    if (kind == 1) return PAL_STONE;
    if (kind == 2) return PAL_REED;
    return PAL_SACK;
}

float pileH(int kind) {
    if (kind == 1) return 16.f;
    if (kind == 2) return 26.f;
    if (kind == 3) return 12.f;
    return 20.f;
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

bool Game::inDoor(float x, float y) const {
    return x >= kDoorL && x <= kDoorR && y >= kDoorT && y <= kDoorB;
}

void Game::resetYard() {
    for (int i = 0; i < kPiles; i++) {
        pile_[i].x = kLay[i].x;
        pile_[i].y = kLay[i].y;
        pile_[i].kind = kLay[i].kind;
        pile_[i].gone = false;
    }
    for (auto& m : mote_) m.life = 0;
    px_ = 140.f;
    py_ = 196.f;
    face_ = 1.f;
    step_ = 0;
    cleared_ = 0;
    carry_ = -1;
    raking_ = false;
    clock_ = kClock;
    shake_ = 0;
}

void Game::toTitle() {
    over_ = false;
    won_ = false;
    reason_ = "";
    blip_ = 0;
    tick_ = 0;
    resetYard();
    mode_ = Mode::Title;
}

void Game::begin() {
    resetYard();
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    blip(196.f, 0.1f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    buildArt(sys.vdp, art_);
    t_ = 0;
    toTitle();
    if (bot_) begin();
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

int Game::nearPile() const {
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float d = dist(px_, py_, pile_[i].x, pile_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0;
    hold = carry_ >= 0;
    int id = carry_;
    if (id < 0) {
        float bd = 1e9f;
        for (int i = 0; i < kPiles; i++) {
            if (pile_[i].gone) continue;
            float d = dist(px_, py_, pile_[i].x, pile_[i].y);
            if (d < bd) {
                bd = d;
                id = i;
            }
        }
    }
    if (id < 0) return;
    float tx = pile_[id].x, ty = pile_[id].y;
    if (carry_ >= 0) {
        tx = 96.f;
        ty = 168.f;
        hold = true;
    } else if (dist(px_, py_, pile_[id].x, pile_[id].y) <= kReach) {
        hold = true;
    }
    float dx = tx - px_, dy = ty - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    if (d < 1.2f) return;
    ix = dx / d;
    iy = dy / d;
}

void Game::humanInput(float& ix, float& iy, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = iy = 0;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (p.down(gs::BTN_UP)) iy -= 1.f;
    if (p.down(gs::BTN_DOWN)) iy += 1.f;
    if (std::fabs(p.axisX) > 0.2f || std::fabs(p.axisY) > 0.2f) {
        ix = p.axisX;
        iy = -p.axisY;
    }
    float m = std::sqrt(ix * ix + iy * iy);
    if (m > 1.f) {
        ix /= m;
        iy /= m;
    }
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
           p.down(gs::BTN_Z);
}

void Game::puff(float x, float y) {
    for (int n = 0; n < 5; n++) {
        for (auto& m : mote_) {
            if (m.life > 0) continue;
            m.x = x;
            m.y = y;
            m.vx = (rnd() - 0.5f) * 60.f;
            m.vy = -16.f - rnd() * 28.f;
            m.life = 0.3f + rnd() * 0.25f;
            break;
        }
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "THE GROUND IS CLEAR";
    blip(523.f, 0.22f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE CLOCK DIED";
    blip(82.f, 0.3f);
}

void Game::updatePlay() {
    float ix, iy;
    bool hold = false;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);

    if (ix != 0.f) face_ = ix < 0 ? -1.f : 1.f;
    float sp = std::sqrt(ix * ix + iy * iy);
    float spd = carry_ >= 0 ? kCarry : kSpeed;
    px_ = std::clamp(px_ + ix * spd * kDt, kL, kR);
    py_ = std::clamp(py_ + iy * spd * kDt, kT, kB);
    if (sp > 0.15f) step_ += kDt * 8.f;

    raking_ = false;
    if (carry_ >= 0 && (pile_[carry_].gone || !hold)) {
        if (!hold) carry_ = -1;
    }
    if (carry_ < 0 && hold) {
        int n = nearPile();
        if (n >= 0) carry_ = n;
    }
    if (carry_ >= 0 && !pile_[carry_].gone) {
        raking_ = true;
        pile_[carry_].x = std::clamp(px_ + face_ * 16.f, 40.f, kR);
        pile_[carry_].y = std::clamp(py_ + 2.f, kT, kB);
        if (inDoor(pile_[carry_].x, pile_[carry_].y)) {
            int id = carry_;
            pile_[id].gone = true;
            carry_ = -1;
            raking_ = false;
            cleared_++;
            puff(pile_[id].x, pile_[id].y);
            blip(300.f + float(cleared_) * 48.f, 0.07f);
            if (cleared_ >= kPiles) win();
        }
    }

    if (mode_ == Mode::Play) {
        clock_--;
        if (clock_ <= 0) {
            clock_ = 0;
            lose();
        }
    }

    for (auto& m : mote_) {
        if (m.life <= 0) continue;
        m.life -= kDt;
        m.x += m.vx * kDt;
        m.y += m.vy * kDt;
    }
    if (shake_ > 0) shake_ -= kDt;
}

void Game::blip(float freq, float hold) {
    blip_ = hold;
    sys_->apu.tone(0, freq, 0.16f);
}

void Game::serviceAudio() {
    if (blip_ > 0) {
        blip_ -= kDt;
        if (blip_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Play && clock_ > 0 && clock_ < 12 * 60) {
        tick_ -= kDt;
        if (tick_ <= 0) {
            tick_ = 0.5f;
            sys_->apu.tone(1, clock_ < 6 * 60 ? 700.f : 392.f, 0.07f);
        }
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (startPressed() || bot_) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updatePlay();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (!bot_ && startPressed()) {
        toTitle();
    }
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
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

void Game::yard(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(4, 7, 12);
    uint16_t skyHor = gs::rgb4(13, 12, 9);
    if (mode_ == Mode::Lost) skyHor = gs::rgb4(10, 4, 3);
    else if (mode_ == Mode::Won) skyHor = gs::rgb4(10, 13, 8);
    v.setFogColor(skyHor);
    v.roadTime = int(t_ * 60.f);
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
        float tt = (float(y) + 0.5f - kHorizon) / span;
        tt = std::max(tt, 0.02f);
        rd.on = true;
        rd.cx = 200.f + shx + std::sin(t_ * 0.3f) * 4.f;
        rd.hw = 70.f + tt * 150.f;
        rd.v = (1.f / tt) * 40.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 0;
        rd.band = (int(std::floor(rd.v * 0.15f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        v.lineFog[y] = uint8_t(std::clamp((1.f / tt - 8.f) / 20.f, 0.f, 1.f) * 8.f);
        v.lineBackdrop[y] = mixC(gs::rgb4(5, 7, 3), gs::rgb4(2, 3, 1), tt);
    }
}

void Game::messages() {
    int pal = mode_ == Mode::Lost ? PAL_ALERT : mode_ == Mode::Won ? PAL_GOOD : PAL_GOLD;
    if (mode_ == Mode::Title) {
        text("S3 MILL CLER", 176.f, 28.f, 0.55f, PAL_GOLD);
        text("CLEAR THE GROUND", 176.f, 52.f, 0.38f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 176.f, 68.f, 0.32f, PAL_TEXT);
        text("HOLD TO RAKE  INTO THE DOOR", 176.f, 210.f, 0.28f, PAL_TEXT);
    } else if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 176.f, 36.f, 0.4f, PAL_GOOD);
    } else if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 176.f, 36.f, 0.42f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 176.f, 36.f, 0.45f, PAL_GOLD);
    } else {
        char buf[16];
        int sec = clock_ / 60;
        std::snprintf(buf, sizeof buf, "%02d", sec);
        text(buf, 286.f, 28.f, 0.5f, clock_ < 10 * 60 ? PAL_ALERT : PAL_CLOCK);
        std::snprintf(buf, sizeof buf, "%d LEFT", kPiles - cleared_);
        text(buf, 70.f, 28.f, 0.36f, pal);
        if (raking_) text("RAKE", px_, py_ - 46.f, 0.28f, PAL_GOLD);
    }
}

void Game::draw() {
    float shx = 0;
    if (shake_ > 0) shx = std::sin(t_ * 40.f) * 2.f;
    sys_->vdp.clearSprites();
    messages();
    int fr = int(step_) & 1;
    spr(art_.shadow, px_ + shx, py_ + 4.f, 14.f, PAL_FX, false, 0, false, true);
    spr(art_.hand[fr], px_ + shx, py_, 42.f, PAL_FIGURE, face_ < 0, 0, true, false);
    spr(art_.rake, px_ + face_ * 16.f + shx, py_ - 8.f, 16.f, PAL_WOOD, face_ < 0, 0, false, false);
    spr(art_.clock, 286.f, 52.f, 28.f, PAL_CLOCK, false, 0, false, false);

    struct Item {
        float y;
        int kind;
        int id;
    };
    Item items[8];
    int n = 0;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        items[n++] = {pile_[i].y, 0, i};
    }
    for (auto& m : mote_) {
        if (m.life > 0 && n < 8) items[n++] = {m.y, 1, int(&m - mote_)};
    }
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.y > b.y; });
    for (int i = 0; i < n; i++) {
        if (items[i].kind == 0) {
            const Pile& p = pile_[items[i].id];
            spr(art_.shadow, p.x + shx, p.y + 3.f, 10.f, PAL_FX, false, 0, false, true);
            spr(pileArt(art_, p.kind), p.x + shx, p.y, pileH(p.kind), pilePal(p.kind), false, 0, true, false);
        } else {
            const Mote& m = mote_[items[i].id];
            spr(art_.dust, m.x, m.y, 8.f + m.life * 8.f, PAL_FX, false, 0, false, false);
        }
    }

    spr(art_.door, 90.f + shx, 186.f, 36.f, mode_ == Mode::Won ? PAL_GOOD : PAL_MILL, false, 0, true, false);
    spr(art_.mill, 62.f + shx, 198.f, 128.f, PAL_MILL, false, 0, true, false);
    int sail = int(t_ * 3.f) & 1;
    spr(art_.sail[sail], 62.f + shx, 78.f, 64.f, PAL_WOOD, false, 0, false, false);
    yard(shx);
}

}  // namespace mill
