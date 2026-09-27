#include "game/mazebell.h"

#include <cstdio>
#include <queue>

namespace {
constexpr int kStep = 6;

const char* kRows[mazebell::MH] = {
    "###########", "#S..#...F.#", "###.#.###.#", "#...#...#.#", "#.###.#.#.#",
    "#.#...#...#", "#.#.#####.#", "#...#..P.B#", "###########",
};
}  // namespace

namespace mazebell {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    build();
    rules_ = prove();
    art_.bake(sys.vdp, hedge_.data(), bx_, by_, fx_, fy_, px_, py_);
    sys.vdp.B.enabled = false;
    sys.vdp.A.enabled = true;
    dead_ = 0;
    tryNo_ = 1;
    steps_ = 0;
    won_ = false;
    rung_ = false;
    over_ = false;
    why_ = rules_ ? "OPEN" : "RULES";
    mode_ = bot_ ? Mode::Play : Mode::Title;
    beginTry();
    if (!rules_) {
        over_ = true;
        mode_ = Mode::Over;
    }
}

void Game::build() {
    hedge_.assign(size_t(MW * MH), 1);
    sx_ = sy_ = bx_ = by_ = fx_ = fy_ = px_ = py_ = 1;
    for (int y = 0; y < MH; y++) {
        for (int x = 0; x < MW; x++) {
            char c = kRows[y][x];
            hedge_[size_t(y * MW + x)] = (c == '#') ? 1 : 0;
            if (c == 'S') {
                sx_ = x;
                sy_ = y;
            } else if (c == 'B') {
                bx_ = x;
                by_ = y;
            } else if (c == 'F') {
                fx_ = x;
                fy_ = y;
            } else if (c == 'P') {
                px_ = x;
                py_ = y;
            }
        }
    }
}

bool Game::blocked(int x, int y) const {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return hedge_[size_t(y * MW + x)] != 0;
}

bool Game::deathAt(int x, int y) const { return (x == fx_ && y == fy_) || (x == px_ && y == py_); }

bool Game::routeAround() {
    route_.clear();
    const int N = MW * MH;
    std::vector<int> parent(size_t(N), -1);
    std::vector<char> vis(size_t(N), 0);
    std::queue<int> q;
    int s = sy_ * MW + sx_;
    int e = by_ * MW + bx_;
    q.push(s);
    vis[size_t(s)] = 1;
    parent[size_t(s)] = s;
    const int dir[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!q.empty()) {
        int cur = q.front();
        q.pop();
        if (cur == e) break;
        int x = cur % MW;
        int y = cur / MW;
        for (int k = 0; k < 4; k++) {
            int nx = x + dir[k][0];
            int ny = y + dir[k][1];
            if (blocked(nx, ny) || deathAt(nx, ny)) continue;
            int ni = ny * MW + nx;
            if (vis[size_t(ni)]) continue;
            vis[size_t(ni)] = 1;
            parent[size_t(ni)] = cur;
            q.push(ni);
        }
    }
    if (!vis[size_t(e)]) return false;
    std::vector<int> rev;
    for (int c = e; c != s; c = parent[size_t(c)]) rev.push_back(c);
    for (int i = int(rev.size()) - 1; i >= 0; --i) route_.push_back(rev[size_t(i)]);
    return int(route_.size()) >= 12 && int(route_.size()) <= 48;
}

bool Game::prove() {
    if (blocked(sx_, sy_) || blocked(bx_, by_)) return false;
    if (deathAt(sx_, sy_) || deathAt(bx_, by_)) return false;
    if (!deathAt(fx_, fy_) || !deathAt(px_, py_)) return false;
    if (fx_ == px_ && fy_ == py_) return false;
    if (!routeAround()) return false;
    for (int cell : route_) {
        int x = cell % MW;
        int y = cell / MW;
        if (deathAt(x, y)) return false;
    }
    int last = route_.back();
    return last == by_ * MW + bx_;
}

void Game::beginTry() {
    cx_ = tx_ = sx_;
    cy_ = ty_ = sy_;
    moving_ = false;
    tick_ = 0;
    face_ = 0;
    flip_ = false;
    routeAt_ = 0;
    hold_ = 0;
    tryNo_ = dead_ + 1;
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

void Game::arrive() {
    cx_ = tx_;
    cy_ = ty_;
    moving_ = false;
    tick_ = 0;
    steps_++;
    if (cx_ == bx_ && cy_ == by_) ring();
    else if (deathAt(cx_, cy_)) dieTry();
}

void Game::ring() {
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    why_ = "BELL";
    mode_ = Mode::Ring;
    hold_ = bot_ ? 36 : 90;
    blip(880);
}

void Game::dieTry() {
    dead_++;
    tryNo_ = dead_;
    blip(140);
    if (dead_ >= 3) {
        won_ = false;
        rung_ = false;
        why_ = "THIRD";
        mode_ = Mode::Over;
        over_ = true;
        hold_ = 0;
        return;
    }
    why_ = "DIED";
    mode_ = Mode::Dead;
    hold_ = bot_ ? 10 : 40;
}

void Game::readMove() {
    if (moving_ || mode_ != Mode::Play) return;
    int dx = 0, dy = 0;
    if (bot_) {
        if (routeAt_ >= int(route_.size())) return;
        int n = route_[size_t(routeAt_)];
        dx = (n % MW) - cx_;
        dy = (n / MW) - cy_;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) dx = -1;
        else if (pad.down(gs::BTN_RIGHT)) dx = 1;
        else if (pad.down(gs::BTN_UP)) dy = -1;
        else if (pad.down(gs::BTN_DOWN)) dy = 1;
    }
    if (!dx && !dy) return;
    if (dx && dy) return;
    int nx = cx_ + dx;
    int ny = cy_ + dy;
    faceDir(dx, dy);
    if (blocked(nx, ny)) {
        blip(90);
        return;
    }
    tx_ = nx;
    ty_ = ny;
    moving_ = true;
    tick_ = 0;
    if (bot_) routeAt_++;
    blip(420);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (toneLeft_ > 0 && --toneLeft_ == 0) hush();
    bellPh_++;
    if (mode_ == Mode::Title) {
        if (sys.pad.anyPressed()) {
            mode_ = Mode::Play;
            beginTry();
            blip(520);
        }
    } else if (mode_ == Mode::Play) {
        if (moving_) {
            tick_++;
            if (tick_ >= kStep) arrive();
        }
        readMove();
    } else if (mode_ == Mode::Dead) {
        if (--hold_ <= 0) {
            mode_ = Mode::Play;
            beginTry();
        }
    } else if (mode_ == Mode::Ring) {
        if (--hold_ <= 0) {
            over_ = true;
            mode_ = Mode::Over;
        }
        if ((bellPh_ % 14) == 0) blip(660.f + float((bellPh_ / 14) % 3) * 80.f);
    } else if (mode_ == Mode::Over && !bot_) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            dead_ = 0;
            steps_ = 0;
            won_ = false;
            rung_ = false;
            over_ = false;
            why_ = "OPEN";
            mode_ = Mode::Title;
            beginTry();
        }
    }
    draw();
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    toneLeft_ = 5;
}

void Game::hush() {
    if (sys_) sys_->apu.tone(0, 0, 0);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (int i = 0; s[i]; i++) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c <= 32 || c > 95) continue;
        int x = col + i;
        if (x < 0 || x > 39 || row < 0 || row > 27) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.fontBase + (c - 32), pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::put(const gs::Image& img, int x, int y, int pal, bool flip, bool shadow) {
    if (!sys_ || img.w <= 0 || img.h <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::center(const gs::Image& img, int y, int pal) {
    put(img, (gs::SCREEN_W - img.w) / 2, y, pal);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int n = y < 28 ? 1 : (y < OY ? 2 : 3);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(n, n + 1, n + 3);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::drawTitle() {
    sys_->vdp.A.enabled = false;
    center(art_.poster, 28, PAL_TITLE);
    center(art_.wordTitle, 116, PAL_INK);
    center(art_.wordRule, 148, PAL_GOLD);
    center(art_.wordMove, 168, PAL_DIM);
    center(art_.wordGo, 190, PAL_INK);
}

void Game::draw() {
    if (!sys_) return;
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    backdrop();
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    sys_->vdp.A.enabled = true;
    int ox = 0, oy = 0;
    if (moving_) {
        ox = (tx_ - cx_) * tick_ * CELL / kStep;
        oy = (ty_ - cy_) * tick_ * CELL / kStep;
    }
    int px = OX + cx_ * CELL + ox;
    int py = OY + cy_ * CELL + oy - 8;
    int swing = (mode_ == Mode::Ring) ? ((bellPh_ / 3) % 5) - 2 : 0;
    put(art_.shadow, px + 2, OY + cy_ * CELL + oy + 12, PAL_SHADE, false, true);
    put(art_.bell, OX + bx_ * CELL + 1 + swing, OY + by_ * CELL, PAL_BELL);
    put(art_.fake, OX + fx_ * CELL + 1, OY + fy_ * CELL, PAL_FAKE);
    int stride = moving_ && ((tick_ / 3) & 1);
    put(art_.body[face_][stride], px, py, PAL_YOU, flip_);

    char line[40];
    std::snprintf(line, sizeof(line), "TRY %d OF 3", tryNo_ < 1 ? 1 : (tryNo_ > 3 ? 3 : tryNo_));
    hud(1, 1, line, PAL_INK);
    std::snprintf(line, sizeof(line), "DEAD %d", dead_);
    hud(30, 1, line, dead_ ? PAL_RED : PAL_DIM);
    if (mode_ == Mode::Ring || (won_ && rung_)) hudC(26, "THE BELL RINGS", PAL_GOLD);
    else if (mode_ == Mode::Dead) hudC(26, "THAT TRY DIED", PAL_RED);
    else if (mode_ == Mode::Over && !won_) hudC(26, "THIRD TRY DIED", PAL_RED);
    else hudC(26, "FIND THE BRIGHT BELL", PAL_INK);
}

}  // namespace mazebell
