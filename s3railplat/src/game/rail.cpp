#include "game/rail.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace railplat {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float MARK = 58.f;      // world metres of the platform stripe
constexpr float WINDOW = 0.55f;   // how close "level" is
constexpr float STOP_V = 0.18f;
constexpr float HOLD_NEED = 0.85f;
constexpr float CREW = 26.f;      // seconds before the berth is taken
constexpr float BUFFER = MARK + 3.4f;
constexpr float PPM = 7.2f;       // pixels per metre
constexpr int RAIL_Y = 168;
constexpr int COACH_FEET = 168;
constexpr int DOOR_SCREEN = 108;  // screen x of the coach door while rolling

gs::FMPatch chuff() {
    gs::FMPatch p;
    p.fb = 0.22f;
    p.op[0] = {1.f, 1.f, 0.02f, 0.12f, 0.4f, 0.08f};
    p.op[1] = {2.f, 0.5f, 0.01f, 0.1f, 0.3f, 0.05f};
    p.op[2] = {0.5f, 0.3f, 0.02f, 0.2f, 0.2f, 0.1f};
    p.op[3] = {3.f, 0.15f, 0.01f, 0.08f, 0.15f, 0.05f};
    p.vol = 0.16f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (held_) return 3;
    if (std::fabs(pos_ - MARK) < 4.5f) return 2;
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

void Game::whistle() {
    whistle_ = 0.45f;
    sys_->apu.tone(1, 740.f, 0.18f);
}

void Game::fail(const char* why) {
    std::snprintf(why_, sizeof why_, "%s", why);
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    engineOn_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.noiseBurst(0.35f, 1800.f, 0.25f);
}

void Game::winRun() {
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    engineOn_ = false;
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
    engineOn_ = true;
    for (Puff& p : puffs_) p.age = 0;
    sys_->apu.setPatch(0, chuff());
    sys_->apu.keyOn(0, 70.f, 0.2f);
}

void Game::botControls(float& thr, float& brk) const {
    float remain = MARK - pos_;
    float v = speed_;
    // Brake harder than the model so the door settles on the stripe, not past it.
    float decel = 8.4f;
    float stopDist = (v * v) / (2.f * decel);
    if (remain < 2.4f) {
        float want = std::max(0.f, remain) * 1.35f;
        if (v > want + 0.05f) brk = 1.f;
        else if (remain > 0.12f && v < want * 0.55f) thr = 0.45f;
        else if (v > 0.04f) brk = 0.55f;
        return;
    }
    if (stopDist > remain - 1.1f) {
        brk = 1.f;
        return;
    }
    if (v < 13.5f) thr = 1.f;
}

void Game::updateRoll(float dt) {
    const gs::Pad& pad = sys_->pad;
    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_C)) whistle();

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

    speed_ += thr * 5.6f * dt;
    speed_ -= brk * 9.2f * dt;
    speed_ -= speed_ * 0.22f * dt;
    if (speed_ < 0) speed_ = 0;
    if (speed_ > 16.f) speed_ = 16.f;
    pos_ += speed_ * dt;
    time_ += dt;

    if (brk > 0.5f && !wasBraking_) {
        blip_ = 0.08f;
        sys_->apu.noiseBurst(0.12f, 900.f, 0.05f);
    }
    wasBraking_ = brk > 0.5f;

    puffT_ += dt * (0.4f + speed_ * 0.15f);
    if (puffT_ > 0.18f && speed_ > 1.f) {
        puffT_ = 0;
        for (Puff& p : puffs_) {
            if (p.age <= 0) {
                p.age = 0.01f;
                p.x = pos_ * PPM;
                break;
            }
        }
    }
    for (Puff& p : puffs_) {
        if (p.age > 0) {
            p.age += dt;
            if (p.age > 1.1f) p.age = 0;
        }
    }

    float err = std::fabs(pos_ - MARK);
    bool quiet = speed_ < STOP_V;
    if (quiet && err <= WINDOW) {
        hold_ += dt;
        held_ = hold_ > 0.2f;
        if (hold_ >= HOLD_NEED) winRun();
    } else {
        hold_ = 0;
        held_ = false;
        if (quiet && time_ > 1.2f && err > WINDOW + 0.15f) {
            fail(pos_ < MARK ? "STOPPED SHORT" : "NOT LEVEL");
        } else if (pos_ >= BUFFER) {
            fail("ROLLED THE BUFFER");
        } else if (time_ >= CREW) {
            fail("BERTH TAKEN");
        }
    }
}

void Game::audio(float dt) {
    if (whistle_ > 0) {
        whistle_ -= dt;
        if (whistle_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (blip_ > 0) blip_ -= dt;
    if (engineOn_ && mode_ == Mode::Roll) {
        float f = 62.f + speed_ * 9.f + thr_ * 18.f;
        sys_->apu.setFreq(0, f);
        sys_->apu.setVol(0, 0.08f + speed_ * 0.012f + thr_ * 0.06f);
    } else if (!engineOn_) {
        sys_->apu.setVol(0, 0);
    }
    if (fanStep_ >= 0) {
        fanT_ += dt;
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        if (fanStep_ < 4 && fanT_ > 0.12f) {
            sys_->apu.tone(2, notes[fanStep_], 0.16f);
            fanStep_++;
            fanT_ = 0;
        } else if (fanStep_ >= 4 && fanT_ > 0.35f) {
            sys_->apu.tone(2, 0, 0);
            fanStep_ = -1;
        }
    }
}

void Game::sky() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H);
        int r = int(4 + (1.f - u) * 6);
        int g = int(6 + (1.f - u) * 5);
        int b = int(10 + (1.f - u) * 4);
        if (y > 150) {
            r = 4;
            g = 5;
            b = 4;
        }
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = y < 40 ? uint8_t(3) : 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::drawPoster() {
    blit(art_.logo, (gs::SCREEN_W - art_.logo.w) * 0.5f, 18, PAL_AMBER);
    blit(art_.tag, (gs::SCREEN_W - art_.tag.w) * 0.5f, 52, PAL_RED);
    int phase = int(t_ * 6.f) % 2;
    blit(art_.coach[phase], 18, float(COACH_FEET - art_.coach[0].h), PAL_COACH);
    blit(art_.plat, 150, float(RAIL_Y - art_.plat.h + 8), PAL_PLAT);
    blit(art_.lamp, 292, float(RAIL_Y - art_.lamp.h), PAL_LAMP);
    int seg = std::max(1, int(art_.rail.w));
    for (int x = -seg; x < gs::SCREEN_W + seg; x += seg) blit(art_.rail, float(x), float(RAIL_Y), PAL_LAND);
    blit(art_.hill, 20, 118, PAL_LAND, 0, 0, 6);
    blit(art_.tree, 250, 112, PAL_LAND, 0, 0, 2);
    blit(art_.cloud, std::fmod(t_ * 10.f, 360.f) - 40.f, 70, PAL_LAND);
    float age = std::fmod(t_, 0.8f);
    blit(art_.puff, 28.f, float(COACH_FEET - 70) - age * 20.f, PAL_FX, 8 + int(age * 10), 8 + int(age * 10),
         int(age * 8));
}

void Game::drawRun() {
    float cam = pos_ * PPM - DOOR_SCREEN;
    int phase = int(pos_ * 2.f) % 2;
    if (phase < 0) phase = 0;
    float coachX = pos_ * PPM - 73.f - cam;
    blit(art_.coach[phase], coachX, float(COACH_FEET - art_.coach[0].h), PAL_COACH);
    for (const Puff& p : puffs_) {
        if (p.age <= 0) continue;
        int sz = 7 + int(p.age * 12);
        blit(art_.puff, p.x - cam - 70.f, COACH_FEET - 58.f - p.age * 26.f, PAL_FX, sz, sz, int(p.age * 10));
    }
    float platX = MARK * PPM - 81.f - cam;
    blit(art_.plat, platX, float(RAIL_Y - art_.plat.h + 8), PAL_PLAT);
    blit(art_.buffer, BUFFER * PPM - cam, float(RAIL_Y - art_.buffer.h + 4), PAL_LAMP);
    blit(art_.lamp, platX + 150.f, float(RAIL_Y - art_.lamp.h), PAL_LAMP);

    int seg = std::max(1, int(art_.rail.w));
    int i0 = int(std::floor(cam / seg)) - 1;
    for (int i = i0; i < i0 + 16; i++) blit(art_.rail, float(i * seg) - cam, float(RAIL_Y), PAL_LAND);

    auto band = [&](int spacing, int y, const gs::Image& img, int fog, float parallax) {
        float c = cam * parallax;
        int a = int(std::floor(c / spacing)) - 2;
        for (int i = a; i < a + 10; i++) {
            float sx = float(i * spacing) - c;
            int bob = int((unsigned(i) * 17u) % 5u) - 2;
            blit(img, sx, float(y + bob), PAL_LAND, 0, 0, fog);
        }
    };
    band(170, 112, art_.tree, 2, 0.65f);
    band(240, 122, art_.hill, 7, 0.28f);
    band(300, 36, art_.cloud, 1, 0.1f);
}

void Game::drawBanner() {
    const gs::Image* img = nullptr;
    int pal = PAL_TEXT;
    if (mode_ == Mode::Won) {
        img = &art_.banLevel;
        pal = PAL_GREEN;
    } else if (mode_ == Mode::Lost) {
        if (std::strcmp(why_, "STOPPED SHORT") == 0) img = &art_.banShort;
        else if (std::strcmp(why_, "ROLLED THE BUFFER") == 0 || std::strcmp(why_, "NOT LEVEL") == 0)
            img = &art_.banPast;
        else img = &art_.banCrew;
        pal = PAL_RED;
    } else if (held_) {
        img = &art_.banLevel;
        pal = PAL_AMBER;
    }
    if (!img) return;
    blit(*img, (gs::SCREEN_W - img->w) * 0.5f, 58, pal);
}

void Game::hud() {
    sys_->vdp.HUD.clear();
    if (mode_ == Mode::Title) {
        hudCenter(10, "STOP THE DOOR ON THE STRIPE", PAL_TEXT);
        hudCenter(12, "Z THROTTLE    X BRAKE    C HORN", PAL_DIM);
        if (int(t_ * 2.f) % 2 == 0) hudCenter(22, "ENTER TO ROLL", PAL_AMBER);
        hudCenter(25, "THE CLOCK TAKES THE BERTH", PAL_RED);
        return;
    }
    char line[48];
    float left = std::max(0.f, CREW - time_);
    std::snprintf(line, sizeof line, "BERTH %4.1f", left);
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
        else if (time_ < 2.2f) hudCenter(4, "STOP LEVEL WITH THE PLATFORM", PAL_DIM);
    } else if (mode_ == Mode::Won) {
        std::snprintf(line, sizeof line, "LEVEL  %4.1f", time_);
        hudCenter(12, line, PAL_GREEN);
        if (!bot_) hudCenter(16, "ENTER FOR ANOTHER RUN", PAL_DIM);
    } else if (mode_ == Mode::Lost) {
        hudCenter(12, why_, PAL_RED);
        if (!bot_) hudCenter(16, "ENTER TO TRY AGAIN", PAL_DIM);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sky();
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
    sys.apu.setEcho(0.1f, 0.18f, 0.1f);
    engineOn_ = false;
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

}  // namespace railplat
