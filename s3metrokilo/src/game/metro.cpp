#include "game/metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace metro {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PPM = 16.f;
constexpr float RAIL_Y = 186.f;
constexpr float CAB_X = 96.f;
constexpr float HOP_V = 440.f;
constexpr float GRAV = 1140.f;
constexpr float MIN_SPD = 8.8f;
constexpr float MAX_SPD = 15.2f;
constexpr float CREW_SPD = 10.35f;
constexpr float T_APEX = HOP_V / GRAV;
constexpr float RAIL_R = 14.f;
constexpr float CAT_R = 11.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layTrack();
    won_ = false;
    over_ = false;
    meters_ = 0;
    t_ = 0;
    std::snprintf(result_, sizeof result_, "S3 METROKILO  FAIL  unfinished");
    if (bot_) resetRun();
    else mode_ = Mode::Title;
    sys.apu.setMaster(0.72f);
}

void Game::layTrack() {
    wheels_.clear();
    float z = 42.f;
    int n = 0;
    while (z < 980.f) {
        wheels_.push_back({z, (n % 2) == 0 ? Kind::Rail : Kind::Catenary});
        z += 28.f;
        if ((n % 4) == 3) z += 10.f;
        n++;
    }
}

void Game::resetRun() {
    mode_ = Mode::Run;
    odo_ = 0;
    crew_ = 0;
    speed_ = 11.4f;
    lift_ = 0;
    vy_ = 0;
    onRail_ = true;
    ducked_ = false;
    hopBuf_ = 0;
    raceT_ = 0;
    modeT_ = 0;
    hopSnd_ = 0;
    shake_ = 0;
    close_ = false;
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
        const Wheel* rail = nullptr;
        const Wheel* cat = nullptr;
        float bestR = 1e9f, bestC = 1e9f;
        for (const Wheel& w : wheels_) {
            float dz = w.z - odo_;
            if (w.kind == Kind::Rail) {
                if (dz < -0.8f || dz > 20.f) continue;
                if (dz < bestR) {
                    bestR = dz;
                    rail = &w;
                }
            } else if (dz > -3.f && dz < bestC && dz < 9.f) {
                bestC = dz;
                cat = &w;
            }
        }
        in.power = speed_ < 14.6f;
        in.brake = false;
        if (cat) {
            float dz = cat->z - odo_;
            if (dz < 3.2f && dz > -2.4f) in.duck = true;
        }
        if (rail && onRail_ && !in.duck) {
            float tHit = (rail->z - odo_) / std::max(speed_, 0.1f);
            bool blocked = false;
            if (cat) {
                float th = (cat->z - odo_) / std::max(speed_, 0.1f);
                if (th > -0.05f && th < 0.9f) blocked = true;
            }
            if (!blocked && tHit <= T_APEX + 0.08f && tHit >= T_APEX - 0.06f) in.hop = true;
        }
        return in;
    }
    const gs::Pad& p = sys_->pad;
    in.power = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.down(gs::BTN_Z) || p.accel > 0.25f;
    in.brake = p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.25f;
    in.duck = p.down(gs::BTN_DOWN);
    in.hop = p.pressed(gs::BTN_UP) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    return in;
}

void Game::runLogic(float dt) {
    In in = controls();
    ducked_ = in.duck && onRail_;
    if (in.brake) speed_ -= 9.5f * dt;
    else if (in.power) speed_ += 7.2f * dt;
    else speed_ -= 1.8f * dt;
    speed_ = std::clamp(speed_, MIN_SPD, MAX_SPD);
    odo_ += speed_ * dt;
    crew_ += CREW_SPD * dt;
    raceT_ += dt;

    if (in.hop) hopBuf_ = 8;
    if (hopBuf_ > 0 && onRail_) {
        onRail_ = false;
        ducked_ = false;
        vy_ = HOP_V;
        hopBuf_ = 0;
        hopSnd_ = 0.14f;
        if (!sys_->headless) sys_->rumble(0.12f, 0.28f, 45);
    } else if (hopBuf_ > 0)
        hopBuf_--;

    if (!onRail_) {
        vy_ -= GRAV * dt;
        lift_ += vy_ * dt;
        if (lift_ <= 0.f) {
            lift_ = 0.f;
            vy_ = 0.f;
            onRail_ = true;
            shake_ = std::max(shake_, 2);
        }
    }

    meters_ = std::min(1000, int(odo_));
    if (odo_ >= 1000.f) {
        odo_ = 1000.f;
        meters_ = 1000;
        if (crew_ >= 1000.f) finishFail("crew clock");
        else finishWin();
        return;
    }
    if (crew_ >= 1000.f) {
        crew_ = 1000.f;
        finishFail("crew clock");
        return;
    }
    collide();
}

void Game::collide() {
    close_ = false;
    const float feet = RAIL_Y - lift_;
    const float head = feet - (ducked_ ? 22.f : 40.f);
    const float left = CAB_X - 18.f;
    const float right = CAB_X + 28.f;
    for (const Wheel& w : wheels_) {
        float dz = w.z - odo_;
        if (dz < -5.f || dz > 9.f) continue;
        if (dz > 0.f && dz < 6.f) close_ = true;
        float sx = CAB_X + dz * PPM;
        float sy = w.kind == Kind::Rail ? RAIL_Y - 14.f : RAIL_Y - 78.f;
        float rad = (w.kind == Kind::Rail ? RAIL_R : CAT_R) - 1.f;
        float nx = std::clamp(sx, left, right);
        float ny = std::clamp(sy, head, feet);
        float dx = sx - nx, dy = sy - ny;
        if (dx * dx + dy * dy < rad * rad) {
            finishFail(w.kind == Kind::Rail ? "rail wheel" : "catenary wheel");
            return;
        }
    }
}

void Game::finishWin() {
    mode_ = Mode::Win;
    won_ = true;
    modeT_ = 0;
    std::snprintf(result_, sizeof result_,
                  "S3 METROKILO  WIN  kilometer ahead of the crew clock  1000 m  wheels untouched  (%.1f s)",
                  raceT_);
}

void Game::finishFail(const char* why) {
    mode_ = Mode::Spill;
    won_ = false;
    modeT_ = 0;
    shake_ = 8;
    onRail_ = true;
    lift_ = 0;
    vy_ = 0;
    meters_ = std::min(999, int(odo_));
    std::snprintf(result_, sizeof result_,
                  "S3 METROKILO  FAIL  %s at %d m  crew %.0f m  (%.1f s)", why, meters_, crew_, raceT_);
    if (!sys_->headless) sys_->rumble(0.7f, 0.85f, 160);
    sys_->apu.noiseBurst(0.38f, 640.f, 0.28f);
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
        if (modeT_ > 0.6f) {
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
        float phase = std::fmod(odo_ * 2.1f, 1.f);
        if (phase < 0.08f) {
            v0 = 0.04f;
            f0 = 55.f + speed_ * 4.f;
        }
        if (hopSnd_ > 0.f) {
            v1 = 0.055f;
            f1 = 180.f + hopSnd_ * 900.f;
            hopSnd_ -= DT;
        }
    } else if (mode_ == Mode::Win) {
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        int step = int(modeT_ / 0.14f);
        if (step >= 0 && step < 4) {
            v1 = 0.06f;
            f1 = notes[step];
        }
    } else if (mode_ == Mode::Title && std::fmod(t_, 1.8f) < 0.05f) {
        v1 = 0.028f;
        f1 = 294.f;
    }
    sys_->apu.tone(0, f0, v0);
    sys_->apu.tone(1, f1, v1);
    sys_->apu.tone(2, 0, 0);
    if (!sys_->headless) {
        if (mode_ == Mode::Win) sys_->setLight(40, 160, 90);
        else if (mode_ == Mode::Spill || mode_ == Mode::Fail) sys_->setLight(190, 36, 28);
        else if (close_) sys_->setLight(200, 120, 30);
        else sys_->setLight(40, 70, 130);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    float pix = (mode_ == Mode::Title) ? t_ * 22.f : odo_ * PPM;
    int shake = shake_ ? (int(t_ * 60.f) & 1 ? shake_ : -shake_) : 0;
    v.A.scroll(-int(std::lround(pix)) + shake, 0);
    v.B.scroll(-int(std::lround(pix * 0.35f)), 0);
    v.A.enabled = true;
    v.B.enabled = true;
    bool flash = mode_ == Mode::Spill && modeT_ < 0.14f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H);
        int r = 1 + int(2 * (1.f - u));
        int g = 2 + int(u * 2);
        int b = 4 + int((1.f - u) * 3);
        if (flash) r = std::min(15, r + 6);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(2, 3, 5));
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
    v.hudEnabled = true;
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
        text(8, 18, "UP HOP    DOWN DUCK", PAL_HUD);
        text(8, 19, "A POWER   B BRAKE", PAL_HUD);
        text(6, 21, "FINISH THE KILOMETER", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) text(12, 24, "START THE METRO", PAL_ALERT);
        return;
    }
    char line[48];
    int show = mode_ == Mode::Win ? 1000 : meters_;
    float gap = crew_ - odo_;
    std::snprintf(line, sizeof line, "%4d M  %4.1f S", show, raceT_);
    text(1, 0, line, PAL_HUD);
    std::snprintf(line, sizeof line, "CREW %s%3.0f M", gap >= 0.f ? "+" : "", gap);
    text(22, 0, line, gap > 0.f ? PAL_ALERT : PAL_HUD);
    std::string bar = "KILO ";
    int filled = std::min(10, show / 100);
    for (int i = 0; i < 10; i++) bar += (i < filled) ? '#' : '-';
    text(1, 1, bar, close_ && mode_ == Mode::Run ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Run) text(1, 26, "DON'T TOUCH WHEELS", PAL_HUD);
    if (mode_ == Mode::Pause) text(16, 12, "PAUSED", PAL_ALERT);
    if (mode_ == Mode::Fail) {
        text(7, 12, "WHEEL OR CREW CLOCK", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) text(13, 14, "START RETRIES", PAL_HUD);
    }
    if (mode_ == Mode::Win) {
        text(4, 11, "KILOMETER AHEAD OF CREW", PAL_HUD);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) text(13, 13, "START AGAIN", PAL_HUD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    backdrop();
    hud();

    auto worldX = [&](float z) { return CAB_X + (z - (mode_ == Mode::Title ? 0.f : odo_)) * PPM; };

    if (mode_ == Mode::Title) {
        image(art_.title, (gs::SCREEN_W - art_.title.w) * 0.5f, 36.f, PAL_TITLE);
        image(art_.sub, (gs::SCREEN_W - art_.sub.w) * 0.5f, 78.f, PAL_TITLE);
        int hand = int(t_ * 2.f) & 7;
        blit(art_.clock[hand], 146.f, 100.f, PAL_CLOCK);
        float gx = 24.f + std::fmod(t_ * 40.f, 260.f);
        blit(art_.wheel[int(t_ * 8.f) & 1], gx, RAIL_Y - 36.f, PAL_WHEEL);
    }

    if (mode_ != Mode::Title) {
        for (const Wheel& w : wheels_) {
            float x = worldX(w.z);
            if (x < -48.f || x > gs::SCREEN_W + 36.f) continue;
            if (w.kind == Kind::Rail) {
                int frame = int(std::floor(odo_ * 2.5f + w.z)) & 1;
                blit(art_.wheel[frame], x - 18.f, RAIL_Y - 32.f, PAL_WHEEL);
            } else {
                blit(art_.hang, x - 16.f, RAIL_Y - 78.f - 26.f, PAL_HANG);
            }
        }
        float cx = worldX(crew_);
        if (cx > -40.f && cx < gs::SCREEN_W + 20.f)
            blit(art_.crew, cx - 24.f, RAIL_Y - 52.f, PAL_CREW);
        float station = worldX(1000.f);
        if (station > -8.f && station < gs::SCREEN_W + 8.f)
            blit(art_.lamp, station - 8.f, 48.f, PAL_CLOCK);
        int hand = int(std::fmod(raceT_ * 0.7f, 8.f));
        if (hand < 0) hand += 8;
        blit(art_.clock[hand], 286.f, 18.f, PAL_CLOCK);
    }

    float feet = RAIL_Y - ((mode_ == Mode::Title) ? 0.f : lift_);
    if (mode_ == Mode::Spill || mode_ == Mode::Fail) {
        blit(art_.spill, CAB_X - 36.f, feet - 36.f, PAL_CAR);
    } else if (ducked_ && mode_ == Mode::Run) {
        blit(art_.duck, CAB_X - art_.duck.w * 0.45f, feet - float(art_.duck.h), PAL_CAR);
    } else {
        int frame = int(std::floor(mode_ == Mode::Title ? t_ * 4.f : odo_ * 1.6f)) & 1;
        const gs::Mipped& car = art_.car[frame];
        float bob = (mode_ == Mode::Title) ? std::sin(t_ * 2.2f) * 2.f : 0.f;
        blit(car, CAB_X - car.w * 0.42f, feet - float(car.h) + bob, PAL_CAR);
    }
}

}  // namespace metro
