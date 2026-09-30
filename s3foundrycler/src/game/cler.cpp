#include "game/cler.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace fcler {
namespace {

constexpr float kFeet = 186.f;
constexpr float kSpeed = 96.f;
constexpr int kHp = 42;
constexpr int kClock = 52 * 60;
constexpr float kPileX[6] = {78.f, 118.f, 158.f, 198.f, 238.f, 276.f};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::pilesLeft() const {
    int n = 0;
    for (const Pile& p : pile_)
        if (p.hp > 0) n++;
    return n;
}

bool Game::allClear() const { return pilesLeft() == 0; }

float Game::ladleX() const { return 168.f + std::sin(t_ * 0.9f) * 108.f; }

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    blip_ = 0.07f;
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= 1.f / 60.f;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
}

void Game::puff(float x, float y) {
    for (Mote& m : mote_) {
        if (m.life > 0.f) continue;
        m.x = x;
        m.y = y;
        m.vx = (px_ < x ? 18.f : -18.f);
        m.vy = -22.f;
        m.life = 0.35f;
        return;
    }
}

void Game::tickMotes() {
    for (Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        m.life -= 1.f / 60.f;
        m.x += m.vx / 60.f;
        m.y += m.vy / 60.f;
        m.vy += 30.f / 60.f;
    }
}

void Game::resetFloor() {
    for (int i = 0; i < kPiles; i++) {
        pile_[i].x = kPileX[i];
        pile_[i].hp = kHp;
    }
    for (Mote& m : mote_) m.life = 0;
    px_ = 42.f;
    face_ = 1.f;
    walk_ = 0;
    t_ = 0;
    clock_ = kClock;
    stun_ = 0;
    sweeping_ = false;
    secMark_ = clock_ / 60;
}

void Game::begin() {
    resetFloor();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    gate_ = 10;
    blip(220.f);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    gate_ = 12;
    resetFloor();
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    sweeping_ = false;
    blip(660.f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    clock_ = 0;
    sweeping_ = false;
    blip(90.f);
}

int Game::nearestPile() const {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].hp <= 0) continue;
        float d = std::fabs(pile_[i].x - px_);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, bool& hold) {
    ix = 0;
    hold = false;
    int i = nearestPile();
    if (i < 0) return;
    float dx = pile_[i].x - px_;
    float bar = ladleX();
    auto blocked = [&](float x) { return std::fabs(x - bar) < 26.f; };
    if (std::fabs(dx) > 12.f) {
        float dir = dx > 0.f ? 1.f : -1.f;
        if (blocked(px_) || blocked(px_ + dir * 10.f)) ix = 0;
        else ix = dir;
    } else {
        hold = true;
    }
}

void Game::humanInput(float& ix, bool& hold) {
    const gs::Pad& pad = sys_->pad;
    ix = 0;
    if (pad.down(gs::BTN_LEFT)) ix -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) ix += 1.f;
    if (std::fabs(pad.axisX) > 0.25f) ix = pad.axisX;
    if (ix > 1.f) ix = 1.f;
    if (ix < -1.f) ix = -1.f;
    hold = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C);
}

void Game::updatePlay() {
    float ix = 0;
    bool hold = false;
    if (bot_) botInput(ix, hold);
    else humanInput(ix, hold);
    if (stun_ > 0) {
        stun_--;
        ix = 0;
        hold = false;
    }
    t_ += 1.f / 60.f;
    if (std::fabs(ix) > 0.15f) {
        face_ = ix > 0 ? 1.f : -1.f;
        float nx = px_ + ix * kSpeed / 60.f;
        nx = std::clamp(nx, 28.f, 300.f);
        if (!bot_ && std::fabs(nx - ladleX()) < 16.f) {
            stun_ = 28;
            clock_ -= 60;
            blip(140.f);
            nx = px_;
        }
        px_ = nx;
        walk_ += 0.18f;
    }
    sweeping_ = false;
    if (hold) {
        for (Pile& p : pile_) {
            if (p.hp <= 0) continue;
            if (std::fabs(p.x - px_) > 20.f) continue;
            p.hp--;
            sweeping_ = true;
            face_ = p.x >= px_ ? 1.f : -1.f;
            if ((p.hp % 6) == 0) puff(p.x, kFeet - 8.f);
            if (p.hp == 0) blip(520.f);
        }
        if (sweeping_) sys_->apu.noise(0.04f, 0.35f, false);
        else sys_->apu.noise(0, 0, false);
    } else {
        sys_->apu.noise(0, 0, false);
    }
    if (allClear()) {
        win();
        return;
    }
    if (clock_ > 0) clock_--;
    int sec = clock_ / 60;
    if (sec != secMark_) {
        secMark_ = sec;
        if (sec <= 8) blip(sec <= 3 ? 980.f : 360.f);
    }
    if (clock_ <= 0) lose();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.resize(64, 32);
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    for (int y = 22; y < 28; y++)
        for (int x = 0; x < 40; x++) sys.vdp.A.set(x, y, gs::entry(art_.floor, PAL_BRICK));
    sys.apu.setMaster(0.5f);
    if (bot_) begin();
    else toTitle();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -400, 400));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h * 0.5f)), -400, 400));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    float pulse = 0.5f + 0.5f * std::sin(t_ * 6.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H - 1);
        int r = int(1 + u * 4 + (y > 150 ? pulse * 2.f : 0));
        int g = int(u * 1.5f);
        int b = 1;
        if (r > 15) r = 15;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = uint8_t(y < 40 ? (40 - y) / 6 : 0);
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();

    spr(art_.furnace, 36.f, 132.f, 92.f, PAL_EMBER, false, 0);
    float mouth = 0.5f + 0.5f * std::sin(t_ * 7.f);
    spr(art_.spark, 30.f + mouth * 6.f, 118.f, 8.f, PAL_EMBER, false, 0);
    spr(art_.spark, 44.f, 108.f - mouth * 8.f, 6.f, PAL_EMBER, false, 1);

    spr(art_.bin, 300.f, 162.f, 36.f, PAL_IRON, false, 0);
    spr(art_.clock, 168.f, 28.f, 30.f, PAL_CLOCK, false, 0);

    float lx = ladleX();
    for (int i = 0; i < 5; i++) {
        float u = (i + 1) / 6.f;
        spr(art_.link, 168.f + (lx - 168.f) * u, 46.f + u * 78.f, 8.f, PAL_IRON, false, 0);
    }
    spr(art_.ladle, lx, 132.f, 18.f, PAL_IRON, lx < 168.f, 0);

    for (const Pile& p : pile_) {
        if (p.hp <= 0) continue;
        float h = 12.f + 14.f * (float(p.hp) / float(kHp));
        spr(art_.slag, p.x, kFeet - h * 0.35f, h, PAL_SLAG, false, 0);
    }

    for (const Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        spr(art_.spark, m.x, m.y, 5.f + m.life * 6.f, PAL_EMBER, false, 0);
    }

    int step = int(walk_) & 1;
    float bob = sweeping_ ? std::sin(t_ * 18.f) * 2.f : 0.f;
    spr(art_.crew[step], px_, kFeet - 24.f + bob, 48.f, PAL_CREW, face_ < 0, 0);
    float rakeX = px_ + face_ * 18.f;
    float rakeY = kFeet - 16.f + (sweeping_ ? std::sin(t_ * 22.f) * 3.f : 0.f);
    spr(art_.rake, rakeX, rakeY, 16.f, PAL_IRON, face_ < 0, 0);

    char buf[32];
    int sec = clock_ / 60;
    if (sec < 0) sec = 0;
    std::snprintf(buf, sizeof(buf), "%d:%02d", sec / 60, sec % 60);
    hud(33, 1, buf, sec <= 8 ? PAL_ALERT : PAL_CLOCK);
    std::snprintf(buf, sizeof(buf), "LEFT %d", pilesLeft());
    hud(1, 1, buf, PAL_TEXT);

    if (mode_ == Mode::Title) {
        hudC(8, "FOUNDRY CLER", PAL_TEXT);
        hudC(10, "CLEAR THE GROUND", PAL_EMBER);
        hudC(12, "BEFORE THE CLOCK DIES", PAL_CLOCK);
        hudC(16, "ARROWS MOVE", PAL_TEXT);
        hudC(17, "A SWEEPS THE SLAG", PAL_TEXT);
        hudC(20, "START", PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_TEXT);
    } else if (mode_ == Mode::Won) {
        hudC(11, "THE GROUND IS CLEAR", PAL_GOOD);
    } else if (mode_ == Mode::Lost) {
        hudC(11, "THE CLOCK DIED", PAL_ALERT);
    } else if (stun_ > 0) {
        hudC(3, "LADLE", PAL_ALERT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (gate_ > 0) gate_--;
    if (mode_ == Mode::Title) {
        t_ += 1.f / 60.f;
        if (gate_ == 0 && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || bot_)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updatePlay();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (gate_ == 0 && pad.pressed(gs::BTN_START) && !bot_) {
        toTitle();
    }
    tickMotes();
    serviceAudio();
    draw();
}

}  // namespace fcler
