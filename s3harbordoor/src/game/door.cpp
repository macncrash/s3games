#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace harbordoor {

namespace {
constexpr float kHold = 180.f;
constexpr float kDoorX = 62.f;
constexpr float kGunX = 78.f;
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
    hull_ = 100;
    sunk_ = 0;
    py_ = 120;
    fire_ = 0;
    spawn_ = 0.6f;
    craft_.clear();
    bolts_.clear();
    puffs_.clear();
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
    bool up = false, down = false, fire = false;
    if (bot_) {
        const Craft* best = nullptr;
        for (const Craft& c : craft_) {
            if (!best || c.x < best->x) best = &c;
        }
        if (best) {
            if (py_ < best->y - 4.f) down = true;
            else if (py_ > best->y + 4.f) up = true;
            if (std::fabs(py_ - best->y) < 16.f) fire = true;
        }
    } else {
        const gs::Pad& p = sys_->pad;
        up = p.down(gs::BTN_UP) || p.axisY > 0.3f;
        down = p.down(gs::BTN_DOWN) || p.axisY < -0.3f;
        fire = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C);
    }
    if (up) py_ -= 170.f * dt;
    if (down) py_ += 170.f * dt;
    py_ = std::clamp(py_, 46.f, 198.f);

    fire_ -= dt;
    if (fire && fire_ <= 0.f) {
        fire_ = 0.09f;
        bolts_.push_back({kGunX + 18.f, py_});
        blip(740.f);
    }

    spawn_ -= dt;
    float gap = std::max(0.48f, 1.7f - t_ * 0.007f);
    if (spawn_ <= 0.f && t_ < kHold - 1.f) {
        spawn_ = gap;
        Craft c;
        c.base = 52.f + rnd() * 140.f;
        c.y = c.base;
        c.x = 340.f;
        c.amp = 4.f + rnd() * 10.f;
        c.freq = 1.2f + rnd() * 1.4f;
        c.phase = rnd() * 6.2f;
        bool ram = t_ > 25.f && rnd() < 0.2f;
        c.kind = ram ? 1 : 0;
        c.hp = ram ? 3 : 1;
        c.dmg = ram ? 18 : 8;
        c.vx = -(32.f + rnd() * 22.f + t_ * 0.08f);
        if (ram) c.vx *= 0.7f;
        craft_.push_back(c);
    }

    for (Bolt& b : bolts_) b.x += 320.f * dt;
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return b.x > 340.f; }), bolts_.end());

    for (Craft& c : craft_) {
        c.x += c.vx * dt;
        c.y = c.base + std::sin(t_ * c.freq + c.phase) * c.amp;
        c.y = std::clamp(c.y, 48.f, 200.f);
    }

    for (auto it = bolts_.begin(); it != bolts_.end();) {
        bool hit = false;
        for (auto ct = craft_.begin(); ct != craft_.end(); ++ct) {
            float hw = ct->kind ? 26.f : 20.f;
            if (std::fabs(it->y - ct->y) < 12.f && it->x > ct->x - hw && it->x < ct->x + 8.f) {
                ct->hp--;
                hit = true;
                if (ct->hp <= 0) {
                    sunk_++;
                    puffs_.push_back({ct->x, ct->y, 0.45f});
                    craft_.erase(ct);
                    blip(220.f);
                }
                break;
            }
        }
        if (hit) it = bolts_.erase(it);
        else ++it;
    }

    for (auto ct = craft_.begin(); ct != craft_.end();) {
        if (ct->x < kDoorX) {
            hull_ = std::max(0, hull_ - ct->dmg);
            puffs_.push_back({kDoorX, ct->y, 0.4f});
            shake_ = 0.2f;
            blip(90.f);
            ct = craft_.erase(ct);
        } else ++ct;
    }

    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());

    t_ += dt;
    if (hull_ <= 0) {
        mode_ = Mode::Lost;
        banner_ = 0;
        won_ = false;
    } else if (t_ >= kHold) {
        mode_ = Mode::Won;
        banner_ = 0;
        won_ = true;
    }
}

void Game::harbor() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 36 ? 2 : 4;
        v.lineBackdrop[y] = gs::rgb4(1, sky, 6 + (y > 36 ? (y - 36) / 28 : 0));
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r = {};
        if (y >= 40 && y < 216) {
            r.on = true;
            r.cx = 190.f;
            r.hw = 150.f;
            r.v = float(y) * 3.f + t_ * 40.f;
            r.pal = PAL_WATER;
            r.band = (y / 8) & 1;
            r.style = 2;
            r.left = r.right = gs::GROUND_WATER;
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

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

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
    harbor();
    float ox = shake_ > 0 ? std::sin(t_ * 80.f) * 2.f : 0.f;
    spr(art_.door, 28.f + ox, 120.f, 196.f, PAL_IRON);
    spr(art_.gun, kGunX + ox, py_, 22.f, PAL_GUN);
    for (const Craft& c : craft_) {
        if (c.kind)
            spr(art_.ram, c.x, c.y, 28.f, PAL_RAM);
        else
            spr(art_.skiff, c.x, c.y, 20.f, PAL_SKIFF);
    }
    for (const Bolt& b : bolts_) spr(art_.bolt, b.x, b.y, 5.f, PAL_SHOT);
    for (const Puff& p : puffs_) spr(art_.splash, p.x, p.y, 14.f + (0.45f - p.t) * 20.f, PAL_FOAM);

    int left = std::max(0, int(std::ceil(kHold - t_)));
    if (mode_ != Mode::Hold) left = mode_ == Mode::Won ? 0 : left;
    char clock[16];
    std::snprintf(clock, sizeof(clock), "%d:%02d", left / 60, left % 60);
    hud(1, 1, "DOOR", PAL_HUD);
    hud(6, 1, std::to_string(hull_), hull_ < 30 ? PAL_ALERT : PAL_GOLD);
    hud(32, 1, clock, PAL_HUD);
    hud(28, 1, "HOLD", PAL_HUD);

    if (mode_ == Mode::Title) {
        word("S3 HARBOR DOOR", 190.f, 70.f, 14.f, PAL_GOLD);
        word("HOLD THE DOOR", 190.f, 92.f, 11.f, PAL_HUD);
        word("THREE MINUTES", 190.f, 112.f, 10.f, PAL_HUD);
        word("START", 190.f, 150.f, 12.f, (sys_->frame / 20) & 1 ? PAL_GOLD : PAL_HUD);
    } else if (mode_ == Mode::Won) {
        word("THE DOOR HELD", 190.f, 78.f, 14.f, PAL_GOLD);
    } else if (mode_ == Mode::Lost) {
        word("THE DOOR FELL", 190.f, 78.f, 14.f, PAL_ALERT);
    }
}

}  // namespace harbordoor
