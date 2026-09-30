#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace kilo {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PPM = 16.f;
constexpr float GROUND_Y = 184.f;
constexpr float BUS_X = 92.f;
constexpr float JUMP_V = 430.f;
constexpr float GRAV = 1120.f;
constexpr float MIN_SPD = 8.5f;
constexpr float MAX_SPD = 14.6f;
constexpr float CREW_SPD = 10.55f;
constexpr float T_APEX = JUMP_V / GRAV;
constexpr float GROUND_R = 15.f;
constexpr float HANG_R = 12.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    won_ = false;
    over_ = false;
    meters_ = 0;
    t_ = 0;
    std::snprintf(result_, sizeof result_, "S3 BUSKILO  FAIL  missed the end  the leg");
    if (bot_) resetRun();
    else mode_ = Mode::Title;
    sys.apu.setMaster(0.75f);
}

void Game::buildCourse() {
    wheels_.clear();
    float z = 48.f;
    int n = 0;
    while (z < 960.f) {
        if ((n % 2) == 0) wheels_.push_back({z, Kind::Ground});
        else wheels_.push_back({z, Kind::Hang});
        z += 26.f;
        if ((n % 5) == 4) z += 8.f;
        n++;
    }
}

void Game::resetRun() {
    mode_ = Mode::Run;
    odo_ = 0;
    crew_ = 0;
    speed_ = 11.2f;
    lift_ = 0;
    vy_ = 0;
    grounded_ = true;
    duck_ = false;
    hopBuf_ = 0;
    raceT_ = 0;
    modeT_ = 0;
    jumpSnd_ = 0;
    shake_ = 0;
    danger_ = false;
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
        const Wheel* hang = nullptr;
        float bestG = 1e9f, bestH = 1e9f;
        for (const Wheel& w : wheels_) {
            float dz = w.z - odo_;
            if (w.kind == Kind::Ground) {
                if (dz < -0.6f || dz > 18.f) continue;
                if (dz < bestG) {
                    bestG = dz;
                    ground = &w;
                }
            } else if (dz > -3.f && dz < bestH && dz < 8.f) {
                bestH = dz;
                hang = &w;
            }
        }
        in.pedal = speed_ < 14.2f;
        in.brake = false;
        if (hang) {
            float dz = hang->z - odo_;
            if (dz < 3.4f && dz > -2.6f) in.duck = true;
        }
        if (ground && grounded_ && !in.duck) {
            float tHit = (ground->z - odo_) / std::max(speed_, 0.1f);
            bool blocked = false;
            if (hang) {
                float th = (hang->z - odo_) / std::max(speed_, 0.1f);
                if (th > -0.05f && th < 0.85f) blocked = true;
            }
            if (!blocked && tHit <= T_APEX + 0.07f && tHit >= T_APEX - 0.05f) in.hop = true;
        }
        return in;
    }
    const gs::Pad& p = sys_->pad;
    in.pedal = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.down(gs::BTN_Z) || p.accel > 0.25f;
    in.brake = p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.25f;
    in.duck = p.down(gs::BTN_DOWN);
    in.hop = p.pressed(gs::BTN_UP) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    return in;
}

void Game::runLogic(float dt) {
    In in = controls();
    duck_ = in.duck && grounded_;
    if (in.brake) speed_ -= 10.f * dt;
    else if (in.pedal) speed_ += 7.5f * dt;
    else speed_ -= 2.2f * dt;
    speed_ = std::clamp(speed_, MIN_SPD, MAX_SPD);
    odo_ += speed_ * dt;
    crew_ += CREW_SPD * dt;
    raceT_ += dt;

    if (in.hop) hopBuf_ = 8;
    if (hopBuf_ > 0 && grounded_) {
        grounded_ = false;
        duck_ = false;
        vy_ = JUMP_V;
        hopBuf_ = 0;
        jumpSnd_ = 0.12f;
        if (!sys_->headless) sys_->rumble(0.1f, 0.3f, 40);
    } else if (hopBuf_ > 0)
        hopBuf_--;

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
        if (crew_ >= 1000.f) finishFail("missed the end");
        else finishWin();
        return;
    }
    if (crew_ >= 1000.f) {
        crew_ = 1000.f;
        finishFail("missed the end");
        return;
    }
    collide();
}

void Game::collide() {
    danger_ = false;
    const float feet = GROUND_Y - lift_;
    const float head = feet - (duck_ ? 38.f : 68.f);
    const float left = BUS_X - 16.f;
    const float right = BUS_X + 22.f;
    for (const Wheel& w : wheels_) {
        float dz = w.z - odo_;
        if (dz < -4.f || dz > 8.f) continue;
        if (dz > 0.f && dz < 5.5f) danger_ = true;
        float sx = BUS_X + dz * PPM;
        float sy = w.kind == Kind::Ground ? GROUND_Y - 16.f : GROUND_Y - 64.f;
        float rad = (w.kind == Kind::Ground ? GROUND_R : HANG_R) - 1.f;
        float nx = std::clamp(sx, left, right);
        float ny = std::clamp(sy, head, feet);
        if ((sx - nx) * (sx - nx) + (sy - ny) * (sy - ny) < rad * rad) {
            finishFail(w.kind == Kind::Ground ? "ground wheel" : "hanging wheel");
            return;
        }
    }
}

void Game::finishWin() {
    mode_ = Mode::Win;
    won_ = true;
    modeT_ = 0;
    std::snprintf(result_, sizeof result_,
                  "S3 BUSKILO  WIN  kilometer clear of wheels  leg finished  1000 m  (%.1f s)", raceT_);
}

void Game::finishFail(const char* why) {
    mode_ = Mode::Spill;
    won_ = false;
    modeT_ = 0;
    shake_ = 8;
    grounded_ = true;
    lift_ = 0;
    vy_ = 0;
    meters_ = std::min(999, int(odo_));
    std::snprintf(result_, sizeof result_,
                  "S3 BUSKILO  FAIL  %s  the leg  %d m  crew %.0f m  (%.1f s)", why, meters_, crew_, raceT_);
    if (!sys_->headless) sys_->rumble(0.75f, 0.85f, 160);
    sys_->apu.noiseBurst(0.4f, 780.f, 0.26f);
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
            mode_ = Mode::Pause;
            break;
        }
        runLogic(DT);
        break;
    case Mode::Pause:
        if (modeEdge || go()) mode_ = Mode::Run;
        break;
    case Mode::Spill:
        modeT_ += DT;
        shake_ = std::max(0, shake_ - 1);
        if (modeT_ > 0.65f) {
            mode_ = Mode::Fail;
            modeT_ = 0;
        }
        break;
    case Mode::Fail:
    case Mode::Win:
        modeT_ += DT;
        if (!bot_ && go()) resetRun();
        break;
    }
    draw();
    mix();
    if (mode_ == Mode::Win || mode_ == Mode::Fail) over_ = true;
}

void Game::mix() {
    float f0 = 0, v0 = 0, f1 = 0, v1 = 0;
    if (mode_ == Mode::Run) {
        float phase = std::fmod(odo_ * 1.7f, 1.f);
        if (phase < 0.07f) {
            v0 = 0.04f;
            f0 = 55.f + speed_ * 4.f;
        }
        if (jumpSnd_ > 0.f) {
            v1 = 0.06f;
            f1 = 160.f + jumpSnd_ * 900.f;
            jumpSnd_ -= DT;
        }
    } else if (mode_ == Mode::Win) {
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        int step = int(modeT_ / 0.13f);
        if (step >= 0 && step < 4) {
            v1 = 0.06f;
            f1 = notes[step];
        }
    } else if (mode_ == Mode::Title && std::fmod(t_, 1.6f) < 0.06f) {
        v1 = 0.03f;
        f1 = 262.f;
    }
    sys_->apu.tone(0, f0, v0);
    sys_->apu.tone(1, f1, v1);
    sys_->apu.tone(2, 0, 0);
    if (!sys_->headless) {
        if (mode_ == Mode::Win) sys_->setLight(40, 170, 80);
        else if (mode_ == Mode::Spill || mode_ == Mode::Fail) sys_->setLight(200, 40, 30);
        else if (danger_) sys_->setLight(210, 110, 30);
        else sys_->setLight(40, 90, 160);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    float pix = (mode_ == Mode::Title) ? t_ * 18.f : odo_ * PPM;
    int shake = shake_ ? (int(t_ * 60.f) & 1 ? shake_ : -shake_) : 0;
    v.A.scroll(-int(std::lround(pix)) + shake, 0);
    v.B.scroll(-int(std::lround(pix * 0.28f)), 0);
    v.A.enabled = true;
    v.B.enabled = true;
    bool flash = mode_ == Mode::Spill && modeT_ < 0.16f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y < 150 ? y / 150.f : 1.f;
        int r = int(4 + (12 - 4) * u);
        int g = int(5 + (9 - 5) * u);
        int b = int(8 + (6 - 8) * u);
        if (flash) r = std::min(15, r + 5);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(10, 8, 6));
}

void Game::blit(const gs::Mipped& m, float x, float y, int pal, bool flip) {
    if (m.w < 1 || m.h < 1) return;
    if (x < -140.f || y < -140.f || x > gs::SCREEN_W + 40.f || y > gs::SCREEN_H + 40.f) return;
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
            int tile = art_.font[c - 32];
            if (!tile) continue;
            v.HUD.set(x, row, gs::entry(tile, pal));
        }
    };
    if (mode_ == Mode::Title) {
        text(8, 20, "UP HOP   DOWN DUCK THE BUS", PAL_HUD);
        text(10, 21, "A THROTTLE   B BRAKE", PAL_HUD);
        text(7, 22, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) text(15, 24, "START", PAL_ALERT);
        return;
    }
    char line[48];
    int show = mode_ == Mode::Win ? 1000 : meters_;
    float gap = crew_ - odo_;
    std::snprintf(line, sizeof line, "%4d M  %4.1f S", show, raceT_);
    text(1, 0, line, PAL_HUD);
    std::snprintf(line, sizeof line, "CREW %s%3.0f M", gap >= 0.f ? "+" : "", gap);
    text(22, 0, line, gap > 0.f ? PAL_ALERT : PAL_HUD);
    std::string bar = "LEG  ";
    int filled = std::min(10, show / 100);
    for (int i = 0; i < 10; i++) bar += (i < filled) ? '#' : '-';
    text(1, 1, bar, danger_ && mode_ == Mode::Run ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Run) text(1, 26, "DON'T TOUCH WHEELS", PAL_HUD);
    if (mode_ == Mode::Pause) text(16, 12, "PAUSED", PAL_ALERT);
    if (mode_ == Mode::Fail) {
        text(8, 12, "THE LEG FAILS", PAL_ALERT);
        text(6, 13, "WHEEL OR MISSED END", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) text(13, 15, "START RETRIES", PAL_HUD);
    }
    if (mode_ == Mode::Win) {
        text(4, 11, "KILOMETER  LEG FINISHED", PAL_HUD);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) text(13, 13, "START DRIVES", PAL_HUD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    backdrop();
    hud();

    auto worldX = [&](float z) { return BUS_X + (z - (mode_ == Mode::Title ? 0.f : odo_)) * PPM; };

    if (mode_ == Mode::Title) {
        image(art_.title, (gs::SCREEN_W - art_.title.w) * 0.5f, 24.f, PAL_TITLE);
        image(art_.sub, (gs::SCREEN_W - art_.sub.w) * 0.5f, 64.f, PAL_TITLE);
        int hand = int(t_ * 3.f) & 7;
        blit(art_.clock[hand], 146.f, 88.f, PAL_CLOCK);
        float gx = 40.f + std::fmod(t_ * 36.f, 280.f);
        blit(art_.wheel[int(t_ * 8.f) & 1], gx, GROUND_Y - 34.f, PAL_WHEEL);
    }

    if (mode_ != Mode::Title) {
        for (float post = 80.f; post < 1000.f; post += 120.f) {
            float x = worldX(post);
            if (x < -30.f || x > gs::SCREEN_W + 10.f) continue;
            blit(art_.post, x, GROUND_Y - 88.f, PAL_CITY);
        }
        for (const Wheel& w : wheels_) {
            float x = worldX(w.z);
            if (x < -40.f || x > gs::SCREEN_W + 30.f) continue;
            if (w.kind == Kind::Ground) {
                int frame = int(std::floor(odo_ * 3.f + w.z)) & 1;
                blit(art_.wheel[frame], x - 18.f, GROUND_Y - 16.f - 18.f, PAL_WHEEL);
            } else {
                blit(art_.hang, x - 17.f, GROUND_Y - 64.f - 24.f, PAL_HANG);
            }
        }
        float cx = worldX(crew_);
        if (cx > -40.f && cx < gs::SCREEN_W + 10.f) blit(art_.crew, cx - 24.f, GROUND_Y - 36.f, PAL_CREW);
        int hand = int(std::fmod(raceT_ * 0.8f, 8.f));
        if (hand < 0) hand += 8;
        blit(art_.clock[hand], 286.f, 6.f, PAL_CLOCK);
    }

    float feet = GROUND_Y - ((mode_ == Mode::Title) ? 0.f : lift_);
    if (mode_ == Mode::Spill || mode_ == Mode::Fail) {
        blit(art_.spill, BUS_X - 40.f, feet - 36.f, PAL_BUS);
    } else if (duck_ && mode_ == Mode::Run) {
        blit(art_.duck, BUS_X - 36.f, feet - float(art_.duck.h), PAL_BUS);
    } else {
        int frame = int(std::floor((mode_ == Mode::Title ? t_ * 6.f : odo_ * 2.4f))) & 1;
        const gs::Mipped& bus = art_.ride[frame];
        float bob = (mode_ == Mode::Title) ? std::sin(t_ * 2.f) * 2.f : 0.f;
        blit(bus, BUS_X - bus.w * 0.38f, feet - float(bus.h) + bob, PAL_BUS, false);
    }
}

}  // namespace kilo
