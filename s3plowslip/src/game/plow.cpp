#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace plow {
namespace {

constexpr float TIDE = 38.f;
constexpr float SLIP_X = 160.f;
constexpr float BERTH_Y = 58.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
}

void Game::resetRun() {
    x_ = 168.f;
    y_ = 198.f;
    yaw_ = 0.15f;
    speed_ = 0;
    hold_ = 0;
    banner_ = 0;
    scrape_ = 0;
    scrapes_ = 0;
    berthed_ = false;
    tide_ = TIDE;
    score_ = 0;
    won_ = false;
    mode_ = Mode::Run;
}

void Game::tone(float freq, float vol) { sys_->apu.tone(0, freq, vol); }

void Game::stick(float& steer, bool& thrust, bool& brake) {
    steer = 0;
    thrust = false;
    brake = false;
    if (bot_) {
        float err = SLIP_X - x_;
        float wantYaw = clampf(err / 30.f, -0.75f, 0.75f);
        if (y_ < 100.f) wantYaw = clampf(err / 20.f, -0.4f, 0.4f);
        if (y_ < 74.f) wantYaw = clampf(err / 14.f, -0.18f, 0.18f);
        float wantSp = 42.f;
        if (y_ < 140.f) wantSp = 26.f;
        if (y_ < 100.f) wantSp = 14.f;
        if (y_ < 78.f) wantSp = 8.f;
        if (y_ < 66.f) wantSp = 4.f;
        if (std::fabs(err) > 10.f && y_ < 110.f) wantSp = std::min(wantSp, 16.f);
        if (y_ < 70.f && std::fabs(err) < 4.f && std::fabs(yaw_) < 0.2f) wantSp = 2.2f;
        steer = clampf((wantYaw - yaw_) * 5.f, -1.f, 1.f);
        thrust = speed_ < wantSp - 0.6f;
        brake = speed_ > wantSp + 0.7f;
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.2f) steer = p.axisX;
    thrust = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.accel > 0.2f;
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.2f;
}

void Game::update(float dt) {
    t_ += dt;
    if (mode_ == Mode::Title) {
        if (bot_ || sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A)) resetRun();
        tone(0, 0);
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        banner_ += dt;
        tone(mode_ == Mode::Win ? 440.f : 90.f, banner_ < 0.6f ? 0.12f : 0.f);
        if (banner_ > 0.35f) over_ = true;
        if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) {
            over_ = false;
            mode_ = Mode::Title;
        }
        return;
    }

    tide_ -= dt;
    float steer = 0;
    bool thrust = false, brake = false;
    stick(steer, thrust, brake);
    yaw_ = clampf(yaw_ + steer * 1.55f * dt, -1.f, 1.f);
    if (thrust) speed_ += 48.f * dt;
    else if (brake) speed_ -= 78.f * dt;
    else speed_ -= 10.f * dt;
    speed_ = clampf(speed_, 0.f, 54.f);

    x_ += yaw_ * (8.f + speed_ * 0.72f) * dt;
    y_ -= speed_ * (1.f - 0.12f * std::fabs(yaw_)) * dt;

    auto hit = [&](float wall, int side) {
        x_ = wall;
        speed_ *= 0.42f;
        yaw_ = clampf(yaw_ - side * 0.35f, -1.f, 1.f);
        scrape_ = 0.35f;
        scrapes_++;
        sys_->apu.noiseBurst(0.25f, 1800.f, 0.08f);
    };
    if (y_ < 128.f) {
        if (x_ < 126.f) hit(126.f, -1);
        if (x_ > 194.f) hit(194.f, 1);
    }
    if (y_ < 96.f) {
        if (x_ < 146.f) hit(146.f, -1);
        if (x_ > 174.f) hit(174.f, 1);
    }
    if (y_ > 210.f) y_ = 210.f;
    if (x_ < 36.f) x_ = 36.f;
    if (x_ > 284.f) x_ = 284.f;
    if (scrape_ > 0) scrape_ -= dt;

    if (y_ < 46.f) {
        y_ = 46.f;
        speed_ *= 0.2f;
        scrape_ = 0.25f;
    }

    bool lined = std::fabs(x_ - SLIP_X) < 7.f && std::fabs(yaw_) < 0.22f && speed_ < 8.f && y_ < 68.f && y_ > 46.f;
    if (lined) hold_ += dt;
    else hold_ = std::max(0.f, hold_ - dt * 0.5f);

    if (hold_ > 0.45f) {
        berthed_ = true;
        won_ = true;
        mode_ = Mode::Win;
        float left = std::max(0.f, tide_);
        score_ = 1000 + int(left * 40.f) - scrapes_ * 25;
        if (score_ < 100) score_ = 100;
        banner_ = 0;
        return;
    }
    if (tide_ <= 0.f) {
        tide_ = 0;
        won_ = false;
        mode_ = Mode::Lose;
        score_ = 0;
        banner_ = 0;
    }
    tone(48.f + speed_ * 2.2f, speed_ > 1.f ? 0.05f : 0.f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (m.w < 1 || h < 1.f) return;
    float s = h / float(m.h);
    float w = m.w * s;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.x = int16_t(cx - w * 0.5f);
    sp.y = int16_t(cy - h * 0.5f);
    sp.w = int16_t(std::max(1.f, w));
    sp.h = int16_t(h);
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const std::string& s, float x, float y, int pal, int align) {
    float w = 0;
    for (char ch : s) w += (ch == ' ' ? 4.f : 6.f);
    if (align == 1) x -= w * 0.5f;
    if (align == 2) x -= w;
    for (char ch : s) {
        if (ch >= 32 && ch < 127 && ch != ' ') spr(art_.glyph[ch - 32], x + 2.f, y, 7.f, pal);
        x += (ch == ' ' ? 4.f : 6.f);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    float tideU = clampf(tide_ / TIDE, 0.f, 1.f);
    // HUD first: earlier sprites are drawn on top of the harbour.
    char line[64];
    if (mode_ == Mode::Title) {
        text("S3 PLOWSLIP", 160, 48, PAL_HUD, 1);
        text("TAKE THE PLOW", 160, 78, PAL_HUD, 1);
        text("BERTH IN THE SLIP", 160, 92, PAL_HUD, 1);
        text("BEFORE THE TIDE TURNS", 160, 106, PAL_HUD, 1);
        text("THE CLOCK IS THE OTHER CREW", 160, 128, PAL_HUD, 1);
        text("ARROWS STEER AND DRIVE", 160, 156, PAL_HUD, 1);
        text("A THRUST   B BRAKE", 160, 170, PAL_HUD, 1);
        if (int(t_ * 2.f) % 2 == 0) text("PRESS START", 160, 196, PAL_HUD, 1);
    } else {
        int sec = int(std::ceil(tide_));
        if (sec < 0) sec = 0;
        std::snprintf(line, sizeof(line), "TIDE %d", sec);
        text(line, 8, 12, PAL_HUD, 0);
        text("OTHER CREW", 312, 12, PAL_HUD, 2);
        int bar = int((1.f - tideU) * 18);
        std::string marks(size_t(std::max(0, bar)), '>');
        if (!marks.empty()) text(marks, 200, 24, PAL_HUD, 0);
        std::snprintf(line, sizeof(line), "OFF %d", int(std::fabs(x_ - SLIP_X)));
        text(line, 8, 24, PAL_HUD, 0);
        if (mode_ == Mode::Win) {
            text("PLOW BERTHED", 160, 150, PAL_HUD, 1);
            std::snprintf(line, sizeof(line), "SCORE %d", score_);
            text(line, 160, 166, PAL_HUD, 1);
        } else if (mode_ == Mode::Lose) {
            text("TIDE TURNED", 160, 150, PAL_HUD, 1);
            text("OTHER CREW TOOK THE SLIP", 160, 166, PAL_HUD, 1);
        } else if (hold_ > 0.05f) {
            text("HOLD THE BERTH", 160, 188, PAL_HUD, 1);
        } else if (scrape_ > 0.f) {
            text("EASE OFF THE PIER", 160, 188, PAL_HUD, 1);
        }
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 18) c = gs::rgb4(2, 3, 6);
        else if (y < 34) c = gs::rgb4(3, 5, 8);
        else {
            int deep = 4 + (y - 34) / 28;
            int drop = int((1.f - tideU) * 3.f);
            deep = std::max(1, deep - drop);
            if (y > 150 && tideU < 0.35f) c = gs::rgb4(5, 4, 2);
            else c = gs::rgb4(1, deep, deep + 3);
        }
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    // Earlier sprites sit on top, so the plow is submitted before the piers.
    float crewX = 250.f - (1.f - tideU) * 70.f;
    if (mode_ == Mode::Lose) crewX = 168.f;
    float crewY = mode_ == Mode::Lose ? 64.f : 96.f;
    spr(art_.crewBoat, crewX, crewY, 36, PAL_CREW, true);
    spr(art_.crew, crewX - 6, crewY - 16, 14, PAL_CREW);
    spr(art_.crew, crewX + 4, crewY - 14, 14, PAL_CREW);
    if (speed_ > 4.f && mode_ == Mode::Run) spr(art_.wake, x_, y_ + 26.f, 8.f + speed_ * 0.15f, PAL_WAKE);
    int face = int(std::lround(clampf(yaw_, -1.f, 1.f) * 2.f)) + 2;
    face = std::max(0, std::min(4, face));
    float bob = std::sin(t_ * 3.f + y_ * 0.05f) * 1.2f;
    spr(art_.plow[face], x_, y_ + bob, 52, PAL_PLOW);

    spr(art_.pier, 104, 78, 150, PAL_PIER);
    spr(art_.pier, 216, 78, 150, PAL_PIER);
    spr(art_.bulk, 160, 38, 16, PAL_PIER);
    spr(art_.buoy, 132, 132, 16, PAL_BUOY);
    spr(art_.buoy, 188, 132, 16, PAL_BUOY);
    spr(art_.buoy, 132, 168, 14, PAL_BUOY);
    spr(art_.buoy, 188, 168, 14, PAL_BUOY);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.f / 60.f);
    draw();
}

}  // namespace plow
