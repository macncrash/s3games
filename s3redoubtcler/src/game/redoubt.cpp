#include "game/redoubt.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 156.f;
constexpr float kReach = 26.f;
constexpr int kClock = 48 * 60;
constexpr float kYardL = 22.f, kYardR = 304.f, kYardT = 164.f, kYardB = 212.f;
constexpr float kStartX = 160.f, kStartY = 198.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[7] = {
    {48.f, 176.f, 0},  {96.f, 204.f, 1},  {132.f, 172.f, 2}, {176.f, 208.f, 3},
    {214.f, 174.f, 4}, {258.f, 202.f, 5}, {294.f, 178.f, 6},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float workNeed(int kind) {
    if (kind == 2) return 0.62f;
    if (kind == 4) return 0.50f;
    if (kind == 0) return 0.42f;
    if (kind == 5) return 0.36f;
    if (kind == 1) return 0.30f;
    if (kind == 6) return 0.28f;
    return 0.22f;
}

const char* kindName(int kind) {
    if (kind == 1) return "FASCINE";
    if (kind == 2) return "GABION";
    if (kind == 3) return "STAKES";
    if (kind == 4) return "KEG";
    if (kind == 5) return "WHEEL";
    if (kind == 6) return "CRATE";
    return "SPOIL";
}

const gs::Mipped& foulArt(const Art& a, int kind) {
    if (kind == 1) return a.fascine;
    if (kind == 2) return a.gabion;
    if (kind == 3) return a.stake;
    if (kind == 4) return a.keg;
    if (kind == 5) return a.wheel;
    if (kind == 6) return a.crate;
    return a.spoil;
}

int foulPal(int kind) {
    if (kind == 1 || kind == 3) return PAL_WOOD;
    if (kind == 2) return PAL_WICKER;
    if (kind == 4) return PAL_POWDER;
    if (kind == 5) return PAL_IRON;
    if (kind == 6) return PAL_WOOD;
    return PAL_EARTH;
}

float foulH(int kind) {
    if (kind == 2) return 32.f;
    if (kind == 4) return 28.f;
    if (kind == 5) return 26.f;
    if (kind == 1) return 16.f;
    if (kind == 6) return 22.f;
    if (kind == 3) return 24.f;
    return 20.f;
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

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (cleared_ >= 5) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

void Game::resetYard() {
    for (int i = 0; i < kFouls; i++) {
        foul_[i].x = kLay[i].x;
        foul_[i].y = kLay[i].y;
        foul_[i].kind = kLay[i].kind;
        foul_[i].work = 0.f;
        foul_[i].gone = false;
    }
    for (Mote& m : mote_) m.life = 0.f;
    working_ = false;
    moving_ = false;
    focus_ = -1;
    cleared_ = 0;
    px_ = kStartX;
    py_ = kStartY;
    face_ = -1.f;
    step_ = 0.f;
    shake_ = 0.f;
    swing_ = 0.f;
    clock_ = kClock;
    rng_ = 7;
}

void Game::begin() {
    resetYard();
    won_ = false;
    over_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    blip(294.f, 0.12f);
}

void Game::toTitle() {
    if (sys_) {
        sys_->apu.silence();
        sys_->apu.noise(0.f, 400.f);
    }
    blip_ = 0.f;
    tick_ = 0.f;
    working_ = false;
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

int Game::focusFoul() const {
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kFouls; i++) {
        if (foul_[i].gone) continue;
        float d = dist(px_, py_, foul_[i].x, foul_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0.f;
    hold = false;
    int id = -1;
    float best = 1e9f;
    for (int i = 0; i < kFouls; i++) {
        if (foul_[i].gone) continue;
        float d = dist(px_, py_, foul_[i].x, foul_[i].y);
        if (d < best) {
            best = d;
            id = i;
        }
    }
    if (id < 0) return;
    float dx = foul_[id].x - px_, dy = foul_[id].y - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    hold = d <= kReach;
    if (d < 1.2f) return;
    ix = dx / d;
    iy = dy / d;
    if (hold) {
        ix *= 0.12f;
        iy *= 0.12f;
    }
}

void Game::humanInput(float& ix, float& iy, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = iy = 0.f;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (p.down(gs::BTN_UP)) iy -= 1.f;
    if (p.down(gs::BTN_DOWN)) iy += 1.f;
    if (std::fabs(p.axisX) + std::fabs(p.axisY) > 0.2f) {
        ix = p.axisX;
        iy = -p.axisY;
    }
    float m = std::sqrt(ix * ix + iy * iy);
    if (m > 1.f) {
        ix /= m;
        iy /= m;
    }
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X);
}

void Game::move(float ix, float iy) {
    moving_ = std::fabs(ix) + std::fabs(iy) > 0.05f;
    if (std::fabs(ix) > 0.15f) face_ = ix < 0.f ? -1.f : 1.f;
    px_ = std::clamp(px_ + ix * kSpeed * kDt, kYardL, kYardR);
    py_ = std::clamp(py_ + iy * kSpeed * kDt, kYardT, kYardB);
    if (moving_) step_ += kDt * 8.f;
}

void Game::puff(float x, float y) {
    for (int n = 0; n < 3; n++) {
        for (Mote& m : mote_) {
            if (m.life > 0.f) continue;
            m.x = x;
            m.y = y;
            m.vx = (rnd() - 0.5f) * 48.f;
            m.vy = -18.f - rnd() * 28.f;
            m.life = 0.3f + rnd() * 0.28f;
            break;
        }
    }
}

void Game::updateWork(bool hold) {
    focus_ = focusFoul();
    working_ = false;
    if (focus_ < 0) return;
    Foul& f = foul_[focus_];
    if (hold) {
        working_ = true;
        swing_ += kDt * 10.f;
        f.work += kDt / workNeed(f.kind);
        if (int(swing_ * 2.f) != int((swing_ - kDt * 10.f) * 2.f)) puff(f.x, f.y - 6.f);
        if (f.work >= 1.f) {
            f.gone = true;
            cleared_++;
            shake_ = 3.5f;
            puff(f.x, f.y);
            blip(cleared_ == kFouls ? 523.f : 349.f, 0.1f);
            focus_ = -1;
            working_ = false;
        }
    } else if (f.work > 0.f) {
        f.work = std::max(0.f, f.work - kDt * 0.22f);
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "GROUND CLEAR";
    blip(659.f, 0.28f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE WATCH IS OVER";
    blip(98.f, 0.32f);
}

void Game::updatePlay() {
    float ix, iy;
    bool hold;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);
    move(ix, iy);
    updateWork(hold);
    if (clock_ > 0) clock_--;
    if (cleared_ >= kFouls) win();
    else if (clock_ <= 0) lose();
    if (clock_ > 0 && clock_ < 8 * 60 && (clock_ % 60) == 0) blip(740.f, 0.04f);
}

void Game::blip(float freq, float hold) {
    blip_ = hold;
    tick_ = freq;
}

void Game::serviceAudio() {
    if (!sys_) return;
    if (blip_ > 0.f) {
        sys_->apu.tone(0, tick_, 0.16f);
        blip_ -= kDt;
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (working_) sys_->apu.noise(0.1f, 1400.f);
    else sys_->apu.noise(0.f, 400.f);
}

void Game::fadeMotes() {
    for (Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        m.life -= kDt;
        m.x += m.vx * kDt;
        m.y += m.vy * kDt;
        m.vy += 46.f * kDt;
    }
    if (shake_ > 0.f) shake_ -= kDt * 8.f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (startPressed()) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updatePlay();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (startPressed() && !bot_) {
        toTitle();
    }
    fadeMotes();
    serviceAudio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.img = m.pick(h);
    float sc = h / float(m.h);
    s.w = std::max(1, int(std::lround(m.w * sc)));
    s.h = std::max(1, int(std::lround(h)));
    float sh = shake_ > 0.f ? (rnd() - 0.5f) * shake_ : 0.f;
    s.x = int(std::lround(cx - s.w * 0.5f + sh));
    s.y = int(std::lround((feet ? cy - s.h : cy) + sh * 0.2f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float cx = x;
    for (; *s; ++s) {
        unsigned char ch = (unsigned char)*s;
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        int gi = (ch >= 32 && ch < 128) ? ch - 32 : 0;
        float h = 7.f * scale;
        float w = 8.f * scale;
        spr(art_.glyph[gi], cx + w * 0.5f, y, h, pal, false, 0, false);
        cx += 6.f * scale;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        uint16_t sky = mix4(gs::rgb4(4, 2, 6), gs::rgb4(12, 5, 2), std::min(1.f, t * 1.15f));
        if (y > 140) sky = mix4(gs::rgb4(8, 4, 2), gs::rgb4(6, 5, 2), (y - 140) / 80.f);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = y < 40 ? uint8_t(3) : 0;
        v.road[y].on = false;
    }
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = false;
}

void Game::works() {
    float wave = std::sin(t_ * 1.6f) * 2.f;
    spr(art_.rampart, 160.f, 128.f, 78.f, PAL_EARTH, false, 1, true);
    spr(art_.merlon, 70.f, 78.f, 20.f, PAL_EARTH, false, 1, true);
    spr(art_.merlon, 250.f, 78.f, 20.f, PAL_EARTH, false, 1, true);
    spr(art_.embrasure, 160.f, 92.f, 12.f, PAL_SHADE, false, 0, false);
    spr(art_.flag, 40.f + wave, 70.f, 34.f, PAL_FLAG, false, 0, true);
    spr(art_.revet, 28.f, 168.f, 40.f, PAL_WOOD, false, 0, true);
    spr(art_.revet, 300.f, 166.f, 38.f, PAL_WOOD, false, 0, true);
}

void Game::fouls() {
    int order[kFouls];
    for (int i = 0; i < kFouls; i++) order[i] = i;
    std::sort(order, order + kFouls, [&](int a, int b) { return foul_[a].y < foul_[b].y; });
    bool drew = false;
    for (int i : order) {
        if (!drew && foul_[i].y > py_) {
            sapper();
            drew = true;
        }
        const Foul& f = foul_[i];
        if (f.gone) {
            spr(art_.scar, f.x, f.y, 8.f, PAL_EARTH, false, 2, true);
            continue;
        }
        spr(art_.shadow, f.x, f.y + 2.f, 8.f, PAL_SHADE, false, 0, true, true);
        float bob = (f.work > 0.f) ? std::sin(swing_ * 7.f) * 1.5f : 0.f;
        spr(foulArt(art_, f.kind), f.x, f.y + bob, foulH(f.kind) * (1.f - 0.25f * f.work), foulPal(f.kind), false, 0,
            true);
    }
    if (!drew) sapper();
}

void Game::sapper() {
    int fr = moving_ ? (int(step_) & 1) : 0;
    spr(art_.shadow, px_, py_ + 2.f, 8.f, PAL_SHADE, false, 0, true, true);
    spr(art_.sapper[fr], px_, py_, 50.f, PAL_SAPPER, face_ < 0.f, 0, true);
    float sx = px_ + face_ * 16.f;
    float sy = py_ - 16.f + (working_ ? std::sin(swing_ * 9.f) * 7.f : 0.f);
    spr(art_.mattock, sx, sy, working_ ? 24.f : 20.f, PAL_IRON, face_ < 0.f, 0, true);
}

void Game::messages() {
    char buf[48];
    if (mode_ == Mode::Title) {
        text("S3 REDOUBT CLER", 62.f, 18.f, 2.f, PAL_GOLD);
        text("CLEAR THE GROUND", 78.f, 132.f, 1.5f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 58.f, 150.f, 1.5f, PAL_ALERT);
        if (int(t_ * 2.f) & 1) text("PRESS START", 104.f, 172.f, 1.5f, PAL_GOOD);
        text("ARROWS MOVE   HOLD A TO CLEAR", 40.f, 206.f, 1.f, PAL_TEXT);
        return;
    }
    int sec = std::max(0, clock_) / 60;
    std::snprintf(buf, sizeof(buf), "WATCH %02d", sec);
    text(buf, 8.f, 6.f, 2.f, sec < 8 ? PAL_ALERT : PAL_GOLD);
    std::snprintf(buf, sizeof(buf), "LEFT %d", kFouls - cleared_);
    text(buf, 220.f, 8.f, 1.5f, PAL_TEXT);
    if (mode_ == Mode::Play && focus_ >= 0) {
        const Foul& f = foul_[focus_];
        std::snprintf(buf, sizeof(buf), "CLEAR %s", kindName(f.kind));
        text(buf, 8.f, 28.f, 1.f, PAL_GOOD);
        int bars = int(f.work * 8.f);
        char bar[12];
        for (int i = 0; i < 8; i++) bar[i] = i < bars ? '#' : '-';
        bar[8] = 0;
        text(bar, 8.f, 40.f, 1.f, PAL_GOLD);
    }
    if (mode_ == Mode::Pause) text("PAUSED", 120.f, 96.f, 2.f, PAL_TEXT);
    if (mode_ == Mode::Won) {
        text("GROUND CLEAR", 78.f, 90.f, 2.f, PAL_GOOD);
        text("THE WATCH HOLDS", 86.f, 114.f, 1.5f, PAL_GOLD);
    }
    if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 52.f, 90.f, 2.f, PAL_ALERT);
        text("THE GROUND STAYS FOUL", 64.f, 114.f, 1.f, PAL_TEXT);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sky();
    messages();
    for (const Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        spr(art_.dust, m.x, m.y, 6.f, PAL_DUST, false, 0, true);
    }
    if (mode_ == Mode::Title) {
        spr(art_.shadow, 160.f, 208.f, 8.f, PAL_SHADE, false, 0, true, true);
        spr(art_.sapper[int(t_ * 3.f) & 1], 160.f, 206.f, 48.f, PAL_SAPPER, false, 0, true);
        spr(art_.mattock, 178.f, 186.f, 22.f, PAL_IRON, false, 0, true);
        for (int i = 0; i < kFouls; i++)
            spr(foulArt(art_, foul_[i].kind), foul_[i].x * 0.72f + 44.f, 118.f + float(i % 3) * 4.f,
                foulH(foul_[i].kind) * 0.7f, foulPal(foul_[i].kind), false, 2, true);
    } else {
        fouls();
    }
    works();
}

}  // namespace rcler
