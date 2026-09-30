#include "game/kart.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace kartboom {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float HORIZON = 86.0f;
constexpr float CAM_H = 1.2f;
constexpr float FOCAL = 260.0f;
constexpr float ROAD_HW = 3.35f;
constexpr float BOOM_Z = 248.0f;
constexpr float BOOM_SPAN = 10.0f;
constexpr float MAX_SPD = 24.0f;
constexpr int NCONE = 8;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::center(float z) const {
    return 2.15f * std::sin(z * 0.031f) + 1.05f * std::sin(z * 0.078f + 0.7f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 12));
    sys.apu.setEcho(0.18f, 0.25f, 0.12f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::startRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    drive_ = 100;
    score_ = 0;
    z_ = 0;
    x_ = center(0);
    vx_ = 0;
    speed_ = 0;
    hold_ = 0;
    shake_ = 0;
    flash_ = 0;
    lift_ = 0;
    hits_ = 0;
    why_[0] = 0;
    t_ = 0;
    static const float oz[NCONE] = {28, 52, 78, 104, 132, 158, 186, 210};
    static const float ox[NCONE] = {1.85f, -1.95f, 2.15f, -1.7f, 2.05f, -2.2f, 1.75f, -1.9f};
    ncones_ = NCONE;
    for (int i = 0; i < NCONE; i++) {
        cones_[i].z = oz[i];
        cones_[i].x = center(oz[i]) + ox[i];
        cones_[i].hit = false;
    }
}

void Game::bot(float& steer, float& gas, float& brake) {
    float look = 7.0f + speed_ * 0.42f;
    float aim = center(z_ + look);
    float err = aim - x_;
    steer = clampf(err * 0.85f - vx_ * 1.15f, -1.0f, 1.0f);
    float dz = (BOOM_Z + 3.4f) - z_;
    float want = 18.0f;
    if (dz < 40.0f) want = std::max(1.6f, dz * 0.48f);
    if (dz < 2.2f) want = 0.0f;
    gas = 0;
    brake = 0;
    if (speed_ < want - 0.4f) gas = speed_ < want - 6.0f ? 1.0f : 0.65f;
    else if (speed_ > want + 0.35f) brake = speed_ > want + 3.0f ? 1.0f : 0.5f;
    float lane = x_ - center(z_);
    if (std::fabs(lane) > ROAD_HW - 0.7f) {
        steer = clampf((center(z_) - x_) * 1.4f - vx_, -1.0f, 1.0f);
        if (speed_ > 12.0f) {
            gas = 0;
            brake = 0.35f;
        }
    }
}

void Game::physics(float steer, float gas, float brake) {
    float curve = center(z_ + 6.0f) - center(z_);
    float push = curve * speed_ * 0.085f;
    vx_ += (steer * 28.0f - push - vx_ * 3.2f) * DT;
    vx_ = clampf(vx_, -14.0f, 14.0f);
    x_ += vx_ * DT;

    float a = gas * 16.0f - brake * 26.0f - speed_ * 0.42f;
    float lane = x_ - center(z_);
    bool off = std::fabs(lane) > ROAD_HW;
    if (off) a -= 9.0f;
    speed_ += a * DT;
    speed_ = clampf(speed_, 0.0f, off ? 11.0f : MAX_SPD);
    z_ += speed_ * DT;

    if (off && speed_ > 2.0f) {
        drive_ -= 1;
        flash_ = 0.05f;
    }

    for (int i = 0; i < ncones_; i++) {
        Cone& c = cones_[i];
        if (c.hit) continue;
        if (std::fabs(c.z - z_) < 1.15f && std::fabs(c.x - x_) < 0.72f) {
            c.hit = true;
            hits_++;
            drive_ -= 34;
            vx_ += (x_ > c.x ? 4.5f : -4.5f);
            speed_ *= 0.72f;
            shake_ = 0.35f;
            flash_ = 0.2f;
            sys_->apu.noiseBurst(0.4f, 700.0f, 0.16f);
            sys_->rumble(0.45f, 0.2f, 90);
        }
    }
    drive_ = std::max(0, drive_);
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    if (drive_ <= 0) {
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        std::snprintf(why_, sizeof why_, "THE DRIVE IS DEAD");
        sys_->apu.noiseBurst(0.5f, 180.0f, 0.35f);
        return;
    }
    float lane = std::fabs(x_ - center(BOOM_Z));
    bool in = z_ >= BOOM_Z && z_ <= BOOM_Z + BOOM_SPAN && std::fabs(x_ - center(z_)) < 1.25f;
    if (in && speed_ < 2.2f) {
        hold_ += DT;
        if (hold_ >= 0.45f) {
            mode_ = Mode::Win;
            over_ = true;
            won_ = true;
            score_ = drive_ * 10 + std::max(0, 8 - hits_) * 50;
            lift_ = 0;
            sys_->setLight(40, 180, 70);
            sys_->rumble(0.2f, 0.08f, 140);
            return;
        }
    } else {
        hold_ = 0;
    }
    if (z_ > BOOM_Z + BOOM_SPAN + 6.0f) {
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        std::snprintf(why_, sizeof why_, lane > 2.4f ? "MISSED THE BOOM" : "OVERSHOT THE BOOM");
        sys_->apu.tone(0, 90.0f, 0.08f);
    }
    if (t_ > 90.0f && mode_ == Mode::Run) {
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        std::snprintf(why_, sizeof why_, "THE YARD CLOSED");
    }
}

void Game::spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog) {
    if (h < 2.0f || m.h < 1) return;
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::max(1, int(std::lround(h * float(m.w) / float(m.h)))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(foot - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::glyphText(const char* s, int x, int y, int scale, int pal) {
    int cw = 6 * scale;
    for (const char* p = s; *p; ++p) {
        unsigned c = static_cast<unsigned char>(*p);
        if (c < 32 || c > 127) c = '?';
        gs::Sprite sp;
        sp.img = art_.glyph[c - 32];
        sp.x = int16_t(x);
        sp.y = int16_t(y);
        sp.w = int16_t(std::max(1, 5 * scale));
        sp.h = int16_t(std::max(1, 7 * scale));
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += cw;
    }
}

void Game::paintRoad() {
    gs::VDP& v = sys_->vdp;
    float sh = shake_ > 0 ? std::sin(t_ * 90.0f) * shake_ * 4.0f : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(HORIZON)) {
            float u = y / HORIZON;
            int r = 2 + int(4 * u);
            int g = 4 + int(5 * u);
            int b = 10 + int(3 * (1.0f - u));
            if (mode_ == Mode::Win) {
                g += 2;
                b += 1;
            }
            if (mode_ == Mode::Fail) r += 3;
            v.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
            v.lineFog[y] = 0;
            v.road[y].on = false;
        } else {
            float dy = float(y) - HORIZON + 0.5f;
            float d = CAM_H * FOCAL / dy;
            float wz = z_ + d;
            float ppm = FOCAL / d;
            float bend = (center(wz) - x_) * ppm;
            gs::RoadLine& rd = v.road[y];
            rd.on = true;
            rd.cx = 160.0f + bend + sh;
            rd.hw = ROAD_HW * ppm;
            rd.v = wz * 40.0f;
            rd.pal = PAL_ROAD;
            rd.band = (int(wz) & 1) ? 1 : 0;
            rd.style = 1;
            rd.left = gs::GROUND_LAND;
            rd.right = gs::GROUND_LAND;
            float fog = clampf((1.0f - (float(y) - HORIZON) / (gs::SCREEN_H - HORIZON)) * 8.0f, 0.0f, 9.0f);
            v.lineFog[y] = uint8_t(fog);
            v.lineBackdrop[y] = gs::rgb4(1, 5, 2);
        }
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    paintRoad();

    auto project = [&](float wz, float wx, float& sx, float& sy, float& scale) {
        float d = wz - z_;
        if (d < 0.35f) d = 0.35f;
        sy = HORIZON + CAM_H * FOCAL / d;
        scale = FOCAL / d;
        sx = 160.0f + (wx - x_) * scale;
    };

    struct Item {
        float sy, sx, h;
        int kind;
        int fog;
    };
    Item items[16];
    int n = 0;
    auto push = [&](float wz, float wx, float worldH, int kind) {
        float sx, sy, sc;
        project(wz, wx, sx, sy, sc);
        if (sy < HORIZON - 4 || sy > gs::SCREEN_H + 20) return;
        if (n >= 16) return;
        float fog = clampf((wz - z_) / 18.0f, 0.0f, 10.0f);
        items[n++] = {sy, sx, worldH * sc, kind, int(fog)};
    };
    for (int i = 0; i < ncones_; i++) {
        if (!cones_[i].hit) push(cones_[i].z, cones_[i].x, 0.85f, 1);
    }
    push(BOOM_Z + 3.0f, center(BOOM_Z + 3.0f) + 1.6f, 6.2f, 2);

    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.sy < b.sy; });
    for (int i = 0; i < n; i++) {
        if (items[i].kind == 1) spr(art_.cone, items[i].sx, items[i].sy, items[i].h, PAL_CONE, false, items[i].fog);
        else spr(art_.boom, items[i].sx, items[i].sy + 8, items[i].h, PAL_BOOM, false, items[i].fog);
    }

    if (mode_ != Mode::Title) {
        bool bare = mode_ == Mode::Win;
        spr(bare ? art_.kartEmpty : art_.kart, 160.0f + vx_ * 0.4f, 214.0f, 78.0f, PAL_KART, vx_ < -0.4f);
        if (mode_ == Mode::Win) {
            lift_ += DT;
            float y = 150.0f - lift_ * 36.0f;
            spr(art_.drive, 188.0f, y, 28.0f, PAL_DRIVE);
        }
    } else {
        spr(art_.kart, 168.0f, 196.0f, 70.0f, PAL_KART);
        spr(art_.boom, 250.0f, 150.0f, 78.0f, PAL_BOOM);
        spr(art_.drive, 118.0f, 150.0f, 26.0f, PAL_DRIVE);
    }

    if (mode_ == Mode::Title) {
        glyphText("S3 KARTBOOM", 62, 16, 2, PAL_HUD);
        glyphText("DELIVER THE DRIVE", 70, 42, 1, PAL_HUD);
        glyphText("TO THE BOOM", 100, 54, 1, PAL_HUD);
        glyphText("ARROWS STEER", 40, 188, 1, PAL_HUD);
        glyphText("A GAS   B BRAKE", 168, 188, 1, PAL_HUD);
        glyphText("START", 136, 204, 1, PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "DRIVE %d", drive_);
        glyphText(buf, 8, 6, 2, drive_ < 40 ? PAL_HUD : PAL_HUD);
        int pct = int(clampf(z_ / BOOM_Z, 0.0f, 1.0f) * 100.0f);
        std::snprintf(buf, sizeof buf, "YARD %d", pct);
        glyphText(buf, 220, 8, 1, PAL_HUD);
        if (mode_ == Mode::Run && z_ > BOOM_Z - 30.0f) glyphText("BOOM AHEAD", 104, 28, 1, PAL_HUD);
        if (mode_ == Mode::Win) {
            glyphText("DRIVE DELIVERED", 58, 48, 2, PAL_HUD);
            glyphText("THE BOOM HAS IT", 82, 72, 1, PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            glyphText("DELIVERY FAILED", 58, 48, 2, PAL_HUD);
            glyphText(why_, 70, 74, 1, PAL_HUD);
            glyphText("A RETRIES", 112, 96, 1, PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    if (flash_ > 0) flash_ = std::max(0.0f, flash_ - DT);

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRun();
    } else if (mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START))) startRun();
    } else if (mode_ == Mode::Run) {
        float steer = 0, gas = 0, brake = 0;
        if (bot_) {
            bot(steer, gas, brake);
        } else {
            if (pad.down(gs::BTN_LEFT)) steer -= 1;
            if (pad.down(gs::BTN_RIGHT)) steer += 1;
            if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
            if (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.15f) gas = 1;
            if (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.15f) brake = 1;
            if (gas > 0 && brake > 0) gas = 0;
        }
        physics(steer, gas, brake);
        judge();
        float roll = 0.01f + speed_ * 0.004f;
        sys.apu.noise(mode_ == Mode::Run ? roll : 0.0f, 380.0f + speed_ * 28.0f, false);
        if (gas > 0.2f) sys.apu.tone(1, 70.0f + speed_ * 6.0f, 0.03f);
        else sys.apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Win) {
        static const float notes[] = {523.0f, 659.0f, 784.0f, 1046.0f};
        int step = int(lift_ / 0.16f);
        if (step >= 0 && step < 4 && lift_ - step * 0.16f < DT * 1.5f) sys.apu.tone(0, notes[step], 0.06f);
    }
    draw();
}

}  // namespace kartboom
