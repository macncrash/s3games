#include "foundry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace foundry {
namespace {
constexpr float FLOOR = 190.f;
constexpr float WORLD = 2100.f;
constexpr float HOME = 108.f;
constexpr float SPEED = 138.f;
constexpr float JUMP = -318.f;
constexpr float GRAV = 920.f;
constexpr float PERIOD = 2.45f;
constexpr float ON = 0.92f;
constexpr float ROLL_LO = 260.f;
constexpr float ROLL_HI = 1800.f;
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Fail || over_) return 4;
    if (carry_ && px_ < 900.f) return 3;
    if (carry_) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    sys.apu.setMaster(0.5f);
    pours_.clear();
    const float xs[] = {460.f, 740.f, 1040.f, 1340.f, 1620.f};
    const float ph[] = {0.2f, 1.15f, 0.55f, 1.7f, 0.9f};
    for (int i = 0; i < 5; i++) pours_.push_back({xs[i], ph[i]});
    rolls_.clear();
    rolls_.push_back({520.f, -118.f});
    rolls_.push_back({980.f, -146.f});
    rolls_.push_back({1460.f, -102.f});
    if (bot_) bootPlay();
}

void Game::bootPlay() {
    mode_ = Mode::Play;
    carry_ = false;
    lives_ = 3;
    px_ = 150.f;
    py_ = FLOOR;
    vx_ = vy_ = 0;
    bx_ = 1960.f;
    face_ = 1;
    inv_ = 0;
    onGround_ = true;
    over_ = false;
    won_ = false;
    t_ = 0;
}

bool Game::pourHot(const Pour& p, float t) const {
    float u = std::fmod(t + p.phase, PERIOD);
    if (u < 0) u += PERIOD;
    return u < ON;
}

bool Game::blocked(float dir) const {
    if (dir == 0) return false;
    for (const Pour& p : pours_) {
        float dx = p.x - px_;
        if (dir * dx > -6.f && dir * dx < 52.f) {
            if (pourHot(p, t_) || pourHot(p, t_ + 0.32f)) return true;
        }
    }
    return false;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.12f);
    sys_->apu.noiseBurst(0.2f, freq, 0.08f);
}

void Game::tryGrab() {
    if (carry_ || !onGround_) return;
    if (std::abs(px_ - bx_) < 26.f && std::abs(py_ - FLOOR) < 4.f) {
        carry_ = true;
        blip(660.f);
    }
}

void Game::hurt() {
    if (inv_ > 0 || mode_ != Mode::Play) return;
    if (carry_) {
        carry_ = false;
        bx_ = std::clamp(px_, 200.f, 1940.f);
    }
    lives_--;
    inv_ = 1.15f;
    vy_ = -160.f;
    vx_ = -face_ * 70.f;
    onGround_ = false;
    blip(140.f);
    if (lives_ <= 0) lose("THE BANNER IS STILL OUT");
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE BANNER IS BACK IN THE FOUNDRY";
    blip(520.f);
}

void Game::lose(const char* why) {
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    reason_ = why;
}

void Game::botThink() {
    float goal = carry_ ? 48.f : bx_;
    float dir = 0;
    if (std::abs(goal - px_) > 8.f) dir = goal > px_ ? 1.f : -1.f;
    bool block = blocked(dir == 0 ? face_ : dir);
    bool hop = false;
    if (onGround_) {
        for (const Roll& r : rolls_) {
            float dx = r.x - px_;
            bool closing = dx * r.vx < 0.f;
            if (closing && std::abs(dx) < 52.f && std::abs(dx) > 6.f) hop = true;
            if (std::abs(dx) < 24.f) hop = true;
        }
    }
    if (block) {
        vx_ = 0;
        if (hop) vx_ = -dir * SPEED * 0.45f;
    } else {
        if (hop && onGround_) vy_ = JUMP, onGround_ = false;
        vx_ = dir * (carry_ ? SPEED * 0.86f : SPEED);
    }
    if (dir != 0) face_ = dir > 0 ? 1 : -1;
}

void Game::update(float dt) {
    t_ += dt;
    if (inv_ > 0) inv_ -= dt;
    for (Roll& r : rolls_) {
        r.x += r.vx * dt;
        if (r.x < ROLL_LO) r.x = ROLL_HI;
        if (r.x > ROLL_HI) r.x = ROLL_LO;
    }
    if (bot_) botThink();
    px_ += vx_ * dt;
    if (!onGround_) vy_ += GRAV * dt;
    py_ += vy_ * dt;
    if (py_ >= FLOOR) {
        py_ = FLOOR;
        vy_ = 0;
        onGround_ = true;
    }
    px_ = std::clamp(px_, 24.f, WORLD - 24.f);
    if (mode_ != Mode::Play) return;

    const float bodyL = px_ - 10.f;
    const float bodyR = px_ + 10.f;
    const float head = py_ - 32.f;
    if (inv_ <= 0) {
        for (const Pour& p : pours_) {
            if (!pourHot(p, t_)) continue;
            if (bodyR > p.x - 12.f && bodyL < p.x + 12.f && head < FLOOR && py_ > 70.f) {
                hurt();
                break;
            }
        }
    }
    if (inv_ <= 0 && onGround_ && mode_ == Mode::Play) {
        for (const Roll& r : rolls_) {
            if (bodyR > r.x - 16.f && bodyL < r.x + 16.f) {
                hurt();
                break;
            }
        }
    }
    tryGrab();
    if (carry_ && px_ < HOME && onGround_) win();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y + float(g.h) * scale, std::max(8.f, float(g.h) * scale), pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip) {
    if (h < 2.f || m.h <= 0) return;
    float s = h / float(m.h);
    float w = float(m.w) * s;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.x = int(std::lround(cx - cam_ - w * 0.5f));
    sp.y = int(std::lround(foot - h));
    sp.w = std::max(1, int(std::lround(w)));
    sp.h = std::max(1, int(std::lround(h)));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H);
        int r = int(1 + u * 6);
        int g = int(1 + u * 2);
        int b = 2;
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = uint8_t(u > 0.82f ? 3 : 0);
        vdp.road[y].on = false;
    }
    cam_ = std::clamp(px_ - 150.f, 0.f, WORLD - float(gs::SCREEN_W));

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        if (wx < cam_ - 80 || wx > cam_ + gs::SCREEN_W + 80) return;
        spr(m, wx, foot, h, pal, flip);
    };

    world(art_.arch, 78.f, FLOOR + 2.f, 86.f, PAL_BRICK, false);
    for (float x = 220.f; x < 900.f; x += 180.f) world(art_.stack, x, FLOOR, 52.f, PAL_SOOT, false);
    world(art_.stack, 1880.f, FLOOR, 64.f, PAL_SOOT, false);

    int step = int(t_ * 8.f) & 1;
    for (const Pour& p : pours_) {
        world(art_.ladle, p.x, 58.f, 28.f, PAL_IRON, false);
        if (pourHot(p, t_)) world(art_.pour, p.x, FLOOR, 128.f, PAL_MELT, false);
    }
    for (const Roll& r : rolls_) world(art_.ingot, r.x, FLOOR, 20.f, PAL_IRON, false);

    float bannerX = carry_ ? px_ + face_ * 16.f : bx_;
    float bannerFoot = carry_ ? py_ - 6.f : FLOOR;
    if (!(inv_ > 0 && int(t_ * 14.f) & 1))
        world(art_.worker[step], px_, py_, 46.f, PAL_YOU, face_ < 0);
    world(art_.banner[step], bannerX, bannerFoot, carry_ ? 52.f : 64.f, PAL_BANNER, false);

    if (mode_ == Mode::Title) {
        text("S3 FOUNDRY BANN", 160, 48, 2.f, PAL_HUD);
        text("BRING THE BANNER BACK", 160, 92, 1.f, PAL_BANNER);
        text("ANYTHING ELSE IS A LOSS", 160, 112, 1.f, PAL_ALERT);
        text("START", 160, 150, 1.f, PAL_OK);
    } else if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char buf[40];
        std::snprintf(buf, sizeof(buf), "LIVES %d", lives_);
        hud(1, 1, buf, PAL_HUD);
        hud(28, 1, carry_ ? "BANNER" : "FETCH", carry_ ? PAL_OK : PAL_ALERT);
        if (mode_ == Mode::Pause) hudC(12, "PAUSED", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(10, "THE BANNER IS BACK", PAL_OK);
        hudC(12, "IN THE FOUNDRY", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(12, reason_, PAL_ALERT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START)) bootPlay();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        if (!bot_) {
            float dir = 0;
            if (pad.down(gs::BTN_LEFT)) dir -= 1.f;
            if (pad.down(gs::BTN_RIGHT)) dir += 1.f;
            bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
            vx_ = dir * (carry_ ? SPEED * 0.86f : SPEED);
            if (dir != 0) face_ = dir > 0 ? 1 : -1;
            if (jump && onGround_) {
                vy_ = JUMP;
                onGround_ = false;
            }
        }
        if (!over_) update(dt);
    }
    if (mode_ == Mode::Play && bot_ && !over_) {
        /* update already ran */
    }
    draw();
    if (mode_ != Mode::Play && mode_ != Mode::Pause) {
        /* title still animates the cloth */
        t_ += dt;
    }
}

}  // namespace foundry
