#include "game/metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace metroplat {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float MARK = 74.f;
constexpr float WINDOW = 0.5f;
constexpr float STOP_V = 0.16f;
constexpr float HOLD_NEED = 0.9f;
constexpr float SERVICE = 28.f;
constexpr float WALL = MARK + 4.6f;
constexpr float PPM = 6.4f;
constexpr int DECK_Y = 176;
constexpr int DOOR_SCREEN = 112;
constexpr int DOOR_LOCAL = 86;

gs::FMPatch motors() {
    gs::FMPatch p;
    p.fb = 0.18f;
    p.op[0] = {1.f, 0.8f, 0.04f, 0.2f, 0.35f, 0.1f};
    p.op[1] = {1.5f, 0.4f, 0.02f, 0.16f, 0.25f, 0.08f};
    p.op[2] = {0.5f, 0.25f, 0.03f, 0.22f, 0.2f, 0.12f};
    p.op[3] = {3.5f, 0.12f, 0.01f, 0.1f, 0.12f, 0.06f};
    p.vol = 0.14f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (held_) return 3;
    if (std::fabs(pos_ - MARK) < 5.f) return 2;
    if (mode_ == Mode::Roll || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::blit(const gs::Image& img, float x, float y, int pal, int w, int h, int fog) {
    if (img.w < 1 || img.h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(w > 0 ? w : img.w);
    s.h = int16_t(h > 0 ? h : img.h);
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    if (s.x > gs::SCREEN_W + 64 || s.y > gs::SCREEN_H + 24 || s.x + s.w < -64 || s.y + s.h < -32) return;
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c > 95) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudCenter(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hudText(20 - n / 2, row, s, pal);
}

void Game::bell() {
    bell_ = 0.35f;
    sys_->apu.tone(1, 880.f, 0.16f);
}

void Game::fail(const char* why) {
    std::snprintf(why_, sizeof why_, "%s", why);
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    motorsOn_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.noiseBurst(0.3f, 1400.f, 0.22f);
}

void Game::winRun() {
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    motorsOn_ = false;
    sys_->apu.keyOff(0);
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::startRun() {
    mode_ = Mode::Roll;
    over_ = false;
    won_ = false;
    held_ = false;
    pos_ = 0;
    speed_ = 0;
    time_ = 0;
    hold_ = 0;
    thr_ = 0;
    brk_ = 0;
    why_[0] = 0;
    fanStep_ = -1;
    wasBraking_ = false;
    motorsOn_ = true;
    for (Spark& p : sparks_) p.age = 0;
    sys_->apu.setPatch(0, motors());
    sys_->apu.keyOn(0, 55.f, 0.16f);
}

void Game::botControls(float& thr, float& brk) const {
    float remain = MARK - pos_;
    float v = speed_;
    float decel = 9.2f;
    float stopDist = (v * v) / (2.f * decel);
    if (remain < 2.2f) {
        float want = std::max(0.f, remain) * 1.25f;
        if (v > want + 0.04f) brk = 1.f;
        else if (remain > 0.1f && v < want * 0.5f) thr = 0.4f;
        else if (v > 0.03f) brk = 0.5f;
        return;
    }
    if (stopDist > remain - 1.15f) {
        brk = 1.f;
        return;
    }
    if (v < 14.f) thr = 1.f;
}

void Game::updateRoll(float dt) {
    const gs::Pad& pad = sys_->pad;
    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_C)) bell();

    float thr = 0, brk = 0;
    if (bot_) botControls(thr, brk);
    else {
        if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_A) || pad.down(gs::BTN_Z)) thr = 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X) ||
            pad.down(gs::BTN_TURBO))
            brk = 1.f;
    }
    if (thr > 0 && brk > 0) thr = 0;
    thr_ = thr;
    brk_ = brk;

    speed_ += thr * 6.1f * dt;
    speed_ -= brk * 10.2f * dt;
    speed_ -= speed_ * 0.26f * dt;
    if (speed_ < 0) speed_ = 0;
    if (speed_ > 17.f) speed_ = 17.f;
    pos_ += speed_ * dt;
    time_ += dt;

    if (brk > 0.5f && !wasBraking_) {
        blip_ = 0.07f;
        sys_->apu.noiseBurst(0.1f, 700.f, 0.04f);
    }
    wasBraking_ = brk > 0.5f;

    sparkT_ += dt * (0.5f + speed_ * 0.12f);
    if (sparkT_ > 0.14f && speed_ > 1.2f) {
        sparkT_ = 0;
        for (Spark& p : sparks_) {
            if (p.age <= 0) {
                p.age = 0.01f;
                p.x = pos_ * PPM;
                break;
            }
        }
    }
    for (Spark& p : sparks_) {
        if (p.age > 0) {
            p.age += dt;
            if (p.age > 0.7f) p.age = 0;
        }
    }

    float err = std::fabs(pos_ - MARK);
    bool quiet = speed_ < STOP_V;
    if (quiet && err <= WINDOW) {
        hold_ += dt;
        held_ = hold_ > 0.18f;
        if (hold_ >= HOLD_NEED) winRun();
    } else {
        hold_ = 0;
        held_ = false;
        if (quiet && time_ > 1.1f && err > WINDOW + 0.12f) {
            fail(pos_ < MARK ? "STOPPED SHORT" : "NOT LEVEL");
        } else if (pos_ >= WALL) {
            fail("HIT THE END WALL");
        } else if (time_ >= SERVICE) {
            fail("SERVICE OVER");
        }
    }
}

void Game::audio(float dt) {
    if (bell_ > 0) {
        bell_ -= dt;
        if (bell_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (blip_ > 0) blip_ -= dt;
    if (motorsOn_ && mode_ == Mode::Roll) {
        float f = 48.f + speed_ * 7.f + thr_ * 16.f;
        sys_->apu.setFreq(0, f);
        sys_->apu.setVol(0, 0.06f + speed_ * 0.01f + thr_ * 0.05f);
    } else if (!motorsOn_) {
        sys_->apu.setVol(0, 0);
    }
    if (fanStep_ >= 0) {
        fanT_ += dt;
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        if (fanStep_ < 4 && fanT_ > 0.11f) {
            sys_->apu.tone(2, notes[fanStep_], 0.14f);
            fanStep_++;
            fanT_ = 0;
        } else if (fanStep_ >= 4 && fanT_ > 0.32f) {
            sys_->apu.tone(2, 0, 0);
            fanStep_ = -1;
        }
    }
}

void Game::tunnel() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = 1, g = 1, b = 2;
        if (y < 28) {
            r = 2;
            g = 2;
            b = 3;
        } else if (y > 150) {
            r = 2;
            g = 2;
            b = 2;
        }
        if ((y % 18) == 8 && y < 140) {
            r = 6;
            g = 7;
            b = 5;
        }
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = y < 36 ? uint8_t(6) : uint8_t(1);
        sys_->vdp.road[y].on = false;
    }
}

void Game::drawPoster() {
    blit(art_.logo, (gs::SCREEN_W - art_.logo.w) * 0.5f, 14, PAL_AMBER);
    blit(art_.tag, (gs::SCREEN_W - art_.tag.w) * 0.5f, 50, PAL_TEXT);
    int phase = int(t_ * 5.f) % 2;
    blit(art_.car[phase], 8, float(DECK_Y - art_.car[0].h), PAL_CAR);
    blit(art_.plat, 148, float(DECK_Y - art_.plat.h + 14), PAL_PLAT);
    blit(art_.signal, 300, float(DECK_Y - art_.signal.h - 8), PAL_LAMP);
    int seg = std::max(1, int(art_.sleeper.w));
    for (int x = -seg; x < gs::SCREEN_W + seg; x += seg) blit(art_.sleeper, float(x), float(DECK_Y), PAL_TUNNEL);
    blit(art_.arch, 40, 78, PAL_TUNNEL, 0, 0, 4);
    blit(art_.clock, 270, 78, PAL_LAMP);
    float age = std::fmod(t_, 0.6f);
    blit(art_.spark, 18.f, float(DECK_Y - 8) - age * 6.f, PAL_FX, 6 + int(age * 8), 6 + int(age * 8), int(age * 8));
}

void Game::drawRun() {
    float cam = pos_ * PPM - DOOR_SCREEN;
    int phase = int(pos_ * 2.2f) % 2;
    if (phase < 0) phase = 0;
    float carX = pos_ * PPM - float(DOOR_LOCAL) - cam;
    blit(art_.car[phase], carX, float(DECK_Y - art_.car[0].h), PAL_CAR);
    for (const Spark& p : sparks_) {
        if (p.age <= 0) continue;
        int sz = 5 + int(p.age * 10);
        blit(art_.spark, p.x - cam - 40.f, DECK_Y - 6.f - p.age * 10.f, PAL_FX, sz, sz, int(p.age * 12));
    }
    float platX = MARK * PPM - 94.f - cam;
    blit(art_.plat, platX, float(DECK_Y - art_.plat.h + 14), PAL_PLAT);
    blit(art_.bench, platX + 18.f, float(DECK_Y - art_.plat.h + 28), PAL_PLAT);
    blit(art_.clock, platX + 150.f, float(DECK_Y - art_.plat.h + 6), PAL_LAMP);
    blit(art_.signal, WALL * PPM - cam, float(DECK_Y - art_.signal.h), PAL_LAMP);

    int seg = std::max(1, int(art_.sleeper.w));
    int i0 = int(std::floor(cam / seg)) - 1;
    for (int i = i0; i < i0 + 16; i++) blit(art_.sleeper, float(i * seg) - cam, float(DECK_Y), PAL_TUNNEL);

    float c = cam * 0.45f;
    int a = int(std::floor(c / 120.f)) - 2;
    for (int i = a; i < a + 8; i++) blit(art_.arch, float(i * 120) - c, 86, PAL_TUNNEL, 0, 0, 5);
}

void Game::drawBanner() {
    const gs::Image* img = nullptr;
    int pal = PAL_TEXT;
    if (mode_ == Mode::Won) {
        img = &art_.banLevel;
        pal = PAL_GREEN;
    } else if (mode_ == Mode::Lost) {
        if (std::strcmp(why_, "STOPPED SHORT") == 0) img = &art_.banShort;
        else if (std::strcmp(why_, "HIT THE END WALL") == 0 || std::strcmp(why_, "NOT LEVEL") == 0)
            img = &art_.banPast;
        else img = &art_.banCrew;
        pal = PAL_RED;
    } else if (held_) {
        img = &art_.banLevel;
        pal = PAL_AMBER;
    }
    if (!img) return;
    blit(*img, (gs::SCREEN_W - img->w) * 0.5f, 52, pal);
}

void Game::hud() {
    sys_->vdp.HUD.clear();
    if (mode_ == Mode::Title) {
        hudCenter(10, "PUT THE DOORS ON THE YELLOW LINE", PAL_TEXT);
        hudCenter(12, "Z POWER    X BRAKE    C BELL", PAL_DIM);
        if (int(t_ * 2.f) % 2 == 0) hudCenter(22, "ENTER TO LEAVE THE TUNNEL", PAL_AMBER);
        hudCenter(25, "THE SERVICE CLOCK CLOSES THE STOP", PAL_RED);
        return;
    }
    char line[48];
    float left = std::max(0.f, SERVICE - time_);
    std::snprintf(line, sizeof line, "CLOCK %4.1f", left);
    int timePal = left < 4.f ? PAL_RED : left < 8.f ? PAL_AMBER : PAL_TEXT;
    hudText(1, 0, "PLATFORM", PAL_DIM);
    hudText(26, 0, line, timePal);
    if (mode_ == Mode::Roll || mode_ == Mode::Pause) {
        float gap = MARK - pos_;
        if (std::fabs(gap) <= WINDOW) std::snprintf(line, sizeof line, "LEVEL");
        else if (gap > 0) std::snprintf(line, sizeof line, "SHORT %4.1f", gap);
        else std::snprintf(line, sizeof line, "PAST  %4.1f", -gap);
        hudText(1, 1, line, std::fabs(gap) <= WINDOW ? PAL_GREEN : PAL_AMBER);
        std::snprintf(line, sizeof line, "SPD %4.1f", speed_);
        hudText(16, 1, line, PAL_TEXT);
        if (mode_ == Mode::Pause) hudCenter(6, "PAUSED", PAL_AMBER);
        else if (time_ < 2.4f) hudCenter(4, "STOP LEVEL WITH THE PLATFORM", PAL_DIM);
    } else if (mode_ == Mode::Won) {
        std::snprintf(line, sizeof line, "LEVEL  %4.1f", time_);
        hudCenter(12, line, PAL_GREEN);
        if (!bot_) hudCenter(16, "ENTER FOR ANOTHER STOP", PAL_DIM);
    } else if (mode_ == Mode::Lost) {
        hudCenter(12, why_, PAL_RED);
        if (!bot_) hudCenter(16, "ENTER TO TRY AGAIN", PAL_DIM);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    tunnel();
    if (mode_ == Mode::Title) drawPoster();
    else {
        drawBanner();
        drawRun();
    }
    hud();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.resize(64, 32);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    motorsOn_ = false;
    if (bot_) startRun();
    else mode_ = Mode::Title;
    audio(DT);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    audio(DT);
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Roll;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && pad.pressed(gs::BTN_START)) startRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else updateRoll(DT);
    draw();
}

}  // namespace metroplat
