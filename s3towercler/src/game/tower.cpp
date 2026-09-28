#include "game/tower.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tower {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 168.f;
constexpr float kReach = 30.f;
constexpr int kClock = 48 * 60;
constexpr float kYardL = 20.f, kYardR = 300.f, kYardT = 158.f, kYardB = 214.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {46.f, 186.f, 0}, {98.f, 206.f, 1}, {150.f, 174.f, 2},
    {198.f, 208.f, 3}, {248.f, 178.f, 4}, {296.f, 198.f, 5},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float sweepNeed(int kind) {
    if (kind == 2) return 0.42f;
    if (kind == 1) return 0.34f;
    if (kind == 3) return 0.32f;
    if (kind == 5) return 0.28f;
    if (kind == 4) return 0.26f;
    return 0.22f;
}

const char* kindName(int kind) {
    if (kind == 1) return "CRATE";
    if (kind == 2) return "STONE";
    if (kind == 3) return "BARREL";
    if (kind == 4) return "PLANK";
    if (kind == 5) return "LAMP";
    return "IVY";
}

const gs::Mipped& pileArt(const Art& a, int kind) {
    if (kind == 1) return a.crate;
    if (kind == 2) return a.stone;
    if (kind == 3) return a.barrel;
    if (kind == 4) return a.plank;
    if (kind == 5) return a.lamp;
    return a.ivy;
}

int pilePal(int kind) {
    if (kind == 0) return PAL_IVY;
    if (kind == 1 || kind == 4) return PAL_WOOD;
    if (kind == 2) return PAL_STONE;
    if (kind == 3) return PAL_RUST;
    return PAL_GOLD;
}

float pileH(int kind) {
    if (kind == 3) return 28.f;
    if (kind == 4) return 16.f;
    if (kind == 5) return 30.f;
    if (kind == 1) return 24.f;
    return 22.f;
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
    sweeping_ = false;
    moving_ = false;
    focus_ = -1;
    cleared_ = 0;
    px_ = 160.f;
    py_ = 200.f;
    face_ = -1.f;
    step_ = 0.f;
    shake_ = 0.f;
    clock_ = float(kClock);
    chime_ = int(clock_ / 60.f);
    rng_ = 7;
}

void Game::begin() {
    resetYard();
    won_ = false;
    over_ = false;
    mode_ = Mode::Play;
    blip(392.f, 0.08f);
}

void Game::toTitle() {
    if (sys_) {
        sys_->apu.silence();
        sys_->apu.noise(0.f, 400.f);
    }
    blip_ = 0.f;
    tick_ = 0.f;
    sweeping_ = false;
    won_ = false;
    over_ = false;
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
    int id = -1;
    float best = 1e9f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float d = dist(px_, py_, pile_[i].x, pile_[i].y);
        if (d < best) {
            best = d;
            id = i;
        }
    }
    if (id < 0) return;
    float dx = pile_[id].x - px_, dy = pile_[id].y - py_;
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
    if (p.axisX != 0.f || p.axisY != 0.f) {
        ix = p.axisX;
        iy = -p.axisY;
    }
    float m = std::sqrt(ix * ix + iy * iy);
    if (m > 1.f) {
        ix /= m;
        iy /= m;
    }
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_B) || p.accel > 0.4f;
}

void Game::move(float ix, float iy) {
    moving_ = ix != 0.f || iy != 0.f;
    if (!moving_) return;
    px_ = std::clamp(px_ + ix * kSpeed * kDt, kYardL, kYardR);
    py_ = std::clamp(py_ + iy * kSpeed * kDt, kYardT, kYardB);
    if (std::fabs(ix) > 0.15f) face_ = ix < 0.f ? -1.f : 1.f;
    step_ += kDt * 8.f;
}

void Game::puff(float x, float y) {
    for (Mote& m : mote_) {
        if (m.life > 0.f) continue;
        m.x = x;
        m.y = y;
        m.vx = (rnd() - 0.5f) * 50.f;
        m.vy = -20.f - rnd() * 30.f;
        m.life = 0.35f + rnd() * 0.2f;
        return;
    }
}

void Game::updateWork(bool hold) {
    focus_ = focusPile();
    sweeping_ = false;
    if (focus_ < 0 || !hold) return;
    sweeping_ = true;
    Pile& p = pile_[focus_];
    p.work += kDt;
    if (int(t_ * 12.f) != int((t_ - kDt) * 12.f)) puff(p.x, p.y - 6.f);
    if (p.work < sweepNeed(p.kind)) return;
    p.gone = true;
    cleared_++;
    shake_ = 3.f;
    for (int n = 0; n < 4; n++) puff(p.x + (rnd() - 0.5f) * 10.f, p.y);
    blip(cleared_ == kPiles ? 659.f : 440.f, 0.1f);
    sweeping_ = false;
    focus_ = -1;
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    sweeping_ = false;
    blip(523.f, 0.25f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    sweeping_ = false;
    blip(146.f, 0.3f);
}

void Game::updatePlay() {
    float ix, iy;
    bool hold = false;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);
    move(ix, iy);
    updateWork(hold);
    clock_ -= 1.f;
    int sec = int(clock_ / 60.f);
    if (sec != chime_ && sec >= 0) {
        chime_ = sec;
        if (sec <= 8) blip(220.f, 0.05f);
    }
    if (cleared_ >= kPiles) win();
    else if (clock_ <= 0.f) {
        clock_ = 0.f;
        lose();
    }
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
    if (sweeping_) sys_->apu.noise(0.1f, 900.f);
    else sys_->apu.noise(0.f, 400.f);
    if (mode_ == Mode::Play && clock_ < 8.f * 60.f) sys_->apu.tone(1, 110.f, 0.04f);
    else sys_->apu.tone(1, 0.f, 0.f);
}

void Game::fadeMotes() {
    for (Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        m.life -= kDt;
        m.x += m.vx * kDt;
        m.y += m.vy * kDt;
        m.vy += 50.f * kDt;
    }
    if (shake_ > 0.f) shake_ -= kDt * 10.f;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.img = m.pick(h);
    float sc = h / float(m.h);
    s.w = std::max(1, int(std::lround(m.w * sc)));
    s.h = std::max(1, int(std::lround(h)));
    float sh = shake_ > 0.f ? (rnd() - 0.5f) * shake_ : 0.f;
    s.x = int(std::lround(cx - s.w * 0.5f + sh));
    s.y = int(std::lround((feet ? cy - s.h : cy) + sh * 0.15f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float cx = x;
    for (; *s; ++s) {
        unsigned char ch = (unsigned char)*s;
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        int gi = (ch >= 32 && ch < 128) ? ch - 32 : 0;
        float h = 14.f * scale;
        float w = 12.f * scale;
        spr(art_.glyph[gi], cx + w * 0.5f, y, h, pal, false, 0, false);
        cx += 8.f * scale;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    float alarm = (mode_ == Mode::Play && clock_ < 8.f * 60.f) ? (0.5f + 0.5f * std::sin(t_ * 8.f)) : 0.f;
    if (mode_ == Mode::Lost) alarm = 0.7f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        uint16_t col = mix4(gs::rgb4(2, 3, 7), gs::rgb4(8, 9, 11), std::min(1.f, u * 1.3f));
        if (y > 140) col = mix4(gs::rgb4(4, 6, 3), gs::rgb4(6, 5, 3), (y - 140) / 80.f);
        if (alarm > 0.f) col = mix4(col, gs::rgb4(8, 2, 2), alarm * 0.45f);
        v.lineBackdrop[y] = col;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = false;
    v.clearSprites();
}

void Game::towerDraw() {
    spr(art_.tower, 160.f, 150.f, 128.f, PAL_STONE, false, 0, true);
    float frac = std::clamp(clock_ / float(kClock), 0.f, 1.f);
    int step = int((1.f - frac) * 7.99f);
    if (step < 0) step = 0;
    if (step > 7) step = 7;
    spr(art_.hand[step], 160.f, 52.f, 16.f, PAL_GOLD, false, 0, false);
}

void Game::piles() {
    int order[kPiles];
    for (int i = 0; i < kPiles; i++) order[i] = i;
    std::sort(order, order + kPiles, [&](int a, int b) { return pile_[a].y > pile_[b].y; });
    for (int n = 0; n < kPiles; n++) {
        const Pile& p = pile_[order[n]];
        if (p.gone) continue;
        float h = pileH(p.kind);
        if (order[n] == focus_ && sweeping_) h *= 0.92f + 0.08f * std::sin(t_ * 28.f);
        spr(pileArt(art_, p.kind), p.x, p.y, h, pilePal(p.kind), false, 0, true);
    }
    for (const Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        spr(art_.mote, m.x, m.y, 5.f, PAL_DUST, false, 0, false);
    }
}

void Game::keeperDraw() {
    int fr = moving_ ? (int(step_) & 1) : 0;
    float bob = sweeping_ ? std::sin(t_ * 22.f) * 2.f : 0.f;
    spr(art_.keeper[fr], px_, py_ + bob, 40.f, PAL_KEEPER, face_ < 0.f, 0, true);
    float bx = px_ + face_ * 12.f;
    float by = py_ - 6.f + (sweeping_ ? std::sin(t_ * 22.f) * 4.f : 0.f);
    spr(art_.broom, bx, by, 26.f, PAL_WOOD, face_ < 0.f, 0, true);
}

void Game::messages() {
    char buf[48];
    int sec = std::max(0, int(std::ceil(clock_ / 60.f)));
    std::snprintf(buf, sizeof(buf), "CLOCK %02d", sec);
    text(buf, 8.f, 6.f, 1.f, clock_ < 8.f * 60.f ? 3 : 1);
    std::snprintf(buf, sizeof(buf), "LEFT %d", kPiles - cleared_);
    text(buf, 210.f, 6.f, 1.f, 2);

    if (mode_ == Mode::Title) {
        text("S3 TOWERCLER", 64.f, 70.f, 1.f, 2);
        text("ONE TOWER", 96.f, 96.f, 1.f, 1);
        text("CLEAR THE GROUND", 56.f, 116.f, 1.f, 1);
        text("BEFORE THE CLOCK DIES", 32.f, 136.f, 1.f, 3);
        text("ARROWS MOVE  A SWEEP", 40.f, 168.f, 1.f, 4);
        if (int(t_ * 2.f) & 1) text("START", 128.f, 192.f, 1.f, 2);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 112.f, 100.f, 1.f, 2);
    } else if (mode_ == Mode::Won) {
        text("GROUND CLEAR", 72.f, 168.f, 1.f, 2);
        text("THE TOWER STANDS", 48.f, 190.f, 1.f, 1);
    } else if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 64.f, 168.f, 1.f, 3);
        text("GROUND STILL FOUL", 48.f, 190.f, 1.f, 1);
    } else if (focus_ >= 0) {
        float need = sweepNeed(pile_[focus_].kind);
        float u = std::clamp(pile_[focus_].work / need, 0.f, 1.f);
        std::snprintf(buf, sizeof(buf), "%s", kindName(pile_[focus_].kind));
        text(buf, pile_[focus_].x - 20.f, pile_[focus_].y - pileH(pile_[focus_].kind) - 16.f, 1.f, 1);
        int bars = int(u * 6.f + 0.5f);
        char bar[8] = {};
        for (int i = 0; i < bars && i < 6; i++) bar[i] = '#';
        if (bars > 0) text(bar, pile_[focus_].x - 12.f, pile_[focus_].y - pileH(pile_[focus_].kind) - 4.f, 1.f, 2);
    }
}

void Game::draw() {
    sky();
    messages();
    keeperDraw();
    piles();
    towerDraw();
}

}  // namespace tower
