#include "game/lot.h"

#include <cstdio>
#include <cmath>

namespace lot {
namespace {
constexpr float GROUND = 168.f;
constexpr float WORLD = 1680.f;
constexpr float GOAL = 1480.f;
constexpr float GRAV = 0.34f;
constexpr float JUMP = -9.4f;
constexpr int LIMIT = 75;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.road[y].on = false;
        int band = y < 90 ? 0x224 : (y < 140 ? 0x335 : 0x446);
        sys.vdp.lineBackdrop[y] = gs::rgb4((band >> 8) & 15, (band >> 4) & 15, band & 15);
        sys.vdp.lineFog[y] = 0;
    }
    cars_[0] = {360.f, 2.25f, 70.f, 0};
    cars_[1] = {820.f, -2.45f, 74.f, 1};
    cars_[2] = {1240.f, 2.1f, 68.f, 0};
    cars_[3] = {80.f, -2.35f, 72.f, 1};
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    held_ = false;
    lives_ = 3;
    px_ = 56.f;
    py_ = GROUND;
    vx_ = vy_ = 0;
    pouchX_ = 210.f;
    onGround_ = true;
    inv_ = 0;
    clock_ = 0;
    face_ = 1;
    cam_ = 0;
}

void Game::finish(bool crossed) {
    mode_ = crossed ? Mode::Won : Mode::Lost;
    won_ = crossed;
    over_ = true;
    vx_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Play) return 1;
    if (mode_ == Mode::Won) return 2;
    return 3;
}

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (std::fabs(vx_) < 0.4f) return art_.stand;
    return (runTick_ / 8) & 1 ? art_.runA : art_.runB;
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = right = jump = false;
    float target = held_ ? GOAL + 24.f : pouchX_;
    bool threat = false;
    if (onGround_) {
        for (const Car& c : cars_) {
            for (int f = 0; f < 16; f++) {
                float x = c.x + c.speed * float(f);
                float cl = x - c.w * 0.5f;
                float cr = x + c.w * 0.5f;
                if (px_ + 14.f > cl && px_ - 14.f < cr) {
                    threat = true;
                    break;
                }
            }
            if (threat) break;
        }
    }
    if (threat) {
        jump = true;
        return;
    }
    if (px_ < target - 6.f) right = true;
    else if (px_ > target + 6.f) left = true;
}

void Game::stepPlay(bool left, bool right, bool jump) {
    clock_ += 1.f / 60.f;
    if (clock_ >= LIMIT) {
        finish(false);
        return;
    }
    if (inv_ > 0) inv_ -= 1.f / 60.f;

    float ax = 0;
    if (left) ax -= 1;
    if (right) ax += 1;
    if (ax != 0) face_ = ax > 0 ? 1 : -1;
    vx_ += ax * 0.55f;
    if (ax == 0) vx_ *= 0.72f;
    if (vx_ > 2.6f) vx_ = 2.6f;
    if (vx_ < -2.6f) vx_ = -2.6f;

    if (jump && onGround_) {
        vy_ = JUMP;
        onGround_ = false;
    }
    vy_ += GRAV;
    if (vy_ > 8.f) vy_ = 8.f;
    px_ += vx_;
    py_ += vy_;
    if (px_ < 16.f) px_ = 16.f;
    if (px_ > WORLD - 16.f) px_ = WORLD - 16.f;
    if (py_ >= GROUND) {
        py_ = GROUND;
        vy_ = 0;
        onGround_ = true;
    }
    if (std::fabs(vx_) > 0.4f) runTick_++;

    if (!held_ && std::fabs(px_ - pouchX_) < 22.f && onGround_) held_ = true;

    if (inv_ <= 0 && onGround_) {
        float bodyL = px_ - 10.f;
        float bodyR = px_ + 10.f;
        for (const Car& c : cars_) {
            float cl = c.x - c.w * 0.5f + 6.f;
            float cr = c.x + c.w * 0.5f - 6.f;
            if (bodyR > cl && bodyL < cr) {
                lives_--;
                inv_ = 1.1f;
                held_ = false;
                pouchX_ = px_ - face_ * 36.f;
                if (pouchX_ < 40.f) pouchX_ = 40.f;
                vx_ = -face_ * 2.2f;
                vy_ = -3.2f;
                onGround_ = false;
                sys_->rumble(0.4f, 0.2f, 90);
                if (lives_ <= 0) finish(false);
                break;
            }
        }
    }

    for (Car& c : cars_) {
        c.x += c.speed;
        if (c.x < -80.f) c.x = WORLD + 40.f;
        if (c.x > WORLD + 80.f) c.x = -40.f;
    }

    if (held_ && px_ >= GOAL) finish(true);

    float want = px_ - 130.f;
    if (want < 0) want = 0;
    if (want > WORLD - gs::SCREEN_W) want = WORLD - gs::SCREEN_W;
    cam_ += (want - cam_) * 0.18f;
}

void Game::spr(const gs::Mipped& m, float cx, float footY, float h, int pal, bool flip) {
    if (m.h <= 0) return;
    gs::Sprite s;
    s.img = m.pick(h);
    s.h = int(h);
    s.w = int(h * (float(m.w) / float(m.h)));
    if (s.w < 1) s.w = 1;
    s.x = int(cx - s.w * 0.5f);
    s.y = int(footY - s.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s) {
    for (int i = 0; s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c > 126) c = ' ';
        sys_->vdp.HUD.set(col + i, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    int hs = int(cam_);
    vdp.B.scroll(hs, 0);

    for (int i = 0; i < 7; i++) {
        float x = 180.f + i * 210.f;
        spr(art_.lamp, x - cam_, GROUND + 8.f, 70.f, PAL_FX, false);
    }
    for (int i = 0; i < 5; i++) {
        float x = 320.f + i * 240.f;
        spr(art_.cone, x - cam_, GROUND + 6.f, 18.f, PAL_FX, false);
    }
    spr(art_.booth, 1540.f - cam_, GROUND + 8.f, 78.f, PAL_BOOTH, false);

    for (const Car& c : cars_) {
        const gs::Mipped& img = c.kind ? art_.carB : art_.car;
        spr(img, c.x - cam_, GROUND + 4.f, 32.f, PAL_CAR, c.speed < 0);
    }

    bool blink = inv_ > 0 && (int(t_ * 12.f) & 1);
    if (!blink) spr(heroSprite(), px_ - cam_, py_, 52.f, PAL_PLAYER, face_ < 0);

    float pouchDraw = held_ ? px_ + face_ * 12.f : pouchX_;
    float pouchFoot = held_ ? py_ - 18.f : GROUND + 2.f;
    spr(art_.pouch, pouchDraw - cam_, pouchFoot, 18.f, PAL_POUCH, false);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 78.f, 28.f, PAL_HUD, false);
        spr(art_.sub, 160.f, 108.f, 12.f, PAL_HUD, false);
        hud(8, 20, "ONE LOT");
        hud(6, 22, "A JUMP  START");
    } else if (mode_ == Mode::Won) {
        hud(6, 3, "THE POUCH IS ACROSS");
        hud(10, 5, "IT IS DONE");
    } else if (mode_ == Mode::Lost) {
        hud(8, 3, "THE LOT IS LOST");
    } else {
        char buf[32];
        int left = LIMIT - int(clock_);
        if (left < 0) left = 0;
        std::snprintf(buf, sizeof(buf), "LIVES %d", lives_);
        hud(1, 1, buf);
        std::snprintf(buf, sizeof(buf), "TIME %d", left);
        hud(28, 1, buf);
        hud(1, 26, held_ ? "CARRYING" : "GET THE POUCH");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    sys.vdp.roadTime++;

    bool left = sys.pad.down(gs::BTN_LEFT);
    bool right = sys.pad.down(gs::BTN_RIGHT);
    bool jump = sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_B);
    bool start = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C);

    if (mode_ == Mode::Title) {
        titleWait_++;
        if (start || jump || (bot_ && titleWait_ > 8)) begin();
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, jump);
        stepPlay(left, right, jump);
    }

    if (mode_ == Mode::Play && held_) sys.apu.tone(0, 180.f + std::sin(t_ * 6.f) * 20.f, 0.04f);
    else if (mode_ == Mode::Won) sys.apu.tone(0, 440.f, t_ < 0.2f ? 0.1f : 0.02f);
    else sys.apu.tone(0, 0, 0);

    draw();
}

}  // namespace lot
