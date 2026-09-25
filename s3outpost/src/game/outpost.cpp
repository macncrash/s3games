#include "game/outpost.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace outpost {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kCx = 160.f;
constexpr float kCy = 112.f;
constexpr float kWire = 34.f;
constexpr float kThrow = 100.f;
constexpr float kReveal = 54.f;
constexpr float kLamp = 40.f;
constexpr float kDawn = 46.f;
constexpr float kSpeed = 112.f;
constexpr float kPi = 3.14159265f;

float wrap(float a) {
    while (a > kPi) a -= kPi * 2.f;
    while (a < -kPi) a += kPi * 2.f;
    return a;
}

uint16_t mix4(uint16_t a, uint16_t b, float u) {
    u = std::clamp(u, 0.f, 1.f);
    auto ch = [&](int shift) {
        int ca = (a >> shift) & 15;
        int cb = (b >> shift) & 15;
        return int(std::lround(ca + (cb - ca) * u));
    };
    return gs::rgb4(ch(8), ch(4), ch(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 1;
}

void Game::quiet() {
    for (int i = 0; i < 4; i++) sys_->apu.keyOff(i);
    for (int i = 0; i < 3; i++) sys_->apu.tone(i, 0, 0);
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    beep_[ch] = hold;
}

void Game::fanfare(bool dawn) {
    quiet();
    gs::FMPatch bell{};
    bell.alg = 7;
    bell.vol = 0.2f;
    bell.op[0].mul = 1.f;
    bell.op[0].level = 1.f;
    bell.op[0].ar = 0.01f;
    bell.op[0].dr = 0.45f;
    bell.op[0].sl = 0.15f;
    bell.op[0].rr = 0.9f;
    bell.op[1].mul = 2.f;
    bell.op[1].level = 0.32f;
    bell.op[1].ar = 0.01f;
    bell.op[1].dr = 0.3f;
    bell.op[1].sl = 0.05f;
    bell.op[1].rr = 0.7f;
    bell.op[2].mul = 3.f;
    bell.op[2].level = 0.16f;
    bell.op[2].ar = 0.02f;
    bell.op[2].dr = 0.25f;
    bell.op[2].sl = 0.f;
    bell.op[2].rr = 0.5f;
    bell.op[3].mul = 4.2f;
    bell.op[3].level = 0.08f;
    bell.op[3].ar = 0.02f;
    bell.op[3].dr = 0.2f;
    bell.op[3].sl = 0.f;
    bell.op[3].rr = 0.4f;
    if (!dawn) {
        sys_->apu.setPatch(1, bell);
        sys_->apu.keyOn(1, 98.f, 0.2f);
        sys_->apu.noiseBurst(0.42f, 140.f, 0.35f);
        return;
    }
    sys_->apu.setPatch(1, bell);
    sys_->apu.setPatch(2, bell);
    sys_->apu.setPatch(3, bell);
    sys_->apu.keyOn(1, 523.f, 0.18f);
    sys_->apu.keyOn(2, 659.f, 0.15f);
    sys_->apu.keyOn(3, 784.f, 0.12f);
}

void Game::paintGround() {
    gs::Plane& b = sys_->vdp.B;
    for (int cy = 5; cy < 28; ++cy) {
        for (int cx = 0; cx < 40; ++cx) {
            float x = cx * 8.f + 4.f;
            float y = cy * 8.f + 4.f;
            float dx = x - kCx;
            float dy = y - kCy;
            float r = std::hypot(dx, dy);
            int tile = art_.grass;
            if (r < 16.f) tile = art_.yard;
            else if (r < 28.f || (std::fabs(dx) < 12.f && dy < -8.f && dy > -78.f)) tile = art_.dirt;
            else if (r > 108.f) tile = art_.moss;
            else if (((cx * 5 + cy * 3) % 7) == 0) tile = art_.moss;
            b.set(cx, cy, gs::entry(tile, PAL_GROUND));
        }
    }
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    down_ = 0;
    post_ = 100;
    score_ = 0;
    stock_ = 8;
    spawnIx_ = 0;
    t_ = 0;
    face_ = -kPi * 0.5f;
    px_ = kCx;
    py_ = kCy;
    fireCd_ = tossCd_ = lamp_ = lampCd_ = regen_ = 0;
    muzzle_ = shake_ = hurt_ = 0;
    ping_ = 0.7f;
    foes_.clear();
    flares_.clear();
    bolts_.clear();
    sparks_.clear();
    pops_.clear();

    gs::FMPatch drone{};
    drone.alg = 7;
    drone.vol = 0.05f;
    drone.tone = 280.f;
    drone.echo = 0.18f;
    drone.op[0].mul = 1.f;
    drone.op[0].level = 1.f;
    drone.op[0].ar = 0.6f;
    drone.op[0].dr = 1.2f;
    drone.op[0].sl = 0.8f;
    drone.op[0].rr = 0.8f;
    drone.op[1].mul = 2.01f;
    drone.op[1].level = 0.18f;
    drone.op[1].ar = 0.8f;
    drone.op[1].dr = 1.f;
    drone.op[1].sl = 0.6f;
    drone.op[1].rr = 0.6f;
    drone.op[2].mul = 0.5f;
    drone.op[2].level = 0.1f;
    drone.op[2].sl = 0.5f;
    drone.op[3].mul = 3.f;
    drone.op[3].level = 0.06f;
    drone.op[3].sl = 0.4f;
    sys_->apu.setPatch(0, drone);
    sys_->apu.keyOn(0, 49.f, 0.04f);
    sys_->setLight(48, 26, 8);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paintGround();
    sys.vdp.setFogColor(gs::rgb4(1, 1, 4));
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.setLight(18, 22, 48);
    }
}

void Game::flareGround(const Flare& f, float& x, float& y) const {
    if (f.t >= f.flight) {
        x = f.lx;
        y = f.ly;
        return;
    }
    float u = f.t / std::max(f.flight, 0.01f);
    x = f.ox + (f.lx - f.ox) * u;
    y = f.oy + (f.ly - f.oy) * u;
}

bool Game::revealed(float x, float y) const {
    if (lamp_ > 0.f && std::hypot(x - px_, y - py_) <= kLamp) return true;
    for (const Flare& f : flares_) {
        float gx, gy;
        flareGround(f, gx, gy);
        float rad = f.t < f.flight ? 14.f : kReveal;
        if (std::hypot(x - gx, y - gy) <= rad) return true;
    }
    return false;
}

bool Game::landingCovers(const Foe& e) const {
    for (const Flare& f : flares_) {
        if (std::hypot(f.lx - e.x, f.ly - e.y) <= kReveal - 6.f) return true;
    }
    return false;
}

float Game::lightAt(float x, float y) const {
    return revealed(x, y) ? 1.f : 0.f;
}

void Game::shoot() {
    if (fireCd_ > 0.f || mode_ != Mode::Watch) return;
    fireCd_ = 0.15f;
    float tx = px_ + std::cos(face_) * 130.f;
    float ty = py_ + std::sin(face_) * 130.f;
    float best = 0.50f;
    for (const Foe& e : foes_) {
        if (!e.alive || !revealed(e.x, e.y)) continue;
        float d = std::hypot(e.x - px_, e.y - py_);
        if (d < 6.f || d > 220.f) continue;
        float diff = std::fabs(wrap(std::atan2(e.y - py_, e.x - px_) - face_));
        if (diff < best) {
            best = diff;
            tx = e.x;
            ty = e.y;
        }
    }
    float dx = tx - px_;
    float dy = ty - py_;
    float d = std::hypot(dx, dy);
    if (d < 1.f) d = 1.f;
    dx /= d;
    dy /= d;
    Bolt b;
    b.px = b.x = px_ + dx * 14.f;
    b.py = b.y = py_ + dy * 14.f;
    b.vx = dx * 560.f;
    b.vy = dy * 560.f;
    b.life = 0.40f;
    bolts_.push_back(b);
    muzzle_ = 0.05f;
    blip(2, 160.f, 0.06f, 0.04f);
    sys_->apu.noiseBurst(0.26f, 6200.f, 0.045f);
}

void Game::tossFlare() {
    if (stock_ <= 0 || tossCd_ > 0.f || mode_ != Mode::Watch) return;
    stock_ -= 1;
    tossCd_ = 0.36f;
    Flare f;
    f.ox = px_;
    f.oy = py_;
    f.lx = px_ + std::cos(face_) * kThrow;
    f.ly = py_ + std::sin(face_) * kThrow;
    f.t = 0;
    f.flight = 0.18f;
    f.burn = 3.5f;
    flares_.push_back(f);
    blip(1, 480.f, 0.07f, 0.1f);
}

void Game::kill(Foe& e) {
    e.alive = false;
    down_ += 1;
    score_ += e.points;
    pops_.push_back({e.x, e.y - 18.f, 0.75f, e.points, false});
    blip(2, 720.f, 0.07f, 0.06f);
    for (int i = 0; i < 5 && int(sparks_.size()) < 24; i++) {
        float a = e.bob + i * 1.25f;
        sparks_.push_back({e.x, e.y, std::cos(a) * 36.f, std::sin(a) * 28.f, 0.28f});
    }
}

void Game::strike(Foe& e) {
    e.alive = false;
    post_ = std::max(0, post_ - e.dmg);
    hurt_ = 0.4f;
    shake_ = 6.f;
    pops_.push_back({e.x, e.y - 12.f, 0.8f, e.dmg, true});
    sys_->rumble(0.8f, 0.35f, 160);
    sys_->apu.noiseBurst(0.48f, 160.f, 0.22f);
    if (post_ <= 0) beginLoss();
}

void Game::beginWin() {
    if (mode_ != Mode::Watch) return;
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    score_ += post_ * 8;
    fanfare(true);
    sys_->setLight(255, 170, 40);
}

void Game::beginLoss() {
    if (mode_ != Mode::Watch && mode_ != Mode::Pause) return;
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    post_ = 0;
    fanfare(false);
    sys_->setLight(180, 16, 8);
}

void Game::think(float& mx, float& my, bool& fire, bool& toss, bool& lamp) {
    mx = my = 0;
    fire = toss = lamp = false;
    const Foe* lit = nullptr;
    const Foe* dark = nullptr;
    float litT = 1e9f, darkT = 1e9f;
    auto eta = [](const Foe& e) { return (std::hypot(e.x - kCx, e.y - kCy) - kWire) / e.speed; };
    for (const Foe& e : foes_) {
        if (!e.alive) continue;
        float tw = eta(e);
        if (revealed(e.x, e.y)) {
            if (tw < litT) {
                litT = tw;
                lit = &e;
            }
        } else if (!landingCovers(e) && tw < darkT) {
            darkT = tw;
            dark = &e;
        }
    }
    auto aim = [&](float x, float y) { face_ = std::atan2(y - py_, x - px_); };
    if (lit && (!dark || litT <= darkT + 0.25f)) {
        aim(lit->x, lit->y);
        if (fireCd_ <= 0.f) fire = true;
        return;
    }
    if (!dark) {
        float hx = kCx - px_, hy = kCy - py_;
        float hd = std::hypot(hx, hy);
        if (hd > 5.f) {
            mx = hx / hd;
            my = hy / hd;
        }
        return;
    }
    float ax = 0, ay = 0, n = 0;
    for (const Foe& e : foes_) {
        if (!e.alive || revealed(e.x, e.y) || landingCovers(e)) continue;
        if (std::hypot(e.x - dark->x, e.y - dark->y) > 42.f) continue;
        ax += e.x;
        ay += e.y;
        n += 1.f;
    }
    if (n < 1.f) {
        ax = dark->x;
        ay = dark->y;
    } else {
        ax /= n;
        ay /= n;
    }
    aim(ax, ay);
    float d = std::hypot(ax - px_, ay - py_);
    float err = std::fabs(d - kThrow);
    if (stock_ > 0 && tossCd_ <= 0.f && err <= kReveal * 0.7f) {
        toss = true;
        return;
    }
    if (err > 8.f && d > 1.f) {
        float dirx = (ax - px_) / d;
        float diry = (ay - py_) / d;
        float step = d > kThrow ? 1.f : -1.f;
        mx = dirx * step;
        my = diry * step;
    }
    if (stock_ <= 0 && d <= kLamp && lampCd_ <= 0.f) lamp = true;
}

void Game::update(float dt) {
    t_ += dt;
    if (t_ >= kDawn) {
        beginWin();
        return;
    }
    if (fireCd_ > 0) fireCd_ -= dt;
    if (tossCd_ > 0) tossCd_ -= dt;
    if (lamp_ > 0) lamp_ -= dt;
    if (lampCd_ > 0) lampCd_ -= dt;
    if (stock_ >= 8) regen_ = 0;
    else {
        regen_ += dt;
        if (regen_ >= 3.5f) {
            stock_ += 1;
            regen_ -= 3.5f;
            blip(0, 760.f, 0.045f, 0.05f);
        }
    }

    static const Spawn kNight[] = {
        {1.1f, 0.20f, 104.f, Kind::Skulk}, {3.5f, 2.70f, 100.f, Kind::Skulk},  {5.9f, -0.35f, 96.f, Kind::Skulk},
        {8.4f, 1.15f, 100.f, Kind::Runner}, {10.8f, 2.15f, 92.f, Kind::Skulk}, {13.2f, 0.55f, 112.f, Kind::Skulk},
        {15.6f, 3.00f, 98.f, Kind::Skulk},  {15.95f, 2.78f, 108.f, Kind::Skulk}, {18.5f, 1.55f, 90.f, Kind::Runner},
        {21.0f, -0.15f, 110.f, Kind::Skulk}, {23.6f, 2.35f, 102.f, Kind::Brute}, {26.4f, 0.85f, 94.f, Kind::Skulk},
        {29.0f, 3.20f, 100.f, Kind::Runner}, {31.6f, 1.90f, 96.f, Kind::Skulk}, {34.0f, 0.40f, 88.f, Kind::Skulk},
        {35.2f, 2.50f, 106.f, Kind::Brute}, {36.5f, -0.50f, 102.f, Kind::Runner},
    };
    while (spawnIx_ < int(sizeof kNight / sizeof kNight[0]) && t_ >= kNight[spawnIx_].t) {
        const Spawn& s = kNight[spawnIx_++];
        Foe e;
        e.kind = s.kind;
        e.x = kCx + std::cos(s.ang) * s.rad;
        e.y = kCy + std::sin(s.ang) * s.rad;
        e.bob = s.ang * 3.f;
        e.speed = 16.f;
        e.hp = 1;
        e.points = 100;
        e.dmg = 28;
        e.hit = 14.f;
        if (s.kind == Kind::Runner) {
            e.speed = 22.f;
            e.points = 160;
            e.dmg = 34;
            e.hit = 13.f;
        } else if (s.kind == Kind::Brute) {
            e.speed = 12.f;
            e.hp = 2;
            e.points = 280;
            e.dmg = 46;
            e.hit = 18.f;
        }
        foes_.push_back(e);
    }

    float mx = 0, my = 0;
    bool fire = false, toss = false, lamp = false;
    if (bot_) {
        think(mx, my, fire, toss, lamp);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_RIGHT)) mx += 1.f;
        if (pad.down(gs::BTN_LEFT)) mx -= 1.f;
        if (pad.down(gs::BTN_DOWN)) my += 1.f;
        if (pad.down(gs::BTN_UP)) my -= 1.f;
        if (mx == 0.f && std::fabs(pad.axisX) > 0.2f) mx = pad.axisX;
        float m = std::hypot(mx, my);
        if (m > 1.f) {
            mx /= m;
            my /= m;
        }
        if (m > 0.15f) face_ = std::atan2(my, mx);
        fire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A);
        toss = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X);
        lamp = pad.pressed(gs::BTN_TURBO) || pad.pressed(gs::BTN_Y);
    }
    px_ = std::clamp(px_ + mx * kSpeed * dt, 22.f, 298.f);
    py_ = std::clamp(py_ + my * kSpeed * dt, 48.f, 200.f);
    if (lamp && lamp_ <= 0.f && lampCd_ <= 0.f) {
        lamp_ = 0.55f;
        lampCd_ = 4.4f;
        blip(1, 280.f, 0.06f, 0.08f);
    }
    if (toss) tossFlare();
    if (fire) shoot();

    for (Flare& f : flares_) {
        float before = f.t;
        f.t += dt;
        if (!f.popped && before < f.flight && f.t >= f.flight) {
            f.popped = true;
            blip(1, 960.f, 0.09f, 0.1f);
            sys_->apu.noiseBurst(0.16f, 2000.f, 0.05f);
        }
    }

    ping_ -= dt;
    if (ping_ <= 0.f) {
        ping_ = 0.85f;
        const Foe* near = nullptr;
        float best = 1e9f;
        for (const Foe& e : foes_) {
            if (!e.alive || revealed(e.x, e.y)) continue;
            float d = std::hypot(e.x - kCx, e.y - kCy);
            if (d < best) {
                best = d;
                near = &e;
            }
        }
        if (near) {
            float u = std::clamp(1.f - (best - kWire) / 110.f, 0.f, 1.f);
            blip(0, 170.f + u * 620.f, 0.035f + u * 0.04f, 0.07f);
        }
    }

    if (mode_ != Mode::Watch) return;
    for (Foe& e : foes_) {
        if (!e.alive || mode_ != Mode::Watch) continue;
        float dx = kCx - e.x;
        float dy = kCy - e.y;
        float d = std::hypot(dx, dy);
        if (d <= kWire) strike(e);
        else {
            e.x += dx / d * e.speed * dt;
            e.y += dy / d * e.speed * dt;
        }
    }
    if (mode_ != Mode::Watch) return;

    for (Bolt& b : bolts_) {
        float ox = b.x, oy = b.y;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.life -= dt;
        if (b.life <= 0.f) continue;
        for (Foe& e : foes_) {
            if (!e.alive || !revealed(e.x, e.y)) continue;
            float vx = b.x - ox, vy = b.y - oy;
            float wx = e.x - ox, wy = e.y - oy;
            float vv = vx * vx + vy * vy;
            float u = vv > 0.001f ? std::clamp((wx * vx + wy * vy) / vv, 0.f, 1.f) : 0.f;
            float hit = std::hypot(ox + vx * u - e.x, oy + vy * u - e.y);
            if (hit > e.hit) continue;
            b.life = 0;
            e.hp -= 1;
            if (e.hp <= 0) kill(e);
            else blip(2, 400.f, 0.05f, 0.04f);
            break;
        }
    }

    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& e) { return !e.alive; }), foes_.end());
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return b.life <= 0.f; }), bolts_.end());
    flares_.erase(std::remove_if(flares_.begin(), flares_.end(),
                                 [](const Flare& f) { return f.t > f.flight + f.burn; }),
                  flares_.end());
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp<long>(std::lround(w), 1L, 2000L));
    s.h = int16_t(std::clamp<long>(std::lround(h), 1L, 2000L));
    float jx = std::sin(t_ * 48.f) * shake_;
    s.x = int16_t(std::lround(cx - s.w * 0.5f + jx));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal & 15);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 17.f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, float(g.h) * scale, pal, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::sky() {
    float u = 0;
    if (mode_ == Mode::Won) u = 1.f;
    else if (mode_ != Mode::Title) u = std::clamp(t_ / kDawn, 0.f, 1.f);
    float flash = std::clamp(hurt_ / 0.4f, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 40) {
            float g = y / 39.f;
            uint16_t night = mix4(gs::rgb4(1, 1, 5), gs::rgb4(4, 2, 8), g);
            uint16_t dawn = mix4(gs::rgb4(4, 6, 12), gs::rgb4(14, 8, 4), g);
            c = mix4(night, dawn, u);
            if (flash > 0) c = mix4(c, gs::rgb4(12, 2, 2), flash * 0.75f);
        } else {
            c = gs::rgb4(1, 2, 1);
        }
        sys_->vdp.lineBackdrop[y] = c;
    }
    sys_->vdp.setFogColor(mix4(gs::rgb4(1, 1, 4), gs::rgb4(12, 7, 4), u));
}

void Game::draw() {
    sky();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float u = 0;
    if (mode_ == Mode::Won) u = 1.f;
    else if (mode_ != Mode::Title) u = std::clamp(t_ / kDawn, 0.f, 1.f);
    const int cells = 28;
    int fill = int(std::lround(u * cells));
    if (fill > cells) fill = cells;
    for (int i = 0; i < cells; i++) v.HUD.set(2 + i, 0, gs::entry(i < fill ? art_.pipOn : art_.pipDim, PAL_HUD));
    v.HUD.set(2 + cells, 0, gs::entry(art_.pipMark, PAL_HUD));
    hud(31, 1, "MARK", PAL_HUD);
    if (mode_ != Mode::Title) {
        int left = int(std::ceil(std::max(0.f, kDawn - t_)));
        char num[8];
        std::snprintf(num, sizeof num, "%02d", left);
        hud(36, 1, num, PAL_HUD);
    }

    if (mode_ == Mode::Title) {
        text("S3 OUTPOST", 160.f, 46.f, 1.15f, PAL_HUD, 0);
        hudC(7, "FLARES SHOW THEM", PAL_HUD);
        hudC(8, "LAST UNTIL THE DAWN MARK", PAL_HUD);
        hudC(10, "ARROWS AIM", PAL_HUD);
        hudC(11, "C FIRE   X FLARE", PAL_HUD);
        hudC(12, "SPACE LAMP", PAL_HUD);
        if ((sys_->frame / 30) % 2 == 0) hudC(15, "ENTER TO WATCH", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("DAWN HOLDS", 160.f, 64.f, 1.05f, PAL_HUD, 0);
        hudC(11, "THE WIRE HELD", PAL_HUD);
        hudC(13, "ENTER", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        text("WIRE LOST", 160.f, 64.f, 1.05f, PAL_HUD, 0);
        hudC(11, "THEY CAME IN THE DARK", PAL_HUD);
        hudC(13, "ENTER", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
    } else if (t_ < 6.f) {
        hudC(26, "FACE THE TICK   X FLARE   C FIRE", PAL_HUD);
    }

    char stat[48];
    std::snprintf(stat, sizeof stat, "POST %d  FLARE %d  DOWN %d", post_, stock_, down_);
    hud(1, 27, stat, post_ < 40 ? 9 : PAL_HUD);

    for (const Pop& p : pops_) {
        char b[8];
        std::snprintf(b, sizeof b, "%d", p.pts);
        text(b, p.x, p.y, 0.55f, p.bad ? 9 : PAL_HUD, 0);
    }
    for (const Spark& s : sparks_) spr(art_.flare, s.x, s.y, 8.f + s.t * 10.f, PAL_FLARE);
    for (const Bolt& b : bolts_) {
        bool left = b.vx < 0;
        spr(art_.bolt, b.x, b.y, 6.f, PAL_FLARE, left);
    }

    float cs = std::cos(face_), sn = std::sin(face_);
    int step = int(t_ * 8.f) & 1;
    if (muzzle_ > 0) spr(art_.flare, px_ + cs * 16.f, py_ + sn * 16.f, 12.f, PAL_FLARE);
    spr(art_.sentry[step], px_, py_, 34.f, PAL_SENT, cs < 0);

    if (mode_ == Mode::Title) {
        spr(art_.stalker[0], 236.f, 168.f, 34.f, PAL_THEM, true);
        spr(art_.flare, 228.f, 160.f, 14.f, PAL_FLARE);
    }

    const Foe* urgent = nullptr;
    float urgentT = 1e9f;
    for (const Foe& e : foes_) {
        if (!e.alive || revealed(e.x, e.y)) continue;
        float tw = (std::hypot(e.x - kCx, e.y - kCy) - kWire) / e.speed;
        if (tw < urgentT) {
            urgentT = tw;
            urgent = &e;
        }
    }
    for (const Foe& e : foes_) {
        if (!e.alive) continue;
        if (!revealed(e.x, e.y)) {
            if (std::hypot(e.x - px_, e.y - py_) < 48.f) spr(art_.glint, e.x, e.y - 6.f, 8.f, PAL_THEM);
            continue;
        }
        bool left = e.x > kCx;
        if (e.kind == Kind::Brute) spr(art_.brute[step], e.x, e.y, 46.f, PAL_BRUTE, left);
        else spr(art_.stalker[step], e.x, e.y, 34.f, PAL_THEM, left);
    }
    if (urgent && (mode_ == Mode::Watch || mode_ == Mode::Pause)) {
        float a = std::atan2(urgent->y - kCy, urgent->x - kCx);
        float pulse = 12.f + std::sin(t_ * 10.f) * 2.f;
        spr(art_.chev, kCx + std::cos(a) * (kWire + 16.f), kCy + std::sin(a) * (kWire + 16.f), pulse, PAL_FLARE);
    }
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        spr(art_.aim, px_ + cs * kThrow, py_ + sn * kThrow, stock_ > 0 ? 12.f : 8.f, PAL_FLARE, false, stock_ > 0 ? 0 : 9);
    }

    for (const Flare& f : flares_) {
        float gx, gy;
        flareGround(f, gx, gy);
        float lift = 0;
        if (f.t < f.flight) {
            float fu = f.t / std::max(f.flight, 0.01f);
            lift = std::sin(fu * kPi) * 22.f;
        }
        float h = f.t < f.flight ? 11.f : 15.f + std::sin(f.t * 14.f) * 2.f;
        spr(art_.flare, gx, gy - lift, h, PAL_FLARE);
    }

    spr(art_.hut, kCx, kCy + 2.f, 40.f, PAL_HUT);
    spr(art_.mast, kCx + 18.f, kCy - 6.f, 44.f, PAL_HUT);
    for (int i = 0; i < 10; i++) {
        float a = i * (kPi * 2.f / 10.f);
        spr(art_.post, kCx + std::cos(a) * 44.f, kCy + std::sin(a) * 44.f, 16.f, PAL_HUT);
    }
    static const float kTrees[][2] = {{34.f, 76.f},  {86.f, 54.f},  {140.f, 50.f}, {214.f, 52.f}, {276.f, 68.f},
                                      {304.f, 112.f}, {26.f, 156.f}, {52.f, 198.f}, {168.f, 206.f}, {292.f, 184.f}};
    for (const auto& tr : kTrees) {
        spr(art_.shadow, tr[0], tr[1] + 12.f, 16.f, PAL_FX, false, 0, true);
        spr(art_.tree, tr[0], tr[1], 42.f, PAL_PINE);
    }
    spr(art_.shadow, kCx, kCy + 14.f, 20.f, PAL_FX, false, 0, true);

    if (u < 0.72f) {
        static const float kStars[][2] = {{18.f, 8.f},  {46.f, 18.f}, {96.f, 6.f},  {150.f, 16.f},
                                          {196.f, 8.f}, {244.f, 18.f}, {286.f, 7.f}, {318.f, 16.f}};
        for (const auto& st : kStars) spr(art_.flare, st[0], st[1], 5.f, PAL_FX);
    }
    if (u > 0.08f) {
        float sx = 36.f + u * 230.f;
        float sy = 24.f - u * 8.f;
        spr(art_.sun, sx, sy, 10.f + u * 8.f, PAL_FLARE);
    }

    if (lamp_ > 0) spr(art_.glow, px_, py_, kLamp * 2.f, PAL_FLARE);
    if (mode_ == Mode::Title) spr(art_.glow, 230.f, 172.f, kReveal * 2.f, PAL_FLARE);
    for (const Flare& f : flares_) {
        float gx, gy;
        flareGround(f, gx, gy);
        float rad = f.t < f.flight ? 18.f : kReveal;
        spr(art_.glow, gx, gy, rad * 2.f, PAL_FLARE);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    for (int i = 0; i < 3; i++) {
        if (beep_[i] <= 0) continue;
        beep_[i] -= kDt;
        if (beep_[i] <= 0) sys.apu.tone(i, 0, 0);
    }
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - kDt * 12.f);
    if (hurt_ > 0) hurt_ = std::max(0.f, hurt_ - kDt);
    if (muzzle_ > 0) muzzle_ = std::max(0.f, muzzle_ - kDt);
    for (Spark& s : sparks_) {
        s.t -= kDt;
        s.x += s.vx * kDt;
        s.y += s.vy * kDt;
    }
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& s) { return s.t <= 0; }), sparks_.end());
    for (Pop& p : pops_) {
        p.t -= kDt;
        p.y -= 14.f * kDt;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    if (mode_ == Mode::Title) {
        if (start || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) beginWatch();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (start || back) mode_ = Mode::Watch;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (start || pad.pressed(gs::BTN_C))) {
            quiet();
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            sys.setLight(18, 22, 48);
        }
    } else if (!bot_ && (start || back)) {
        mode_ = Mode::Pause;
    } else {
        update(kDt);
    }
    draw();
}

}  // namespace outpost
