#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace redoubtbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 2680.0f;
constexpr float kHomeX = 150.0f;
constexpr float kSpawn = 280.0f;
constexpr float kBanner0 = 2360.0f;
constexpr float kMinX = 70.0f;
constexpr float kMaxX = 2520.0f;
constexpr float kRun = 168.0f;
constexpr float kWatch = 78.0f;
constexpr int kLanes = 3;
constexpr int kGabions = 9;
constexpr int kRaiders = 5;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::laneFeet(int lane) const {
    static const float kFeet[3] = {204.0f, 168.0f, 132.0f};
    return kFeet[std::clamp(lane, 0, 2)];
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 1100.0f) return 2;
    return 3;
}

void Game::resetRun() {
    auto raid = [&](int i, float a, float b, float sp, int lane) {
        Raider& w = raider_[i];
        w.minX = a;
        w.maxX = b;
        w.x = (a + b) * 0.5f;
        w.speed = sp;
        w.dir = (i & 1) ? -1.0f : 1.0f;
        w.lane = lane;
    };
    raid(0, 460.0f, 760.0f, 34.0f, 1);
    raid(1, 880.0f, 1180.0f, 38.0f, 2);
    raid(2, 1240.0f, 1560.0f, 32.0f, 0);
    raid(3, 1620.0f, 1980.0f, 40.0f, 2);
    raid(4, 1880.0f, 2280.0f, 30.0f, 1);

    const float gx[kGabions] = {620, 1420, 2060, 420, 1120, 1840, 820, 1540, 2140};
    const int gl[kGabions] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    for (int i = 0; i < kGabions; i++) {
        gabion_[i].x = gx[i];
        gabion_[i].lane = gl[i];
    }

    px_ = kSpawn;
    vx_ = 0;
    face_ = 1;
    lane_ = 1;
    has_ = false;
    bannerX_ = kBanner0;
    bannerLane_ = 2;
    lives_ = 4;
    watch_ = kWatch;
    hop_ = hopCd_ = inv_ = laneCd_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = clampf(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(420.0f, 0.05f, 0.07f);
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    vx_ = 0;
    shake_ = 0.35f;
    sys_->rumble(0.25f, 0.6f, 180);
    sys_->setLight(40, 140, 50);
    blip(620.0f, 0.08f, 0.2f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    sys_->rumble(0.7f, 0.2f, 160);
    sys_->setLight(140, 20, 16);
    sys_->apu.noiseBurst(0.35f, 120.0f, 0.25f);
}

void Game::hit(float fromX) {
    if (inv_ > 0 || hop_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.05f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 140.0f;
    px_ = clampf(px_ + away * 10.0f, kMinX, kMaxX);
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.36f, 380.0f, 0.12f);
    sys_->rumble(0.55f, 0.25f, 90);
    if (has_) {
        has_ = false;
        bannerX_ = clampf(px_, 360.0f, 2400.0f);
        bannerLane_ = lane_;
    }
    if (lives_ <= 0) lose();
}

bool Game::blocked(int lane, float x, int dir, float reach) const {
    for (int i = 0; i < kGabions; i++) {
        if (gabion_[i].lane != lane) continue;
        float rel = gabion_[i].x - x;
        if (dir == 0) {
            if (std::abs(rel) < 26.0f) return true;
        } else if (dir > 0) {
            if (rel > -8.0f && rel < reach) return true;
        } else if (rel < 8.0f && rel > -reach) {
            return true;
        }
    }
    for (int i = 0; i < kRaiders; i++) {
        if (raider_[i].lane != lane) continue;
        float rel = raider_[i].x - x;
        if (dir == 0) {
            if (std::abs(rel) < 28.0f) return true;
        } else if (dir > 0) {
            if (rel > -12.0f && rel < reach) return true;
        } else if (rel < 12.0f && rel > -reach) {
            return true;
        }
    }
    return false;
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& hop) {
    left = right = up = down = hop = false;
    // The ditch is the quiet terrace. Climb to the crest only to take the banner
    // and again to plant it on the staff.
    const bool nearPrize = !has_ && px_ > bannerX_ - 160.0f;
    const bool plant = has_ && px_ < 340.0f;
    const int goalLane = plant || nearPrize ? (plant ? 2 : bannerLane_) : 0;
    const float goalX = has_ ? (kHomeX - 8.0f) : bannerX_;
    int travel = 0;
    if (!plant && goalX > px_ + 6.0f) travel = 1;
    else if (has_ && px_ > kHomeX) travel = -1;
    else if (!has_ && px_ > bannerX_ + 8.0f) travel = -1;

    auto clearLane = [&](int lane) { return lane >= 0 && lane < kLanes && !blocked(lane, px_, travel == 0 ? 1 : travel, 80.0f); };

    if (lane_ != goalLane && laneCd_ <= 0.0f) {
        int step = goalLane > lane_ ? 1 : -1;
        if (clearLane(lane_ + step) && !blocked(lane_ + step, px_, 0, 30.0f)) {
            if (step > 0) up = true;
            else down = true;
            return;
        }
    }
    if (travel != 0 && hop_ <= 0.0f && blocked(lane_, px_, travel, 72.0f)) {
        if (hopCd_ <= 0.0f) hop = true;
        else return;
    }
    if (travel > 0) {
        right = true;
        face_ = 1;
    } else if (travel < 0) {
        left = true;
        face_ = -1;
    }
}

void Game::stepPlay(bool left, bool right, bool up, bool down, bool hop) {
    playT_ += DT;
    watch_ -= DT;
    if (watch_ <= 0.0f) {
        watch_ = 0;
        lose();
        return;
    }
    if (inv_ > 0) inv_ -= DT;
    if (hopCd_ > 0) hopCd_ -= DT;
    if (hop_ > 0) hop_ -= DT;
    if (laneCd_ > 0) laneCd_ -= DT;

    for (Raider& w : raider_) {
        w.x += w.dir * w.speed * DT;
        if (w.x >= w.maxX) {
            w.x = w.maxX;
            w.dir = -1.0f;
        } else if (w.x <= w.minX) {
            w.x = w.minX;
            w.dir = 1.0f;
        }
    }

    if (hop && hopCd_ <= 0.0f && hop_ <= 0.0f) {
        hop_ = 0.52f;
        hopCd_ = 0.62f;
        blip(300.0f, 0.04f, 0.05f);
    }

    if (laneCd_ <= 0.0f && hop_ <= 0.0f) {
        if (up && !down && lane_ < kLanes - 1) {
            lane_++;
            laneCd_ = 0.16f;
        } else if (down && !up && lane_ > 0) {
            lane_--;
            laneCd_ = 0.16f;
        }
    }

    float target = 0;
    if (right && !left) {
        target = kRun;
        face_ = 1;
    } else if (left && !right) {
        target = -kRun;
        face_ = -1;
    }
    float accel = 2200.0f * DT;
    if (vx_ < target) vx_ = std::min(target, vx_ + accel);
    else vx_ = std::max(target, vx_ - accel);

    float next = px_ + vx_ * DT;
    if (hop_ <= 0.0f && blocked(lane_, next, vx_ >= 0 ? 1 : -1, 22.0f)) {
        vx_ = 0;
    } else {
        px_ = clampf(next, kMinX, kMaxX);
    }

    if (hop_ <= 0.0f && inv_ <= 0.0f) {
        for (int i = 0; i < kRaiders; i++) {
            if (raider_[i].lane != lane_) continue;
            if (std::abs(raider_[i].x - px_) < 20.0f) {
                hit(raider_[i].x);
                break;
            }
        }
    }

    if (!has_ && lane_ == bannerLane_ && std::abs(px_ - bannerX_) < 28.0f) {
        has_ = true;
        blip(520.0f, 0.06f, 0.08f);
        sys_->rumble(0.2f, 0.35f, 70);
    }

    if (has_ && lane_ == 2 && px_ <= kHomeX + 18.0f) win();

    cam_ = clampf(px_ - 150.0f, 0.0f, kWorld - gs::SCREEN_W);
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (int i = 0; i < int(s.size()); i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c < 32 || c > 127) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float feet, float h, int pal, bool flip) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.0f, 400.0f)));
    s.h = int16_t(std::lround(clampf(h, 1.0f, 300.0f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f - cam_));
    s.y = int16_t(std::lround(feet - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = gs::rgb4(5, 7, 10);
        if (y > 78) sky = gs::rgb4(7, 8, 6);
        if (y > 118) sky = gs::rgb4(6, 6, 3);
        if (y > 168) sky = gs::rgb4(5, 4, 2);
        if (mode_ == Mode::Victory && y < 78) sky = gs::rgb4(4, 8, 6);
        if (mode_ == Mode::Over && y < 78) sky = gs::rgb4(8, 4, 3);
        vdp.lineBackdrop[y] = sky;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.HUD.clear();
    vdp.clearSprites();

    float jx = std::sin(t_ * 40.0f) * shake_ * 3.0f;
    spr(art_.works, 160.0f + jx, 196.0f, 78.0f, PAL_EARTH, false);
    spr(art_.staff, kHomeX + jx, laneFeet(2) + 8.0f, 56.0f, PAL_WOOD, false);

    for (int i = 0; i < 8; i++) {
        float x = 360.0f + i * 280.0f;
        spr(art_.puff, x, 118.0f, 10.0f, PAL_BURST, false);
    }
    for (int i = 0; i < kGabions; i++) spr(art_.bag, gabion_[i].x, laneFeet(gabion_[i].lane) + 2.0f, 22.0f, PAL_EARTH, false);

    if (!has_) spr(art_.banner, bannerX_, laneFeet(bannerLane_) - 2.0f, 40.0f, PAL_BANNER, false);

    for (int i = 0; i < kRaiders; i++) {
        const Raider& w = raider_[i];
        spr(art_.foe, w.x, laneFeet(w.lane), 36.0f, PAL_FOE, w.dir < 0);
    }

    float lift = hop_ > 0 ? 16.0f : 0.0f;
    const gs::Mipped& body = hop_ > 0 ? art_.hop : art_.stand;
    bool blink = inv_ > 0 && (int(playT_ * 20.0f) & 1);
    if (!blink) spr(body, px_ + jx, laneFeet(lane_) - lift, 40.0f, PAL_HERO, face_ < 0);
    if (has_) spr(art_.banner, px_ + face_ * 14.0f + jx, laneFeet(lane_) - lift - 18.0f, 28.0f, PAL_BANNER, face_ < 0);

    if (mode_ == Mode::Title) {
        hudC(3, "S3 REDOUBT", PAL_AMBER);
        hudC(6, "BRING THE BANNER BACK", PAL_TEXT);
        hudC(8, "MISS THAT AND THE WATCH IS OVER", PAL_BAD);
        hudC(15, "ARROWS  MOVE THE TERRACE", PAL_TEXT);
        hudC(16, "A  HOP THE GABION", PAL_AMBER);
        hudC(18, "PLANT IT ON THE STAFF", PAL_GOOD);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
        hudC(12, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(4, "BANNER BACK", PAL_GOOD);
        hudC(6, "THE WATCH HOLDS", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hudC(4, "THE WATCH IS OVER", PAL_BAD);
        hudC(6, "THE BANNER NEVER CAME BACK", PAL_TEXT);
    } else {
        int sec = int(std::ceil(watch_));
        char buf[40];
        std::snprintf(buf, sizeof buf, "WATCH %d", sec);
        hud(1, 1, buf, watch_ < 15.0f ? PAL_BAD : PAL_AMBER);
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hud(30, 1, buf, PAL_TEXT);
        hudC(26, has_ ? "BRING IT TO THE STAFF" : "THE BANNER IS OUT ON THE CREST", has_ ? PAL_GOOD : PAL_TEXT);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (beep_ > 0.0f) beep_ -= DT;
    else sys.apu.tone(0, 0, 0);

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            bool left = false, right = false, up = false, down = false, hop = false;
            if (bot_) bot(left, right, up, down, hop);
            else {
                left = pad.down(gs::BTN_LEFT);
                right = pad.down(gs::BTN_RIGHT);
                up = pad.pressed(gs::BTN_UP);
                down = pad.pressed(gs::BTN_DOWN);
                hop = pad.pressed(gs::BTN_A);
            }
            stepPlay(left, right, up, down, hop);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (!bot_ && pad.pressed(gs::BTN_START)) {
        begin();
    }
    draw();
}

}  // namespace redoubtbann
