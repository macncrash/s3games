#include "sally.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string>

namespace sally {

namespace {

constexpr float kHorizon = 72.f;
constexpr float kNear = 0.16f;
constexpr float kFar = 1.05f;
constexpr float kPitSpeed = 0.9f;

const float kSpawnAt[] = {1.0f, 2.35f, 3.7f, 5.05f, 6.4f, 7.75f};
const int kSpawnLane[] = {0, 2, 1, 0, 2, 1};

gs::Bitmap machineBody(int body, int trim, int glass, int lamp) {
    gs::Bitmap b(56, 36);
    b.ellipse(28, 22, 22, 12, body);
    b.poly({{28, 3}, {46, 18}, {10, 18}}, trim);
    b.rect(16, 14, 24, 10, glass);
    b.ellipse(28, 16, 4, 3, lamp);
    b.rect(8, 22, 8, 9, 1);
    b.rect(40, 22, 8, 9, 1);
    b.rect(22, 24, 12, 5, trim);
    b.outline(1, true);
    return b;
}

void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cols) {
    int i = 1;
    for (uint16_t c : cols) {
        if (i < 16) v.setColor(p * 16 + i, c);
        i++;
    }
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Dead) return 3;
    if (stallMark_ > 0) return 2;
    return 1;
}

void Game::buildArt() {
    gs::VDP& v = sys_->vdp;
    auto up = [&](const gs::Bitmap& b) { return gs::uploadMipped(v, b); };
    art_.you = up(machineBody(3, 4, 6, 7));
    art_.rival[0] = up(machineBody(3, 4, 5, 7));
    art_.rival[1] = up(machineBody(4, 3, 6, 8));
    art_.rival[2] = up(machineBody(5, 3, 2, 7));
    art_.rival[3] = up(machineBody(2, 5, 4, 6));

    gs::Bitmap rut(40, 24);
    rut.ellipse(20, 12, 16, 8, 2);
    rut.ellipse(20, 12, 9, 4, 3);
    rut.ellipse(14, 10, 3, 2, 4);
    rut.outline(1, true);
    art_.rut = up(rut);

    gs::Bitmap smoke(32, 32);
    smoke.ellipse(16, 18, 10, 8, 2);
    smoke.ellipse(10, 12, 6, 5, 3);
    smoke.ellipse(22, 11, 5, 4, 4);
    art_.smoke = up(smoke);

    gs::Bitmap gate(96, 48);
    gate.rect(4, 16, 14, 30, 2);
    gate.rect(78, 16, 14, 30, 2);
    gate.rect(4, 8, 88, 12, 3);
    gate.rect(40, 18, 16, 22, 4);
    gate.outline(1, false);
    art_.gate = up(gate);

    gs::Bitmap lamp(16, 16);
    lamp.ellipse(8, 8, 6, 6, 3);
    lamp.ellipse(8, 8, 3, 3, 4);
    art_.lamp = up(lamp);

    for (int i = 0; i < 96; i++) {
        std::string s(1, char(32 + i));
        gs::TextStyle st;
        st.scale = 2;
        st.color = 1;
        st.spacing = 0;
        gs::Bitmap g = gs::textBitmap(s, st);
        if (g.w < 1 || g.h < 1) g = gs::Bitmap(4, 4);
        art_.glyph[i] = up(g);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    gs::VDP& v = sys.vdp;
    v.setFogColor(gs::rgb4(4, 3, 3));
    auto C = gs::rgb4;
    v.setColor(0, C(0, 0, 0));
    pal(v, PAL_HUD, {C(1, 1, 1), C(14, 13, 10), C(8, 7, 5), C(15, 12, 6)});
    pal(v, PAL_GATE, {C(1, 1, 1), C(5, 4, 4), C(8, 6, 5), C(12, 8, 4), C(3, 3, 4)});
    pal(v, PAL_YOU, {C(1, 1, 1), C(2, 4, 2), C(4, 7, 3), C(8, 10, 5), C(2, 3, 4), C(14, 12, 5), C(15, 15, 10)});
    pal(v, PAL_A, {C(1, 1, 1), C(6, 2, 2), C(10, 3, 2), C(4, 1, 1), C(12, 10, 6), C(15, 14, 8)});
    pal(v, PAL_B, {C(1, 1, 1), C(2, 3, 6), C(4, 5, 9), C(8, 8, 6), C(12, 12, 10), C(6, 7, 8)});
    pal(v, PAL_C, {C(1, 1, 1), C(6, 5, 2), C(10, 8, 3), C(3, 3, 2), C(12, 9, 4), C(14, 13, 8)});
    pal(v, PAL_D, {C(1, 1, 1), C(3, 3, 3), C(6, 6, 6), C(9, 4, 2), C(12, 10, 8), C(4, 5, 6)});
    pal(v, PAL_RUT, {C(1, 1, 1), C(3, 2, 2), C(5, 4, 3), C(8, 7, 5)});
    pal(v, PAL_SMOKE, {C(2, 2, 2), C(6, 6, 6), C(9, 9, 8), C(12, 12, 11)});
    pal(v, PAL_WIN, {C(1, 1, 1), C(14, 12, 6), C(8, 14, 6), C(15, 15, 12)});

    int rp = 12 * 16;
    v.setColor(rp + 1, C(3, 5, 2));
    v.setColor(rp + 2, C(2, 4, 2));
    v.setColor(rp + 3, C(5, 6, 3));
    v.setColor(rp + 4, C(4, 4, 3));
    v.setColor(rp + 5, C(3, 3, 2));
    v.setColor(rp + 6, C(5, 4, 3));
    v.setColor(rp + 7, C(4, 3, 3));
    v.setColor(rp + 8, C(7, 6, 5));
    v.setColor(rp + 9, C(3, 3, 2));
    v.setColor(rp + 10, C(6, 5, 4));
    v.setColor(rp + 11, C(2, 3, 4));
    v.setColor(rp + 12, C(3, 4, 5));
    v.setColor(rp + 13, C(6, 7, 8));
    v.setColor(rp + 14, C(8, 7, 4));
    v.setColor(rp + 15, C(7, 6, 4));

    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.18f, 0.25f, 0.12f);
    buildArt();

    pack_[0] = {0, 0.82f, true};
    pack_[1] = {2, 0.64f, true};
    pack_[2] = {1, 0.50f, true};
    pack_[3] = {0, 0.36f, true};
}

void Game::begin() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    score_ = 0;
    hull_ = 2;
    stalled_ = 0;
    stallMark_ = 0;
    script_ = 0;
    t_ = 0;
    lane_ = 1;
    showLane_ = 1.f;
    reason_ = "THE SALLY STALLED";
    pack_[0] = {0, 0.82f, true};
    pack_[1] = {2, 0.64f, true};
    pack_[2] = {1, 0.50f, true};
    pack_[3] = {0, 0.36f, true};
    for (auto& h : pits_) h = {};
}

void Game::project(float lane, float z, float& x, float& y, float& h) const {
    float nz = std::clamp(z, 0.04f, 1.f);
    float ny = 1.f - nz;
    y = kHorizon + ny * (206.f - kHorizon);
    float hw = 14.f + ny * ny * 148.f;
    float cx = 160.f + bend_ * ny;
    x = cx + (lane - 1.f) * hw * 0.78f;
    h = 10.f + ny * 46.f;
}

void Game::paintRoad() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(scroll_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / 223.f;
        int skyR = int(2 + (1.f - u) * 6);
        int skyB = int(6 + (1.f - u) * 4);
        int skyG = int(2 + u * 3);
        if (y > kHorizon) {
            skyR = 3;
            skyG = 3;
            skyB = 3;
        }
        v.lineBackdrop[y] = gs::rgb4(skyR, skyG, skyB);
        v.lineFog[y] = y < kHorizon ? uint8_t(2 + y / 18) : uint8_t(std::max(0, 8 - (y - int(kHorizon)) / 12));
        gs::RoadLine& r = v.road[y];
        if (y <= kHorizon) {
            r.on = false;
            continue;
        }
        float ny = (y - kHorizon) / (223.f - kHorizon);
        r.on = true;
        r.cx = 160.f + bend_ * ny;
        r.hw = 16.f + ny * ny * 150.f;
        r.v = scroll_ + (1.f - ny) * 80.f;
        r.pal = 12;
        r.band = (int(r.v / 18.f) & 1) ? 1 : 0;
        r.style = gs::ROAD_RUTS;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }
}

void Game::botPick() {
    float threat[3] = {9.f, 9.f, 9.f};
    for (const auto& h : pits_) {
        if (!h.live || h.z > 0.78f || h.z < 0.08f) continue;
        threat[h.lane] = std::min(threat[h.lane], h.z);
    }
    int best = lane_;
    float far = -1.f;
    for (int i = 0; i < 3; i++) {
        if (threat[i] > far + 0.001f) {
            far = threat[i];
            best = i;
        }
    }
    lane_ = best;
    showLane_ = float(lane_);
}

void Game::update(float dt) {
    if (mode_ == Mode::Title) {
        titleTicks_++;
        scroll_ += 30.f * dt;
        bend_ = std::sin(scroll_ * 0.01f) * 36.f;
        if (bot_ && titleTicks_ > 50) begin();
        if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A)) begin();
        return;
    }
    if (mode_ != Mode::Run) {
        scroll_ += 10.f * dt;
        bend_ = std::sin(scroll_ * 0.01f) * 20.f;
        if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) begin();
        return;
    }

    t_ += dt;
    scroll_ += 90.f * dt;
    bend_ = std::sin(scroll_ * 0.012f) * 42.f;
    score_ = stalled_ * 100 + int(t_ * 10.f);

    if (!bot_) {
        float ax = sys_->pad.axisX;
        bool flick = std::fabs(ax) > 0.55f && !stick_;
        stick_ = std::fabs(ax) > 0.55f;
        if (sys_->pad.pressed(gs::BTN_LEFT) || (flick && ax < 0)) lane_ = std::max(0, lane_ - 1);
        if (sys_->pad.pressed(gs::BTN_RIGHT) || (flick && ax > 0)) lane_ = std::min(2, lane_ + 1);
    } else {
        botPick();
    }
    if (!bot_) {
        float goal = float(lane_);
        showLane_ += std::clamp(goal - showLane_, -dt * 7.f, dt * 7.f);
    }

    while (script_ < 6 && t_ >= kSpawnAt[script_]) {
        for (auto& h : pits_) {
            if (h.live) continue;
            h.live = true;
            h.lane = kSpawnLane[script_];
            h.z = kFar;
            h.prev = kFar;
            break;
        }
        script_++;
        sys_->apu.noiseBurst(0.25f, 1400.f, 0.15f);
    }

    for (auto& h : pits_) {
        if (!h.live) continue;
        h.prev = h.z;
        h.z -= kPitSpeed * dt;
        for (auto& r : pack_) {
            if (!r.live || r.lane != h.lane) continue;
            if (h.prev >= r.z && h.z < r.z) {
                r.live = false;
                stalled_++;
                stallMark_ = 40;
                score_ = stalled_ * 100 + int(t_ * 10.f);
                sys_->apu.noiseBurst(0.45f, 700.f, 0.3f);
                sys_->apu.tone(2, 90.f, 0.12f);
            }
        }
        int shown = bot_ ? lane_ : int(std::round(showLane_));
        if (h.prev >= kNear && h.z < kNear && shown == h.lane) {
            hull_--;
            h.live = false;
            sys_->apu.noiseBurst(0.6f, 400.f, 0.35f);
            if (hull_ <= 0) {
                mode_ = Mode::Dead;
                won_ = false;
                over_ = true;
                reason_ = "THE SALLY STALLED";
                sys_->apu.tone(0, 0, 0);
                sys_->apu.tone(1, 0, 0);
                return;
            }
        }
        if (h.z < 0.02f) h.live = false;
    }
    if (stallMark_ > 0) stallMark_--;

    bool any = false;
    for (const auto& r : pack_)
        if (r.live) any = true;
    bool pits = false;
    for (const auto& h : pits_)
        if (h.live) pits = true;
    if (!any && !pits && script_ >= 6 && hull_ > 0) {
        mode_ = Mode::Victory;
        won_ = true;
        over_ = true;
        score_ += 500;
        reason_ = "THE LAST MACHINE STILL RUNNING";
        sys_->apu.keyOn(0, 220.f, 0.2f);
        sys_->apu.keyOn(1, 330.f, 0.15f);
    }

    float eng = 55.f + std::sin(t_ * 40.f) * 2.f;
    sys_->apu.tone(0, eng, mode_ == Mode::Run ? 0.07f : 0.f);
    sys_->apu.tone(1, eng * 1.5f, mode_ == Mode::Run ? 0.04f : 0.f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog) {
    if (h < 2.f || m.h < 1) return;
    float sc = h / float(m.h);
    gs::Sprite s;
    s.img = m.pick(h);
    s.h = int(h);
    s.w = std::max(1, int(m.w * sc));
    s.x = int(std::round(cx - s.w * 0.5f));
    s.y = int(std::round(cy - s.h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float cx = x;
    for (const char* p = s; *p; p++) {
        int gi = int(*p) - 32;
        if (gi < 0 || gi > 95) gi = 0;
        const gs::Mipped& g = art_.glyph[gi];
        float h = std::max(8.f, g.h * scale);
        float w = g.h > 0 ? h * (float(g.w) / float(g.h)) : 8.f;
        if (*p != ' ') spr(g, cx + w * 0.5f, y + h, h, pal, 0);
        cx += w + scale * 2.f;
    }
}

void Game::banner(const char* s, float y, int pal) {
    float w = 0;
    for (const char* p = s; *p; p++) {
        int gi = int(*p) - 32;
        if (gi < 0 || gi > 95) gi = 0;
        const gs::Mipped& g = art_.glyph[gi];
        float h = 18.f;
        float gw = g.h > 0 ? h * (float(g.w) / float(g.h)) : 8.f;
        w += gw + 2.f;
    }
    text(s, 160.f - w * 0.5f, y, 18.f / 14.f, pal);
}

void Game::draw() {
    paintRoad();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    if (mode_ == Mode::Title || t_ < 1.2f) {
        float x, y, h;
        project(1.f, 0.92f, x, y, h);
        spr(art_.gate, x, y + 8.f, h * 1.6f, PAL_GATE, 6);
        spr(art_.lamp, x - h * 0.7f, y - h * 0.2f, h * 0.35f, PAL_WIN, 4);
        spr(art_.lamp, x + h * 0.7f, y - h * 0.2f, h * 0.35f, PAL_WIN, 4);
    }

    // First sprite owns the pixel, so the player and nearer machines go in first.
    {
        float x, y, h;
        project(showLane_, 0.10f, x, y, h);
        if (mode_ != Mode::Dead) spr(art_.you, x, y + 6.f, h, PAL_YOU, 0);
        else spr(art_.smoke, x, y, h, PAL_SMOKE, 0);
    }
    int order[4] = {0, 1, 2, 3};
    std::sort(order, order + 4, [&](int a, int b) { return pack_[a].z < pack_[b].z; });
    // near (small z) first
    for (int k = 0; k < 4; k++) {
        const Rival& r = pack_[order[k]];
        float x, y, h;
        project(float(r.lane), r.z, x, y, h);
        int fog = int((r.z) * 10.f);
        if (r.live) spr(art_.rival[order[k]], x, y, h * 0.85f, PAL_A + order[k], fog);
        else spr(art_.smoke, x, y - 2.f, h * 0.7f, PAL_SMOKE, fog);
    }
    for (const auto& pit : pits_) {
        if (!pit.live) continue;
        float x, y, h;
        project(float(pit.lane), pit.z, x, y, h);
        spr(art_.rut, x, y + h * 0.15f, h * 0.45f, PAL_RUT, int(pit.z * 8));
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        banner("ONE SALLY", 28.f, PAL_HUD);
        banner("LAST MACHINE RUNNING", 54.f, PAL_WIN);
        banner("LEFT RIGHT  START", 188.f, PAL_HUD);
    } else if (mode_ == Mode::Run) {
        std::snprintf(buf, sizeof buf, "HULL %d   STALLED %d", hull_, stalled_);
        text(buf, 8.f, 6.f, 1.f, PAL_HUD);
        text("ONE SALLY", 8.f, 24.f, 1.f, PAL_WIN);
    } else if (mode_ == Mode::Victory) {
        banner("LAST MACHINE", 64.f, PAL_WIN);
        banner("STILL RUNNING", 90.f, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        banner(buf, 130.f, PAL_HUD);
    } else {
        banner("SALLY STALLED", 80.f, PAL_A);
        banner(reason_, 110.f, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.f / 60.f);
    draw();
}

}  // namespace sally
