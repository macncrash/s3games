#include "game/shuffle.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "console/gfx.h"

namespace shuffle {

namespace {

constexpr float BOARD_X = 108.f;
constexpr float BOARD_Y = 24.f;
constexpr float BOARD_W = 104.f;
constexpr float BOARD_H = 190.f;
constexpr float RAIL = 6.f;
constexpr float R = 5.f;
constexpr float VMAX = 6.4f;
constexpr float FRICTION = 0.968f;
constexpr int PER_SIDE = 3;

float playL() { return BOARD_X + RAIL; }
float playR() { return BOARD_X + BOARD_W - RAIL; }
float dropY() { return BOARD_Y + 8.f; }
float releaseY() { return BOARD_Y + BOARD_H - 16.f; }
float nearY() { return BOARD_Y + BOARD_H - RAIL; }

}  // namespace

void Game::tone(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 8;
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 1 + (y * 4) / gs::SCREEN_H;
        vdp.lineBackdrop[y] = gs::rgb4(1, g, 2);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 2, 2));

    // 0 HUD ink, 1 board, 2 brass, 3 house red, 4 chalk
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 2),
                            gs::rgb4(14, 4, 3), gs::rgb4(6, 12, 6), gs::rgb4(8, 8, 7)};
    const uint16_t wood[] = {0,
                             gs::rgb4(12, 8, 4),
                             gs::rgb4(10, 6, 3),
                             gs::rgb4(7, 4, 2),
                             gs::rgb4(4, 2, 1),
                             gs::rgb4(15, 14, 10),
                             gs::rgb4(9, 6, 3),
                             gs::rgb4(3, 2, 1)};
    const uint16_t brass[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(4, 3, 1)};
    const uint16_t red[] = {0, gs::rgb4(15, 8, 6), gs::rgb4(12, 2, 2), gs::rgb4(7, 1, 1), gs::rgb4(3, 0, 0)};
    const uint16_t chalk[] = {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 12, 4), gs::rgb4(2, 1, 1)};
    auto loadPal = [&](int pal, const uint16_t* c, int n) {
        for (int i = 0; i < n; i++) vdp.setColor(pal * 16 + i, c[i]);
    };
    loadPal(0, hud, 7);
    loadPal(1, wood, 8);
    loadPal(2, brass, 5);
    loadPal(3, red, 5);
    loadPal(4, chalk, 4);

    gs::Bitmap board{int(BOARD_W), int(BOARD_H)};
    uint32_t n = 0x12345u;
    for (int y = 0; y < board.h; y++) {
        for (int x = 0; x < board.w; x++) {
            n = n * 1664525u + 1013904223u;
            int edge = (x < 5 || y < 5 || x >= board.w - 5 || y >= board.h - 5) ? 1 : 0;
            int grain = (n >> 28) & 3;
            int c = edge ? 4 : (grain == 0 ? 3 : (grain == 1 ? 2 : 1));
            if (!edge && ((y / 2) & 1) && grain == 3) c = 6;
            board.set(x, y, c);
        }
    }
    auto hline = [&](int y, int c) {
        for (int t = 0; t < 2; t++)
            for (int x = 6; x < board.w - 6; x++) board.set(x, y + t, c);
    };
    // Drop, then 3 / 2 / 1. The trough is the dark cap at the far end.
    for (int y = 0; y < 8; y++)
        for (int x = 6; x < board.w - 6; x++) board.set(x, y, 7);
    hline(8, 5);
    hline(36, 5);
    hline(64, 5);
    hline(92, 5);
    for (int y = 8; y < 36; y++)
        for (int x = 6; x < 10; x++) board.set(x, y, 6);

    gs::TextStyle num;
    num.scale = 2;
    num.color = 7;
    num.shadow = 5;
    const char* labels = "321";
    const int rows[] = {14, 42, 70};
    for (int i = 0; i < 3; i++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, labels[i]), num);
        board.blit(g, 12, rows[i]);
    }
    board_ = gs::uploadImage(vdp, board);

    auto puck = [&](int body, int mid, int hi) {
        gs::Bitmap b(14, 14);
        b.ellipse(7, 7, 6.2f, 6.2f, 4);
        b.ellipse(7, 7, 5.2f, 5.2f, body);
        b.ellipse(6.2f, 6.0f, 3.2f, 2.6f, mid);
        b.ellipse(5.2f, 5.0f, 1.4f, 1.1f, hi);
        return gs::uploadImage(vdp, b);
    };
    puck_[0] = puck(3, 2, 1);
    puck_[1] = puck(3, 2, 1);

    gs::Bitmap mark(10, 8);
    mark.poly({{5, 0}, {10, 8}, {0, 8}}, 1);
    mark_ = gs::uploadImage(vdp, mark);

    gs::Bitmap chip(4, 4);
    chip.rect(0, 0, 4, 4, 1);
    chip_ = gs::uploadImage(vdp, chip);

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    for (int ch = 32; ch < 127; ch++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(ch)), st);
        glyphs_[ch - 32] = gs::uploadImage(vdp, g);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    you_ = house_ = 0;
    rack_ = 1;
    over_ = won_ = false;
    aim_ = (playL() + playR()) * 0.5f;
    power_ = 0.62f;
    mode_ = bot_ ? Mode::Play : Mode::Title;
    newRack();
}

void Game::newRack() {
    discs_.clear();
    thrown_ = 0;
    banner_ = 0;
    lastPts_[0] = lastPts_[1] = 0;
    wait_ = bot_ ? 10 : 20;
}

int Game::zoneAt(float y) const {
    float z0 = dropY();
    if (y < z0) return 0;
    if (y < z0 + 28.f) return 3;
    if (y < z0 + 56.f) return 2;
    if (y < z0 + 84.f) return 1;
    return 0;
}

bool Game::resting(const std::vector<Disc>& d) const {
    for (const Disc& p : d)
        if (p.on && std::fabs(p.vx) + std::fabs(p.vy) > 0.045f) return false;
    return true;
}

void Game::physics(std::vector<Disc>& d, int steps) {
    const float L = playL() + R, Rt = playR() - R, far = dropY(), near = nearY() - R;
    for (int s = 0; s < steps; s++) {
        for (Disc& p : d) {
            if (!p.on) continue;
            p.x += p.vx;
            p.y += p.vy;
            if (p.x < L) {
                p.x = L;
                p.vx = std::fabs(p.vx) * 0.42f;
            } else if (p.x > Rt) {
                p.x = Rt;
                p.vx = -std::fabs(p.vx) * 0.42f;
            }
            if (p.y > near) {
                p.y = near;
                p.vy = -std::fabs(p.vy) * 0.28f;
            }
            if (p.y < far) p.on = false;
        }
        for (int iter = 0; iter < 3; iter++) {
            for (size_t i = 0; i < d.size(); i++) {
                if (!d[i].on) continue;
                for (size_t j = i + 1; j < d.size(); j++) {
                    if (!d[j].on) continue;
                    float dx = d[j].x - d[i].x, dy = d[j].y - d[i].y;
                    float d2 = dx * dx + dy * dy;
                    float min = R * 2.f;
                    if (d2 >= min * min || d2 < 1e-6f) continue;
                    float dist = std::sqrt(d2);
                    float nx = dx / dist, ny = dy / dist;
                    float pen = min - dist;
                    d[i].x -= nx * pen * 0.5f;
                    d[i].y -= ny * pen * 0.5f;
                    d[j].x += nx * pen * 0.5f;
                    d[j].y += ny * pen * 0.5f;
                    float rv = (d[j].vx - d[i].vx) * nx + (d[j].vy - d[i].vy) * ny;
                    if (rv < 0.f) {
                        float jimp = -(1.08f) * rv * 0.5f;
                        d[i].vx -= jimp * nx;
                        d[i].vy -= jimp * ny;
                        d[j].vx += jimp * nx;
                        d[j].vy += jimp * ny;
                    }
                }
            }
        }
        for (Disc& p : d) {
            if (!p.on) continue;
            p.vx *= FRICTION;
            p.vy *= FRICTION;
            if (std::fabs(p.vx) + std::fabs(p.vy) < 0.04f) p.vx = p.vy = 0;
        }
        if (resting(d)) break;
    }
}

void Game::launch(int side, float x, float power) {
    Disc p;
    p.side = side;
    p.x = std::max(playL() + R, std::min(playR() - R, x));
    p.y = releaseY();
    p.vx = 0;
    p.vy = -std::max(0.15f, std::min(1.f, power)) * VMAX;
    p.on = true;
    discs_.push_back(p);
    thrown_++;
    tone(side ? 140.f : 220.f, 0.08f);
}

bool Game::planShot(int side, float targetY, float& x, float& power) {
    const float lanes[] = {playL() + 16.f, playL() + 32.f, (playL() + playR()) * 0.5f, playR() - 32.f, playR() - 16.f};
    float bestErr = 1e9f;
    bool found = false;
    for (float lane : lanes) {
        bool blocked = false;
        for (const Disc& p : discs_) {
            if (p.on && std::fabs(p.x - lane) < R * 2.6f) blocked = true;
        }
        if (blocked) continue;
        float lo = 0.18f, hi = 0.98f, pick = 0.6f, err = 1e9f;
        for (int it = 0; it < 12; it++) {
            float mid = (lo + hi) * 0.5f;
            std::vector<Disc> sim = discs_;
            Disc s;
            s.side = side;
            s.x = lane;
            s.y = releaseY();
            s.vy = -mid * VMAX;
            s.on = true;
            sim.push_back(s);
            physics(sim, 500);
            const Disc& end = sim.back();
            float y = end.on ? end.y : 0.f;
            float e = std::fabs(y - targetY);
            if (end.on && e < err) {
                err = e;
                pick = mid;
            }
            if (!end.on || y < targetY) hi = mid;
            else lo = mid;
        }
        if (err < bestErr) {
            bestErr = err;
            x = lane;
            power = pick;
            found = err < 4.f;
        }
    }
    if (!found) {
        x = (playL() + playR()) * 0.5f;
        power = 0.62f;
    }
    return found;
}

void Game::scoreRack() {
    float best[2] = {1e9f, 1e9f};
    for (const Disc& p : discs_) {
        if (!p.on || zoneAt(p.y) <= 0) continue;
        best[p.side] = std::min(best[p.side], p.y);
    }
    int leader = -1;
    if (best[0] < best[1]) leader = 0;
    else if (best[1] < best[0]) leader = 1;
    int pts[2] = {};
    if (leader >= 0) {
        float beat = best[1 - leader];
        for (const Disc& p : discs_) {
            int z = p.on ? zoneAt(p.y) : 0;
            if (p.side == leader && z > 0 && p.y < beat) pts[leader] += z;
        }
    }
    you_ += pts[0];
    house_ += pts[1];
    lastPts_[0] = pts[0];
    lastPts_[1] = pts[1];
    tone(pts[0] >= pts[1] ? 330.f : 110.f, 0.1f);
}

void Game::blitText(const std::string& s, float x, float y, int pal) {
    float cx = x;
    for (char ch : s) {
        int i = int((unsigned char)ch) - 32;
        if (i < 0 || i > 95) {
            cx += 4;
            continue;
        }
        const gs::Image& g = glyphs_[i];
        if (ch != ' ' && g.w) {
            gs::Sprite sp;
            sp.img = g;
            sp.x = int16_t(cx);
            sp.y = int16_t(y);
            sp.w = g.w;
            sp.h = g.h;
            sp.pal = uint8_t(pal);
            sys_->vdp.sprite(sp);
        }
        cx += (g.w ? g.w : 4) + 1;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();

    auto spr = [&](gs::Image img, float x, float y, float w, float h, int pal, bool shadow = false) {
        if (!img.w || !img.h || w < 1 || h < 1) return;
        gs::Sprite s;
        s.img = img;
        s.x = int16_t(std::lround(x));
        s.y = int16_t(std::lround(y));
        s.w = int16_t(std::lround(w));
        s.h = int16_t(std::lround(h));
        s.pal = uint8_t(pal);
        s.shadow = shadow;
        vdp.sprite(s);
    };

    if (mode_ == Mode::Title) {
        blitText("SHUFFLE SEVEN", 78, 78, 0);
        blitText("FIRST TO SEVEN", 74, 96, 4);
        blitText("A SHORT DECK SHUFFLE", 58, 112, 0);
        blitText("PRESS START", 100, 140, 4);
    } else {
        blitText("YOU", 8, 8, 4);
        blitText(std::to_string(you_), 8, 20, 0);
        blitText("HOUSE", 262, 8, 0);
        blitText(std::to_string(house_), 262, 20, 0);
        blitText("TO 7", 146, 6, 0);

        if (mode_ == Mode::Play && thrown_ < PER_SIDE * 2 && resting(discs_)) {
            float ax = std::max(playL() + R, std::min(playR() - R, aim_));
            spr(mark_, ax - 5, releaseY() + 4, 10, 8, thrown_ & 1 ? 3 : 2);
            int bars = std::max(1, int(power_ * 10));
            for (int i = 0; i < bars; i++) spr(chip_, 8, 180 - i * 6, 8, 4, 4);
            blitText(thrown_ & 1 ? "HOUSE" : "YOU", 8, 48, thrown_ & 1 ? 0 : 4);
        }
        if (mode_ == Mode::Banner) {
            blitText(lastPts_[0] || lastPts_[1] ? "RACK" : "RACK", 136, 100, 0);
            std::string line = "+" + std::to_string(lastPts_[0]) + "  +" + std::to_string(lastPts_[1]);
            blitText(line, 112, 114, 4);
        }
        if (mode_ == Mode::End) {
            blitText(won_ ? "YOU WIN" : "HOUSE WINS", won_ ? 112 : 100, 96, 4);
            blitText("FIRST TO SEVEN", 74, 112, 0);
        }

        for (const Disc& p : discs_) {
            if (!p.on) continue;
            spr(puck_[p.side], p.x - 7, p.y - 7, 14, 14, p.side ? 3 : 2);
            spr(chip_, p.x - 6, p.y - 2, 12, 6, 1, true);
        }
    }
    spr(board_, BOARD_X, BOARD_Y, BOARD_W, BOARD_H, 1);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ <= 0) sys.apu.tone(0, 0, 0);

    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Play;
            wait_ = 8;
        }
        draw();
        return;
    }

    if (mode_ == Mode::End) {
        draw();
        return;
    }

    if (mode_ == Mode::Banner) {
        if (--banner_ <= 0) {
            bool youWin = you_ >= 7 && you_ > house_;
            bool houseWin = house_ >= 7 && house_ > you_;
            if (youWin || houseWin) {
                won_ = youWin;
                over_ = true;
                mode_ = Mode::End;
                tone(youWin ? 440.f : 90.f, 0.12f);
            } else {
                rack_++;
                mode_ = Mode::Play;
                newRack();
            }
        }
        draw();
        return;
    }

    bool calm = resting(discs_);
    if (calm && thrown_ >= PER_SIDE * 2) {
        scoreRack();
        mode_ = Mode::Banner;
        banner_ = 50;
        draw();
        return;
    }

    int side = thrown_ & 1;
    if (calm && thrown_ < PER_SIDE * 2) {
        if (wait_ > 0) wait_--;
        bool go = false;
        float x = aim_, pwr = power_;
        if (bot_ || side == 1) {
            if (wait_ <= 0) {
                float target = side == 0 ? dropY() + 14.f : dropY() + 22.f;
                planShot(side, target, x, pwr);
                aim_ = x;
                power_ = pwr;
                go = true;
            }
        } else {
            if (sys.pad.down(gs::BTN_LEFT)) aim_ -= 1.4f;
            if (sys.pad.down(gs::BTN_RIGHT)) aim_ += 1.4f;
            if (sys.pad.axisX) aim_ += sys.pad.axisX * 1.6f;
            if (sys.pad.down(gs::BTN_UP)) power_ += 0.012f;
            if (sys.pad.down(gs::BTN_DOWN)) power_ -= 0.012f;
            aim_ = std::max(playL() + R, std::min(playR() - R, aim_));
            power_ = std::max(0.2f, std::min(0.95f, power_));
            if (sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_B)) {
                x = aim_;
                pwr = power_;
                go = true;
            }
        }
        if (go) {
            launch(side, x, pwr);
            wait_ = 12;
        }
    } else if (!calm) {
        physics(discs_, 1);
    }

    draw();
}

}  // namespace shuffle
