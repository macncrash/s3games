#include "game/court.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace bocceseven {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int SUB = 4;
constexpr float DRAG = 1.7f;
constexpr float STOP = 7.f;
constexpr float BOUNCE = 0.42f;
constexpr float HIT = 0.72f;
constexpr float X0 = 30.f;
constexpr float X1 = 292.f;
constexpr float Y0 = 56.f;
constexpr float Y1 = 188.f;
constexpr float R = 6.5f;
constexpr float RJ = 3.6f;
constexpr float LAUNCH_X = 48.f;
constexpr float LAUNCH_Y = 112.f;
constexpr int YOU = 0;
constexpr int THEM = 1;
constexpr int PALLINO = 2;

float radius(int side) { return side == PALLINO ? RJ : R; }

void coast(float& x, float& y, float& vx, float& vy, float h) {
    float sp = std::hypot(vx, vy);
    if (sp < STOP) {
        vx = vy = 0;
        return;
    }
    x += vx * h;
    y += vy * h;
    float k = std::exp(-DRAG * h);
    vx *= k;
    vy *= k;
    if (std::hypot(vx, vy) < STOP) vx = vy = 0;
}

float rangeOf(float speed) {
    float x = 0, y = 0, vx = speed, vy = 0;
    const float h = DT / float(SUB);
    for (int i = 0; i < 60 * SUB * 8; i++) {
        if (std::hypot(vx, vy) < STOP) break;
        coast(x, y, vx, vy, h);
    }
    return x;
}

float speedFor(float dist) {
    if (dist < 2.f) return STOP + 1.f;
    float lo = 0.f, hi = 30.f;
    while (rangeOf(hi) < dist && hi < 8000.f) hi *= 1.6f;
    for (int i = 0; i < 22; i++) {
        float mid = 0.5f * (lo + hi);
        if (rangeOf(mid) < dist) lo = mid;
        else hi = mid;
    }
    return hi;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paintCourt();
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.65f);
    you_ = them_ = ends_ = 0;
    endNo_ = 1;
    mode_ = Mode::Title;
    say_ = "FIRST TO SEVEN";
    clock_ = 0;
}

void Game::paintCourt() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.B.clear();
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 40; cx++) {
            bool lane = cx >= 3 && cx <= 36 && cy >= 7 && cy <= 22;
            bool rail = lane && (cy == 7 || cy == 22 || cx == 3 || cx == 36);
            int tile = rail ? art_.rail : lane ? art_.dust : art_.lawn;
            int pal = rail ? PAL_RAIL : lane ? PAL_DUST : PAL_LAWN;
            v.B.set(cx, cy, gs::entry(tile, pal));
        }
    }
}

void Game::openEnd() {
    balls_.clear();
    phase_ = Phase::Pallino;
    thrown_ = 0;
    nextSide_ = YOU;
    gainYou_ = gainThem_ = 0;
    aimY_ = LAUNCH_Y;
    aimNudge_ = 0;
    meter_ = 0.4f;
    mode_ = Mode::Aim;
    say_ = "PALLINO";
    charging_ = false;
}

void Game::launch(int side, float tx, float ty) {
    Ball b;
    b.side = side;
    b.x = LAUNCH_X;
    b.y = (side == PALLINO || bot_) ? LAUNCH_Y : aimY_;
    float dx = tx - b.x;
    float dy = ty - b.y;
    float dist = std::hypot(dx, dy);
    if (dist < 1.f) dist = 1.f;
    float sp = speedFor(dist);
    b.vx = sp * dx / dist;
    b.vy = sp * dy / dist;
    balls_.push_back(b);
    toneT_ = 0.12f;
    sys_->apu.tone(0, side == PALLINO ? 740.f : 180.f, 0.08f);
    sys_->apu.noiseBurst(0.2f, side == PALLINO ? 900.f : 420.f, 0.05f);
    mode_ = Mode::Roll;
    clock_ = 0;
}

void Game::botThrow() {
    if (phase_ == Phase::Pallino) {
        launch(PALLINO, 220.f, 112.f);
        return;
    }
    // Four bowls a side. Rest spots sit off each other's paths so the
    // nearer cluster is yours and the far cluster is theirs.
    static const float kYou[4][2] = {{200.f, 84.f}, {206.f, 140.f}, {178.f, 112.f}, {208.f, 68.f}};
    static const float kThem[4][2] = {{102.f, 70.f}, {102.f, 154.f}, {90.f, 78.f}, {126.f, 132.f}};
    int n = 0;
    for (const Ball& b : balls_)
        if (b.live && b.side == nextSide_) n++;
    if (n > 3) n = 3;
    const float* p = nextSide_ == YOU ? kYou[n] : kThem[n];
    launch(nextSide_, p[0], p[1]);
}

void Game::human(const gs::Pad& pad) {
    if (pad.down(gs::BTN_UP)) aimY_ -= 40.f * DT;
    if (pad.down(gs::BTN_DOWN)) aimY_ += 40.f * DT;
    aimY_ = std::clamp(aimY_, Y0 + 16.f, Y1 - 16.f);
    if (pad.down(gs::BTN_LEFT)) aimNudge_ -= DT * 0.35f;
    if (pad.down(gs::BTN_RIGHT)) aimNudge_ += DT * 0.35f;
    aimNudge_ = std::clamp(aimNudge_, -0.55f, 0.55f);
    bool hold = pad.down(gs::BTN_A);
    if (hold) {
        charging_ = true;
        meter_ += meterDir_ * DT * 0.85f;
        if (meter_ > 1.f) {
            meter_ = 1.f;
            meterDir_ = -1.f;
        }
        if (meter_ < 0.18f) {
            meter_ = 0.18f;
            meterDir_ = 1.f;
        }
    } else if (charging_) {
        charging_ = false;
        float reach = 70.f + meter_ * 200.f;
        float ang = aimNudge_ + (aimY_ - LAUNCH_Y) * 0.004f;
        float tx = LAUNCH_X + std::cos(ang) * reach;
        float ty = aimY_ + std::sin(ang) * reach * 0.35f;
        int side = phase_ == Phase::Pallino ? PALLINO : YOU;
        launch(side, tx, ty);
    }
}

void Game::stepBalls() {
    const float h = DT / float(SUB);
    for (int s = 0; s < SUB; s++) {
        for (Ball& b : balls_) {
            if (!b.live) continue;
            coast(b.x, b.y, b.vx, b.vy, h);
            float rad = radius(b.side);
            if (b.x < X0 + rad) {
                b.x = X0 + rad;
                b.vx = std::fabs(b.vx) * BOUNCE;
            }
            if (b.x > X1 - rad) {
                b.x = X1 - rad;
                b.vx = -std::fabs(b.vx) * BOUNCE;
            }
            if (b.y < Y0 + rad) {
                b.y = Y0 + rad;
                b.vy = std::fabs(b.vy) * BOUNCE;
            }
            if (b.y > Y1 - rad) {
                b.y = Y1 - rad;
                b.vy = -std::fabs(b.vy) * BOUNCE;
            }
        }
        for (size_t i = 0; i < balls_.size(); i++) {
            for (size_t j = i + 1; j < balls_.size(); j++) {
                Ball& a = balls_[i];
                Ball& b = balls_[j];
                if (!a.live || !b.live) continue;
                float dx = b.x - a.x;
                float dy = b.y - a.y;
                float dist = std::hypot(dx, dy);
                float need = radius(a.side) + radius(b.side);
                if (dist >= need || dist < 0.001f) continue;
                float nx = dx / dist;
                float ny = dy / dist;
                float push = (need - dist) * 0.5f;
                a.x -= nx * push;
                a.y -= ny * push;
                b.x += nx * push;
                b.y += ny * push;
                float rel = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
                if (rel >= 0) continue;
                float imp = rel * HIT;
                a.vx += imp * nx;
                a.vy += imp * ny;
                b.vx -= imp * nx;
                b.vy -= imp * ny;
                sys_->apu.noiseBurst(0.08f, 600.f, 0.03f);
            }
        }
    }
}

bool Game::settled() const {
    for (const Ball& b : balls_) {
        if (!b.live) continue;
        if (std::hypot(b.vx, b.vy) >= STOP) return false;
    }
    return true;
}

const Game::Ball* Game::pallino() const {
    for (const Ball& b : balls_)
        if (b.live && b.side == PALLINO) return &b;
    return nullptr;
}

void Game::scoreEnd() {
    const Ball* jack = pallino();
    gainYou_ = gainThem_ = 0;
    if (!jack) {
        say_ = "PALLINO OUT";
        mode_ = Mode::Tally;
        clock_ = 0;
        return;
    }
    float bestYou = 1e9f, bestThem = 1e9f;
    int nYou = 0, nThem = 0;
    for (const Ball& b : balls_) {
        if (!b.live || b.side == PALLINO) continue;
        float d = std::hypot(b.x - jack->x, b.y - jack->y);
        if (b.side == YOU) {
            nYou++;
            bestYou = std::min(bestYou, d);
        } else {
            nThem++;
            bestThem = std::min(bestThem, d);
        }
    }
    if (nYou && bestYou < bestThem) {
        for (const Ball& b : balls_) {
            if (!b.live || b.side != YOU) continue;
            float d = std::hypot(b.x - jack->x, b.y - jack->y);
            if (d < bestThem) gainYou_++;
        }
    } else if (nThem && bestThem < bestYou) {
        for (const Ball& b : balls_) {
            if (!b.live || b.side != THEM) continue;
            float d = std::hypot(b.x - jack->x, b.y - jack->y);
            if (d < bestYou) gainThem_++;
        }
    }
    you_ += gainYou_;
    them_ += gainThem_;
    ends_++;
    if (you_ >= 7 && you_ > them_) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        say_ = "FIRST TO SEVEN";
        tone(1);
    } else if (them_ >= 7 && them_ > you_) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Win;
        say_ = "THEY LEAVE";
        tone(2);
    } else {
        say_ = gainYou_ > gainThem_ ? "YOUR POINT" : gainThem_ ? "THEIR POINT" : "NO POINT";
        mode_ = Mode::Tally;
        tone(gainYou_ ? 1 : 2);
    }
    clock_ = 0;
}

void Game::afterRest() {
    if (phase_ == Phase::Pallino) {
        const Ball* jack = pallino();
        if (!jack || jack->x < X0 + 40.f) {
            balls_.clear();
            say_ = "RETHROW";
            mode_ = Mode::Aim;
            return;
        }
        phase_ = Phase::Bowls;
        thrown_ = 0;
        nextSide_ = YOU;
        say_ = "YOUR BOWL";
        mode_ = Mode::Aim;
        return;
    }
    int youN = 0, themN = 0;
    for (const Ball& b : balls_) {
        if (!b.live || b.side == PALLINO) continue;
        if (b.side == YOU) youN++;
        else themN++;
    }
    if (youN >= 4 && themN >= 4) {
        scoreEnd();
        return;
    }
    if (youN <= themN) nextSide_ = YOU;
    else nextSide_ = THEM;
    say_ = nextSide_ == YOU ? "YOUR BOWL" : "THEIR BOWL";
    mode_ = Mode::Aim;
}

void Game::tone(int kind) {
    toneT_ = 0.35f;
    if (kind == 1) {
        sys_->apu.tone(0, 523.f, 0.1f);
        sys_->apu.tone(1, 659.f, 0.08f);
        sys_->apu.tone(2, 784.f, 0.07f);
    } else {
        sys_->apu.tone(0, 196.f, 0.08f);
        sys_->apu.tone(1, 247.f, 0.06f);
    }
}

void Game::hush() {
    if (toneT_ > 0) {
        toneT_ -= DT;
        if (toneT_ <= 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    hush();
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || (bot_ && clock_ > 0.25f)) openEnd();
    } else if (mode_ == Mode::Aim) {
        if (bot_ || (phase_ == Phase::Bowls && nextSide_ == THEM)) botThrow();
        else human(pad);
    } else if (mode_ == Mode::Roll) {
        stepBalls();
        if (settled() && clock_ > 0.15f) afterRest();
    } else if (mode_ == Mode::Tally) {
        if (clock_ > 0.45f || (bot_ && clock_ > 0.05f)) {
            if (you_ >= 7 || them_ >= 7) {
                over_ = true;
                won_ = you_ >= 7 && you_ > them_;
                mode_ = Mode::Win;
            } else {
                endNo_++;
                openEnd();
            }
        }
    } else if (mode_ == Mode::Win) {
        over_ = true;
        if (clock_ > 1.2f) sys.quit();
    }
    draw();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 48) {
            float u = y / 47.f;
            v.lineBackdrop[y] = gs::rgb4(5 + int(u * 4), 8 + int(u * 3), 14 - int(u * 3));
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 7, 3);
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (m.h <= 0 || h <= 1.f) return;
    gs::Sprite s;
    s.img = m.pick(h);
    float sc = h / float(m.h);
    s.w = std::max(1, int(m.w * sc));
    s.h = int(h);
    s.x = int(cx - s.w * 0.5f);
    s.y = int(cy - s.h * 0.5f);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    spr(art_.lamp, 18, 36, 28, PAL_LAMP);
    spr(art_.lamp, 302, 36, 28, PAL_LAMP);
    spr(art_.flag, 36, 30, 26, PAL_FLAG);
    for (int i = 0; i < 4; i++) {
        spr(art_.cypress, 16, 70.f + i * 36.f, 40, PAL_CYPRESS);
        spr(art_.cypress, 304, 78.f + i * 32.f, 44, PAL_CYPRESS);
    }
    for (const Ball& b : balls_) {
        if (!b.live) continue;
        spr(art_.shade, b.x + 3.f, b.y + 5.f, b.side == PALLINO ? 6.f : 8.f, PAL_INK, true);
    }
    for (const Ball& b : balls_) {
        if (!b.live) continue;
        if (b.side == PALLINO) spr(art_.pallino, b.x, b.y, 10, PAL_PALLINO);
        else spr(art_.bowl, b.x, b.y, 16, b.side == YOU ? PAL_YOU : PAL_THEM);
    }
    if (mode_ == Mode::Aim && phase_ != Phase::Pallino) {
        const Ball* jack = pallino();
        if (jack) spr(art_.mark, jack->x, jack->y, 12, PAL_MARK);
    }
    if (mode_ == Mode::Aim && !(bot_ || (phase_ == Phase::Bowls && nextSide_ == THEM))) {
        spr(art_.mark, LAUNCH_X + 18.f, aimY_, 12, PAL_MARK);
    }

    hud(1, 1, "S3 BOCCE SEVEN", PAL_CREAM);
    hud(28, 1, "YOU " + std::to_string(you_), PAL_YOU);
    hud(28, 2, "THEM " + std::to_string(them_), PAL_THEM);
    if (mode_ == Mode::Title) {
        hudC(12, "FIRST TO SEVEN", PAL_CREAM);
        hudC(14, "BOWL THE PALLINO", PAL_INK);
        hudC(16, "NEAREST BALLS SCORE", PAL_INK);
        hudC(20, "A  THROW    START", PAL_CREAM);
    } else if (mode_ == Mode::Aim) {
        hudC(25, say_, PAL_CREAM);
        if (!bot_ && !(phase_ == Phase::Bowls && nextSide_ == THEM)) {
            int cells = std::clamp(int(meter_ * 10.f), 1, 10);
            hud(15, 26, std::string(cells, '|'), PAL_MARK);
        }
    } else if (mode_ == Mode::Tally) {
        hudC(24, say_, PAL_CREAM);
        hudC(25, "+" + std::to_string(gainYou_) + "  /  +" + std::to_string(gainThem_), PAL_INK);
    } else if (mode_ == Mode::Win) {
        hudC(24, won_ ? "YOU LEAVE AT SEVEN" : "MATCH OVER", won_ ? PAL_YOU : PAL_THEM);
        hudC(25, "YOU " + std::to_string(you_) + "  THEM " + std::to_string(them_), PAL_INK);
    } else {
        hudC(25, say_, PAL_INK);
    }
    hud(1, 26, "END " + std::to_string(endNo_), PAL_INK);
}

}  // namespace bocceseven
