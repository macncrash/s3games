#include "game/bunker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 148.f;
constexpr float kReach = 24.f;
constexpr int kClock = 42 * 60;
constexpr float kYardL = 28.f, kYardR = 300.f, kYardT = 168.f, kYardB = 210.f;
constexpr float kStartX = 168.f, kStartY = 196.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {62.f, 178.f, 0}, {118.f, 204.f, 1}, {156.f, 174.f, 2},
    {214.f, 206.f, 3}, {252.f, 176.f, 4}, {292.f, 198.f, 5},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float digNeed(int kind) {
    if (kind == 1) return 0.55f;
    if (kind == 5) return 0.48f;
    if (kind == 0) return 0.40f;
    if (kind == 4) return 0.34f;
    if (kind == 3) return 0.30f;
    return 0.22f;
}

const char* kindName(int kind) {
    if (kind == 1) return "WIRE";
    if (kind == 2) return "SHELL";
    if (kind == 3) return "BAGS";
    if (kind == 4) return "TIMBER";
    if (kind == 5) return "DRUM";
    return "RUBBLE";
}

const gs::Mipped& pileArt(const Art& a, int kind) {
    if (kind == 1) return a.coil;
    if (kind == 2) return a.shell;
    if (kind == 3) return a.bags;
    if (kind == 4) return a.timber;
    if (kind == 5) return a.drum;
    return a.rubble;
}

int pilePal(int kind) {
    if (kind == 1) return PAL_WIRE;
    if (kind == 2 || kind == 5) return PAL_STEEL;
    if (kind == 3) return PAL_SAND;
    if (kind == 4) return PAL_WOOD;
    return PAL_EARTH;
}

float pileH(int kind) {
    if (kind == 2) return 16.f;
    if (kind == 4) return 18.f;
    if (kind == 5) return 32.f;
    if (kind == 1) return 26.f;
    return 24.f;
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
    if (cleared_ >= 4) return 2;
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
        pile_[i].gone = false;
    }
    for (Mote& m : mote_) m.life = 0.f;
    digging_ = false;
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
    rng_ = 1;
}

void Game::begin() {
    resetYard();
    won_ = false;
    over_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    blip(330.f, 0.1f);
}

void Game::toTitle() {
    if (sys_) {
        sys_->apu.silence();
        sys_->apu.noise(0.f, 400.f);
    }
    blip_ = 0.f;
    tick_ = 0.f;
    digging_ = false;
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

int Game::focusPile() const {
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
    ix = iy = 0.f;
    hold = false;
    int id = 0;
    float best = 1e9f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float d = dist(px_, py_, pile_[i].x, pile_[i].y);
        if (d < best) {
            best = d;
            id = i;
        }
    }
    if (pile_[id].gone) return;
    float dx = pile_[id].x - px_, dy = pile_[id].y - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    hold = d <= kReach;
    if (d < 1.4f) return;
    ix = dx / d;
    iy = dy / d;
    if (hold) {
        ix *= 0.15f;
        iy *= 0.15f;
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
    for (int n = 0; n < 4; n++) {
        for (Mote& m : mote_) {
            if (m.life > 0.f) continue;
            m.x = x;
            m.y = y;
            m.vx = (rnd() - 0.5f) * 40.f;
            m.vy = -20.f - rnd() * 30.f;
            m.life = 0.35f + rnd() * 0.25f;
            break;
        }
    }
}

void Game::updateWork(bool hold) {
    focus_ = focusPile();
    digging_ = false;
    if (focus_ < 0) return;
    Pile& p = pile_[focus_];
    if (hold) {
        digging_ = true;
        swing_ += kDt * 9.f;
        p.work += kDt / digNeed(p.kind);
        if (int(swing_ * 2.f) != int((swing_ - kDt * 9.f) * 2.f)) puff(p.x, p.y - 4.f);
        if (p.work >= 1.f) {
            p.gone = true;
            cleared_++;
            shake_ = 3.f;
            puff(p.x, p.y);
            blip(cleared_ == kPiles ? 523.f : 392.f, 0.12f);
            focus_ = -1;
            digging_ = false;
        }
    } else if (p.work > 0.f) {
        p.work = std::max(0.f, p.work - kDt * 0.35f);
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "GROUND CLEAR";
    blip(659.f, 0.25f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE WATCH IS OVER";
    blip(110.f, 0.3f);
}

void Game::updatePlay() {
    float ix, iy;
    bool hold;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);
    move(ix, iy);
    updateWork(hold);
    if (clock_ > 0) clock_--;
    if (cleared_ >= kPiles) win();
    else if (clock_ <= 0) lose();
    if (clock_ < 10 * 60 && (clock_ % 60) == 0) blip(880.f, 0.04f);
}

void Game::blip(float freq, float hold) {
    blip_ = hold;
    tick_ = freq;
}

void Game::serviceAudio() {
    if (!sys_) return;
    if (blip_ > 0.f) {
        sys_->apu.tone(0, tick_, 0.18f);
        blip_ -= kDt;
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (digging_) sys_->apu.noise(0.12f, 1800.f);
    else sys_->apu.noise(0.f, 400.f);
}

void Game::fadeMotes() {
    for (Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        m.life -= kDt;
        m.x += m.vx * kDt;
        m.y += m.vy * kDt;
        m.vy += 40.f * kDt;
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
        uint16_t sky = mix4(gs::rgb4(1, 1, 3), gs::rgb4(4, 3, 5), std::min(1.f, t * 1.4f));
        if (y > 150) sky = mix4(gs::rgb4(3, 3, 2), gs::rgb4(5, 4, 2), (y - 150) / 70.f);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = false;
}

void Game::yard() {
    float lamp = 48.f + std::sin(t_ * 0.7f) * 10.f;
    spr(art_.bunker, 160.f, 108.f, 86.f, PAL_CONCRETE, false, 1, true);
    spr(art_.slit, 78.f, 78.f, 10.f, PAL_NIGHT, false, 0, false);
    spr(art_.slit, 242.f, 78.f, 10.f, PAL_NIGHT, false, 0, false);
    spr(art_.lamp, lamp, 48.f, 22.f, PAL_GOLD, false, 0, true);
    spr(art_.beam, lamp + 28.f, 58.f, 14.f, PAL_GOLD, false, 8, false);
    spr(art_.stake, 36.f, 168.f, 36.f, PAL_WIRE, false, 0, true);
    spr(art_.stake, 300.f, 166.f, 34.f, PAL_WIRE, true, 0, true);
}

void Game::piles() {
    int order[kPiles];
    for (int i = 0; i < kPiles; i++) order[i] = i;
    std::sort(order, order + kPiles, [&](int a, int b) { return pile_[a].y < pile_[b].y; });
    bool drew = false;
    for (int i : order) {
        if (!drew && pile_[i].y > py_) {
            sentry();
            drew = true;
        }
        const Pile& p = pile_[i];
        if (p.gone) {
            spr(art_.mark, p.x, p.y, 8.f, PAL_EARTH, false, 2, true);
            continue;
        }
        spr(art_.shadow, p.x, p.y + 2.f, 8.f, PAL_NIGHT, false, 0, true, true);
        float bob = (p.work > 0.f) ? std::sin(swing_ * 6.f) * 2.f : 0.f;
        spr(pileArt(art_, p.kind), p.x, p.y + bob, pileH(p.kind), pilePal(p.kind), false, 0, true);
    }
    if (!drew) sentry();
}

void Game::sentry() {
    int fr = moving_ ? (int(step_) & 1) : 0;
    spr(art_.shadow, px_, py_ + 2.f, 8.f, PAL_NIGHT, false, 0, true, true);
    spr(art_.sentry[fr], px_, py_, 52.f, PAL_SENTRY, face_ < 0.f, 0, true);
    float sx = px_ + face_ * 14.f;
    float sy = py_ - 18.f + (digging_ ? std::sin(swing_ * 8.f) * 6.f : 0.f);
    spr(art_.shovel, sx, sy, digging_ ? 26.f : 22.f, PAL_STEEL, face_ < 0.f, 0, true);
}

void Game::messages() {
    char buf[48];
    if (mode_ == Mode::Title) {
        text("S3 BUNKER CLER", 70.f, 28.f, 2.f, PAL_GOLD);
        text("CLEAR THE GROUND", 78.f, 128.f, 1.5f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 62.f, 146.f, 1.5f, PAL_ALERT);
        if (int(t_ * 2.f) & 1) text("PRESS START", 104.f, 176.f, 1.5f, PAL_GOOD);
        text("ARROWS MOVE   HOLD A TO DIG", 48.f, 204.f, 1.f, PAL_TEXT);
        return;
    }
    int sec = std::max(0, clock_) / 60;
    std::snprintf(buf, sizeof(buf), "WATCH %02d", sec);
    text(buf, 8.f, 6.f, 2.f, sec < 10 ? PAL_ALERT : PAL_GOLD);
    std::snprintf(buf, sizeof(buf), "LEFT %d", kPiles - cleared_);
    text(buf, 220.f, 8.f, 1.5f, PAL_TEXT);
    if (mode_ == Mode::Play && focus_ >= 0) {
        const Pile& p = pile_[focus_];
        std::snprintf(buf, sizeof(buf), "DIG %s", kindName(p.kind));
        text(buf, 8.f, 28.f, 1.f, PAL_GOOD);
        int bars = int(p.work * 8.f);
        char bar[12];
        for (int i = 0; i < 8; i++) bar[i] = i < bars ? '#' : '-';
        bar[8] = 0;
        text(bar, 8.f, 40.f, 1.f, PAL_GOLD);
    }
    if (mode_ == Mode::Pause) text("PAUSED", 120.f, 100.f, 2.f, PAL_TEXT);
    if (mode_ == Mode::Won) {
        text("GROUND CLEAR", 78.f, 96.f, 2.f, PAL_GOOD);
        text("THE WATCH HOLDS", 86.f, 118.f, 1.5f, PAL_GOLD);
    }
    if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 58.f, 96.f, 2.f, PAL_ALERT);
        text("THE GROUND STAYS FOUL", 62.f, 120.f, 1.f, PAL_TEXT);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sky();
    messages();
    for (const Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        spr(art_.dust, m.x, m.y, 6.f + (0.4f - m.life) * 8.f, PAL_FX, false, 0, true);
    }
    if (mode_ == Mode::Title) {
        spr(art_.shadow, 160.f, 202.f, 8.f, PAL_NIGHT, false, 0, true, true);
        spr(art_.sentry[int(t_ * 3.f) & 1], 160.f, 200.f, 56.f, PAL_SENTRY, false, 0, true);
        spr(art_.shovel, 176.f, 178.f, 24.f, PAL_STEEL, false, 0, true);
        for (int i = 0; i < kPiles; i++)
            spr(pileArt(art_, pile_[i].kind), pile_[i].x, pile_[i].y, pileH(pile_[i].kind), pilePal(pile_[i].kind), false, 1,
                true);
    } else {
        piles();
    }
    yard();
}

}  // namespace bcler
