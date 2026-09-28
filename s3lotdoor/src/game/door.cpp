#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace lotdoor {

namespace {
constexpr float kHold = 180.f;
constexpr float kDoorX = 58.f;
constexpr float kGunX = 74.f;
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Hold) return 1;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 0;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.f / 16777216.f);
}

void Game::blip(float freq) {
    beep_ = 0.05f;
    sys_->apu.tone(0, freq, 0.12f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.vdp.hudEnabled = true;
    if (bot_) beginHold();
}

void Game::beginHold() {
    mode_ = Mode::Hold;
    t_ = 0;
    gate_ = 100;
    stopped_ = 0;
    py_ = 128;
    fire_ = 0;
    spawn_ = 0.55f;
    crank_ = 0;
    rigs_.clear();
    flares_.clear();
    sparks_.clear();
    banner_ = 0;
    won_ = false;
    over_ = false;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }
    if (shake_ > 0) shake_ -= dt;

    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginHold();
    } else if (mode_ == Mode::Hold) {
        update(dt);
    } else {
        banner_++;
        if (banner_ > 50) over_ = true;
    }
    draw();
}

void Game::update(float dt) {
    bool up = false, down = false, fire = false, crank = false;
    if (bot_) {
        const Rig* best = nullptr;
        for (const Rig& c : rigs_) {
            if (!best || c.x < best->x) best = &c;
        }
        if (best) {
            if (py_ < best->y - 3.f) down = true;
            else if (py_ > best->y + 3.f) up = true;
            if (std::fabs(py_ - best->y) < 14.f) fire = true;
        }
    } else {
        const gs::Pad& p = sys_->pad;
        up = p.down(gs::BTN_UP) || p.axisY > 0.3f;
        down = p.down(gs::BTN_DOWN) || p.axisY < -0.3f;
        fire = p.down(gs::BTN_A) || p.down(gs::BTN_B);
        crank = p.down(gs::BTN_C);
    }
    if (up) py_ -= 180.f * dt;
    if (down) py_ += 180.f * dt;
    py_ = std::clamp(py_, 48.f, 200.f);

    if (crank) crank_ += dt;
    else crank_ = 0;
    if (crank_ > 0.7f) {
        mode_ = Mode::Lost;
        banner_ = 0;
        won_ = false;
        blip(70.f);
        return;
    }

    fire_ -= dt;
    if (fire && fire_ <= 0.f) {
        fire_ = 0.08f;
        flares_.push_back({kGunX + 16.f, py_});
        blip(680.f);
    }

    spawn_ -= dt;
    float gap = std::max(0.52f, 1.65f - t_ * 0.006f);
    if (spawn_ <= 0.f && t_ < kHold - 1.5f) {
        spawn_ = gap;
        Rig c;
        int lane = int(rnd() * 5.f);
        if (lane > 4) lane = 4;
        c.base = 58.f + float(lane) * 30.f;
        c.y = c.base;
        c.x = 348.f;
        c.amp = 1.5f + rnd() * 3.f;
        c.freq = 0.8f + rnd() * 0.8f;
        c.phase = rnd() * 6.2f;
        bool box = t_ > 30.f && rnd() < 0.22f;
        c.kind = box ? 1 : 0;
        c.hp = box ? 3 : 1;
        c.dmg = box ? 16 : 7;
        c.vx = -(28.f + rnd() * 16.f + t_ * 0.06f);
        if (box) c.vx *= 0.72f;
        rigs_.push_back(c);
    }

    for (Flare& b : flares_) b.x += 340.f * dt;
    flares_.erase(std::remove_if(flares_.begin(), flares_.end(), [](const Flare& b) { return b.x > 340.f; }), flares_.end());

    for (Rig& c : rigs_) {
        c.x += c.vx * dt;
        c.y = c.base + std::sin(t_ * c.freq + c.phase) * c.amp;
        c.y = std::clamp(c.y, 50.f, 202.f);
    }

    for (auto it = flares_.begin(); it != flares_.end();) {
        bool hit = false;
        for (auto ct = rigs_.begin(); ct != rigs_.end(); ++ct) {
            float hw = ct->kind ? 30.f : 22.f;
            if (std::fabs(it->y - ct->y) < 12.f && it->x > ct->x - hw && it->x < ct->x + 10.f) {
                ct->hp--;
                hit = true;
                if (ct->hp <= 0) {
                    stopped_++;
                    sparks_.push_back({ct->x, ct->y, 0.4f});
                    rigs_.erase(ct);
                    blip(240.f);
                }
                break;
            }
        }
        if (hit) it = flares_.erase(it);
        else ++it;
    }

    for (auto ct = rigs_.begin(); ct != rigs_.end();) {
        if (ct->x < kDoorX) {
            gate_ = std::max(0, gate_ - ct->dmg);
            sparks_.push_back({kDoorX + 8.f, ct->y, 0.35f});
            shake_ = 0.18f;
            blip(96.f);
            ct = rigs_.erase(ct);
        } else ++ct;
    }

    for (Spark& p : sparks_) p.t -= dt;
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& p) { return p.t <= 0; }), sparks_.end());

    t_ += dt;
    if (gate_ <= 0) {
        mode_ = Mode::Lost;
        banner_ = 0;
        won_ = false;
    } else if (t_ >= kHold) {
        mode_ = Mode::Won;
        banner_ = 0;
        won_ = true;
    }
}

void Game::lot() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int dusk = y < 28 ? 1 : 0;
        v.lineBackdrop[y] = gs::rgb4(1 + dusk, 1, 2 + (y < 28 ? (28 - y) / 8 : 0));
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r = {};
        if (y >= 36 && y < 218) {
            r.on = true;
            r.cx = 176.f;
            r.hw = 168.f;
            r.v = float(y) * 2.2f;
            r.pal = PAL_LOT;
            r.band = ((y / 10) + int(t_ * 2.f)) & 1;
            r.style = 1;
            r.left = r.right = gs::GROUND_LAND;
        }
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(std::max(1.f, w)));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::word(const std::string& s, float cx, float y, float h, int pal) {
    float width = 0;
    for (char ch : s) {
        if (ch == ' ') {
            width += h * 0.42f;
            continue;
        }
        char c = ch;
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c < 32 || c >= 127) continue;
        const gs::Mipped& g = art_.glyph[int(c) - 32];
        if (g.h < 1) continue;
        width += h * float(g.w) / float(g.h) + 1.f;
    }
    float pen = cx - width * 0.5f;
    for (char ch : s) {
        if (ch == ' ') {
            pen += h * 0.42f;
            continue;
        }
        char c = ch;
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c < 32 || c >= 127) continue;
        const gs::Mipped& g = art_.glyph[int(c) - 32];
        if (g.h < 1) continue;
        float w = h * float(g.w) / float(g.h);
        spr(g, pen + w * 0.5f, y, h, pal);
        pen += w + 1.f;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    lot();
    float ox = shake_ > 0 ? std::sin(t_ * 90.f) * 2.f : 0.f;
    for (int i = 0; i < 4; i++) spr(art_.lamp, 120.f + float(i) * 52.f, 42.f, 36.f, PAL_LAMP);
    spr(art_.gate, 26.f + ox, 124.f, 188.f, PAL_GATE);
    spr(art_.cart, kGunX + ox, py_, 20.f, PAL_CART);
    for (const Rig& c : rigs_) {
        if (c.kind)
            spr(art_.truck, c.x, c.y, 28.f, PAL_TRUCK, true);
        else
            spr(art_.sedan, c.x, c.y, 18.f, PAL_SEDAN, true);
    }
    for (const Flare& b : flares_) spr(art_.flare, b.x, b.y, 5.f, PAL_FLARE);
    for (const Spark& p : sparks_) spr(art_.spark, p.x, p.y, 12.f + (0.4f - p.t) * 18.f, PAL_FLARE);

    int left = std::max(0, int(std::ceil(kHold - t_)));
    if (mode_ == Mode::Won) left = 0;
    char clock[16];
    std::snprintf(clock, sizeof(clock), "%d:%02d", left / 60, left % 60);
    hud(1, 1, "GATE", PAL_HUD);
    hud(6, 1, std::to_string(gate_), gate_ < 30 ? PAL_ALERT : PAL_GOLD);
    hud(32, 1, clock, PAL_HUD);
    hud(27, 1, "HOLD", PAL_HUD);

    if (mode_ == Mode::Title) {
        word("S3 LOT DOOR", 188.f, 72.f, 14.f, PAL_GOLD);
        word("HOLD THE DOOR", 188.f, 94.f, 11.f, PAL_HUD);
        word("THREE MINUTES", 188.f, 114.f, 10.f, PAL_HUD);
        word("START", 188.f, 152.f, 12.f, (sys_->frame / 20) & 1 ? PAL_GOLD : PAL_HUD);
    } else if (mode_ == Mode::Won) {
        word("THE DOOR HELD", 188.f, 80.f, 14.f, PAL_GOLD);
    } else if (mode_ == Mode::Lost) {
        word("THE DOOR FELL", 188.f, 80.f, 14.f, PAL_ALERT);
    }
}

}  // namespace lotdoor
