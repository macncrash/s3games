#include "game/mazechime.h"

#include <cstdio>
#include <queue>

namespace {
constexpr int kStep = 6;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 72;

const char* kRows[mazechime::MH] = {
    "#############",
    "#S..........#",
    "#.###.#####.#",
    "#.#...#...#.#",
    "#.#.###.#.#.#",
    "#.#.#...#.#.#",
    "#.#.#.###.#.#",
    "#...#.....#E#",
    "#############",
};
}  // namespace

namespace mazechime {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    build();
    rules_ = prove();
    art_.bake(sys.vdp, hedge_.data(), ex_, ey_);
    sys.vdp.B.enabled = false;
    sys.vdp.A.enabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    steps_ = 0;
    won_ = false;
    over_ = false;
    chimed_ = false;
    why_ = rules_ ? "OPEN" : "RULES";
    mode_ = bot_ ? Mode::Play : Mode::Title;
    beginRun();
    if (!rules_) {
        over_ = true;
        mode_ = Mode::Over;
    }
}

void Game::build() {
    hedge_.assign(size_t(MW * MH), 1);
    sx_ = sy_ = ex_ = ey_ = 1;
    for (int y = 0; y < MH; y++) {
        for (int x = 0; x < MW; x++) {
            char c = kRows[y][x];
            hedge_[size_t(y * MW + x)] = (c == '#') ? 1 : 0;
            if (c == 'S') {
                sx_ = x;
                sy_ = y;
            } else if (c == 'E') {
                ex_ = x;
                ey_ = y;
            }
        }
    }
}

bool Game::blocked(int x, int y) const {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return hedge_[size_t(y * MW + x)] != 0;
}

bool Game::gateShut(int x, int y) const { return x == ex_ && y == ey_ && !onHour(); }

bool Game::prove() {
    if (blocked(sx_, sy_) || blocked(ex_, ey_)) return false;
    if (sx_ == ex_ && sy_ == ey_) return false;
    route_.clear();
    const int N = MW * MH;
    std::vector<int> parent(size_t(N), -1);
    std::vector<char> vis(size_t(N), 0);
    std::queue<int> q;
    int s = sy_ * MW + sx_;
    int e = ey_ * MW + ex_;
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
            if (blocked(nx, ny)) continue;
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
    return int(route_.size()) >= 8 && int(route_.size()) <= 40;
}

void Game::beginRun() {
    cx_ = tx_ = sx_;
    cy_ = ty_ = sy_;
    moving_ = false;
    tick_ = 0;
    face_ = 0;
    flip_ = false;
    routeAt_ = 0;
    hold_ = 0;
    playFrames_ = 0;
    lastSec_ = -1;
    strikes_ = 0;
    steps_ = 0;
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

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    h = t / 3600;
    m = (t / 60) % 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    split(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    split(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    split(h, m, s);
    return s;
}

void Game::arrive() {
    cx_ = tx_;
    cy_ = ty_;
    moving_ = false;
    tick_ = 0;
    steps_++;
    if (cx_ == ex_ && cy_ == ey_) {
        if (onHour()) leave();
        else fail("EARLY");
    }
}

void Game::leave() {
    won_ = true;
    chimed_ = true;
    why_ = "CHIME";
    mode_ = Mode::Chime;
    hold_ = bot_ ? 28 : 80;
    strikes_ = 0;
    blip(880);
}

void Game::fail(const char* why) {
    won_ = false;
    why_ = why;
    mode_ = Mode::Fail;
    hold_ = bot_ ? 8 : 50;
    blip(110);
}

void Game::readMove() {
    if (moving_ || mode_ != Mode::Play) return;
    int dx = 0, dy = 0;
    if (bot_) {
        if (routeAt_ >= int(route_.size())) return;
        int n = route_[size_t(routeAt_)];
        int nx = n % MW;
        int ny = n / MW;
        if (nx == ex_ && ny == ey_ && !onHour()) return;
        dx = nx - cx_;
        dy = ny - cy_;
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
    if (blocked(nx, ny) || gateShut(nx, ny)) {
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
            beginRun();
            blip(520);
        }
    } else if (mode_ == Mode::Play) {
        if (moving_) {
            tick_++;
            if (tick_ >= kStep) arrive();
        } else {
            readMove();
        }
        if (mode_ == Mode::Play) {
            int prev = clockSec();
            playFrames_++;
            int sec = clockSec();
            if (sec != prev && sec != lastSec_) {
                lastSec_ = sec;
                if (onHour() && !chimed_) {
                    chimed_ = true;
                    blip(740);
                } else if (!onHour()) {
                    blip(sec >= kHourSec - 8 ? 520.f : 280.f);
                }
            }
            if (pastHour()) fail("LATE");
        }
    } else if (mode_ == Mode::Chime) {
        if ((bellPh_ % 10) == 0 && strikes_ < 4) {
            strikes_++;
            blip(520.f + float(strikes_) * 70.f);
        }
        if (--hold_ <= 0) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Fail) {
        if (--hold_ <= 0) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over && !bot_) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            won_ = false;
            over_ = false;
            chimed_ = false;
            why_ = "OPEN";
            mode_ = Mode::Title;
            beginRun();
        }
    }
    draw();
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.06f);
    toneLeft_ = 4;
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

void Game::center(const gs::Image& img, int y, int pal) { put(img, (gs::SCREEN_W - img.w) / 2, y, pal); }

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int n = y < 36 ? 1 : 2;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(n, n + 1, n + 4);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::drawTitle() {
    sys_->vdp.A.enabled = false;
    center(art_.poster, 24, PAL_TITLE);
    center(art_.wordTitle, 108, PAL_INK);
    center(art_.wordRule, 140, PAL_GOLD);
    center(art_.wordMove, 164, PAL_DIM);
    center(art_.wordGo, 188, PAL_INK);
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
    int swing = (mode_ == Mode::Chime || onHour()) ? ((bellPh_ / 3) % 5) - 2 : 0;
    put(art_.body[face_][moving_ && ((tick_ / 3) & 1)], px, py, PAL_YOU, flip_);
    put(art_.gate, OX + ex_ * CELL + 1 + swing, OY + ey_ * CELL, onHour() ? PAL_GOLD : PAL_GATE);
    put(art_.shadow, px + 2, OY + cy_ * CELL + oy + 12, PAL_SHADE, false, true);
    put(art_.clock, 8, 2, PAL_CLOCK);

    int h, m, s;
    split(h, m, s);
    char line[40];
    std::snprintf(line, sizeof(line), "%d:%02d:%02d", h, m, s);
    hud(6, 1, line, onHour() ? PAL_GOLD : PAL_INK);
    std::snprintf(line, sizeof(line), "STEP %d", steps_);
    hud(30, 1, line, PAL_DIM);
    if (mode_ == Mode::Chime || (won_ && over_)) hudC(26, "THE HOUR CHIMES", PAL_GOLD);
    else if (mode_ == Mode::Fail || (over_ && !won_)) hudC(26, why_ && why_[0] == 'L' ? "THE HOUR PASSED" : "TOO SOON", PAL_RED);
    else if (onHour()) hudC(26, "THE GATE IS OPEN", PAL_GOLD);
    else hudC(26, "WAIT FOR THE HOUR", PAL_INK);
}

}  // namespace mazechime
