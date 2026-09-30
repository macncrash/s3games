#include "game/slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace slip {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZ = 84;
constexpr float SLIP_Z = 520.0f;
constexpr float TIDE0 = 42.0f;
constexpr float NEAR = 280.0f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Race) return 1;
    if (mode_ == Mode::Win) return 2;
    return 3;
}

void Game::blip(float freq, float vol) { sys_->apu.tone(1, freq, vol); }

void Game::engine() {
    if (mode_ != Mode::Race) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    float f = 70.0f + std::fabs(spd_) * 7.5f;
    sys_->apu.tone(0, f, 0.045f + std::fabs(spd_) * 0.0015f);
    sys_->apu.noise(splash_ > 0 ? 0.08f : 0.0f, 9000.0f, false);
}

void Game::startRun() {
    mode_ = Mode::Race;
    over_ = false;
    won_ = false;
    crewIn_ = false;
    hold_ = 0;
    tide_ = TIDE0;
    z_ = 0;
    x_ = 0;
    spd_ = 0;
    rz_ = 18;
    rx_ = 0.2f;
    lean_ = 0;
    bump_ = 0;
    splash_ = 0;
    cause_ = "";
    nprop_ = 0;
    auto add = [&](float z, float x, float h, int kind) {
        if (nprop_ < 24) props_[nprop_++] = {z, x, h, kind};
    };
    const float crates[][2] = {{96, 0.62f}, {158, -0.64f}, {228, 0.70f}, {300, -0.58f}, {372, 0.66f}, {444, -0.70f}};
    for (auto& c : crates) add(c[0], c[1], 1.0f, 0);
    for (float z = 40; z < SLIP_Z - 10; z += 46) {
        add(z, -1.22f, 1.6f, 1);
        add(z + 23, 1.22f, 1.6f, 1);
    }
    add(SLIP_Z - 2, -0.78f, 2.1f, 1);
    add(SLIP_Z - 2, 0.78f, 2.1f, 1);
    add(SLIP_Z + 26, 0.05f, 2.4f, 2);
    add(70, -1.45f, 1.1f, 3);
    add(210, 1.48f, 1.1f, 3);
    add(360, -1.50f, 1.1f, 3);
    add(490, 1.40f, 1.1f, 3);
    blip(660, 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    gs::FMPatch horn;
    horn.alg = 4;
    horn.vol = 0.2f;
    horn.op[0].mul = 1;
    horn.op[0].level = 1;
    horn.op[0].ar = 0.01f;
    horn.op[0].dr = 0.2f;
    horn.op[0].sl = 0.4f;
    horn.op[0].rr = 0.3f;
    sys.apu.setPatch(2, horn);
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
}

void Game::project(float wz, float wx, float worldH, float& sx, float& sy, float& sh, int& fog) const {
    float dist = wz - z_;
    if (dist < 0.35f) dist = 0.35f;
    float y = (HORIZ - 4.0f) + NEAR / dist;
    float hw = 360.0f / dist;
    sx = 160.0f + (wx - x_) * hw;
    sy = y;
    sh = std::max(2.0f, worldH * hw * 0.62f);
    fog = int(clampf((dist - 18.0f) * 0.45f, 0.0f, 14.0f));
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y + g.h * scale, g.h * scale, pal, false);
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

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    float steer = 0, gas = 0, brake = 0;
    if (bot_) {
        float remain = SLIP_Z - z_;
        float stopDist = (spd_ * spd_) / (2.0f * 32.0f) + 4.0f;
        if (remain < stopDist) brake = 1;
        else gas = 1;
        if (std::fabs(x_) > 0.04f) steer = x_ > 0 ? -1.0f : 1.0f;
        if (std::fabs(x_) < 0.04f) steer = 0;
    } else {
        if (pad.down(gs::BTN_LEFT)) steer -= 1;
        if (pad.down(gs::BTN_RIGHT)) steer += 1;
        steer += pad.axisX;
        if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.accel > 0.2f) gas = 1;
        if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.2f) brake = 1;
        steer = clampf(steer, -1.0f, 1.0f);
    }

    if (gas && !brake) spd_ += (26.0f - spd_) * 1.7f * dt;
    else if (brake) spd_ -= 34.0f * dt;
    else spd_ -= spd_ * 1.4f * dt;
    spd_ = clampf(spd_, -7.0f, 26.0f);
    lean_ += (steer - lean_) * std::min(1.0f, 8.0f * dt);
    float grip = 1.15f + std::fabs(spd_) * 0.03f;
    x_ += steer * grip * dt;
    if (std::fabs(x_) > 1.08f) {
        splash_ = 0.35f;
        spd_ *= 0.92f;
        x_ += (x_ > 0 ? -1.0f : 1.0f) * 1.6f * dt;
    }
    x_ = clampf(x_, -1.45f, 1.45f);
    z_ += spd_ * dt;
    if (bump_ > 0) bump_ -= dt;
    if (splash_ > 0) splash_ -= dt;

    for (int i = 0; i < nprop_; i++) {
        if (props_[i].kind != 0) continue;
        if (std::fabs(z_ - props_[i].z) < 2.4f && std::fabs(x_ - props_[i].x) < 0.30f && bump_ <= 0) {
            spd_ *= 0.58f;
            x_ += (x_ >= props_[i].x ? 0.18f : -0.18f);
            bump_ = 0.35f;
            blip(140, 0.08f);
            sys_->rumble(0.4f, 0.2f, 80);
        }
    }

    float rCap = (rz_ > SLIP_Z - 36.0f) ? 8.0f : 15.4f;
    rz_ += rCap * dt;
    rx_ = std::sin(rz_ * 0.045f) * 0.28f;
    if (!crewIn_ && rz_ >= SLIP_Z) crewIn_ = true;

    tide_ -= dt;
    if (tide_ < 0) tide_ = 0;

    bool inSlip = z_ >= SLIP_Z - 7.0f && z_ <= SLIP_Z + 12.0f && std::fabs(x_) < 0.32f && std::fabs(spd_) < 2.3f;
    if (inSlip && !crewIn_) {
        won_ = true;
        mode_ = Mode::Win;
        hold_ = 75;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.keyOn(2, 523, 0.18f);
        blip(784, 0.07f);
        if (bot_) {
            std::printf("berthed in the slip  tide %.1fs left  crew %.0f behind\n", tide_, z_ - rz_);
        }
        return;
    }
    if (crewIn_ && !won_) {
        mode_ = Mode::Lose;
        cause_ = "CREW TOOK THE SLIP";
        hold_ = 90;
        sys_->apu.tone(0, 0, 0);
        blip(110, 0.08f);
        return;
    }
    if (tide_ <= 0 && mode_ == Mode::Race) {
        mode_ = Mode::Lose;
        cause_ = "THE TIDE TURNED";
        hold_ = 90;
        sys_->apu.tone(0, 0, 0);
        blip(90, 0.08f);
        return;
    }
    if (z_ > SLIP_Z + 70.0f) {
        mode_ = Mode::Lose;
        cause_ = "MISSED THE SLIP";
        hold_ = 90;
        sys_->apu.tone(0, 0, 0);
        return;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    const uint16_t top = gs::rgb4(1, 2, 6);
    const uint16_t hor = gs::rgb4(14, 7, 3);
    auto mix = [](uint16_t a, uint16_t b, float t) {
        int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
        int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
        return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
    };
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = clampf(y / float(HORIZ), 0.0f, 1.0f);
        v.lineBackdrop[y] = y < HORIZ + 8 ? mix(top, hor, u * u) : gs::rgb4(1, 3, 6);
        int fog = 0;
        if (y > HORIZ - 24 && y < HORIZ + 30) fog = int((1.0f - std::fabs(y - HORIZ) / 30.0f) * 7);
        v.lineFog[y] = uint8_t(fog);
        v.road[y].on = false;
    }
    v.roadTime = int(t_ * 60);
}

void Game::quay() {
    gs::VDP& v = sys_->vdp;
    for (int y = HORIZ; y < gs::SCREEN_H; y++) {
        float dist = NEAR / float(y - (HORIZ - 4));
        float hw = 360.0f / dist;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.0f - x_ * hw;
        r.hw = hw;
        r.v = (z_ + dist) * 26.0f;
        r.pal = PAL_FIELD;
        float wz = z_ + dist;
        bool bay = wz > SLIP_Z - 10.0f && wz < SLIP_Z + 16.0f;
        r.band = bay ? 1 : ((int(wz / 18.0f) & 1) ? 1 : 0);
        r.style = 1;
        r.left = gs::GROUND_WATER;
        r.right = gs::GROUND_WATER;
    }
}

void Game::actors() {
    struct S {
        float depth;
        float sx, sy, sh;
        int pal, fog, kind;
        bool flip;
    };
    S list[32];
    int n = 0;
    auto push = [&](float depth, float sx, float sy, float sh, int pal, int fog, int kind, bool flip) {
        if (n < 32 && sh >= 2) list[n++] = {depth, sx, sy, sh, pal, fog, kind, flip};
    };
    for (int i = 0; i < nprop_; i++) {
        float dist = props_[i].z - z_;
        if (dist < 0.5f || dist > 70.0f) continue;
        float sx, sy, sh;
        int fog;
        project(props_[i].z, props_[i].x, props_[i].h, sx, sy, sh, fog);
        int pal = PAL_WOOD;
        if (props_[i].kind == 2) pal = PAL_BOAT;
        else if (props_[i].kind == 1) pal = PAL_POST;
        else if (props_[i].kind == 3) pal = PAL_FX;
        push(dist, sx, sy, sh, pal, fog, props_[i].kind, props_[i].x > 0 && props_[i].kind == 2);
    }
    float rdist = rz_ - z_;
    if (rdist > 0.6f && rdist < 70.0f) {
        float sx, sy, sh;
        int fog;
        project(rz_, rx_, 1.15f, sx, sy, sh, fog);
        push(rdist, sx, sy, sh, PAL_CREW, fog, 4, rx_ < x_);
    } else if (rdist < -0.4f && rdist > -12.0f) {
        push(0.5f, 160.0f + (rx_ - x_) * 70.0f, 206, 34, PAL_CREW, 0, 4, true);
    }
    std::sort(list, list + n, [](const S& a, const S& b) { return a.depth < b.depth; });
    // Earlier sprites draw on top, so the near kart goes in first.
    float bob = std::sin(t_ * 18.0f) * (2.0f + std::fabs(spd_) * 0.12f);
    spr(art_.kart, 160.0f + lean_ * 10.0f, 188.0f + bob, 76.0f, PAL_YOU, lean_ < -0.15f, 0, false);
    gs::Sprite shad;
    shad.x = 128;
    shad.y = 176;
    shad.w = 64;
    shad.h = 16;
    shad.img = art_.splash.lv[0];
    shad.pal = PAL_FX;
    shad.shadow = true;
    sys_->vdp.sprite(shad);
    for (int i = 0; i < n; i++) {
        const gs::Mipped* m = &art_.crate;
        if (list[i].kind == 1) m = &art_.post;
        else if (list[i].kind == 2) m = &art_.boat;
        else if (list[i].kind == 3) m = &art_.buoy;
        else if (list[i].kind == 4) m = &art_.kart;
        spr(*m, list[i].sx, list[i].sy, list[i].sh, list[i].pal, list[i].flip, list[i].fog, false);
    }
    if (splash_ > 0) spr(art_.splash, 160.0f + (x_ > 0 ? 36.0f : -36.0f), 200, 28, PAL_FX, false, 0, false);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sky();
    if (mode_ == Mode::Title) {
        text("KARTSLIP", 160, 48, 3.0f, PAL_HUD, 0);
        text("BERTH BEFORE THE TIDE", 160, 92, 1.0f, PAL_HUD, 0);
        hudC(16, "THE OTHER CREW IS THE CLOCK", 2);
        hudC(18, "ARROWS STEER", 1);
        hudC(19, "A GAS    B BRAKE", 1);
        hudC(21, "START TO LAUNCH", 3);
        hud(1, 26, "S3 KARTSLIP", 8);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, 8);
        float sx, sy, sh;
        int fog;
        z_ = 0;
        x_ = 0;
        project(36, 0.15f, 2.2f, sx, sy, sh, fog);
        // A still of the quay behind the title, kart waiting.
        quay();
        spr(art_.kart, 168, 176, 70, PAL_YOU, false, 0, false);
        spr(art_.boat, sx, sy, sh, PAL_BOAT, false, fog, false);
        return;
    }
    quay();
    actors();
    char buf[64];
    int tidePal = tide_ < 8.0f && (int(t_ * 6) & 1) ? 4 : 3;
    std::snprintf(buf, sizeof buf, "TIDE %4.1f", tide_);
    hud(1, 1, buf, tidePal);
    float gap = rz_ - z_;
    if (gap >= 0) std::snprintf(buf, sizeof buf, "CREW +%.0f", gap);
    else std::snprintf(buf, sizeof buf, "CREW -%.0f", -gap);
    hud(40 - int(std::strlen(buf)) - 1, 1, buf, gap > 0 ? 4 : 5);
    std::snprintf(buf, sizeof buf, "SPD %02.0f", std::fabs(spd_));
    hud(1, 26, buf, 1);
    if (mode_ == Mode::Race && z_ > SLIP_Z - 40.0f && std::fabs(spd_) > 2.3f) hudC(24, "SLOW TO BERTH", 3);
    if (mode_ == Mode::Win) {
        hudC(10, "BERTHED", 5);
        hudC(12, "BEFORE THE TIDE", 1);
        std::snprintf(buf, sizeof buf, "TIDE LEFT %4.1f", tide_);
        hudC(14, buf, 3);
    } else if (mode_ == Mode::Lose) {
        hudC(11, cause_, 4);
        hudC(13, "THE SLIP IS GONE", 1);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (mode_ == Mode::Title) {
        bool go = bot_ ? t_ > 0.35f : (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A));
        if (go) startRun();
    } else if (mode_ == Mode::Race) {
        update(DT);
        engine();
    } else {
        sys.apu.tone(0, 0, 0);
        if (hold_ > 0) hold_--;
        else over_ = true;
    }
    draw();
}

}  // namespace slip
