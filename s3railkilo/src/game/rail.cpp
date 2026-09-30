#include "game/rail.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace railkilo {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PPM = 14.f;
constexpr float RAIL_Y = 168.f;
constexpr float CAR_X = 86.f;
constexpr float HOP_V = 380.f;
constexpr float GRAV = 1020.f;
constexpr float MIN_SPD = 9.0f;
constexpr float MAX_SPD = 13.2f;
constexpr float CREW_SPD = 10.05f;
constexpr float T_APEX = HOP_V / GRAV;
constexpr float SCRAP_R = 13.f;
constexpr float HANG_R = 12.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layRail();
    won_ = false;
    over_ = false;
    meters_ = 0;
    t_ = 0;
    std::snprintf(result_, sizeof result_, "S3 RAILKILO  FAIL  unfinished");
    if (bot_) resetRun();
    else mode_ = Mode::Title;
    sys.apu.setMaster(0.75f);
}

void Game::layRail() {
    wheels_.clear();
    float z = 62.f;
    int n = 0;
    while (z < 955.f) {
        wheels_.push_back({z, (n % 2) == 0 ? Kind::Scrap : Kind::Hang});
        z += 31.f;
        if ((n % 4) == 3) z += 14.f;
        n++;
    }
}

void Game::resetRun() {
    mode_ = Mode::Run;
    odo_ = 0;
    crew_ = 0;
    speed_ = 11.0f;
    lift_ = 0;
    vy_ = 0;
    onRail_ = true;
    ducked_ = false;
    hopBuf_ = 0;
    raceT_ = 0;
    modeT_ = 0;
    hopSnd_ = 0;
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
        const Wheel* scrap = nullptr;
        const Wheel* hang = nullptr;
        float bestS = 1e9f, bestH = 1e9f;
        for (const Wheel& w : wheels_) {
            float dz = w.z - odo_;
            if (w.kind == Kind::Scrap) {
                if (dz < -0.5f || dz > 16.f) continue;
                if (dz < bestS) {
                    bestS = dz;
                    scrap = &w;
                }
            } else if (dz > -2.5f && dz < bestH && dz < 7.f) {
                bestH = dz;
                hang = &w;
            }
        }
        in.throttle = speed_ < 12.9f;
        if (hang) {
            float dz = hang->z - odo_;
            if (dz < 3.4f && dz > -3.8f) in.duck = true;
        }
        if (scrap && onRail_ && !in.duck) {
            float tHit = (scrap->z - odo_) / std::max(speed_, 0.1f);
            bool blocked = false;
            if (hang) {
                float th = (hang->z - odo_) / std::max(speed_, 0.1f);
                if (th > -0.04f && th < 0.8f) blocked = true;
            }
            if (!blocked && tHit <= T_APEX + 0.06f && tHit >= T_APEX - 0.05f) in.hop = true;
        }
        return in;
    }
    const gs::Pad& p = sys_->pad;
    in.throttle = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.down(gs::BTN_Z) || p.accel > 0.25f;
    in.brake = p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.25f;
    in.duck = p.down(gs::BTN_DOWN);
    in.hop = p.pressed(gs::BTN_UP) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    return in;
}

void Game::runLogic(float dt) {
    In in = controls();
    ducked_ = in.duck && onRail_;
    if (in.brake) speed_ -= 9.f * dt;
    else if (in.throttle) speed_ += 6.4f * dt;
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
        hopSnd_ = 0.11f;
        if (!sys_->headless) sys_->rumble(0.12f, 0.28f, 40);
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
    danger_ = false;
    const float feet = RAIL_Y - lift_;
    const float head = feet - (ducked_ ? 30.f : 48.f);
    const float left = CAR_X - 22.f;
    const float right = CAR_X + 28.f;
    for (const Wheel& w : wheels_) {
        float dz = w.z - odo_;
        if (dz < -3.5f || dz > 7.f) continue;
        if (dz > -0.2f && dz < 4.8f) danger_ = true;
        float sx = CAR_X + dz * PPM;
        float sy = w.kind == Kind::Scrap ? RAIL_Y - 17.f : RAIL_Y - 58.f;
        float rad = (w.kind == Kind::Scrap ? SCRAP_R : HANG_R) - 1.f;
        float nx = std::clamp(sx, left, right);
        float ny = std::clamp(sy, head, feet);
        float dx = sx - nx, dy = sy - ny;
        if (dx * dx + dy * dy < rad * rad) {
            finishFail(w.kind == Kind::Scrap ? "scrap wheel" : "hanging wheel");
            return;
        }
    }
}

void Game::finishWin() {
    mode_ = Mode::Win;
    won_ = true;
    modeT_ = 0;
    std::snprintf(result_, sizeof result_,
                  "S3 RAILKILO  WIN  kilometer on the rail ahead of the other crew  wheels untouched  (%.1f s)",
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
                  "S3 RAILKILO  FAIL  %s at %d m  crew %.0f m  (%.1f s)", why, meters_, crew_, raceT_);
    if (!sys_->headless) sys_->rumble(0.7f, 0.8f, 150);
    sys_->apu.noiseBurst(0.38f, 640.f, 0.24f);
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
            f0 = 55.f + speed_ * 6.f;
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
        if (mode_ == Mode::Win) sys_->setLight(40, 160, 70);
        else if (mode_ == Mode::Spill || mode_ == Mode::Fail) sys_->setLight(190, 40, 28);
        else if (danger_) sys_->setLight(200, 100, 30);
        else sys_->setLight(40, 60, 90);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    float pix = (mode_ == Mode::Title) ? t_ * 16.f : odo_ * PPM;
    int shake = shake_ ? (int(t_ * 60.f) & 1 ? shake_ : -shake_) : 0;
    v.A.scroll(-int(std::lround(pix)) + shake, 0);
    v.B.scroll(-int(std::lround(pix * 0.22f)), 0);
    v.A.enabled = true;
    v.B.enabled = true;
    bool flash = mode_ == Mode::Spill && modeT_ < 0.14f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y < 140 ? y / 140.f : 1.f;
        int r = int(2 + 6 * u);
        int g = int(3 + 4 * u);
        int b = int(6 + 2 * u);
        if (flash) r = std::min(15, r + 6);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(6, 6, 7));
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
        text(8, 20, "UP HOP    DOWN DUCK", PAL_HUD);
        text(8, 21, "A THROTTLE    B BRAKE", PAL_HUD);
        text(6, 22, "FINISH THE KILO UNTOUCHED", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) text(15, 25, "START", PAL_ALERT);
        return;
    }
    char line[48];
    int show = mode_ == Mode::Win ? 1000 : meters_;
    float gap = crew_ - odo_;
    std::snprintf(line, sizeof line, "%4d M  %4.1f S", show, raceT_);
    text(1, 0, line, PAL_HUD);
    std::snprintf(line, sizeof line, "CREW %s%3.0f M", gap >= 0.f ? "+" : "", gap);
    text(22, 0, line, gap > 0.f ? PAL_ALERT : PAL_HUD);
    std::string bar = "RAIL ";
    int filled = std::min(10, show / 100);
    for (int i = 0; i < 10; i++) bar += (i < filled) ? '#' : '-';
    text(1, 1, bar, danger_ && mode_ == Mode::Run ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Run) text(1, 26, "DO NOT TOUCH THE WHEELS", PAL_HUD);
    if (mode_ == Mode::Pause) text(16, 12, "PAUSED", PAL_ALERT);
    if (mode_ == Mode::Fail) {
        text(8, 12, "WHEEL OR CREW CLOCK", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) text(13, 14, "START RETRIES", PAL_HUD);
    }
    if (mode_ == Mode::Win) {
        text(4, 11, "KILOMETER  WHEELS CLEAR", PAL_HUD);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) text(13, 13, "START AGAIN", PAL_HUD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    backdrop();
    hud();

    auto worldX = [&](float z) { return CAR_X + (z - (mode_ == Mode::Title ? 0.f : odo_)) * PPM; };

    if (mode_ == Mode::Title) {
        image(art_.title, (gs::SCREEN_W - art_.title.w) * 0.5f, 26.f, PAL_TITLE);
        image(art_.sub, (gs::SCREEN_W - art_.sub.w) * 0.5f, 64.f, PAL_TITLE);
        int hand = int(t_ * 2.f) % 6;
        blit(art_.clock[hand], 148.f, 88.f, PAL_CLOCK);
        float gx = 24.f + std::fmod(t_ * 28.f, 260.f);
        blit(art_.scrap[int(t_ * 6.f) & 1], gx, RAIL_Y - 34.f, PAL_WHEEL);
        blit(art_.ride[int(t_ * 6.f) & 1], 36.f, RAIL_Y - float(art_.ride[0].h), PAL_SPEEDER);
    }

    if (mode_ != Mode::Title) {
        for (float post = 100.f; post <= 1000.f; post += 100.f) {
            float x = worldX(post);
            if (x < -24.f || x > gs::SCREEN_W + 8.f) continue;
            blit(art_.post, x, RAIL_Y - 64.f, PAL_YARD);
        }
        for (const Wheel& w : wheels_) {
            float x = worldX(w.z);
            if (x < -48.f || x > gs::SCREEN_W + 24.f) continue;
            if (w.kind == Kind::Scrap) {
                int frame = int(std::floor(odo_ * 2.f + w.z)) & 1;
                blit(art_.scrap[frame], x - 17.f, RAIL_Y - 34.f, PAL_WHEEL);
            } else {
                blit(art_.gantry, x - 18.f, RAIL_Y - 72.f - 22.f, PAL_GANTRY);
            }
        }
        float cx = worldX(crew_);
        if (cx > -40.f && cx < gs::SCREEN_W + 8.f)
            blit(art_.crew, cx - 20.f, RAIL_Y - 36.f, PAL_CREW);

        const gs::Mipped* car = &art_.ride[int(odo_ * 4.f) & 1];
        if (mode_ == Mode::Spill || mode_ == Mode::Fail) car = &art_.spill;
        else if (ducked_) car = &art_.duck;
        blit(*car, CAR_X - 36.f, RAIL_Y - lift_ - float(car->h), PAL_SPEEDER);
    }
}

}  // namespace railkilo
