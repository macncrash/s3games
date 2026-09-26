#include "game/mazemark.h"

#include "version.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace mazemark {
namespace {

constexpr int PAL_MAZE = 0;
constexpr int PAL_BODY = 1;
constexpr int PAL_INK = 2;
constexpr int PAL_COIN = 4;
constexpr int PAL_GOLD = 5;
constexpr int PAL_GREEN = 6;
constexpr int PAL_ALERT = 7;
constexpr int PAL_LOGO = 8;
constexpr int PAL_TREE = 9;
constexpr int PAL_MOON = 10;

constexpr int kStep = 8;
constexpr int kLift = 36;
constexpr int kWait = 18;
constexpr float kPi = 3.14159265f;

// S is the step-off, C the coin, G the cold gate. The coin is a dead end.
constexpr char kMap[MH][MW + 1] = {
    "###############",
    "#S..#.........#",
    "#.#.#.#######.#",
    "#.#...#.....#.#",
    "#.###.#.###.#.#",
    "#.....#.#...#.#",
    "###.###.#.###.#",
    "#C......#....G#",
    "###############",
};

const int kStar[][2] = {{18, 10}, {46, 18}, {300, 8}, {286, 22}, {12, 96}, {304, 118}, {8, 168}, {306, 186}};
const int kTree[][2] = {{2, 78}, {6, 150}, {292, 64}, {286, 132}, {296, 188}};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildWorld();
    art_.bake(sys.vdp, hedge_.data(), ex_, ey_, mx_, my_, sx_, sy_);
    sys.vdp.B.enabled = false;
    resetRun();
    mode_ = bot_ ? Mode::Play : Mode::Title;
    sys.vdp.A.enabled = mode_ != Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (toneLeft_ > 0 && --toneLeft_ == 0) quiet();
    if (bump_ > 0) bump_--;

    const gs::Pad& pad = sys.pad;
    bool action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);

    if (mode_ == Mode::Title) {
        if (back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || action) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (start || action)) begin();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && (start || back)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        quiet();
    } else if (mode_ == Mode::Play) {
        updateMotion();
        if (mode_ == Mode::Play) readMove();
    } else if (mode_ == Mode::Lift) {
        updateLift();
    }
    draw();
}

void Game::buildWorld() {
    hedge_.assign(size_t(MW * MH), 1);
    sx_ = sy_ = ex_ = ey_ = mx_ = my_ = 1;
    for (int y = 0; y < MH; y++) {
        for (int x = 0; x < MW; x++) {
            char c = kMap[y][x];
            hedge_[size_t(y * MW + x)] = c == '#' ? 1 : 0;
            if (c == 'S') {
                sx_ = x;
                sy_ = y;
            } else if (c == 'C') {
                mx_ = x;
                my_ = y;
            } else if (c == 'G') {
                ex_ = x;
                ey_ = y;
            }
        }
    }
    buildRoute();
}

void Game::buildRoute() {
    route_.clear();
    const int N = MW * MH;
    std::vector<int> parent(size_t(N), -1);
    std::vector<char> seen(size_t(N), 0);
    std::vector<int> q;
    int s = sy_ * MW + sx_;
    int e = my_ * MW + mx_;
    q.push_back(s);
    seen[size_t(s)] = 1;
    parent[size_t(s)] = s;
    const int dir[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (size_t qi = 0; qi < q.size(); qi++) {
        int cur = q[qi];
        if (cur == e) break;
        int x = cur % MW;
        int y = cur / MW;
        for (int k = 0; k < 4; k++) {
            int nx = x + dir[k][0];
            int ny = y + dir[k][1];
            if (blocked(nx, ny)) continue;
            int ni = ny * MW + nx;
            if (seen[size_t(ni)]) continue;
            seen[size_t(ni)] = 1;
            parent[size_t(ni)] = cur;
            q.push_back(ni);
        }
    }
    if (!seen[size_t(e)]) return;
    std::vector<int> rev;
    for (int c = e; c != s; c = parent[size_t(c)]) rev.push_back(c);
    for (int i = int(rev.size()) - 1; i >= 0; --i) route_.push_back(rev[size_t(i)]);
}

bool Game::blocked(int x, int y) const {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return hedge_[size_t(y * MW + x)] != 0;
}

void Game::resetRun() {
    cx_ = tx_ = sx_;
    cy_ = ty_ = sy_;
    moving_ = false;
    flip_ = false;
    face_ = 0;
    steps_ = 0;
    leaves_ = 0;
    tick_ = 0;
    bump_ = 0;
    liftT_ = 0;
    wait_ = 0;
    routeAt_ = 0;
    won_ = false;
    over_ = false;
    lifted_ = false;
}

void Game::toTitle() {
    resetRun();
    mode_ = Mode::Title;
    quiet();
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    blip(523.f, 0.05f, 6);
}

void Game::startLift() {
    if (mode_ != Mode::Play || lifted_ || moving_) return;
    if (cx_ != mx_ || cy_ != my_) return;
    mode_ = Mode::Lift;
    liftT_ = 0;
    blip(698.f, 0.06f, 10);
}

void Game::finish() {
    lifted_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    chord();
}

void Game::fail() {
    won_ = false;
    lifted_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    blip(146.f, 0.07f, 24);
    sys_->apu.tone(1, 98.f, 0.05f);
}

void Game::faceDir(int dx, int dy) {
    if (dx < 0) {
        face_ = 2;
        flip_ = true;
    } else if (dx > 0) {
        face_ = 2;
        flip_ = false;
    } else if (dy < 0) {
        face_ = 1;
        flip_ = false;
    } else if (dy > 0) {
        face_ = 0;
        flip_ = false;
    }
}

bool Game::tryStep(int nx, int ny) {
    int dx = nx - cx_;
    int dy = ny - cy_;
    if (dx * dx + dy * dy != 1) return false;
    if (blocked(nx, ny)) return false;
    faceDir(dx, dy);
    tx_ = nx;
    ty_ = ny;
    moving_ = true;
    tick_ = 0;
    blip((steps_ & 1) ? 392.f : 440.f, 0.04f, 3);
    return true;
}

void Game::updateMotion() {
    if (!moving_) return;
    tick_++;
    if (tick_ < kStep) return;
    cx_ = tx_;
    cy_ = ty_;
    moving_ = false;
    tick_ = 0;
    steps_++;
    if (cx_ == mx_ && cy_ == my_) blip(784.f, 0.05f, 8);
    if (cx_ == ex_ && cy_ == ey_ && !lifted_) {
        leaves_++;
        blip(110.f, 0.06f, 12);
        if (leaves_ >= 3) fail();
    }
}

void Game::updateLift() {
    if (liftT_ < kLift) liftT_++;
    if (liftT_ == kLift / 2) blip(880.f, 0.05f, 6);
    if (liftT_ >= kLift) finish();
}

void Game::readMove() {
    if (moving_) return;
    if (bot_) {
        if (cx_ == mx_ && cy_ == my_) {
            if (wait_ < kWait) {
                wait_++;
                return;
            }
            startLift();
            return;
        }
        if (routeAt_ >= int(route_.size())) return;
        int n = route_[size_t(routeAt_)];
        if (tryStep(n % MW, n / MW)) routeAt_++;
        return;
    }
    const gs::Pad& pad = sys_->pad;
    if ((pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) && cx_ == mx_ && cy_ == my_) {
        startLift();
        return;
    }
    int dx = 0, dy = 0;
    if (pad.down(gs::BTN_LEFT) || pad.axisX < -0.45f) dx = -1;
    else if (pad.down(gs::BTN_RIGHT) || pad.axisX > 0.45f) dx = 1;
    else if (pad.down(gs::BTN_UP) || pad.axisY > 0.45f) dy = -1;
    else if (pad.down(gs::BTN_DOWN) || pad.axisY < -0.45f) dy = 1;
    if (!dx && !dy) return;
    if (!tryStep(cx_ + dx, cy_ + dy)) {
        faceDir(dx, dy);
        if (bump_ == 0) {
            sys_->apu.noiseBurst(0.14f, 640.f, 0.05f);
            bump_ = 14;
        }
    }
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (mode_ == Mode::Title) {
            int u = y < 150 ? y : 150;
            c = gs::rgb4(1, 1 + u / 50, 5 + u / 40);
        } else if (y < 28) {
            c = gs::rgb4(1, 2, 6);
        } else if (y < OY + MH * CELL) {
            c = gs::rgb4(1, 2, 3);
        } else {
            c = gs::rgb4(1, 1, 2);
        }
        sys_->vdp.lineBackdrop[y] = c;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::pulse() {
    int hot = ((sys_->frame / 8) & 1) ? 15 : 12;
    int gate = ((sys_->frame / 10) & 1) ? 15 : 11;
    gs::VDP& vdp = sys_->vdp;
    vdp.setColor(PAL_MAZE * 16 + 12, gs::rgb4(15, hot, 3));
    vdp.setColor(PAL_MAZE * 16 + 15, gs::rgb4(8, gate, 15));
    vdp.setColor(PAL_BODY * 16 + 10, gs::rgb4(15, hot, 4));
    vdp.setColor(PAL_COIN * 16 + 2, gs::rgb4(15, hot, 3));
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.enabled = mode_ != Mode::Title;
    vdp.B.enabled = false;
    backdrop();
    pulse();
    if (mode_ == Mode::Title) drawTitle();
    else drawWorld();
}

void Game::drawTitle() {
    const int cardX = (gs::SCREEN_W - art_.card.w) / 2;
    const int cardY = 34;
    auto spot = [&](int cellX, int cellY, int& sx, int& sy) {
        int bx = cellX * CELL + CELL / 2;
        int by = cellY * CELL + CELL / 2;
        sx = cardX + bx * art_.card.w / (MW * CELL);
        sy = cardY + by * art_.card.h / (MH * CELL);
    };
    int px, py, cx, cy;
    spot(sx_, sy_, px, py);
    spot(mx_, my_, cx, cy);
    int bob = (sys_->frame / 8) & 1;
    // Earlier sprites sit on top of later ones.
    put(art_.logo, (gs::SCREEN_W - art_.logo.w) / 2, 4, art_.logo.w, art_.logo.h, PAL_LOGO, false, false);
    put(art_.coin, cx - 6, cy - 6 - bob, 12, 12, PAL_COIN, false, false);
    put(art_.body[0][0], px - 6, py - 16, 12, 18, PAL_BODY, false, false);
    put(art_.card, cardX, cardY, art_.card.w, art_.card.h, PAL_MAZE, false, false);
    put(art_.moon, 292, 8, art_.moon.w, art_.moon.h, PAL_MOON, false, false);
    for (auto t : kTree) put(art_.tree, t[0], t[1], 18, 26, PAL_TREE, false, false);
    for (auto s : kStar) {
        int tw = ((sys_->frame / 12) & 1) ? 3 : 2;
        put(art_.star, s[0], s[1], tw, tw, PAL_MOON, false, false);
    }

    hudC(17, "A SHORT MAZE", PAL_INK);
    hudC(18, "THE COIN IS THE MARK", PAL_GOLD);
    hudC(20, "STAND ON IT AND LIFT", PAL_INK);
    hudC(21, "THE GATE DOES NOT END IT", PAL_INK);
    hudC(23, "THE THIRD GATE LEAVES IT OPEN", PAL_ALERT);
    hudC(25, "ARROWS WALK    Z LIFTS", PAL_INK);
    hudC(27, "PRESS ENTER", PAL_GOLD);
    hud(1, 27, "V" S3_VERSION, PAL_INK);
}

void Game::drawWorld() {
    if (mode_ == Mode::Win)
        put(art_.finished, (gs::SCREEN_W - art_.finished.w) / 2, 182, art_.finished.w, art_.finished.h, PAL_LOGO, false,
            false);
    else if (mode_ == Mode::Lose)
        put(art_.open, (gs::SCREEN_W - art_.open.w) / 2, 182, art_.open.w, art_.open.h, PAL_ALERT, false, false);

    int pixX = cx_ * CELL;
    int pixY = cy_ * CELL;
    if (moving_) {
        pixX += (tx_ - cx_) * CELL * tick_ / kStep;
        pixY += (ty_ - cy_) * CELL * tick_ / kStep;
    }
    int bob = (moving_ && (tick_ & 4)) ? -1 : 0;
    int shake = (bump_ > 8) ? ((bump_ & 1) ? 1 : -1) : 0;
    int fr = (moving_ && (tick_ & 4)) ? 1 : 0;
    int px = OX + pixX + shake;
    int py = OY + pixY + CELL - art_.body[0][0].h + bob;
    bool ring = (mode_ == Mode::Play && cx_ == mx_ && cy_ == my_ && !moving_) || mode_ == Mode::Lift;
    int rise = (mode_ == Mode::Lift || mode_ == Mode::Win) ? liftT_ * 18 / kLift : 0;
    int coinX = OX + mx_ * CELL + (CELL - art_.coin.w) / 2;
    int coinY = OY + my_ * CELL + (CELL - art_.coin.h) / 2 - rise;
    if (!lifted_ && rise == 0) coinY += (sys_->frame / 10) & 1;

    if (!lifted_ || mode_ == Mode::Win) put(art_.coin, coinX, coinY, art_.coin.w, art_.coin.h, PAL_COIN, false, false);
    if (ring) {
        float spin = sys_->frame * 0.18f;
        for (int i = 0; i < 6; i++) {
            float a = spin + float(i) * kPi / 3.f;
            int dx = int(std::cos(a) * 9.f);
            int dy = int(std::sin(a) * 9.f);
            put(art_.dot, coinX + 5 + dx, coinY + 5 + dy, 3, 3, PAL_COIN, false, false);
        }
    }
    put(art_.body[face_][fr], px, py, art_.body[face_][fr].w, art_.body[face_][fr].h, PAL_BODY, flip_, false);
    put(art_.shadow, px + 2, py + 20, art_.shadow.w, art_.shadow.h, PAL_BODY, false, true);

    put(art_.moon, 296, 10, art_.moon.w, art_.moon.h, PAL_MOON, false, false);
    for (auto t : kTree) put(art_.tree, t[0], t[1], art_.tree.w, art_.tree.h, PAL_TREE, false, false);
    for (auto s : kStar) put(art_.star, s[0], s[1], 3, 3, PAL_MOON, false, false);
    float ft = sys_->frame * 0.04f;
    put(art_.dot, OX + 70 + int(std::sin(ft) * 18), OY + 18 + int(std::cos(ft * 0.7f) * 6), 3, 3, PAL_COIN, false, false);
    put(art_.dot, OX + 180 + int(std::cos(ft * 0.8f) * 14), OY + 36 + int(std::sin(ft) * 8), 3, 3, PAL_COIN, false, false);

    hud(1, 0, "S3 MAZEMARK", PAL_GOLD);
    char buf[24];
    std::snprintf(buf, sizeof buf, "STEPS %d", steps_ > 999 ? 999 : steps_);
    hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_INK);

    if (mode_ == Mode::Pause) {
        hudC(23, "PAUSED", PAL_GOLD);
        hudC(25, "ENTER RESUMES", PAL_INK);
        hudC(27, "ESC TO THE TITLE", PAL_INK);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(26, "THE COIN IS UP", PAL_GREEN);
        hudC(27, bot_ ? "LEAVE" : "ENTER WALKS AGAIN", PAL_INK);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(26, "NO COIN WAS LIFTED", PAL_INK);
        hudC(27, "ENTER WALKS AGAIN", PAL_GOLD);
        return;
    }

    const char* line = "FIND THE COIN";
    int pal = PAL_INK;
    if (cx_ == mx_ && cy_ == my_ && !moving_) {
        line = "ON THE MARK";
        pal = PAL_GOLD;
    } else if (cx_ == ex_ && cy_ == ey_ && !moving_) {
        line = "THE GATE IS NOT THE JOB";
        pal = PAL_ALERT;
    }
    hudC(23, line, pal);
    if (cx_ == mx_ && cy_ == my_ && !moving_) hudC(25, "Z LIFTS THE MARK", PAL_GREEN);
    else hudC(25, "THE GATE DOES NOT END IT", PAL_INK);
    if (leaves_ > 0) {
        std::snprintf(buf, sizeof buf, "GATE %d OF 3", leaves_ > 3 ? 3 : leaves_);
        hudC(27, buf, PAL_ALERT);
    }
}

void Game::blip(float freq, float vol, int hold) {
    sys_->apu.tone(0, freq, vol);
    toneLeft_ = hold;
}

void Game::chord() {
    sys_->apu.tone(0, 523.25f, 0.07f);
    sys_->apu.tone(1, 659.25f, 0.06f);
    sys_->apu.tone(2, 783.99f, 0.05f);
    toneLeft_ = 48;
}

void Game::quiet() {
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::put(const gs::Image& img, int x, int y, int w, int h, int pal, bool flip, bool shadow) {
    if (w <= 0 || h <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c > 95) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

}  // namespace mazemark
