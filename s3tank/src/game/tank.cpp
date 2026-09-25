#include "game/tank.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace tank {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FOCAL = 980.0f;
constexpr float CAM_H = 3.05f;
constexpr float CAM_BACK = 31.0f;
constexpr float HORIZON = 78.0f;
constexpr float STREET_HALF = 4.5f;
constexpr float LANE = 3.15f;
constexpr float HOLD = 2.95f;
constexpr float TANK_H = 2.3f;
constexpr float P_ACC = 7.0f;
constexpr float P_VMAX = 8.6f;
constexpr float P_COAST = 3.4f;
constexpr float P_BRAKE = 12.0f;
constexpr float R_ACC = 1.4f;
constexpr float R_VMAX = 4.8f;
constexpr float HIT_DROP = 3.7f;
constexpr float STOP_SPEED = 0.55f;
constexpr float STALL_TIME = 0.5f;
constexpr float SHELL_V = 32.0f;
constexpr float HIT_R = 1.22f;
constexpr float FIRE_CD = 0.32f;
constexpr float RIVAL_FIRE = 1.28f;
constexpr float LAT = 7.0f;
constexpr float WEAVE_A = 1.15f;
constexpr float WEAVE_W = 0.82f;
constexpr float BLOCK_LO = 8.0f;
constexpr float BLOCK_HI = 120.0f;
constexpr float SPAWN = 2.7f;
constexpr float RAM_X = 1.45f;
constexpr float RAM_Z = 2.15f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch dieselPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.55f;
    p.op[0] = {0.5f, 0.9f, 0.04f, 0.45f, 0.85f, 0.3f};
    p.op[1] = {1.0f, 0.55f, 0.03f, 0.5f, 0.7f, 0.28f, 4.0f};
    p.op[2] = {2.0f, 0.22f, 0.02f, 0.4f, 0.45f, 0.25f};
    p.op[3] = {0.25f, 0.45f, 0.08f, 0.6f, 0.8f, 0.35f};
    p.vol = 0.16f;
    p.drive = 0.62f;
    p.tone = 680.0f;
    p.vibRate = 7.0f;
    p.vibDepth = 0.018f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.3f;
    p.op[0] = {1.0f, 1.0f, 0.01f, 0.18f, 0.7f, 0.12f};
    p.op[1] = {2.0f, 0.45f, 0.01f, 0.2f, 0.5f, 0.14f};
    p.op[2] = {3.0f, 0.25f, 0.02f, 0.22f, 0.35f, 0.16f};
    p.op[3] = {1.0f, 0.35f, 0.01f, 0.2f, 0.55f, 0.14f};
    p.vol = 0.2f;
    p.drive = 0.12f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Duel || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::blip(bool high) {
    sys_->apu.tone(0, high ? 740.0f : 420.0f, 0.06f);
    beep_ = 0.05f;
}

void Game::boom() { sys_->apu.noiseBurst(0.62f, 700.0f, 0.3f); }

void Game::fanfare() {
    fanStep_ = 0;
    fanT_ = 0;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.0f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 120 || s.x + s.w < -120 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, 0, false);
    }
}

void Game::beginDuel() {
    round_++;
    weave0_ = 0.4f + round_ * 1.17f;
    player_ = {};
    player_.z = 28.0f;
    player_.face = 1;
    player_.speed = P_VMAX;
    rival_ = {};
    rival_.z = 60.0f;
    rival_.face = -1;
    rival_.speed = R_VMAX;
    rival_.x = std::sin(weave0_) * WEAVE_A;
    botSide_ = (rival_.x >= 0.0f) ? -1 : 1;
    shells_.clear();
    puffs_.clear();
    hits_ = 0;
    fireCd_ = 0;
    rivalFire_ = 1.45f;
    flashP_ = flashR_ = 0;
    shake_ = 0;
    t_ = 0;
    won_ = false;
    over_ = false;
    why_ = "still rolling";
    mode_ = Mode::Duel;
    if (engineOn_) {
        sys_->apu.tone(1, 110.0f, 0.07f);
        beep_ = 0.08f;
    }
}

void Game::launch(bool playerShot) {
    Body& b = playerShot ? player_ : rival_;
    Shell s;
    s.x = b.x;
    s.z = b.z + b.face * SPAWN;
    s.vz = b.face * SHELL_V;
    s.player = playerShot;
    s.live = true;
    shells_.push_back(s);
    if (playerShot) {
        fireCd_ = FIRE_CD;
        flashP_ = 0.07f;
        sys_->apu.noiseBurst(0.5f, 2100.0f, 0.16f);
        sys_->rumble(0.25f, 0.55f, 70);
    } else {
        flashR_ = 0.07f;
        sys_->apu.noiseBurst(0.35f, 1500.0f, 0.14f);
    }
}

void Game::damage(Body& b, bool playerVictim) {
    b.speed -= HIT_DROP;
    shake_ = 1.0f;
    if (b.speed <= STOP_SPEED) {
        b.speed = 0;
        b.stopped = true;
        if (playerVictim) why_ = "tracks";
    }
    if (!playerVictim) hits_++;
    for (int i = 0; i < 3; i++) {
        Puff p;
        p.x = b.x + (i - 1) * 0.35f;
        p.z = b.z;
        p.t = i * 0.04f;
        p.h = 1.3f + i * 0.35f;
        puffs_.push_back(p);
    }
    if (puffs_.size() > 18) puffs_.erase(puffs_.begin(), puffs_.begin() + int(puffs_.size()) - 18);
    boom();
    sys_->rumble(0.7f, 0.9f, 140);
}

void Game::think(float& steer, bool& fire) {
    steer = 0;
    fire = false;
    if (player_.stopped || rival_.stopped) return;
    const float ahead = (rival_.z - player_.z) * float(player_.face);
    const float shellVz = float(player_.face) * SHELL_V;
    const float relV = shellVz - float(rival_.face) * rival_.speed;
    const float shellZ = player_.z + float(player_.face) * SPAWN;
    const float gapZ = rival_.z - shellZ;
    const float flight = (std::fabs(relV) > 1.0f) ? gapZ / relV : 9.0f;
    const bool toward = flight > 0.08f && flight < 2.6f && ahead > 5.0f && ahead < 58.0f;
    const float pred = std::sin(weave0_ + WEAVE_W * (t_ + std::max(0.05f, flight))) * WEAVE_A;
    const float rx = std::sin(weave0_ + WEAVE_W * t_) * WEAVE_A;

    bool threat = false;
    bool lineHot = false;
    float threatX = 0;
    float threatD = 1.0e9f;
    int mine = 0;
    for (const Shell& s : shells_) {
        if (!s.live) continue;
        if (s.player) {
            mine++;
            continue;
        }
        const float left = (s.z - player_.z) * float(player_.face);
        if (left > 0.0f && left < 22.0f && std::fabs(s.x - pred) < 1.05f) lineHot = true;
        if (left > 0.0f && left < 30.0f && std::fabs(s.x - player_.x) < 1.75f && left < threatD) {
            threat = true;
            threatX = s.x;
            threatD = left;
        }
    }

    if (ahead > 12.0f && std::fabs(rx) > 0.42f) botSide_ = (rx >= 0.0f) ? -1 : 1;

    float target;
    if (threat) {
        float want = (player_.x >= threatX) ? 1.0f : -1.0f;
        float room = (want > 0.0f) ? (LANE - player_.x) : (player_.x + LANE);
        if (room < 1.4f) want = -want;
        target = std::clamp(threatX + want * 1.75f, -LANE, LANE);
    } else if (toward && !lineHot && fireCd_ <= 0.0f && mine < 2) {
        target = pred;
    } else {
        target = float(botSide_) * HOLD;
    }

    steer = std::clamp((target - player_.x) * 2.6f, -1.0f, 1.0f);
    if (toward && !threat && !lineHot && mine < 2 && fireCd_ <= 0.0f && std::fabs(player_.x - pred) < 0.32f) fire = true;
}

void Game::update(float dt) {
    float throttle = 0;
    float steer = 0;
    bool fire = false;
    if (bot_) {
        throttle = 1;
        think(steer, fire);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.25f) throttle = -1;
        if (pad.down(gs::BTN_UP) || pad.accel > 0.25f) throttle = 1;
        if (pad.down(gs::BTN_LEFT)) steer -= 1;
        if (pad.down(gs::BTN_RIGHT)) steer += 1;
        if (std::fabs(pad.axisX) > 0.18f) steer = pad.axisX;
        fire = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
    }

    const float pz0 = player_.z;
    const float rz0 = rival_.z;

    if (!player_.stopped) {
        if (throttle > 0) player_.speed = std::min(P_VMAX, player_.speed + P_ACC * dt);
        else if (throttle < 0) player_.speed = std::max(0.0f, player_.speed - P_BRAKE * dt);
        else player_.speed = std::max(0.0f, player_.speed - P_COAST * dt);
        player_.x = std::clamp(player_.x + steer * LAT * dt, -LANE, LANE);
        player_.z += float(player_.face) * player_.speed * dt;
        if (player_.z >= BLOCK_HI) {
            player_.z = BLOCK_HI;
            player_.face = -1;
        } else if (player_.z <= BLOCK_LO) {
            player_.z = BLOCK_LO;
            player_.face = 1;
        }
    }
    if (!rival_.stopped) {
        rival_.speed = std::min(R_VMAX, rival_.speed + R_ACC * dt);
        rival_.x = std::sin(weave0_ + WEAVE_W * t_) * WEAVE_A;
        rival_.z += float(rival_.face) * rival_.speed * dt;
        if (rival_.z >= BLOCK_HI) {
            rival_.z = BLOCK_HI;
            rival_.face = -1;
        } else if (rival_.z <= BLOCK_LO) {
            rival_.z = BLOCK_LO;
            rival_.face = 1;
        }
    }

    fireCd_ = std::max(0.0f, fireCd_ - dt);
    rivalFire_ -= dt;
    if (fire && !player_.stopped) {
        int mine = 0;
        for (const Shell& s : shells_)
            if (s.live && s.player) mine++;
        if (mine < 2 && fireCd_ <= 0.0f) launch(true);
    }
    if (rivalFire_ <= 0.0f && !rival_.stopped) {
        rivalFire_ = RIVAL_FIRE;
        launch(false);
    }

    for (Shell& s : shells_) {
        if (!s.live) continue;
        const float prev = s.z;
        s.z += s.vz * dt;
        Body& target = s.player ? rival_ : player_;
        const float z0 = s.player ? rz0 : pz0;
        if (!target.stopped) {
            const float a0 = std::min(prev, s.z) - 0.55f;
            const float a1 = std::max(prev, s.z) + 0.55f;
            const float b0 = std::min(z0, target.z) - 0.7f;
            const float b1 = std::max(z0, target.z) + 0.7f;
            if (a1 >= b0 && a0 <= b1 && std::fabs(s.x - target.x) <= HIT_R) {
                s.live = false;
                damage(target, !s.player);
            }
        }
        if (s.z < -30.0f || s.z > 180.0f) s.live = false;
    }
    shells_.erase(std::remove_if(shells_.begin(), shells_.end(), [](const Shell& s) { return !s.live; }), shells_.end());

    if (!player_.stopped) {
        if (player_.speed <= STOP_SPEED) player_.stall += dt;
        else player_.stall = 0;
        if (player_.stall >= STALL_TIME) {
            player_.speed = 0;
            player_.stopped = true;
            why_ = "stalled";
        }
    }

    if (!player_.stopped && !rival_.stopped && std::fabs(player_.z - rival_.z) < RAM_Z && std::fabs(player_.x - rival_.x) < RAM_X) {
        player_.stopped = true;
        rival_.stopped = true;
        player_.speed = 0;
        rival_.speed = 0;
        why_ = "rammed";
        boom();
        shake_ = 1;
    }

    for (Puff& p : puffs_) p.t += dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t > 0.75f; }), puffs_.end());

    if (rival_.stopped && !player_.stopped) {
        mode_ = Mode::Won;
        won_ = true;
        over_ = true;
        fanfare();
    } else if (player_.stopped) {
        mode_ = Mode::Lost;
        won_ = false;
        over_ = true;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 2.4f);
    const float jx = (shake_ > 0) ? std::sin(t_ * 71.0f) * 6.0f * shake_ : 0;
    const float jy = (shake_ > 0) ? std::cos(t_ * 53.0f) * 3.5f * shake_ : 0;
    flashP_ = std::max(0.0f, flashP_ - DT);
    flashR_ = std::max(0.0f, flashR_ - DT);

    float camX, camZ, face;
    float px, pz, rx, rz, pSpeed, rSpeed;
    int pFace, rFace;
    if (mode_ == Mode::Title) {
        float scroll = std::fmod(t_ * 6.5f, 64.0f);
        camZ = 14.0f + scroll;
        camX = std::sin(t_ * 0.32f) * 0.45f;
        face = 1;
        px = camX;
        pz = camZ + CAM_BACK;
        // Off to the side and far enough that the near tank does not cover it.
        rx = 2.55f;
        rz = pz + 27.0f;
        pSpeed = 4.5f;
        rSpeed = 3.0f;
        pFace = 1;
        rFace = -1;
    } else {
        camZ = player_.z - float(player_.face) * CAM_BACK;
        camX = player_.x;
        face = float(player_.face);
        px = player_.x;
        pz = player_.z;
        rx = rival_.x;
        rz = rival_.z;
        pSpeed = player_.speed;
        rSpeed = rival_.speed;
        pFace = player_.face;
        rFace = rival_.face;
    }

    const uint16_t skyTop = gs::rgb4(4, 6, 11);
    const uint16_t skyHor = gs::rgb4(14, 11, 8);
    v.setFogColor(gs::rgb4(12, 10, 8));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(HORIZON)) {
            v.lineBackdrop[y] = lerpC(skyTop, skyHor, y / HORIZON);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y) - HORIZON;
        if (row < 1) row = 1;
        float rel = CAM_H * FOCAL / row;
        float worldZ = camZ + face * rel;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = STREET_HALF * FOCAL / rel;
        r.cx = 160.0f - camX * (FOCAL / rel) + jx;
        r.v = worldZ * 34.0f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(worldZ * 0.22f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = r.right = 0;
        v.lineFog[y] = uint8_t(std::clamp(int((rel - 28.0f) / 9.0f), 0, 12));
        v.lineBackdrop[y] = skyHor;
    }

    if (mode_ == Mode::Title) {
        text("S3 TANK", 160 + jx, 20, 1.15f, PAL_RED);
        text("ONE STREET", 160 + jx, 46, 0.64f, PAL_AMBER);
    } else if (mode_ == Mode::Won) {
        text("THE BLOCK IS YOURS", 160, 36, 0.7f, PAL_AMBER);
    } else if (mode_ == Mode::Lost) {
        text("STOPPED", 160, 36, 1.15f, PAL_RED);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 36, 1.1f, PAL_HUD);
    }

    struct Cmd {
        float rz;
        gs::Sprite s;
    };
    std::vector<Cmd> cmds;
    auto push = [&](float worldX, float worldZ, float worldH, float lift, const gs::Mipped& m, int pal, bool feet,
                    bool shadow, bool flip) {
        float rz = (worldZ - camZ) * face;
        if (rz < 3.2f || rz > 168.0f || m.h < 1) return;
        float sc = FOCAL / rz;
        float h = std::max(2.0f, worldH * sc);
        float w = h * float(m.w) / float(m.h);
        gs::Sprite s;
        s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
        s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
        float sx = 160.0f + (worldX - camX) * sc + jx;
        float sy = HORIZON + (CAM_H - lift) * sc + jy;
        s.x = int16_t(std::lround(sx - s.w * 0.5f));
        s.y = int16_t(std::lround(feet ? sy - s.h : sy - s.h * 0.5f));
        if (s.x > gs::SCREEN_W + 200 || s.x + s.w < -200 || s.y > gs::SCREEN_H + 120 || s.y + s.h < -160) return;
        s.img = m.pick(float(s.h));
        s.pal = uint8_t(pal);
        s.hflip = flip;
        s.fog = uint8_t(std::clamp(int((rz - 34.0f) / 8.0f), 0, 13));
        s.shadow = shadow;
        cmds.push_back({rz, s});
    };

    for (const Prop& p : props_) {
        if (p.kind >= 3 && p.kind < 5) {
            if (p.kind == 3) push(p.x, p.z, 3.5f, 0, art_.lamp, PAL_LAMP, true, false, p.x < 0);
            else push(p.x, p.z, 0.9f, 0, art_.hydrant, PAL_BLDG, true, false, false);
        } else {
            push(p.x, p.z, 7.4f, 0, art_.bldg[p.kind % 3], PAL_BLDG, true, false, p.x < 0);
        }
    }

    auto tankAt = [&](float x, float z, float spd, int phase, bool front, int pal, bool flash) {
        push(x, z, 0.62f, 0, art_.shadow, PAL_SMOKE, true, true, false);
        if (front) push(x, z, TANK_H, 0, art_.front, pal, true, false, false);
        else push(x, z, TANK_H, 0, art_.rear[phase & 1], pal, true, false, false);
        if (flash) {
            float mz = z + (front ? -1.5f : 1.6f);
            push(x, mz, 0.7f, 1.55f, art_.flash, PAL_FX, false, false, false);
        }
        if (spd > 1.0f && (int(z * 3.0f) & 1)) push(x, z - 1.3f, 0.8f, 0.4f, art_.smoke, PAL_SMOKE, false, false, false);
    };

    int pPhase = int(std::floor(std::fabs(pz) * 2.0f)) & 1;
    int rPhase = int(std::floor(std::fabs(rz) * 2.0f)) & 1;
    bool rivalFront = rFace != pFace;
    // Camera looks along the player's face, so the muzzle stub on the rear sprite
    // always points up-screen. A rival facing the camera shows the front plate.
    if (face < 0) rivalFront = rFace != int(face);
    tankAt(px, pz, pSpeed, pPhase, false, PAL_PLAYER, flashP_ > 0);
    tankAt(rx, rz, rSpeed, rPhase, rivalFront, PAL_RIVAL, flashR_ > 0);

    for (const Shell& s : shells_) {
        if (!s.live) continue;
        push(s.x, s.z, 0.55f, 1.15f, art_.shell, s.player ? PAL_FX : PAL_RED, false, false, false);
    }
    for (const Puff& p : puffs_) {
        float k = std::clamp(p.t / 0.7f, 0.0f, 1.0f);
        push(p.x, p.z, p.h * (0.7f + k), 0.6f + k * 1.4f, art_.smoke, PAL_SMOKE, false, false, false);
    }

    std::sort(cmds.begin(), cmds.end(), [](const Cmd& a, const Cmd& b) { return a.rz < b.rz; });
    for (const Cmd& c : cmds) v.sprite(c.s);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(8, "TWO TANKS. ONE BLOCK.", PAL_HUD);
        hudC(10, "STILL MOVING TAKES IT.", PAL_AMBER);
        if ((int(t_ * 2.0f) & 1) == 0) hudC(23, "PRESS START", PAL_HUD);
        hudC(26, "ARROWS DRIVE   C FIRE   HOLD UP", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 1, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Duel || mode_ == Mode::Pause || mode_ == Mode::Won || mode_ == Mode::Lost) {
        int youPal = player_.speed < P_VMAX * 0.42f ? PAL_RED : PAL_GREEN;
        std::snprintf(buf, sizeof buf, "YOU %02.0f", mode_ == Mode::Title ? 0 : player_.speed);
        hud(1, 1, buf, youPal);
        int pn = std::clamp(int(std::lround(player_.speed / P_VMAX * 8.0f)), 0, 8);
        for (int i = 0; i < 8; i++) hud(8 + i, 1, i < pn ? "=" : "-", i < pn ? youPal : PAL_HUD);
        std::snprintf(buf, sizeof buf, "THEM %02.0f", rival_.speed);
        hud(22, 1, buf, PAL_RED);
        int rn = std::clamp(int(std::lround(rival_.speed / R_VMAX * 8.0f)), 0, 8);
        for (int i = 0; i < 8; i++) hud(30 + i, 1, i < rn ? "=" : "-", i < rn ? PAL_RED : PAL_HUD);
        if (mode_ == Mode::Duel && player_.speed < P_VMAX * 0.42f && (int(t_ * 6.0f) & 1)) hudC(25, "KEEP MOVING", PAL_RED);
        if (mode_ == Mode::Pause) {
            hudC(16, "START  RESUME", PAL_HUD);
            hudC(18, "ESC    TITLE", PAL_HUD);
        } else if (mode_ == Mode::Won) {
            hudC(8, "STILL MOVING", PAL_GREEN);
            std::snprintf(buf, sizeof buf, "HITS %d", hits_);
            hudC(10, buf, PAL_HUD);
            hudC(22, "START", PAL_HUD);
        } else if (mode_ == Mode::Lost) {
            hudC(8, "THEY TAKE THE BLOCK", PAL_AMBER);
            hudC(22, "START", PAL_HUD);
        }
    }

    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
        }
    }
    if (engineOn_ && mode_ != Mode::Pause) {
        float spd = (mode_ == Mode::Title) ? 3.5f : player_.speed;
        float burble = 1.0f + 0.04f * std::sin(t_ * 48.0f) * std::sin(t_ * 17.0f);
        sys_->apu.setFreq(0, (46.0f + spd * 3.4f) * burble);
        sys_->apu.setVol(0, mode_ == Mode::Duel ? 0.11f + spd * 0.008f : 0.06f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.0f, 523.0f, 659.0f, 784.0f};
        fanT_ += DT;
        if (fanT_ > 0.12f) {
            if (fanStep_ < 4) sys_->apu.keyOn(1, notes[fanStep_], 0.2f);
            else sys_->apu.keyOff(1);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > 6) fanStep_ = -1;
        }
    }
    if (mode_ == Mode::Won) sys_->setLight(40, 48, 12);
    else if (mode_ == Mode::Lost) sys_->setLight(48, 8, 6);
    else sys_->setLight(18, 22, 10);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    props_.clear();
    for (int i = 0; i < 16; i++) {
        float z = 4.0f + i * 9.5f;
        props_.push_back({-7.7f, z, i % 3});
        props_.push_back({7.7f, z, (i + 2) % 3});
        if ((i % 2) == 0) {
            props_.push_back({-5.35f, z + 4.0f, 3});
            props_.push_back({5.35f, z + 4.0f, 3});
            props_.push_back({-4.85f, z + 6.5f, 4});
        }
    }
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.28f, 0.16f);
    sys.apu.setPatch(0, dieselPatch());
    sys.apu.setPatch(1, hornPatch());
    sys.apu.keyOn(0, 48.0f, 0.07f);
    engineOn_ = true;
    if (bot_) beginDuel();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (!bot_ && mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            beginDuel();
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Duel) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Duel;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
        }
    } else if (!bot_ && (mode_ == Mode::Won || mode_ == Mode::Lost)) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }
    draw();
}

}  // namespace tank
