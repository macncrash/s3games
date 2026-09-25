#include "game/mine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mine {
namespace {

constexpr int HORIZON = 68;
constexpr float YSCALE = 156.f;
constexpr float XSCALE = 380.f;
constexpr float WALL = 1.05f;
constexpr float WARN = 0.78f;
constexpr float SHOT_R = 0.22f;
constexpr float BOLT_V = 1.55f;
constexpr float kEnd = 368.f;

struct Spec {
    float z, off;
    int kind;
};

constexpr Spec kSpec[Game::kProps] = {
    {40, 0.00f, 0}, {66, 0.40f, 1}, {92, -0.38f, 0}, {118, 0.16f, 0}, {144, -0.44f, 1}, {170, 0.34f, 0},
    {196, -0.20f, 0}, {222, 0.42f, 1}, {248, -0.36f, 0}, {274, 0.14f, 0}, {300, -0.40f, 1}, {326, 0.06f, 0},
};

float lane(float z) {
    return 0.34f * std::sin(z * 0.011f) + 0.12f * std::sin(z * 0.024f + 0.6f);
}

gs::FMPatch bellPatch() {
    gs::FMPatch bell;
    bell.alg = 4;
    bell.op[0] = {3.5f, 0.5f, 0.001f, 0.4f, 0.0f, 0.3f};
    bell.op[1] = {1, 0.8f, 0.001f, 1.2f, 0.0f, 0.8f};
    bell.op[2] = {7, 0.3f, 0.001f, 0.2f, 0.0f, 0.3f};
    bell.op[3] = {2, 0.4f, 0.001f, 0.9f, 0.0f, 0.6f};
    bell.vol = 0.16f;
    bell.echo = 0.35f;
    return bell;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.22f, 0.40f, 0.28f);
    gs::FMPatch bell = bellPatch();
    for (int i = 1; i <= 3; i++) sys.apu.setPatch(i, bell);
    resetRun();
    mode_ = Mode::Title;
}

void Game::resetRun() {
    pz_ = 0;
    px_ = lane(0);
    vx_ = 0;
    lives_ = 3;
    wrecks_ = 0;
    shot_ = 0;
    score_ = 0;
    stun_ = 0;
    cool_ = 0;
    flash_ = 0;
    muzzle_ = 0;
    warnT_ = 0;
    shake_ = 0;
    lock_ = false;
    brake_ = false;
    won_ = false;
    over_ = false;
    note_ = "";
    warn_ = "";
    bolt_ = {};
    for (auto& s : sparks_) s.life = 0;
    for (int i = 0; i < kProps; i++) {
        props_[i].z = kSpec[i].z;
        props_[i].x = lane(kSpec[i].z) + kSpec[i].off;
        props_[i].kind = kSpec[i].kind;
        props_[i].shot = false;
        props_[i].flip = (i & 1) != 0;
    }
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Ride;
    sys_->setLight(12, 8, 2);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    tick_++;
    if (flash_ > 0) flash_--;
    if (muzzle_ > 0) muzzle_--;
    if (warnT_ > 0) warnT_--;
    if (cool_ > 0) cool_--;
    shake_ *= 0.90f;
    if (shake_ < 0.02f) shake_ = 0;
    for (auto& s : sparks_) {
        if (s.life <= 0) continue;
        s.life -= 0.04f;
        s.x += s.vx;
        s.z += s.vz;
        s.vh -= 0.0045f;
        s.h += s.vh;
        if (s.h < 0) {
            s.h = 0;
            s.vh *= -0.25f;
        }
    }
    if (mode_ == Mode::Title) updateTitle();
    else if (mode_ == Mode::Ride) updateRide();
    draw();
    audio();
}

void Game::updateTitle() {
    if (bot_) {
        if (tick_ > 10) begin();
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_A)) begin();
}

void Game::botPlan(float& steer, bool& brake, bool& fire) const {
    float target = lane(pz_);
    const Prop* aim = nullptr;
    float best = 1e9f;
    for (const auto& p : props_) {
        if (p.shot) continue;
        float dz = p.z - pz_;
        if (dz < 1.6f || dz > 36.f) continue;
        if (dz < best) {
            best = dz;
            aim = &p;
        }
    }
    brake = false;
    fire = false;
    if (aim) {
        float c = lane(pz_);
        target = std::clamp(aim->x, c - 0.70f, c + 0.70f);
        float err = std::fabs(px_ - aim->x);
        float dz = aim->z - pz_;
        if (dz < 9.f && err > SHOT_R * 0.75f) brake = true;
        bool inbound = bolt_.live && bolt_.z <= aim->z + 0.2f && std::fabs(bolt_.x - aim->x) <= SHOT_R;
        if (!inbound && err <= SHOT_R * 0.62f && dz <= 18.f && dz >= 2.15f) fire = true;
    }
    float err = target - px_;
    steer = std::clamp(err * 6.2f - vx_ * 18.f, -1.f, 1.f);
}

void Game::spawnBolt() {
    if (bolt_.live || cool_ > 0 || stun_ > 0) return;
    bolt_.live = true;
    bolt_.x = px_;
    bolt_.z = pz_ + 1.7f;
    cool_ = 12;
    muzzle_ = 5;
    sys_->apu.noiseBurst(0.30f, 7200.f, 0.05f);
    sys_->apu.keyOn(1, 480.f + shot_ * 22.f, 0.11f);
    sys_->rumble(0.05f, 0.35f, 50);
}

void Game::moveBolt() {
    if (!bolt_.live) return;
    float nz = bolt_.z + BOLT_V;
    for (auto& p : props_) {
        if (p.shot) continue;
        if (bolt_.z <= p.z && nz >= p.z && std::fabs(bolt_.x - p.x) <= SHOT_R) {
            p.shot = true;
            shot_++;
            bolt_.live = false;
            burst(p);
            return;
        }
    }
    if (nz > pz_ + 36.f) bolt_.live = false;
    else bolt_.z = nz;
}

void Game::burst(const Prop& p) {
    shake_ = std::max(shake_, 0.55f);
    flash_ = 4;
    sys_->apu.noiseBurst(0.28f, 2400.f, 0.07f);
    sys_->apu.keyOn(2, 320.f + shot_ * 28.f, 0.13f);
    sys_->rumble(0.25f, 0.55f, 90);
    for (int i = 0; i < 8; i++) {
        Spark& s = sparks_[(shot_ * 8 + i) % 40];
        float a = i * 0.78f + p.x;
        s.x = p.x;
        s.z = p.z;
        s.h = 0.25f + (i & 3) * 0.12f;
        s.vx = std::sin(a) * 0.035f;
        s.vz = -0.015f + (i & 1) * 0.01f;
        s.vh = 0.02f + (i & 3) * 0.012f;
        s.life = 1.f;
        s.kind = p.kind;
    }
}

bool Game::hurt(const char* why) {
    if (stun_ > 0) return false;
    wrecks_++;
    lives_--;
    stun_ = 42;
    shake_ = 1.f;
    vx_ = 0;
    bolt_.live = false;
    warn_ = why;
    warnT_ = 42;
    sys_->apu.noiseBurst(0.48f, 160.f, 0.20f);
    sys_->apu.keyOn(1, 78.f, 0.15f);
    sys_->rumble(0.85f, 0.55f, 200);
    sys_->setLight(12, 1, 1);
    if (lives_ <= 0) {
        lose(why);
        return true;
    }
    return false;
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    score_ = shot_ * 100 + (wrecks_ == 0 ? 800 : 0);
    note_ = "DAYLIGHT";
    const float notes[] = {523.3f, 659.3f, 784.0f};
    for (int i = 0; i < 3; i++) {
        sys_->apu.setPan(i + 1, (i - 1) * 0.55f);
        sys_->apu.keyOn(i + 1, notes[i], 0.16f);
    }
    sys_->setLight(15, 12, 4);
}

void Game::lose(const char* why) {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    note_ = why;
    score_ = shot_ * 100;
    sys_->setLight(6, 1, 1);
}

void Game::updateRide() {
    if (mode_ != Mode::Ride) return;
    float steer = 0;
    bool brake = false;
    bool fire = false;
    if (bot_) {
        if (stun_ == 0) botPlan(steer, brake, fire);
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) steer -= 1;
        if (p.down(gs::BTN_RIGHT)) steer += 1;
        if (std::fabs(p.axisX) > 0.15f) steer = p.axisX;
        brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.45f;
        fire = p.pressed(gs::BTN_C) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_TURBO);
    }
    steer = std::clamp(steer, -1.f, 1.f);
    brake_ = brake;
    if (fire) spawnBolt();

    float moved = 0;
    if (stun_ > 0) {
        stun_--;
        vx_ *= 0.4f;
    } else {
        vx_ += steer * 0.0042f;
        vx_ *= 0.88f;
        px_ += vx_;
        float ramp = std::min(1.f, pz_ / 12.f);
        float spd = brake ? 0.08f : (0.14f + 0.11f * ramp);
        pz_ += spd;
        moved = spd;
    }
    moveBolt();
    if (mode_ != Mode::Ride || stun_ > 0) {
        lock_ = false;
        return;
    }

    float prev = pz_ - moved;
    for (auto& p : props_) {
        if (p.shot || prev >= p.z || pz_ < p.z) continue;
        bool dead = hurt("missed a prop");
        if (!dead) {
            pz_ = std::max(0.f, p.z - 9.f);
            px_ = lane(pz_);
            vx_ = 0;
        } else {
            px_ = lane(pz_);
        }
        lock_ = false;
        return;
    }

    float off = px_ - lane(pz_);
    if (std::fabs(off) > WALL) {
        bool dead = hurt("hit the wall");
        px_ = lane(pz_);
        vx_ = 0;
        if (dead) return;
    }

    lock_ = false;
    for (const auto& p : props_) {
        if (p.shot) continue;
        float dz = p.z - pz_;
        if (dz >= 2.f && dz <= 20.f && std::fabs(px_ - p.x) <= SHOT_R) lock_ = true;
    }

    if (pz_ >= kEnd) {
        int left = 0;
        for (const auto& p : props_)
            if (!p.shot) left++;
        if (left == 0) win();
        else lose("prop left standing");
    }
}

void Game::project(float x, float z, float h, float& sx, float& sy, float& dz) const {
    dz = z - camZ_;
    float d = std::max(0.45f, dz);
    sx = 160.f + shakeX_ + (x - camX_) * (XSCALE / d);
    sy = float(HORIZON) + YSCALE / d + shakeY_ - h * (XSCALE / d);
}

int Game::fogOf(float dz) const {
    int f = int(std::clamp(dz * 0.22f, 0.f, 13.f));
    float open = daylight();
    if (open > 0.f && dz > 5.f) f = int(f * (1.f - 0.8f * open));
    if (flash_ > 0) f = std::max(0, f - 4);
    return std::clamp(f, 0, 16);
}

float Game::daylight() const {
    if (mode_ == Mode::Won) return 1.f;
    if (mode_ != Mode::Ride && mode_ != Mode::Lost) return 0.f;
    float d = kEnd - pz_;
    if (d > 56.f) return 0.f;
    if (d < 0.f) d = 0.f;
    return 1.f - d / 56.f;
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camZ_ = 26.f + std::sin(tick_ * 0.045f) * 3.f;
        camX_ = lane(camZ_);
    } else {
        camZ_ = pz_;
        camX_ = px_;
    }
    shakeX_ = std::sin(tick_ * 2.2f) * shake_ * 6.f;
    shakeY_ = std::cos(tick_ * 1.6f) * shake_ * 3.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float destH, int pal, int fog, bool hflip, bool foot,
               bool shadow) {
    if (destH < 1.5f || m.h <= 0) return;
    float destW = destH * (float(m.w) / float(m.h));
    float top = foot ? cy - destH : cy - destH * 0.5f;
    gs::Sprite s;
    s.w = std::max(1, int(std::lround(destW)));
    s.h = std::max(1, int(std::lround(destH)));
    s.x = int(std::lround(cx - destW * 0.5f));
    s.y = int(std::lround(top));
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = hflip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float destW, float destH, int pal, int fog, bool shadow) {
    if (destW < 1.f || destH < 1.f || m.h <= 0) return;
    gs::Sprite s;
    s.w = std::max(1, int(std::lround(destW)));
    s.h = std::max(1, int(std::lround(destH)));
    s.x = int(std::lround(cx - destW * 0.5f));
    s.y = int(std::lround(cy - destH));
    s.img = m.pick(destH);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudCenter(int row, const char* s, int pal) {
    hudText(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::hudRight(int row, const char* s, int pal) {
    hudText(40 - int(std::strlen(s)), row, s, pal);
}

void Game::banner(const char* s, float cy, float destH, int pal) {
    const float gap = destH * 0.08f;
    float width = 0;
    for (const char* p = s; *p; ++p) {
        if (*p == ' ') {
            width += destH * 0.50f;
            continue;
        }
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        width += destH * (float(g.w) / float(std::max(1, int(g.h)))) + gap;
    }
    float x = 160.f + shakeX_ - width * 0.5f;
    for (const char* p = s; *p; ++p) {
        if (*p == ' ') {
            x += destH * 0.50f;
            continue;
        }
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float w = destH * (float(g.w) / float(std::max(1, int(g.h))));
        float h = destH;
        gs::Sprite sp;
        sp.w = std::max(1, int(std::lround(w)));
        sp.h = std::max(1, int(std::lround(h)));
        sp.x = int(std::lround(x));
        sp.y = int(std::lround(cy - h * 0.5f));
        sp.img = g.pick(h);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += w + gap;
    }
}

void Game::skyRoad() {
    gs::VDP& v = sys_->vdp;
    float open = daylight();
    int fr = 1 + int(9 * open);
    int fg = 1 + int(7 * open);
    int fb = 2 + int(3 * open);
    v.setFogColor(gs::rgb4(std::min(fr, 15), std::min(fg, 15), std::min(fb, 15)));
    bool near = mode_ == Mode::Ride && std::fabs(camX_ - lane(camZ_)) > WARN;
    v.setColor(PAL_ROAD * 16 + 4, near ? gs::rgb4(13, 2, 1) : gs::rgb4(7, 5, 3));
    v.setColor(PAL_ROAD * 16 + 5, near ? gs::rgb4(7, 1, 1) : gs::rgb4(4, 3, 2));
    int lamp = (tick_ / 4) % 6 == 0 ? 11 : 15;
    v.setColor(PAL_CART * 16 + 8, gs::rgb4(lamp, lamp, std::min(15, lamp)));
    v.setColor(PAL_WOOD * 16 + 7, gs::rgb4(15, 12 + (tick_ / 6) % 3, 10));

    float drift = camZ_ * 16.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float climb = (HORIZON - y) / float(HORIZON);
        if (climb < 0) climb = 0;
        v.A.hscroll[y] = int16_t(-camX_ * (16.f + 34.f * climb) + shakeX_);
        v.A.vscroll[y] = int16_t(drift);
        if (y < HORIZON) {
            float t = y / float(HORIZON);
            int r = 1 + int(t * 2 + open * 10 * t);
            int g = 1 + int(t * 1 + open * 8 * t);
            int b = 2 + int(open * 4 * t);
            v.lineBackdrop[y] = gs::rgb4(std::min(r, 15), std::min(g, 15), std::min(b, 15));
            int fog = int((1.f - t) * 3.f + t * (11.f - 9.f * open));
            v.lineFog[y] = uint8_t(std::clamp(fog, 0, 16));
            v.road[y].on = false;
            continue;
        }
        float dz = YSCALE / float(y - HORIZON);
        float z = camZ_ + dz;
        int fog = fogOf(dz);
        v.lineFog[y] = uint8_t(fog);
        int br = 1 + int(open * 8);
        int bg = 1 + int(open * 6);
        v.lineBackdrop[y] = gs::rgb4(std::min(br, 15), std::min(bg, 15), 2);
        gs::RoadLine& rl = v.road[y];
        rl.on = true;
        rl.cx = 160.f + shakeX_ + (lane(z) - camX_) * (XSCALE / dz);
        rl.hw = WALL * XSCALE / dz;
        rl.v = z * 22.f;
        rl.pal = PAL_ROAD;
        rl.band = (int(std::floor(z / 2.4f)) & 1) ? 1 : 0;
        rl.style = 0;
        rl.left = rl.right = gs::GROUND_LAND;
    }
}

void Game::drawWorld() {
    struct Item {
        float dz;
        int kind;
        int index;
        float side;
    };
    Item items[48];
    int n = 0;
    auto push = [&](float dz, int kind, int index, float side) {
        if (n < 48 && dz > 0.65f && dz < 42.f) items[n++] = {dz, kind, index, side};
    };
    for (int i = 0; i < kProps; i++) {
        if (props_[i].shot) continue;
        push(props_[i].z - camZ_, props_[i].kind == 1 ? 1 : 0, i, 0);
    }
    float step = 10.f;
    float z0 = std::ceil((camZ_ + 1.4f) / step) * step;
    for (int i = 0; i < 5; i++) {
        float z = z0 + i * step;
        if (z > kEnd - 4.f) break;
        push(z - camZ_, 2, i, -1.f);
        push(z - camZ_, 2, i, 1.f);
        push(z - camZ_, 3, i, 0);
    }
    float exitDz = kEnd - camZ_;
    if (exitDz < 60.f) push(std::max(exitDz, 1.3f), 4, 0, 0);

    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.dz < b.dz; });
    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        float sx, sy, dz;
        int fog = fogOf(it.dz);
        if (it.kind == 0 || it.kind == 1) {
            const Prop& p = props_[it.index];
            project(p.x, p.z, 0, sx, sy, dz);
            float worldH = it.kind == 1 ? 0.95f : 1.48f;
            float pixH = std::min(156.f, worldH * XSCALE / std::max(dz, 0.45f));
            const gs::Mipped& img = it.kind == 1 ? art_.ore : art_.prop;
            sprBox(art_.shadow, sx, sy, pixH * 0.7f, pixH * 0.16f, PAL_HUD, fog, true);
            spr(img, sx, sy, pixH, it.kind == 1 ? PAL_ORE : PAL_WOOD, fog, p.flip, true, false);
        } else if (it.kind == 2) {
            float z = camZ_ + it.dz;
            float x = lane(z) + it.side * (WALL + 0.12f);
            project(x, z, 0, sx, sy, dz);
            float pixH = std::min(200.f, 1.72f * XSCALE / std::max(dz, 0.45f));
            spr(art_.rib, sx, sy, pixH, PAL_RIB, fog, it.side < 0, true, false);
        } else if (it.kind == 3) {
            float z = camZ_ + it.dz;
            float span = (WALL + 0.12f) * 2.f * XSCALE / std::max(it.dz, 0.45f);
            project(lane(z), z, 1.62f, sx, sy, dz);
            float bh = std::max(5.f, 0.20f * XSCALE / std::max(it.dz, 0.45f));
            sprBox(art_.cap, sx, sy + bh, std::min(span, 340.f), bh, PAL_RIB, fog, false);
        } else {
            float use = std::max(kEnd - camZ_, 1.5f);
            project(lane(kEnd), camZ_ + use, 0.2f, sx, sy, dz);
            float pixH = std::min(200.f, 2.8f * XSCALE / use);
            int ef = int(std::clamp((kEnd - camZ_ - 6.f) / 5.f, 0.f, 12.f));
            spr(art_.exit, sx, sy + pixH * 0.15f, pixH, PAL_EXIT, ef, false, true, false);
        }
    }
}

void Game::drawCart() {
    float bob = std::sin(camZ_ * 8.f) * 1.2f;
    if (mode_ == Mode::Title) bob = std::sin(tick_ * 0.08f) * 3.f;
    float foot = 206.f + bob + shakeY_;
    float cx = 160.f + shakeX_;
    int frame = int(std::floor(camZ_ * 3.f)) & 1;
    spr(art_.cart[frame], cx, foot, 72.f, PAL_CART, 0, false, true, false);
    float puff = 8.f + float(tick_ % 4);
    spr(art_.dust, cx - 26.f, foot - 2.f, puff, PAL_DUST, 1, false, true, false);
    spr(art_.dust, cx + 26.f, foot - 1.f, puff * 0.9f, PAL_DUST, 1, true, true, false);
    if (muzzle_ > 0) spr(art_.bolt, cx + 10.f, foot - 58.f, 16.f + muzzle_ * 2.f, PAL_BOLT, 0, false, false, false);
}

void Game::draw() {
    camera();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    skyRoad();

    if (mode_ == Mode::Title) {
        banner("S3 MINE", 20, 30, PAL_AMBER);
        banner("SHOOT THE PROPS", 48, 15, PAL_HUD);
        banner("MISS THE WALLS", 68, 15, PAL_HUD);
        hudCenter(11, "ARROWS STEER  C FIRES  DOWN BRAKES", PAL_AMBER);
        if ((tick_ / 30) % 2 == 0) hudCenter(12, "PRESS START", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        banner("DAYLIGHT", 108, 28, PAL_GOOD);
        char b[40];
        std::snprintf(b, sizeof b, "SHOT %d PROPS", shot_);
        hudCenter(16, b, PAL_AMBER);
        hudCenter(17, wrecks_ == 0 ? "WALLS CLEAR" : "PATCHED THE CART", wrecks_ == 0 ? PAL_GOOD : PAL_AMBER);
        if (!bot_) hudCenter(19, "START RIDES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        banner("CAVE-IN", 108, 28, PAL_BAD);
        hudCenter(16, note_, PAL_BAD);
        if (!bot_) hudCenter(18, "START RIDES AGAIN", PAL_HUD);
    }

    if (mode_ != Mode::Title) {
        char b[40];
        hudText(1, 0, "S3 MINE", PAL_AMBER);
        std::snprintf(b, sizeof b, "LIVES %d", std::max(0, lives_));
        hudRight(0, b, lives_ > 1 ? PAL_HUD : PAL_BAD);
        std::snprintf(b, sizeof b, "PROPS %d/%d", shot_, kProps);
        hudText(1, 1, b, PAL_HUD);
        int show = mode_ == Mode::Won ? score_ : shot_ * 100;
        std::snprintf(b, sizeof b, "SCORE %d", show);
        hudRight(1, b, PAL_AMBER);
        if (mode_ == Mode::Ride) {
            if (warnT_ > 0 && warn_[0]) hudCenter(2, warn_, PAL_BAD);
            else if (std::fabs(camX_ - lane(camZ_)) > WARN) hudCenter(2, "WALL", PAL_BAD);
            else if (lock_) hudCenter(2, "FIRE", PAL_GOOD);
            else if (shot_ == 0) hudCenter(2, "STEER ONTO A PROP", PAL_HUD);
        }
    }

    bool showAim = mode_ == Mode::Ride || mode_ == Mode::Title;
    if (showAim) {
        int pal = lock_ ? PAL_GOOD : PAL_AMBER;
        float rh = lock_ ? 15.f + std::sin(tick_ * 0.45f) * 2.f : 13.f;
        spr(art_.reticle, 160.f + shakeX_, float(HORIZON) + 8.f + shakeY_, rh, pal, 0, false, false, false);
        if (mode_ == Mode::Ride) {
            for (int i = 0; i < 4; i++) {
                float dz = 3.2f + i * 2.4f;
                float sy = float(HORIZON) + YSCALE / dz + shakeY_;
                spr(art_.spark, 160.f + shakeX_, sy, lock_ ? 6.f : 4.f, pal, fogOf(dz), false, false, false);
            }
        }
    }

    if (bolt_.live) {
        float sx, sy, dz;
        project(bolt_.x, bolt_.z, 0.42f, sx, sy, dz);
        float pix = std::max(8.f, 0.28f * XSCALE / std::max(dz, 0.45f));
        spr(art_.bolt, sx, sy, std::min(pix, 28.f), PAL_BOLT, 0, false, false, false);
    }
    for (const auto& s : sparks_) {
        if (s.life <= 0) continue;
        float sx, sy, dz;
        project(s.x, s.z, s.h, sx, sy, dz);
        if (dz < 0.5f || dz > 30.f) continue;
        float pix = std::max(3.f, 0.16f * XSCALE / dz);
        int pal = s.kind == 1 ? PAL_ORE : PAL_WOOD;
        spr(art_.spark, sx, sy, pix, pal, fogOf(dz), false, false, false);
    }

    drawCart();
    drawWorld();

    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        const gs::Pad& p = sys_->pad;
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C))) begin();
    }

    if (mode_ == Mode::Ride && std::fabs(camX_ - lane(camZ_)) > WARN && (tick_ % 10) == 0)
        sys_->rumble(0.35f, 0.08f, 40);
    if (mode_ == Mode::Title) sys_->setLight(10, 7, 2);
    else if (mode_ == Mode::Ride && std::fabs(camX_ - lane(camZ_)) <= WARN && stun_ == 0) sys_->setLight(12, 9, 3);
}

void Game::audio() {
    float vol = 0.02f;
    if (mode_ == Mode::Ride && stun_ == 0) vol = brake_ ? 0.035f : 0.055f;
    float f = 48.f + 5.f * std::sin(tick_ * 0.27f);
    if (brake_) f *= 0.75f;
    sys_->apu.tone(0, f, vol);
    float nv = (mode_ == Mode::Ride && stun_ == 0) ? 0.022f : 0.f;
    sys_->apu.noise(nv, brake_ ? 220.f : 640.f, true);
}

}  // namespace mine
