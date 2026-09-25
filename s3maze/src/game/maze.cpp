#include "game/maze.h"

#include <cstdio>
#include <cstring>
#include <queue>

namespace {
int iabs(int v) { return v < 0 ? -v : v; }
constexpr int kStep = 8;
}  // namespace

namespace maze {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildWorld();
    art_.bake(sys.vdp, hedge_.data(), ex_, ey_, sx_, sy_, decoyX_, decoyY_);
    sys.vdp.B.enabled = false;
    cx_ = tx_ = sx_;
    cy_ = ty_ = sy_;
    moving_ = false;
    tick_ = 0;
    steps_ = 0;
    routeAt_ = 0;
    face_ = 0;
    flip_ = false;
    won_ = false;
    over_ = false;
    mode_ = bot_ ? Mode::Play : Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bot_ && mode_ == Mode::Title) mode_ = Mode::Play;
    if (toneLeft_ > 0 && --toneLeft_ == 0) quiet();
    if (bump_ > 0) --bump_;
    if (mode_ == Mode::Title) {
        if (sys.pad.anyPressed()) {
            mode_ = Mode::Play;
            blip();
        }
    } else if (mode_ == Mode::Won) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) resetRun();
    } else {
        updateMotion();
        if (mode_ == Mode::Play) readMove();
    }
    draw();
}

void Game::buildWorld() {
    for (uint32_t n = 1; n <= 400; n++) {
        rng_ = 0x4D415A45u ^ (n * 0x01000193u);
        carve();
        pickDecoy();
        if (decoyX_ < 0) continue;
        if (!buildRoute()) continue;
        if (route_.size() < 26 || route_.size() > 64) continue;
        return;
    }
    rng_ = 0x4D415A45u;
    carve();
    pickDecoy();
    buildRoute();
}

void Game::carve() {
    hedge_.assign(size_t(MW * MH), 1);
    sx_ = 1;
    sy_ = 1;
    ex_ = MW - 2;
    ey_ = MH - 2;
    hedge_[size_t(sy_ * MW + sx_)] = 0;
    std::vector<char> seen(size_t(COLS * ROWS), 0);
    seen[0] = 1;
    struct Cell {
        int x, y;
    };
    std::vector<Cell> stack;
    stack.push_back({0, 0});
    const int dir[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!stack.empty()) {
        Cell c = stack.back();
        int order[4] = {0, 1, 2, 3};
        for (int i = 3; i > 0; --i) {
            int j = rnd(i + 1);
            int tmp = order[i];
            order[i] = order[j];
            order[j] = tmp;
        }
        bool moved = false;
        for (int k = 0; k < 4; k++) {
            int nx = c.x + dir[order[k]][0];
            int ny = c.y + dir[order[k]][1];
            if (nx < 0 || ny < 0 || nx >= COLS || ny >= ROWS) continue;
            if (seen[size_t(ny * COLS + nx)]) continue;
            seen[size_t(ny * COLS + nx)] = 1;
            int wx = c.x * 2 + 1 + dir[order[k]][0];
            int wy = c.y * 2 + 1 + dir[order[k]][1];
            hedge_[size_t(wy * MW + wx)] = 0;
            hedge_[size_t((ny * 2 + 1) * MW + (nx * 2 + 1))] = 0;
            stack.push_back({nx, ny});
            moved = true;
            break;
        }
        if (!moved) stack.pop_back();
    }
    hedge_[size_t(ey_ * MW + ex_)] = 0;
}

void Game::pickDecoy() {
    decoyX_ = decoyY_ = -1;
    int best = -1;
    for (int y = 1; y < MH - 1; y++) {
        for (int x = 1; x < MW - 1; x++) {
            if (blocked(x, y)) continue;
            if ((x == sx_ && y == sy_) || (x == ex_ && y == ey_)) continue;
            if (openCount(x, y) != 1) continue;
            int fromExit = iabs(x - ex_) + iabs(y - ey_);
            int fromStart = iabs(x - sx_) + iabs(y - sy_);
            if (fromExit < 6 || fromStart < 4) continue;
            if (fromExit > best) {
                best = fromExit;
                decoyX_ = x;
                decoyY_ = y;
            }
        }
    }
}

bool Game::buildRoute() {
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
    return !route_.empty();
}

int Game::rnd(int n) {
    rng_ = rng_ * 1664525u + 1013904223u;
    return int(rng_ % uint32_t(n));
}

bool Game::blocked(int x, int y) const {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return hedge_[size_t(y * MW + x)] != 0;
}

int Game::openCount(int x, int y) const {
    int n = 0;
    if (!blocked(x + 1, y)) n++;
    if (!blocked(x - 1, y)) n++;
    if (!blocked(x, y + 1)) n++;
    if (!blocked(x, y - 1)) n++;
    return n;
}

bool Game::tryStep(int nx, int ny) {
    int dx = nx - cx_;
    int dy = ny - cy_;
    if (iabs(dx) + iabs(dy) != 1) return false;
    if (blocked(nx, ny)) return false;
    faceDir(dx, dy);
    tx_ = nx;
    ty_ = ny;
    moving_ = true;
    tick_ = 0;
    blip();
    return true;
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

void Game::readMove() {
    if (moving_) return;
    if (bot_) {
        if (routeAt_ >= int(route_.size())) return;
        int n = route_[size_t(routeAt_)];
        if (tryStep(n % MW, n / MW)) routeAt_++;
        return;
    }
    const gs::Pad& pad = sys_->pad;
    int dx = 0, dy = 0;
    if (pad.down(gs::BTN_LEFT)) dx = -1;
    else if (pad.down(gs::BTN_RIGHT)) dx = 1;
    else if (pad.down(gs::BTN_UP)) dy = -1;
    else if (pad.down(gs::BTN_DOWN)) dy = 1;
    if (!dx && !dy) return;
    if (!tryStep(cx_ + dx, cy_ + dy)) {
        faceDir(dx, dy);
        if (bump_ == 0) {
            thud();
            bump_ = 16;
        }
    }
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
    // The shut gate and every other cell leave the run open. Only the exit ends it.
    if (cx_ == ex_ && cy_ == ey_) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Won;
        chime();
    }
}

void Game::resetRun() {
    cx_ = tx_ = sx_;
    cy_ = ty_ = sy_;
    moving_ = false;
    tick_ = 0;
    steps_ = 0;
    bump_ = 0;
    routeAt_ = 0;
    face_ = 0;
    flip_ = false;
    won_ = false;
    over_ = false;
    mode_ = Mode::Play;
    quiet();
    toneLeft_ = 0;
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();
    glow();
    vdp.A.enabled = mode_ != Mode::Title;
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    tileText(1, 0, "S3 MAZE");
    char buf[16];
    int shown = steps_ > 999 ? 999 : steps_;
    std::snprintf(buf, sizeof buf, "STEPS %3d", shown);
    tileText(40 - int(std::strlen(buf)), 0, buf);
    if (mode_ == Mode::Won && !bot_) tileText(14, 1, "ENTER AGAIN");

    int pixX = cx_ * CELL;
    int pixY = cy_ * CELL;
    if (moving_) {
        pixX += (tx_ - cx_) * CELL * tick_ / kStep;
        pixY += (ty_ - cy_) * CELL * tick_ / kStep;
    }
    int bob = (moving_ && (tick_ & 4)) ? -1 : 0;
    int shake = (bump_ > 10) ? ((bump_ & 1) ? 1 : -1) : 0;
    int px = OX + pixX + shake;
    int py = OY + pixY + CELL - 24 + bob;
    int fr = (moving_ && (tick_ & 4)) ? 1 : 0;
    put(art_.body[face_][fr], px, py, 1, flip_, false);
    put(art_.shadow, px + 1, py + 19, 1, false, true);
    int wing = int((sys_->frame / 12) & 1);
    int perched = OY - 8 + int((sys_->frame / 18) & 1);
    put(art_.bird[wing], OX + 5 * CELL, perched, 4, false, false);

    const gs::Image* msg = &art_.sayFind;
    if (mode_ == Mode::Won) msg = &art_.sayOut;
    else if (!moving_ && cx_ == decoyX_ && cy_ == decoyY_) msg = &art_.sayShut;
    else if (steps_ == 0) msg = &art_.sayOnly;
    center(*msg, gs::SCREEN_H - msg->h - 6, 2);
}

void Game::drawTitle() {
    int y = 4;
    center(art_.titleName, y, 2);
    y += art_.titleName.h + 2;
    center(art_.titleSub, y, 2);
    y += art_.titleSub.h + 4;
    put(art_.titlePic, (gs::SCREEN_W - art_.titlePic.w) / 2, y, 3, false, false);
    y += art_.titlePic.h + 6;
    center(art_.titleRule, y, 2);
    y += art_.titleRule.h + 3;
    center(art_.titleMove, y, 2);
    center(art_.titleGo, gs::SCREEN_H - art_.titleGo.h - 6, 2);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (mode_ == Mode::Title) c = y < 150 ? gs::rgb4(3, 7, 11) : gs::rgb4(1, 4, 3);
        else if (y < 16 || y >= 200) c = gs::rgb4(1, 2, 1);
        else c = gs::rgb4(1, 4, 2);
        sys_->vdp.lineBackdrop[y] = c;
    }
}

void Game::glow() {
    int hot = ((sys_->frame / 12) & 1) ? 15 : 11;
    gs::VDP& vdp = sys_->vdp;
    vdp.setColor(14, gs::rgb4(15, hot, 3));
    vdp.setColor(15, gs::rgb4(15, 15, 10));
    vdp.setColor(3 * 16 + 9, gs::rgb4(15, hot, 4));
    vdp.setColor(1 * 16 + 10, gs::rgb4(15, hot, 3));
    vdp.setColor(1 * 16 + 11, gs::rgb4(15, 15, hot));
}

void Game::tileText(int col, int row, const char* s) {
    for (int i = 0; s[i]; i++) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c <= 32 || c > 95) continue;
        sys_->vdp.HUD.set(col + i, row, gs::entry(art_.fontBase + (c - 32), 2));
    }
}

void Game::put(const gs::Image& img, int x, int y, int pal, bool flip, bool shadow) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = img.w;
    s.h = img.h;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::center(const gs::Image& img, int y, int pal) { put(img, (gs::SCREEN_W - img.w) / 2, y, pal, false, false); }

void Game::blip() {
    sys_->apu.tone(0, 330.f, 0.045f);
    toneLeft_ = 3;
}

void Game::thud() { sys_->apu.noiseBurst(0.16f, 700.f, 0.05f); }

void Game::chime() {
    sys_->apu.tone(0, 523.25f, 0.07f);
    sys_->apu.tone(1, 659.25f, 0.06f);
    sys_->apu.tone(2, 783.99f, 0.05f);
    toneLeft_ = 36;
}

void Game::quiet() {
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

}  // namespace maze
