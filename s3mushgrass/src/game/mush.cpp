#include "mush.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mushgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = kPi * 0.5f;

constexpr float kStartX = 0.f;
constexpr float kStartY = 16.f;
constexpr float kMax = 13.5f;
constexpr float kStop = 0.42f;
constexpr float kHoldNeed = 0.55f;
constexpr float kCrewTime = 26.f;
constexpr float kPlayZoom = 1.55f;
constexpr float kTitleZoom = 0.72f;

constexpr float kGrassX0 = -26.f;
constexpr float kGrassX1 = 26.f;
constexpr float kGrassY0 = 168.f;
constexpr float kGrassY1 = 228.f;
constexpr float kDeepX = 16.f;
constexpr float kDeepY0 = 176.f;
constexpr float kDeepY1 = 214.f;
constexpr float kAimY = 192.f;

const float kTuft[10][2] = {
    {-18.f, 174.f}, {-6.f, 180.f}, {8.f, 176.f},  {18.f, 186.f}, {-14.f, 198.f},
    {4.f, 204.f},   {16.f, 208.f}, {-20.f, 214.f}, {-2.f, 220.f}, {12.f, 194.f},
};
const float kTree[8][2] = {
    {-42.f, 40.f}, {44.f, 52.f}, {-46.f, 90.f}, {48.f, 110.f}, {-40.f, 150.f}, {46.f, 168.f}, {-36.f, 210.f},
    {38.f, 222.f},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

float Game::speed() const { return std::hypot(vx_, vy_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (y_ >= kGrassY0 && x_ > kGrassX0 && x_ < kGrassX1) return 2;
    return 1;
}

bool Game::deepGrass() const {
    return std::fabs(x_) <= kDeepX && y_ >= kDeepY0 && y_ <= kDeepY1;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kNorth;
    vx_ = vy_ = 0.f;
    throttle_ = 0.f;
    race_ = 0.f;
    clock_ = kCrewTime;
    hold_ = 0.f;
    touched_ = false;
    won_ = over_ = false;
    puffCursor_ = 0;
    chimeN_ = chimeStep_ = 0;
    why_[0] = 0;
    for (Puff& p : spray_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = 0.f;
    camY_ = 120.f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    t_ = 0.f;
    if (bot_) {
        begin();
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::controls(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    throttle = 0.f;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C)) throttle = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X)) throttle = -1.f;
    if (p.accel > 0.12f) throttle = p.accel;
    if (p.brake > 0.12f) throttle = -p.brake;
    if (std::fabs(p.axisY) > 0.2f) throttle = std::clamp(p.axisY, -1.f, 1.f);
}

void Game::pilot(float& steer, float& throttle) {
    const float tx = 0.f;
    const float ty = kAimY;
    float err = wrap(std::atan2(ty - y_, tx - x_) - heading_);
    steer = std::clamp(err / 0.32f, -1.f, 1.f);
    float sp = speed();
    if (y_ < 172.f) {
        throttle = std::fabs(err) > 0.7f ? 0.25f : 1.f;
        return;
    }
    if (sp > 1.1f) throttle = -1.f;
    else if (!deepGrass() && y_ < kDeepY0 + 2.f) throttle = 0.45f;
    else if (sp > kStop) throttle = -0.7f;
    else throttle = 0.f;
}

void Game::physics(float dt, float steer, float throttle) {
    throttle_ = throttle;
    float c = std::cos(heading_), s = std::sin(heading_);
    float along = vx_ * c + vy_ * s;
    float lat = -vx_ * s + vy_ * c;
    if (throttle > 0.f) along += throttle * 16.f * dt;
    else if (throttle < 0.f) along += throttle * 28.f * dt;
    float drag = (y_ >= kGrassY0) ? 1.15f : 0.38f;
    along -= along * drag * dt;
    lat -= lat * 5.2f * dt;
    along = std::clamp(along, -3.5f, kMax);
    vx_ = c * along - s * lat;
    vy_ = s * along + c * lat;
    heading_ = wrap(heading_ + steer * (1.5f + std::fabs(along) * 0.04f) * dt);
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    if (x_ < -52.f) {
        x_ = -52.f;
        if (vx_ < 0.f) vx_ = 0.f;
    }
    if (x_ > 52.f) {
        x_ = 52.f;
        if (vx_ > 0.f) vx_ = 0.f;
    }
    if (y_ < 4.f) {
        y_ = 4.f;
        if (vy_ < 0.f) vy_ = 0.f;
    }
    if (y_ > 240.f) {
        y_ = 240.f;
        if (vy_ > 0.f) vy_ = 0.f;
    }

    if (speed() > 5.f && throttle > 0.3f) {
        Puff& p = spray_[puffCursor_];
        p.x = x_ - c * 5.f;
        p.y = y_ - s * 5.f;
        p.life = 0.4f;
        puffCursor_ = (puffCursor_ + 1) % 12;
    }
    for (Puff& p : spray_) p.life -= dt;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vx_ = vy_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    chime(5);
    sys_->rumble(0.25f, 0.1f, 140);
    sys_->setLight(40, 170, 60);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    chime(2);
    sys_->rumble(0.45f, 0.2f, 160);
    sys_->setLight(170, 30, 20);
}

void Game::judge() {
    if (y_ >= kGrassY0 && x_ > kGrassX0 && x_ < kGrassX1) touched_ = true;
    if (y_ > kGrassY1 - 2.f && speed() > 1.2f) {
        fail("missed the grass");
        return;
    }
    if (deepGrass() && speed() <= kStop) {
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else {
        hold_ = 0.f;
    }
    if (clock_ <= 0.f && mode_ == Mode::Run) fail("the other crew took the grass");
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    float rush = std::clamp(speed() / kMax, 0.f, 1.f);
    bool run = mode_ == Mode::Run && throttle_ > 0.15f;
    tone0_ = run ? 48.f + rush * 30.f : 0.f;
    sys_->apu.tone(0, tone0_, run ? 0.03f + rush * 0.02f : 0.f);
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 880.f, 1046.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(1, notes[mode_ == Mode::Win ? n : 0], 0.07f);
            chimeStep_++;
            chimeT_ = 0.15f;
            if (chimeStep_ >= chimeN_) {
                chimeN_ = 0;
                sys_->apu.tone(1, 0, 0);
            }
        }
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    float x = 8.f + float(col) * 8.f;
    float y = 6.f + float(row) * 10.f;
    for (const char* p = s; *p; ++p) {
        int gi = int(*p) - 32;
        if (gi >= 0 && gi < 96) spr(art_.glyph[gi], x, y, 8.f, pal);
        x += 8.f;
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    int col = std::max(0, (40 - n) / 2);
    hud(col, row, s, pal);
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        hudC(18, "LEFT RIGHT STEER", PAL_HUD);
        hudC(19, "UP MUSH   DOWN WHOA", PAL_HUD);
        hudC(21, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        return;
    }
    std::snprintf(line_, sizeof line_, "CREW %4.1f", std::max(0.f, clock_));
    hud(28, 0, line_, clock_ < 8.f ? PAL_ALERT : PAL_HUD);
    const char* order = "COAST";
    if (throttle_ > 0.55f) order = "MUSH";
    else if (throttle_ > 0.08f) order = "EASY";
    else if (throttle_ < -0.35f) order = "WHOA";
    hud(1, 0, order, PAL_HUD);
    if (hold_ > 0.02f) hud(1, 20, "HOLD THE FULL STOP", PAL_HUD);
    else if (touched_) hud(1, 20, "WHOLE TEAM ON THE GRASS", PAL_HUD);
    else hud(1, 20, "THE GRASS IS THE FINISH", PAL_HUD);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal);
}

int Game::sledFrame(float heading) const {
    float u = heading;
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    const uint16_t snow = gs::rgb4(13, 15, 15);
    const uint16_t grass = gs::rgb4(2, 9, 3);
    const uint16_t grassFar = gs::rgb4(1, 6, 2);
    const uint16_t sky = gs::rgb4(5, 9, 13);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy >= kGrassY0 && wy <= kGrassY1 + 8.f) {
            c = lerpC(grass, grassFar, std::clamp((wy - kGrassY0) / 70.f, 0.f, 1.f));
        } else if (wy > kGrassY1) {
            c = grassFar;
        } else {
            c = lerpC(snow, sky, std::clamp(wy / 200.f, 0.f, 1.f));
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    float follow = mode_ == Mode::Title ? 1.f : 0.1f;
    float wantX = mode_ == Mode::Title ? 0.f : x_;
    float wantY = mode_ == Mode::Title ? 110.f : y_;
    float wantZ = mode_ == Mode::Title ? kTitleZoom : kPlayZoom;
    camX_ += (wantX - camX_) * follow;
    camY_ += (wantY - camY_) * follow;
    zoom_ += (wantZ - zoom_) * (mode_ == Mode::Title ? 1.f : 0.08f);

    for (const auto& t : kTree) place(art_.tree, t[0], t[1], 20.f, PAL_TREE);
    place(art_.cabin, 34.f, 206.f, 24.f, PAL_CABIN);
    place(art_.flag, 0.f, kGrassY0 + 4.f, 16.f, PAL_FLAG);
    place(art_.post, -22.f, kGrassY0 + 2.f, 12.f, PAL_POST);
    place(art_.post, 22.f, kGrassY0 + 2.f, 12.f, PAL_POST);
    for (const auto& t : kTuft) place(art_.tuft, t[0], t[1], 8.f, PAL_TUFT);

    float u = 1.f - std::clamp(clock_ / kCrewTime, 0.f, 1.f);
    float cx = 14.f;
    float cy = kStartY + (kAimY - kStartY) * u;
    float ch = kNorth;
    if (mode_ != Mode::Title) place(art_.sled[sledFrame(ch)], cx, cy, 15.f, PAL_CREW);

    for (const Puff& p : spray_) {
        if (p.life > 0.f) place(art_.puff, p.x, p.y, 5.f + (0.4f - p.life) * 6.f, PAL_PUFF);
    }
    place(art_.sled[sledFrame(heading_)], x_, y_, 22.f, PAL_TEAM);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 28, float(art_.title.h), PAL_BANNER);
        spr(art_.sub, 160, 54, float(art_.sub.h) * 0.72f, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160, 96, float(art_.paused.h), PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        spr(art_.landed, 160, 72, float(art_.landed.h), PAL_WIN);
        spr(art_.beaten, 160, 100, float(art_.beaten.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.lost, 160, 90, float(art_.lost.h), PAL_ALERT);
    }
    drawHud();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.anyPressed()) {
            begin();
            mode_ = Mode::Run;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
        }
        draw();
        audio(kDt);
        return;
    }
    if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) showTitle();
        draw();
        audio(kDt);
        return;
    }
    if (p.pressed(gs::BTN_START) && !bot_) {
        mode_ = Mode::Pause;
        draw();
        return;
    }

    float steer = 0.f, throttle = 0.f;
    if (bot_) pilot(steer, throttle);
    else controls(steer, throttle);
    physics(kDt, steer, throttle);
    race_ += kDt;
    clock_ -= kDt;
    judge();
    audio(kDt);
    draw();
}

}  // namespace mushgrass
