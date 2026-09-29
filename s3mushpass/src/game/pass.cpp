#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mushpass {

namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float BODY = 0.16f;
constexpr int HORIZON = 72;
constexpr int NLEGS = 3;
constexpr float STORM = 28.0f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::curve(float z) const {
    return 0.62f * std::sin(z * 0.085f) + 0.28f * std::sin(z * 0.21f + 0.7f);
}

float Game::halfW(float z) const {
    float pinch = 0.5f + 0.5f * std::sin(z * 0.13f);
    return 0.70f - 0.16f * pinch * pinch;
}

void Game::begin() {
    over_ = false;
    won_ = false;
    lives_ = 3;
    legsDone_ = 0;
    leg_ = 0;
    note_ = "";
    flash_ = 0;
    clock_ = STORM;
    scrape_ = 0;
    const float span = 58.0f;
    for (int i = 0; i < NLEGS; i++) {
        legs_[i].startZ = i * span;
        legs_[i].endZ = i * span + 46.0f;
        legs_[i].doneZ = i * span + span;
    }
    startLeg();
    mode_ = Mode::Run;
}

void Game::startLeg() {
    const Leg& L = legs_[leg_];
    pz_ = L.startZ + 1.2f;
    x_ = curve(pz_);
    vx_ = 0;
    stride_ = 0;
    endTaken_ = false;
    hold_ = 0;
}

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.12f); }

void Game::miss(const char* why) {
    note_ = why;
    flash_ = 0.35f;
    lives_--;
    blip(86);
    if (lives_ <= 0 || clock_ <= 0) {
        if (clock_ <= 0) note_ = "STORM CLOSED THE PASS";
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        hold_ = 0;
    } else {
        mode_ = Mode::Miss;
        hold_ = 0.9f;
    }
}

void Game::stormOut() {
    clock_ = 0;
    note_ = "STORM CLOSED THE PASS";
    flash_ = 0.5f;
    blip(70);
    mode_ = Mode::Lose;
    over_ = true;
    won_ = false;
}

void Game::botStick(float& steer, float& power) const {
    const Leg& L = legs_[leg_];
    float look = pz_ + 7.5f;
    float aim = curve(look);
    if (L.endZ - pz_ < 12.0f) aim = curve(L.endZ);
    float err = aim - x_;
    steer = clampf(err * 3.6f - vx_ * 0.55f, -1.0f, 1.0f);
    power = 1.0f;
}

void Game::mush(float dt) {
    const Leg& L = legs_[leg_];
    float steer = 0, power = 0.22f;
    if (bot_) {
        botStick(steer, power);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) steer -= 1;
        if (pad.down(gs::BTN_RIGHT)) steer += 1;
        if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
        steer = clampf(steer, -1.0f, 1.0f);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.2f) power = 1.0f;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) power = 0.0f;
    }
    float des = steer * 1.55f;
    vx_ += (des - vx_) * std::min(1.0f, dt * 6.0f);
    x_ += vx_ * dt;

    float spd = 6.4f + power * 4.6f;
    float z0 = pz_;
    pz_ += spd * dt;
    stride_ += dt * (2.0f + power * 2.6f);
    clock_ -= dt;
    if (scrape_ > 0) scrape_ -= dt;

    float lim = halfW(pz_) - BODY;
    float off = x_ - curve(pz_);
    if (std::fabs(off) > lim) {
        x_ = curve(pz_) + std::copysign(lim, off);
        vx_ *= -0.25f;
        if (scrape_ <= 0) {
            scrape_ = 0.5f;
            clock_ -= 1.6f;
            flash_ = 0.18f;
            blip(110);
        }
    }
    if (clock_ <= 0) {
        stormOut();
        return;
    }

    if (!endTaken_ && z0 < L.endZ && pz_ >= L.endZ) {
        endTaken_ = true;
        float mouth = halfW(L.endZ) - 0.06f;
        if (std::fabs(x_ - curve(L.endZ)) > mouth) {
            miss("MISSED THE END");
            return;
        }
        blip(460);
    }
    if (pz_ >= L.doneZ) {
        if (!endTaken_) {
            miss("MISSED THE END");
            return;
        }
        legsDone_ = leg_ + 1;
        if (leg_ + 1 >= NLEGS) {
            mode_ = Mode::Win;
            over_ = true;
            won_ = true;
            note_ = "PASS CLEAR";
            blip(720);
        } else {
            leg_++;
            startLeg();
            note_ = "LEG MADE";
            hold_ = 0.45f;
        }
    }
}

void Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    float d = wz - pz_;
    if (d < 0.85f) d = 0.85f;
    float n = 1.0f - std::sqrt(clampf((d - 1.2f) / 42.0f, 0.0f, 1.0f));
    sy = float(HORIZON) + n * float(gs::SCREEN_H - 8 - HORIZON);
    ppm = 168.0f / d;
    sx = 160.0f + (wx - x_) * ppm;
}

void Game::spr(const gs::Mipped& m, float cx, float footY, float destH, int pal, bool flip) {
    if (destH < 2.0f) return;
    gs::Sprite s;
    s.img = m.pick(destH);
    s.h = std::max(1, int(destH));
    s.w = std::max(1, int(m.w * destH / float(m.h)));
    s.x = int(cx - s.w * 0.5f);
    s.y = int(footY - s.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    int fog = 0;
    if (clock_ < 10.0f) fog = int((10.0f - clock_) * 0.6f);
    s.fog = uint8_t(clampf(float(fog), 0.0f, 8.0f));
    sys_->vdp.sprite(s);
}

void Game::glyphText(const char* s, int x, int y, int scale, int pal) {
    int cw = 6 * scale;
    for (const char* p = s; *p; p++) {
        unsigned c = unsigned(*p);
        if (c < 32 || c > 127) c = '?';
        gs::Sprite sp;
        sp.img = art_.glyph[c - 32];
        sp.x = int16_t(x);
        sp.y = int16_t(y);
        sp.w = int16_t(5 * scale);
        sp.h = int16_t(7 * scale);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += cw;
    }
}

void Game::paintSky() {
    gs::VDP& v = sys_->vdp;
    float gloom = clampf(1.0f - clock_ / STORM, 0.0f, 1.0f);
    if (mode_ == Mode::Title) gloom = 0.15f + 0.05f * std::sin(t_ * 0.8f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float u = y / float(HORIZON);
            int r = int((7 + 4 * u) * (1.0f - 0.55f * gloom));
            int g = int((8 + 3 * u) * (1.0f - 0.45f * gloom));
            int b = int((13 + 2 * u) * (1.0f - 0.25f * gloom));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(clampf(gloom * 8.0f * (1.0f - u), 0.0f, 10.0f));
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(int(5 - 2 * gloom), int(6 - 2 * gloom), int(8 - gloom));
            float n = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            v.lineFog[y] = uint8_t(clampf((1.0f - n) * (5.0f + 6.0f * gloom), 0.0f, 12.0f));
        }
    }
}

void Game::paintSnow() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 24);
    for (int y = HORIZON; y < gs::SCREEN_H; y++) {
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float d = 1.2f + 42.0f * (1.0f - n) * (1.0f - n);
        float ppm = 168.0f / d;
        float z = pz_ + d;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.0f + (curve(z) - x_) * ppm;
        r.hw = halfW(z) * ppm;
        r.v = pz_ * 14.0f + d * 18.0f;
        r.pal = PAL_SNOW;
        r.band = (int(z) & 1);
        r.style = (int(z / 18.0f) & 1) ? gs::ROAD_SNOW : gs::ROAD_ROCKY;
        r.left = gs::GROUND_SNOWWALL;
        r.right = gs::GROUND_SNOWWALL;
    }
}

void Game::paintWorld() {
    struct Item {
        float z;
        int kind;
        float wx;
    };
    Item items[48];
    int n = 0;
    auto push = [&](float z, int kind, float wx) {
        if (n < 48 && z > pz_ + 0.8f && z < pz_ + 44.0f) items[n++] = {z, kind, wx};
    };
    float base = std::floor(pz_ / 11.0f) * 11.0f;
    for (int i = 0; i < 6; i++) {
        float z = base + i * 11.0f;
        float c = curve(z);
        float w = halfW(z);
        push(z, 0, c - w - 0.85f);
        push(z + 5.0f, 0, c + w + 0.9f);
        if ((i & 1) == 0) push(z + 2.0f, 1, c - w - 1.5f);
    }
    for (int i = 0; i < NLEGS; i++) {
        const Leg& L = legs_[i];
        float c = curve(L.endZ);
        float m = halfW(L.endZ);
        push(L.endZ, 2, c - m);
        push(L.endZ, 2, c + m);
        push(L.endZ - 0.4f, 3, c);
    }
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z > b.z; });
    for (int i = 0; i < n; i++) {
        float sx, sy, ppm;
        project(items[i].wx, items[i].z, sx, sy, ppm);
        if (sx < -60 || sx > 380) continue;
        if (items[i].kind == 0) spr(art_.peak, sx, sy, 5.2f * ppm, PAL_ROCK, items[i].wx > curve(items[i].z));
        else if (items[i].kind == 1) spr(art_.cairn, sx, sy, 1.6f * ppm, PAL_ROCK, false);
        else if (items[i].kind == 2) spr(art_.banner, sx, sy, 2.4f * ppm, PAL_FLAG, items[i].wx > curve(items[i].z));
        else spr(art_.cairn, sx, sy, 2.0f * ppm, PAL_FLAG, false);
    }
    if (clock_ < STORM * 0.72f) {
        int flakes = 10 + int((1.0f - clock_ / STORM) * 16.0f);
        for (int i = 0; i < flakes; i++) {
            float fx = std::fmod(i * 47.0f + t_ * (18.0f + i) + pz_ * 6.0f, 320.0f);
            float fy = std::fmod(i * 29.0f + t_ * (30.0f + (i % 5) * 8.0f), 200.0f);
            gs::Sprite s;
            s.img = art_.flake.pick(6);
            s.w = 5;
            s.h = 5;
            s.x = int(fx);
            s.y = int(fy);
            s.pal = PAL_STORM;
            s.fog = uint8_t(i & 3);
            sys_->vdp.sprite(s);
        }
    }
}

void Game::paintTeam() {
    float bob = std::sin(stride_ * 6.2832f) * 2.0f;
    int fr = int(std::fmod(stride_, 1.0f) * 3.0f);
    if (fr < 0) fr = 0;
    if (fr > 2) fr = 2;
    float foot = 198.0f + bob;
    spr(art_.dog[fr], 146, foot - 30, 18, PAL_TEAM, false);
    spr(art_.dog[(fr + 1) % 3], 174, foot - 36, 16, PAL_TEAM, false);
    spr(art_.sled, 160, foot, 48, PAL_TEAM, vx_ < -0.2f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.75f);
    sys.apu.silence();
    clock_ = STORM;
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        pz_ = 8.0f + std::sin(t_ * 0.3f) * 0.2f;
        x_ = curve(pz_);
        stride_ += DT * 1.5f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            begin();
            blip(540);
        }
    } else if (mode_ == Mode::Run) {
        if (hold_ > 0) hold_ -= DT;
        else mush(DT);
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            blip(180);
        }
    } else if (mode_ == Mode::Miss) {
        hold_ -= DT;
        stride_ += DT * 0.25f;
        if (hold_ <= 0) {
            startLeg();
            mode_ = Mode::Run;
        }
    } else {
        stride_ += DT * 0.35f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }

    if (flash_ > 0) flash_ -= DT;
    paintSky();
    if (flash_ > 0.12f) {
        for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(8, 8, 11);
    }
    paintSnow();
    sys.vdp.clearSprites();

    char buf[56];
    if (mode_ == Mode::Title) {
        glyphText("MUSH PASS", 92, 22, 2, PAL_GOLD);
        glyphText("CLEAR THE PASS", 100, 48, 1, PAL_INK);
        glyphText("BEFORE THE STORM CLOCK", 70, 62, 1, PAL_INK);
        glyphText("MISS THE END AND THE LEG FAILS", 52, 76, 1, PAL_INK);
        glyphText("LEFT RIGHT STEER", 94, 150, 1, PAL_INK);
        glyphText("A MUSH    START", 100, 166, 1, PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof(buf), "LEG %d/%d", std::min(leg_ + 1, NLEGS), NLEGS);
        glyphText(buf, 8, 6, 1, PAL_INK);
        int sec = std::max(0, int(std::ceil(clock_)));
        std::snprintf(buf, sizeof(buf), "STORM %02d", sec);
        glyphText(buf, 188, 6, 1, clock_ < 8.0f ? PAL_FLAG : PAL_GOLD);
        if (mode_ == Mode::Win) {
            glyphText("PASS CLEAR", 86, 34, 2, PAL_GOLD);
            glyphText("AHEAD OF THE STORM", 82, 58, 1, PAL_INK);
        } else if (mode_ == Mode::Lose) {
            glyphText("LEG FAILED", 88, 34, 2, PAL_GOLD);
            glyphText(note_, 70, 58, 1, PAL_INK);
        } else if (note_[0] && (mode_ == Mode::Miss || hold_ > 0)) {
            glyphText(note_, 112, 28, 1, PAL_GOLD);
        } else if (mode_ == Mode::Run && !endTaken_) {
            glyphText("MAKE THE END", 104, 22, 1, PAL_GOLD);
        }
    }
    paintWorld();
    paintTeam();
}

}  // namespace mushpass
