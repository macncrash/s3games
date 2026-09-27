#include "game/boccegold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace boccegold {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kDrag = 86.f;
constexpr float kLeft = 48.f;
constexpr float kRight = 272.f;
constexpr float kHead = 52.f;
constexpr float kFoot = 196.f;

float span(float x, float y) { return std::sqrt(x * x + y * y); }

float bowlR(int w) { return w == 0 ? 4.1f : 7.1f; }

int paintOf(int w) {
    if (w == 1) return PAL_GOLD;
    if (w == 2) return PAL_CREAM;
    if (w == 3) return PAL_RIVAL;
    return PAL_JACK;
}
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.38f);
    reset();
}

void Game::reset() {
    mode_ = Mode::Title;
    who_ = Who::Jack;
    count_ = 0;
    over_ = false;
    won_ = false;
    goldPts_ = 0;
    creamPts_ = 0;
    wait_ = 0;
    aim_ = 0.f;
    power_ = 0.42f;
    powerDir_ = 1.f;
    clock_ = 0.f;
    for (Ball& b : ball_) b = Ball{};
}

void Game::openEnd() {
    who_ = Who::Jack;
    count_ = 0;
    goldPts_ = 0;
    creamPts_ = 0;
    won_ = false;
    over_ = false;
    aim_ = 0.f;
    power_ = 0.5f;
    mode_ = Mode::Aim;
    wait_ = 0;
}

int Game::hand() const {
    if (who_ == Who::Jack) return 0;
    if (who_ == Who::Gold) return 1;
    if (who_ == Who::Cream) return 2;
    return 3;
}

void Game::release() {
    if (count_ >= 4) return;
    Ball& b = ball_[count_++];
    b.who = who_;
    b.live = true;
    b.vx = b.vy = 0.f;
    static const float kHomeX[4] = {160.f, 196.f, 124.f, 176.f};
    b.x = kHomeX[hand()];
    b.y = 184.f;

    float tx = 160.f, ty = 78.f;
    if (bot_) {
        if (who_ == Who::Jack) {
            tx = 158.f;
            ty = 74.f;
        } else if (who_ == Who::Gold) {
            tx = 170.f;
            ty = 88.f;
        } else if (who_ == Who::Cream) {
            tx = 96.f;
            ty = 162.f;
        } else {
            tx = 146.f;
            ty = 124.f;
        }
    } else {
        float reach = (who_ == Who::Jack ? 148.f : 128.f) * (0.26f + power_ * 0.74f);
        float ang = std::clamp(aim_, -0.9f, 0.9f);
        tx = b.x + std::sin(ang) * reach;
        ty = b.y - std::cos(ang) * reach;
    }

    float dx = tx - b.x;
    float dy = ty - b.y;
    float d = span(dx, dy);
    if (d < 1.f) {
        b.x = tx;
        b.y = ty;
    } else {
        float v = std::sqrt(2.f * kDrag * d);
        b.vx = dx / d * v;
        b.vy = dy / d * v;
    }
    if (sys_) sys_->apu.noiseBurst(0.16f, 480.f, 0.05f);
    mode_ = Mode::Roll;
    wait_ = 0;
}

void Game::coast(float dt) {
    for (int i = 0; i < count_; i++) {
        Ball& b = ball_[i];
        if (!b.live) continue;
        float sp = span(b.vx, b.vy);
        if (sp < 1.2f) {
            b.vx = b.vy = 0.f;
            continue;
        }
        float stopIn = sp / kDrag;
        float use = dt;
        bool halt = false;
        if (use >= stopIn) {
            use = stopIn;
            halt = true;
        }
        float dist = sp * use - 0.5f * kDrag * use * use;
        b.x += b.vx / sp * dist;
        b.y += b.vy / sp * dist;
        if (halt) b.vx = b.vy = 0.f;
        else {
            float ns = sp - kDrag * use;
            b.vx = b.vx / sp * ns;
            b.vy = b.vy / sp * ns;
        }
        float r = bowlR(int(b.who));
        if (b.x < kLeft + r) {
            b.x = kLeft + r;
            b.vx = std::fabs(b.vx) * 0.5f;
        }
        if (b.x > kRight - r) {
            b.x = kRight - r;
            b.vx = -std::fabs(b.vx) * 0.5f;
        }
        if (b.y < kHead + r) {
            b.y = kHead + r;
            b.vy = std::fabs(b.vy) * 0.4f;
        }
        if (b.y > kFoot - r) {
            b.y = kFoot - r;
            b.vy = -std::fabs(b.vy) * 0.35f;
        }
    }
}

void Game::shove() {
    for (int i = 0; i < count_; i++) {
        if (!ball_[i].live) continue;
        for (int j = i + 1; j < count_; j++) {
            if (!ball_[j].live) continue;
            float dx = ball_[j].x - ball_[i].x;
            float dy = ball_[j].y - ball_[i].y;
            float need = bowlR(int(ball_[i].who)) + bowlR(int(ball_[j].who));
            float d = span(dx, dy);
            if (d >= need || d < 0.02f) continue;
            float nx = dx / d;
            float ny = dy / d;
            float push = (need - d) * 0.5f + 0.04f;
            ball_[i].x -= nx * push;
            ball_[i].y -= ny * push;
            ball_[j].x += nx * push;
            ball_[j].y += ny * push;
            float rel = (ball_[j].vx - ball_[i].vx) * nx + (ball_[j].vy - ball_[i].vy) * ny;
            if (rel < 0.f) {
                ball_[i].vx += nx * rel * 0.85f;
                ball_[i].vy += ny * rel * 0.85f;
                ball_[j].vx -= nx * rel * 0.85f;
                ball_[j].vy -= ny * rel * 0.85f;
            }
            if (sys_) sys_->apu.tone(1, 210.f, 0.03f);
        }
    }
}

bool Game::still() const {
    for (int i = 0; i < count_; i++) {
        if (!ball_[i].live) continue;
        if (span(ball_[i].vx, ball_[i].vy) > 1.2f) return false;
    }
    return true;
}

void Game::score() {
    const Ball* jack = nullptr;
    for (int i = 0; i < count_; i++)
        if (ball_[i].live && ball_[i].who == Who::Jack) jack = &ball_[i];
    float rival = 1e9f;
    if (jack) {
        for (int i = 0; i < count_; i++) {
            if (!ball_[i].live || ball_[i].who != Who::Rival) continue;
            rival = std::min(rival, span(ball_[i].x - jack->x, ball_[i].y - jack->y));
        }
    }
    goldPts_ = 0;
    creamPts_ = 0;
    if (jack) {
        for (int i = 0; i < count_; i++) {
            if (!ball_[i].live) continue;
            float d = span(ball_[i].x - jack->x, ball_[i].y - jack->y);
            if (d + 0.4f >= rival) continue;
            if (ball_[i].who == Who::Gold) goldPts_ = 2;
            if (ball_[i].who == Who::Cream) creamPts_ = 1;
        }
    }
    // Only the gold counts double. A cream point does not finish the end.
    if (goldPts_ == 2 && creamPts_ == 0) {
        won_ = true;
        mode_ = Mode::Win;
        wait_ = 0;
        if (sys_) {
            sys_->apu.tone(0, 523.f, 0.12f);
            if (!sys_->headless) sys_->rumble(0.3f, 0.5f, 120);
        }
    } else {
        won_ = false;
        mode_ = Mode::Lose;
        wait_ = 0;
        if (sys_) sys_->apu.tone(0, 180.f, 0.1f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) openEnd();
    } else if (mode_ == Mode::Aim) {
        if (who_ == Who::Rival || bot_) {
            release();
        } else {
            if (pad.down(gs::BTN_LEFT)) aim_ -= 1.35f * kDt;
            if (pad.down(gs::BTN_RIGHT)) aim_ += 1.35f * kDt;
            aim_ = std::clamp(aim_, -0.9f, 0.9f);
            power_ += powerDir_ * kDt * 0.8f;
            if (power_ >= 1.f) {
                power_ = 1.f;
                powerDir_ = -1.f;
            }
            if (power_ <= 0.f) {
                power_ = 0.f;
                powerDir_ = 1.f;
            }
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) release();
        }
    } else if (mode_ == Mode::Roll) {
        coast(kDt);
        shove();
        if (still()) {
            wait_++;
            if (wait_ > (bot_ ? 6 : 20)) {
                wait_ = 0;
                if (who_ == Who::Jack) who_ = Who::Gold;
                else if (who_ == Who::Gold) who_ = Who::Cream;
                else if (who_ == Who::Cream) who_ = Who::Rival;
                else {
                    score();
                    who_ = Who::Jack;
                }
                if (mode_ == Mode::Roll) mode_ = Mode::Aim;
            }
        } else {
            wait_ = 0;
        }
    } else if (mode_ == Mode::Win) {
        wait_++;
        if (wait_ == 16) sys.apu.tone(0, 659.f, 0.1f);
        if (wait_ == 32) sys.apu.tone(0, 784.f, 0.14f);
        if (wait_ > 64) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Lose) {
        if (bot_) {
            wait_++;
            if (wait_ > 20) reset();
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            reset();
        }
    }
    paint();
}

void Game::blit(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::line(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::lineC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    line(20 - n / 2, row, s, pal);
}

void Game::paint() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.lineBackdrop[y] = y < 34 ? gs::rgb4(2, 3, 7) : gs::rgb4(1, 5, 2);
    }

    if (mode_ == Mode::Title || mode_ == Mode::Win)
        blit(art_.banner, 160.f, 16.f, float(art_.banner.w), float(art_.banner.h),
             mode_ == Mode::Win ? PAL_WIN : PAL_TITLE);

    for (int i = count_ - 1; i >= 0; i--) {
        if (!ball_[i].live) continue;
        const gs::Image& img = ball_[i].who == Who::Jack ? art_.jack : art_.bowl;
        blit(img, ball_[i].x, ball_[i].y, float(img.w), float(img.h), paintOf(int(ball_[i].who)));
    }

    if (mode_ == Mode::Aim && !bot_ && who_ != Who::Rival) {
        static const float kHomeX[4] = {160.f, 196.f, 124.f, 176.f};
        float sx = kHomeX[hand()];
        float sy = 184.f;
        float reach = 34.f + power_ * 30.f;
        blit(art_.jack, sx + std::sin(aim_) * reach, sy - std::cos(aim_) * reach, 6.f, 6.f, PAL_AIM);
    }

    blit(art_.court, 160.f, 122.f, float(art_.court.w), float(art_.court.h), PAL_COURT);

    line(1, 1, "S3 BOCCE GOLD", PAL_INK);
    char pts[32];
    std::snprintf(pts, sizeof(pts), "GOLD %d  CREAM %d", goldPts_, creamPts_);
    line(22, 1, pts, PAL_TITLE);

    if (mode_ == Mode::Title) lineC(26, "START  ONLY GOLD COUNTS DOUBLE", PAL_HINT);
    else if (mode_ == Mode::Aim && who_ == Who::Jack) lineC(26, "BOWL THE PALLINO", PAL_HINT);
    else if (mode_ == Mode::Aim && who_ == Who::Gold) lineC(26, "GOLD BALL  COUNTS DOUBLE", PAL_TITLE);
    else if (mode_ == Mode::Aim && who_ == Who::Cream) lineC(26, "CREAM BALL  COUNTS ONE", PAL_HINT);
    else if (mode_ == Mode::Aim && who_ == Who::Rival) lineC(26, "THEIR BALL", PAL_INK);
    else if (mode_ == Mode::Win) lineC(26, "ONLY THE GOLD COUNTS DOUBLE", PAL_WIN);
    else if (mode_ == Mode::Lose) lineC(26, "CREAM DOES NOT FINISH IT", PAL_TITLE);
    else lineC(26, "LET THEM REST", PAL_HINT);
}

}  // namespace boccegold
