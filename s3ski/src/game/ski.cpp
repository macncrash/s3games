#include "game/ski.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ski {
namespace {

constexpr float SPEED = 0.26f;
constexpr float GAP = 18.f;
constexpr float FIRST = 32.f;
constexpr float GATE_HALF = 0.52f;
constexpr float BANK = 1.90f;
constexpr float PISTE = 2.20f;
constexpr float SLOW_AT = 1.55f;
constexpr float HORIZON = 74.f;
constexpr float CAM_BACK = 6.6f;
constexpr float DEPTH_K = 820.f;
constexpr float X_SCALE = 960.f;
constexpr float POLE_H = 1.65f;
constexpr float DT = 1.f / 60.f;

constexpr float kOff[Game::kGates] = {-0.48f, 0.52f, -0.58f, 0.42f, 0.64f, -0.36f, -0.66f, 0.50f, -0.44f, 0.38f, -0.55f, 0.24f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float bend(float z) { return std::sin(z * 0.028f) * 0.62f; }

int fogFor(float dz) {
    if (dz > 60.f) return 10;
    if (dz > 40.f) return 6;
    if (dz > 26.f) return 3;
    if (dz > 16.f) return 1;
    return 0;
}

}  // namespace

void Game::layCourse() {
    for (int i = 0; i < kGates; i++) {
        Gate& g = gates_[i];
        g.z = FIRST + float(i) * GAP;
        g.x = bend(g.z) + kOff[i];
        g.red = (i % 2) == 0;
        g.done = false;
        g.clean = false;
    }
    finishZ_ = gates_[kGates - 1].z + 22.f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.09f, 0.28f, 0.16f);
    layCourse();
    mode_ = Mode::Title;
    hold_ = 0;
    t_ = 0;
    tick_ = 0;
    over_ = false;
    won_ = false;
    clean_ = 0;
    misses_ = 0;
    fanStep_ = -1;
}

void Game::beginRun() {
    for (auto& g : gates_) {
        g.done = false;
        g.clean = false;
    }
    clean_ = 0;
    misses_ = 0;
    won_ = false;
    over_ = false;
    z_ = 0;
    x_ = bend(0);
    vx_ = 0;
    camX_ = x_;
    runT_ = 0;
    sayT_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    fanT_ = 0;
    fallRight_ = false;
    mode_ = Mode::Run;
    blip(330.f, 0.08f, 6);
    sys_->apu.tone(1, 220.f, 0.04f);
}

void Game::poseTitle() {
    z_ = 10.f;
    camX_ = bend(z_);
    x_ = camX_ + std::sin(t_ * 1.3f) * 0.22f;
    vx_ = std::cos(t_ * 1.3f) * 0.008f;
}

float Game::steerInput() const {
    if (bot_) {
        int nxt = -1;
        for (int i = 0; i < kGates; i++) {
            if (!gates_[i].done && z_ < gates_[i].z) {
                nxt = i;
                break;
            }
        }
        float tx;
        if (nxt < 0) {
            tx = bend(z_ + 12.f);
        } else {
            tx = gates_[nxt].x;
            float dist = gates_[nxt].z - z_;
            if (nxt + 1 < kGates && dist < 11.f) {
                float u = (11.f - dist) / 11.f;
                tx = gates_[nxt].x * (1.f - 0.2f * u) + gates_[nxt + 1].x * (0.2f * u);
            }
        }
        float pred = x_ + vx_ * 8.f;
        return clampf((tx - pred) * 4.2f - vx_ * 8.f, -1.f, 1.f);
    }
    const gs::Pad& pad = sys_->pad;
    if (std::fabs(pad.axisX) > 0.08f) return clampf(pad.axisX, -1.f, 1.f);
    float s = 0;
    if (pad.down(gs::BTN_LEFT)) s -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) s += 1.f;
    return s;
}

void Game::physics() {
    float steer = steerInput();
    vx_ += steer * 0.0034f;
    vx_ *= 0.93f;
    x_ += vx_;
    float mid = bend(z_);
    if (x_ > mid + BANK) {
        x_ = mid + BANK;
        if (vx_ > 0) vx_ *= -0.2f;
    }
    if (x_ < mid - BANK) {
        x_ = mid - BANK;
        if (vx_ < 0) vx_ *= -0.2f;
    }
    float spd = SPEED;
    if (std::fabs(x_ - mid) > SLOW_AT) spd *= 0.62f;
    float prevZ = z_;
    float prevX = x_;
    z_ += spd;
    runT_ += DT;

    for (int i = 0; i < kGates; i++) {
        Gate& g = gates_[i];
        if (g.done || !(prevZ < g.z && z_ >= g.z)) continue;
        float u = (g.z - prevZ) / std::max(0.0001f, z_ - prevZ);
        float cx = prevX + (x_ - prevX) * u;
        g.done = true;
        if (std::fabs(cx - g.x) <= GATE_HALF) {
            g.clean = true;
            clean_++;
            say("CLEAN", PAL_GREEN, 0.45f);
            blip(460.f + float(clean_) * 32.f, 0.11f, 7);
        } else {
            misses_++;
            shake_ = 6.f;
            say(misses_ >= 3 ? "WIPEOUT" : "MISS", PAL_RED, 0.8f);
            blip(96.f, 0.13f, 12);
            sys_->apu.noiseBurst(0.24f, 1500.f, 0.14f);
            sys_->rumble(0.55f, 0.25f, 140);
            if (misses_ >= 3) {
                fallRight_ = vx_ > 0;
                won_ = false;
                over_ = true;
                mode_ = Mode::Dead;
                break;
            }
        }
    }
    if (mode_ == Mode::Run && z_ >= finishZ_ && misses_ < 3) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        sayT_ = 0;
        fanStep_ = 0;
        fanT_ = 0;
        sys_->rumble(0.2f, 0.45f, 180);
    }
}

void Game::blip(float freq, float vol, int frames) {
    if (!sys_ || fanStep_ >= 0) return;
    sys_->apu.tone(0, freq, vol);
    blip_ = frames;
}

void Game::say(const char* s, int pal, float time) {
    say_ = s;
    sayPal_ = pal;
    sayT_ = time;
}

void Game::audio() {
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Run) a.noise(0.016f, 640.f, true);
    else a.noise(0.f, 400.f, false);

    if (fanStep_ >= 0) {
        fanT_++;
        if (fanT_ == 1 || fanT_ == 9 || fanT_ == 17 || fanT_ == 27) {
            static const float n[4] = {523.f, 659.f, 784.f, 1046.f};
            int i = fanT_ < 9 ? 0 : fanT_ < 17 ? 1 : fanT_ < 27 ? 2 : 3;
            a.tone(0, n[i], 0.14f);
            a.tone(1, n[i] * 0.5f, 0.05f);
        }
        if (fanT_ > 52) {
            a.tone(0, 0, 0);
            a.tone(1, 0, 0);
            fanStep_ = -1;
        }
        return;
    }
    if (blip_ > 0) {
        blip_--;
        if (blip_ == 0) {
            a.tone(0, 0, 0);
            a.tone(1, 0, 0);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    tick_++;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - DT);
    if (shake_ > 0) {
        shake_ *= 0.86f;
        if (shake_ < 0.25f) shake_ = 0;
    }
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        hold_++;
        bool go = false;
        if (bot_) go = hold_ >= 40;
        else
            go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        if (go) beginRun();
        else poseTitle();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            hold_ = 0;
            won_ = false;
            over_ = false;
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            physics();
            camX_ += (x_ - camX_) * 0.22f;
        }
    } else if (mode_ == Mode::Dead || mode_ == Mode::Win) {
        vx_ *= 0.9f;
        if (mode_ == Mode::Win && z_ < finishZ_ + 10.f) z_ += SPEED * 0.35f;
        camX_ += (x_ - camX_) * 0.12f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) beginRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            hold_ = 0;
            won_ = false;
            over_ = false;
        }
    }
    audio();
    draw();
}

Game::Proj Game::project(float wx, float wz) const {
    Proj p;
    float dz = wz - (z_ - CAM_BACK);
    if (dz < 2.8f) return p;
    p.dz = dz;
    float s = 1.f / dz;
    p.x = 160.f + (wx - camX_) * X_SCALE * s;
    p.y = HORIZON + DEPTH_K * s;
    p.ok = p.y > HORIZON && p.y < 250.f;
    return p;
}

void Game::image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip, int fog, bool shadow) {
    if (!sys_ || h < 1.5f || h > 420.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (left > 340.f || top > 240.f || left + w < -20.f || top + h < -30.f) return;
    gs::Sprite s;
    auto q = [](float v) {
        v = clampf(v, -400.f, 800.f);
        return int16_t(std::lround(v));
    };
    s.x = q(left);
    s.y = q(top);
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip, int fog) {
    if (m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    image(m, cx - w * 0.5f, bottom - h, h, pal, flip, fog, false);
}

void Game::skierAt() {
    Proj p = project(x_, z_);
    if (!p.ok) return;
    float jig = shake_ > 0 ? (((tick_ & 1) ? 1.f : -1.f) * shake_ * 0.35f) : 0.f;
    float sx = p.x + jig;
    if (mode_ == Mode::Dead) {
        spr(art_.fall, sx, p.y + 2.f, 40.f, PAL_SKIER, fallRight_, 0);
        return;
    }
    bool carve = std::fabs(vx_) > 0.012f;
    const gs::Mipped& body = carve ? art_.carve : art_.skier;
    image(art_.shadow, sx - 22.f, p.y - 6.f, 12.f, PAL_FX, false, 0, true);
    spr(body, sx, p.y, 76.f, PAL_SKIER, carve && vx_ > 0, 0);
    if (std::fabs(vx_) > 0.015f || mode_ == Mode::Run) {
        float ph = 8.f + float((tick_ / 3) % 5);
        spr(art_.puff, sx - 14.f, p.y + 2.f, ph, PAL_FX, false, 0);
        spr(art_.puff, sx + 12.f, p.y + 1.f, ph * 0.8f, PAL_FX, true, 0);
    }
}

struct Stamp {
    float dz;
    float left, top, h;
    const gs::Mipped* m;
    int pal;
    bool flip;
    int fog;
};

void Game::world() {
    std::vector<Stamp> bits;
    bits.reserve(80);
    auto push = [&](const gs::Mipped& m, float left, float top, float h, float dz, int pal, bool flip) {
        if (h < 3.f || h > 340.f || m.h < 1) return;
        bits.push_back({dz, left, top, h, &m, pal, flip, fogFor(dz)});
    };
    auto pushCenter = [&](const gs::Mipped& m, float cx, float bottom, float h, float dz, int pal, bool flip) {
        float w = h * float(m.w) / float(std::max(1, int(m.h)));
        push(m, cx - w * 0.5f, bottom - h, h, dz, pal, flip);
    };
    auto pushPole = [&](float wx, float wz, bool flip, int pal) {
        Proj p = project(wx, wz);
        if (!p.ok) return;
        float sh = POLE_H * X_SCALE / p.dz;
        if (sh < 4.f || sh > 300.f) return;
        float sw = sh * float(art_.pole.w) / float(std::max(1, int(art_.pole.h)));
        float u0 = kPoleShaft / float(kPoleW);
        float u = flip ? (1.f - u0) : u0;
        push(art_.pole, p.x - u * sw, p.y - sh, sh, p.dz, pal, flip);
    };
    auto pushProp = [&](const gs::Mipped& m, float wx, float wz, float worldH, int pal, bool flip) {
        Proj p = project(wx, wz);
        if (!p.ok) return;
        float sh = worldH * X_SCALE / p.dz;
        pushCenter(m, p.x, p.y, sh, p.dz, pal, flip);
    };

    for (int i = 0; i < kGates; i++) {
        const Gate& g = gates_[i];
        int pal = g.red ? PAL_GATE_R : PAL_GATE_B;
        pushPole(g.x - GATE_HALF, g.z, false, pal);
        pushPole(g.x + GATE_HALF, g.z, true, pal);
    }
    float fz = finishZ_;
    float mid = bend(fz);
    pushPole(mid - 1.15f, fz, false, PAL_BANNER);
    pushPole(mid + 1.15f, fz, true, PAL_BANNER);
    Proj ban = project(mid, fz);
    if (ban.ok) {
        float gap = 2.3f * X_SCALE / ban.dz;
        float bh = clampf(gap * float(art_.banner.h) / float(std::max(1, int(art_.banner.w))), 4.f, 70.f);
        float pole = POLE_H * X_SCALE / ban.dz;
        pushCenter(art_.banner, ban.x, ban.y - pole * 0.62f, bh, ban.dz, PAL_BANNER, false);
    }
    pushPole(bend(16.f) - 1.25f, 16.f, false, PAL_GATE_R);
    pushPole(bend(16.f) + 1.25f, 16.f, true, PAL_GATE_R);

    for (int i = 0; i < 28; i++) {
        float wz = 6.f + float(i) * 11.f;
        float side = (i & 1) ? 1.f : -1.f;
        float j = float((i * 17) % 5) * 0.08f;
        float wx = bend(wz) + side * (PISTE + 0.55f + j);
        int kind = i % 9;
        if (kind == 0) pushProp(art_.man, wx, wz, 1.15f, PAL_MAN, side < 0);
        else if (kind == 4) pushProp(art_.rock, wx, wz, 0.55f, PAL_ROCK, false);
        else pushProp(art_.tree, wx, wz, 2.05f, PAL_TREE, side < 0);
    }

    std::sort(bits.begin(), bits.end(), [](const Stamp& a, const Stamp& b) { return a.dz < b.dz; });
    for (const Stamp& s : bits) image(*s.m, s.left, s.top, s.h, s.pal, s.flip, s.fog, false);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    float jig = 0;
    if (shake_ > 0) jig = ((tick_ & 1) ? shake_ : -shake_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < int(HORIZON)) {
            float u = float(y) / HORIZON;
            int r = 3 + int(u * 8.f);
            int g = 6 + int(u * 7.f);
            int b = 12 + int(u * 3.f);
            v.lineBackdrop[y] = gs::rgb4(r, g, std::min(15, b));
            continue;
        }
        float dz = DEPTH_K / std::max(1.f, float(y) - HORIZON);
        float wz = (z_ - CAM_BACK) + dz;
        float mid = bend(wz);
        gs::RoadLine& rline = v.road[y];
        rline.on = true;
        rline.cx = 160.f + (mid - camX_) * X_SCALE / dz + jig;
        rline.hw = PISTE * X_SCALE / dz;
        rline.v = wz * 34.f;
        rline.pal = PAL_ROAD;
        rline.band = (int(std::floor(wz / 4.5f)) & 1) ? 1 : 0;
        rline.style = 3;
        rline.left = gs::GROUND_SNOWWALL;
        rline.right = gs::GROUND_SNOWWALL;
        int fog = 0;
        if (dz > 58.f) fog = 10;
        else if (dz > 38.f) fog = 6;
        else if (dz > 24.f) fog = 3;
        else if (dz > 15.f) fog = 1;
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = gs::rgb4(12, 14, 15);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
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

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) spr(art_.logo, 160.f, 68.f, 36.f, PAL_GOLD, false, 0);

    skierAt();
    world();

    float drift = std::fmod(t_ * 18.f, 400.f);
    spr(art_.cloud, drift - 40.f, 34.f, 16.f, PAL_FX, false, 1);
    spr(art_.cloud, std::fmod(drift * 0.7f + 180.f, 420.f) - 50.f, 26.f, 20.f, PAL_FX, true, 2);
    spr(art_.cloud, std::fmod(drift * 0.4f + 80.f, 440.f) - 40.f, 40.f, 13.f, PAL_FX, false, 1);

    spr(art_.mount, 78.f, HORIZON + 2.f, 48.f, PAL_MOUNT, false, 4);
    spr(art_.mount, 168.f, HORIZON + 6.f, 58.f, PAL_MOUNT, true, 5);
    spr(art_.mount, 250.f, HORIZON - 2.f, 42.f, PAL_MOUNT, false, 6);
    spr(art_.sun, 286.f, 30.f, 26.f, PAL_SUN, false, 0);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(0, "GATES ON A DOWNHILL", PAL_GOLD);
        hudC(1, "MISS THREE AND THE RUN IS OVER", PAL_HUD);
        hudC(2, "ARROWS STEER    ENTER DROPS", PAL_HUD);
    } else {
        int sec = int(runT_);
        int tenth = int(runT_ * 10.f) % 10;
        std::snprintf(buf, sizeof buf, "%d.%dS", sec, tenth);
        hud(1, 0, "S3 SKI", PAL_GOLD);
        hud(33, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "GATE %d/%d", std::min(clean_ + misses_, kGates), kGates);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "MISS %d/3", std::min(misses_, 3));
        int mp = misses_ == 0 ? PAL_GREEN : misses_ == 1 ? PAL_GOLD : PAL_RED;
        hud(30, 1, buf, mp);
        if (sayT_ > 0) hudC(3, say_, sayPal_);
        if (mode_ == Mode::Pause) {
            hudC(5, "PAUSED", PAL_GOLD);
            hudC(6, "ENTER RESUMES", PAL_HUD);
        } else if (mode_ == Mode::Win) {
            hudC(4, "CLEAR", PAL_GREEN);
            hudC(5, "THE HILL IS YOURS", PAL_GOLD);
            std::snprintf(buf, sizeof buf, "GATES %d   MISSES %d", clean_, misses_);
            hudC(6, buf, PAL_HUD);
            if (!bot_) hudC(7, "ENTER RETRIES", PAL_HUD);
        } else if (mode_ == Mode::Dead) {
            hudC(4, "RUN OVER", PAL_RED);
            hudC(5, "THREE GATES MISSED", PAL_GOLD);
            if (!bot_) hudC(6, "ENTER RETRIES", PAL_HUD);
        }
    }
}

}  // namespace ski
