#include "game/bike.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace bike {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PPM = 18.f;
constexpr float GROUND_Y = 188.f;
constexpr float BIKE_X = 96.f;
constexpr float HIGH_Y = 106.f;
constexpr float JUMP_V = 400.f;
constexpr float GRAV = 1050.f;
constexpr float MIN_SPD = 8.f;
constexpr float MAX_SPD = 14.f;
constexpr float WHEEL_R = 13.f;
constexpr float PLAYER_R = 10.f;
constexpr float STAND_H = 74.f;
constexpr float DUCK_H = 46.f;
constexpr float T_APEX = JUMP_V / GRAV;
constexpr int ANCHOR = 84;
constexpr int MID = 46;

float wrapPhase(float a) {
    float m = std::fmod(a, 1.f);
    return m < 0.f ? m + 1.f : m;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    won_ = false;
    over_ = false;
    meters_ = 0;
    t_ = 0;
    std::snprintf(result_, sizeof result_, "S3 BIKE  FAIL  unfinished");
    if (bot_) resetRun();
    else mode_ = Mode::Title;
    sys.apu.setMaster(0.8f);
}

void Game::buildCourse() {
    wheels_.clear();
    float z = 42.f;
    int n = 0;
    while (z < 968.f) {
        switch (n % 6) {
        case 0: wheels_.push_back({z, Kind::Ground}); z += 24.f; break;
        case 1: wheels_.push_back({z, Kind::High}); z += 20.f; break;
        case 2: wheels_.push_back({z, Kind::Ground}); z += 26.f; break;
        case 3:
            wheels_.push_back({z, Kind::High});
            wheels_.push_back({z + 8.f, Kind::High});
            z += 24.f;
            break;
        case 4:
            wheels_.push_back({z, Kind::Ground});
            wheels_.push_back({z + 20.f, Kind::High});
            z += 40.f;
            break;
        default:
            wheels_.push_back({z, Kind::High});
            wheels_.push_back({z + 18.f, Kind::Ground});
            z += 38.f;
            break;
        }
        n++;
    }
}

void Game::resetRun() {
    mode_ = Mode::Run;
    odo_ = 0;
    speed_ = 10.f;
    lift_ = 0;
    vy_ = 0;
    grounded_ = true;
    duck_ = false;
    hopBuf_ = 0;
    raceT_ = 0;
    modeT_ = 0;
    jumpSnd_ = 0;
    winT_ = 0;
    shake_ = 0;
    danger_ = false;
    burst_ = false;
    won_ = false;
    over_ = false;
    meters_ = 0;
}

bool Game::go() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);
}

Game::In Game::controls() {
    In in{};
    if (bot_) {
        const Wheel* ground = nullptr;
        const Wheel* high = nullptr;
        float bestG = 1e9f, bestH = 1e9f;
        for (const Wheel& w : wheels_) {
            float dz = w.z - odo_;
            if (w.kind == Kind::Ground) {
                if (dz < -0.4f || dz > 16.f) continue;
                if (dz < bestG) {
                    bestG = dz;
                    ground = &w;
                }
            } else if (dz > -2.2f && dz < bestH && dz < 8.f) {
                bestH = dz;
                high = &w;
            }
        }
        in.pedal = speed_ < 12.f;
        in.brake = speed_ > 12.35f;
        if (high) {
            float reach = 1.95f + speed_ * 0.07f;
            float dz = high->z - odo_;
            if (dz < reach && dz > -1.95f) in.duck = true;
        }
        if (ground && grounded_) {
            float tHit = (ground->z - odo_) / std::max(speed_, 0.1f);
            bool blocked = false;
            if (high) {
                float th = (high->z - odo_) / std::max(speed_, 0.1f);
                if (th > -0.05f && th < 0.78f) blocked = true;
            }
            if (!blocked && tHit <= T_APEX + 0.05f && tHit >= T_APEX - 0.05f) in.hop = true;
        }
        return in;
    }
    const gs::Pad& p = sys_->pad;
    in.pedal = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.accel > 0.25f;
    in.brake = p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.brake > 0.25f;
    in.duck = p.down(gs::BTN_DOWN);
    in.hop = p.pressed(gs::BTN_UP) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    return in;
}

void Game::runLogic(float dt) {
    In in = controls();
    duck_ = in.duck;
    if (in.brake) speed_ -= 9.f * dt;
    else if (in.pedal) speed_ += 7.f * dt;
    else speed_ -= 1.6f * dt;
    speed_ = std::clamp(speed_, MIN_SPD, MAX_SPD);
    odo_ += speed_ * dt;
    raceT_ += dt;

    if (in.hop) hopBuf_ = 7;
    if (hopBuf_ > 0 && grounded_) {
        grounded_ = false;
        vy_ = JUMP_V;
        hopBuf_ = 0;
        jumpSnd_ = 0.14f;
        if (!sys_->headless) sys_->rumble(0.12f, 0.35f, 50);
    } else if (hopBuf_ > 0) hopBuf_--;

    if (!grounded_) {
        vy_ -= GRAV * dt;
        lift_ += vy_ * dt;
        if (lift_ <= 0.f) {
            lift_ = 0.f;
            vy_ = 0.f;
            grounded_ = true;
            shake_ = std::max(shake_, 2);
        }
    }

    meters_ = std::min(1000, int(odo_));
    if (odo_ >= 1000.f) {
        odo_ = 1000.f;
        meters_ = 1000;
        finishWin();
        return;
    }
    collide();
}

void Game::collide() {
    danger_ = false;
    const float top = GROUND_Y - (duck_ ? DUCK_H : STAND_H) - lift_;
    const float bot = GROUND_Y - lift_;
    const float left = BIKE_X - 14.f;
    const float right = BIKE_X + 18.f;
    const float wy = GROUND_Y - 14.f - lift_;
    for (const Wheel& w : wheels_) {
        float dz = w.z - odo_;
        if (dz < -3.f || dz > 8.f) continue;
        if (dz > 0.f && dz < 6.5f) danger_ = true;
        float sx = BIKE_X + dz * PPM;
        float sy = w.kind == Kind::Ground ? GROUND_Y - 14.f : HIGH_Y;
        auto circles = [&](float x, float y, float pr) {
            float dx = sx - x, dy = sy - y;
            float rr = WHEEL_R + pr - 1.f;
            return dx * dx + dy * dy < rr * rr;
        };
        bool hit = circles(BIKE_X - 20.f, wy, PLAYER_R) || circles(BIKE_X + 20.f, wy, PLAYER_R);
        float nx = std::clamp(sx, left, right);
        float ny = std::clamp(sy, top, bot);
        float dx = sx - nx, dy = sy - ny;
        float rad = WHEEL_R - 1.f;
        if (dx * dx + dy * dy < rad * rad) hit = true;
        if (hit) {
            finishFail();
            return;
        }
    }
}

void Game::finishWin() {
    mode_ = Mode::Win;
    won_ = true;
    modeT_ = 0;
    winT_ = 0;
    std::snprintf(result_, sizeof result_,
                  "S3 BIKE  WIN  one kilometer clean  1000 m  wheels untouched  (%.1f s)", raceT_);
}

void Game::finishFail() {
    mode_ = Mode::Crash;
    won_ = false;
    modeT_ = 0;
    shake_ = 8;
    burst_ = false;
    grounded_ = true;
    lift_ = 0;
    vy_ = 0;
    meters_ = std::min(999, int(odo_));
    const char* kind = "loose";
    float dz = 0;
    for (const Wheel& w : wheels_) {
        float d = w.z - odo_;
        if (std::fabs(d) < 3.f) {
            kind = w.kind == Kind::Ground ? "ground" : "high";
            dz = d;
            break;
        }
    }
    std::snprintf(result_, sizeof result_,
                  "S3 BIKE  FAIL  touched a %s wheel at %d m  lift %.0f  spd %.1f  dz %.2f", kind, meters_, lift_,
                  speed_, dz);
    if (!sys_->headless) sys_->rumble(0.8f, 0.9f, 180);
    sys_->apu.noiseBurst(0.45f, 900.f, 0.28f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const bool modeEdge = sys.pad.pressed(gs::BTN_MODE);
    switch (mode_) {
    case Mode::Title:
        if (go()) resetRun();
        break;
    case Mode::Run:
        if (!bot_ && modeEdge) {
            held_ = Mode::Run;
            mode_ = Mode::Pause;
            break;
        }
        runLogic(DT);
        break;
    case Mode::Pause:
        if (modeEdge || go()) mode_ = held_;
        break;
    case Mode::Crash:
        modeT_ += DT;
        shake_ = std::max(0, shake_ - 1);
        if (modeT_ > 0.7f) {
            mode_ = Mode::Fail;
            modeT_ = 0;
        }
        break;
    case Mode::Fail:
    case Mode::Win:
        modeT_ += DT;
        winT_ += DT;
        if (!bot_ && go()) resetRun();
        break;
    }
    draw();
    mix();
    if (mode_ == Mode::Win || mode_ == Mode::Fail) over_ = true;
}

void Game::mix() {
    float f0 = 0, v0 = 0, f1 = 0, v1 = 0, f2 = 0, v2 = 0;
    if (mode_ == Mode::Run) {
        if (wrapPhase(odo_ * 1.6f) < 0.08f) {
            v0 = 0.04f;
            f0 = 78.f + speed_ * 6.f;
        }
        if (jumpSnd_ > 0.f) {
            v1 = 0.07f;
            f1 = 180.f + jumpSnd_ * 1400.f;
            jumpSnd_ -= DT;
        }
    } else if (mode_ == Mode::Win) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int step = int(winT_ / 0.14f);
        if (step >= 0 && step < 4) {
            v2 = 0.07f;
            f2 = notes[step];
        }
    } else if (mode_ == Mode::Title && wrapPhase(t_ * 0.5f) < 0.05f) {
        v2 = 0.03f;
        f2 = 392.f;
    }
    sys_->apu.tone(0, f0, v0);
    sys_->apu.tone(1, f1, v1);
    sys_->apu.tone(2, f2, v2);
    if (!sys_->headless) {
        if (mode_ == Mode::Win) sys_->setLight(40, 180, 70);
        else if (mode_ == Mode::Crash || mode_ == Mode::Fail) sys_->setLight(220, 30, 30);
        else if (danger_) sys_->setLight(220, 120, 30);
        else sys_->setLight(40, 80, 180);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    float pix = (mode_ == Mode::Title) ? t_ * 26.f : odo_ * PPM;
    int shake = shake_ ? (int(t_ * 60.f) & 1 ? shake_ : -shake_) : 0;
    v.A.scroll(-int(std::lround(pix)) + shake, 0);
    v.B.scroll(-int(std::lround(pix * 0.38f)), 0);
    v.A.enabled = true;
    v.B.enabled = true;
    bool flash = mode_ == Mode::Crash && modeT_ < 0.18f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y < 150 ? y / 150.f : 1.f;
        int r = int(2 + (12 - 2) * u);
        int g = int(3 + (7 - 3) * u);
        int b = int(8 + (5 - 8) * u);
        if (flash) r = std::min(15, r + 6);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::blit(const gs::Mipped& m, float x, float y, int pal, bool flip) {
    if (m.w < 1 || m.h < 1) return;
    if (x < -90.f || y < -90.f || x > gs::SCREEN_W + 20.f || y > gs::SCREEN_H + 20.f) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(m.w);
    s.h = int16_t(m.h);
    s.img = m.pick(float(m.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::image(const gs::Image& img, float x, float y, int pal) {
    if (!img.w || !img.h) return;
    if (x < -img.w || y < -img.h || x > gs::SCREEN_W || y > gs::SCREEN_H) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud() {
    gs::VDP& v = sys_->vdp;
    v.HUD.clear();
    auto text = [&](int col, int row, const std::string& s, int pal) {
        if (row < 0 || row > 27) return;
        for (size_t i = 0; i < s.size(); i++) {
            int x = col + int(i);
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
            if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
            int tile = art_.font[c];
            if (!tile) continue;
            v.HUD.set(x, row, gs::entry(tile, pal));
        }
    };
    if (mode_ == Mode::Title) {
        text(18, 21, "UP HOP", PAL_INK);
        text(18, 22, "DOWN DUCK", PAL_INK);
        text(18, 23, "Z PEDAL  X BRAKE", PAL_INK);
        if ((int(t_ * 2.f) & 1) == 0) text(18, 25, "START", PAL_ALERT);
        return;
    }
    char line[40];
    int show = mode_ == Mode::Win ? 1000 : meters_;
    float spd = (mode_ == Mode::Run || mode_ == Mode::Win) ? speed_ : 0.f;
    std::snprintf(line, sizeof line, "%4d M   %2.0f M/S", show, spd);
    text(1, 0, line, PAL_INK);
    std::string bar = "KILO ";
    int filled = std::min(10, show / 100);
    for (int i = 0; i < 10; i++) bar += (i < filled) ? '#' : '-';
    text(1, 1, bar, danger_ && mode_ == Mode::Run ? PAL_ALERT : PAL_INK);
    if (mode_ == Mode::Run) text(1, 26, "DON'T TOUCH WHEELS", PAL_INK);
    if (mode_ == Mode::Pause) text(16, 12, "PAUSED", PAL_ALERT);
    if (mode_ == Mode::Fail) {
        text(8, 12, "TOUCHED A WHEEL", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) text(12, 14, "START RETRIES", PAL_INK);
    }
    if (mode_ == Mode::Win) {
        text(6, 12, "ONE KILOMETER CLEAN", PAL_INK);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) text(14, 14, "START RIDES", PAL_INK);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    backdrop();
    hud();

    auto worldX = [&](float z) { return BIKE_X + (z - (mode_ == Mode::Title ? 0.f : odo_)) * PPM; };

    if (mode_ == Mode::Title) {
        image(art_.title, (gs::SCREEN_W - art_.title.w) * 0.5f, 18.f, PAL_TITLE);
        image(art_.sub, (gs::SCREEN_W - art_.sub.w) * 0.5f, 58.f, PAL_TITLE);
        image(art_.rule, (gs::SCREEN_W - art_.rule.w) * 0.5f, 84.f, PAL_TITLE);
    } else if (mode_ == Mode::Win) {
        image(art_.rule, (gs::SCREEN_W - art_.rule.w) * 0.5f, 64.f, PAL_TITLE);
    }

    if (mode_ != Mode::Title) {
        for (const Wheel& w : wheels_) {
            float x = worldX(w.z);
            if (x < -50.f || x > gs::SCREEN_W + 40.f) continue;
            int frame = int(std::floor((odo_ + w.z) * 2.f)) & 3;
            float y = (w.kind == Kind::Ground ? GROUND_Y - 14.f : HIGH_Y) - 20.f;
            blit(art_.wheel[frame], x - 20.f, y, w.kind == Kind::Ground ? PAL_LOW : PAL_HIGH);
        }
    } else {
        float gx = 210.f + std::sin(t_ * 1.3f) * 78.f;
        int frame = int(t_ * 8.f) & 3;
        blit(art_.wheel[frame], gx - 20.f, GROUND_Y - 14.f - 20.f, PAL_LOW);
        float hx = std::fmod(t_ * 42.f, 400.f) - 30.f;
        blit(art_.wheel[(frame + 1) & 3], hx - 20.f, 78.f - 20.f, PAL_HIGH);
    }

    float lift = (mode_ == Mode::Title) ? 0.f : lift_;
    float by = GROUND_Y - float(ANCHOR) - lift;
    float bx = BIKE_X - float(MID);
    const gs::Mipped* rider = &art_.bike[int(odo_ * 2.4f) & 1];
    if (mode_ == Mode::Title) rider = &art_.bike[int(t_ * 6.f) & 1];
    if (duck_ && mode_ == Mode::Run) rider = &art_.duck;
    if (mode_ == Mode::Crash || mode_ == Mode::Fail) rider = &art_.crash;
    blit(*rider, bx + float((shake_ & 1) ? 1 : 0), by, PAL_BIKE, mode_ == Mode::Crash);

    if (mode_ != Mode::Title) {
        for (int i = 0; i < 12; i++) {
            float x = float((int(std::floor(odo_ * PPM)) + i * 32) % 352) - 16.f;
            image(art_.wire, x, HIGH_Y - 1.f, PAL_HIGH);
        }
        for (float z = std::floor((odo_ - 4.f) / 36.f) * 36.f; z < odo_ + 22.f; z += 36.f) {
            if (z < 8.f) continue;
            blit(art_.lamp, worldX(z) - 9.f, GROUND_Y - 78.f, PAL_SIGN);
        }
        const float marks[] = {250.f, 500.f, 750.f, 1000.f};
        for (int i = 0; i < 4; i++) {
            float x = worldX(marks[i]);
            if (i < 3) blit(art_.post[i], x - 20.f, GROUND_Y - 78.f, PAL_SIGN);
            else blit(art_.gantry, x - 10.f, GROUND_Y - 96.f, PAL_SIGN);
        }
    } else {
        for (int i = 0; i < 12; i++) image(art_.wire, float(i * 32 - 8), 77.f, PAL_HIGH);
    }
    blit(art_.shadow, bx + 16.f, GROUND_Y - 8.f + lift * 0.05f, PAL_DUST);
}

}  // namespace bike
