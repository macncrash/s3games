#include "game/mazetape.h"

#include <cstdio>
#include <cstring>
#include <queue>

namespace {
int iabs(int v) { return v < 0 ? -v : v; }
constexpr int kStep = 6;
}  // namespace

namespace mazetape {

bool Game::matched() const {
    if (nDrawer_ != 3) return false;
    bool seen[3] = {};
    for (int i = 0; i < 3; i++) {
        int k = drawer_[i];
        if (!tapeKind(k) || seen[k]) return false;
        seen[k] = true;
    }
    return true;
}

bool Game::held(int i) const {
    if (i < 0 || i > 2) return false;
    for (int d = 0; d < nDrawer_; d++)
        if (drawer_[d] == i) return true;
    return false;
}

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < nDrawer_; i++) s += kindPay(drawer_[i]);
    return s;
}

const char* Game::modeName() const {
    if (mode_ == Mode::Title) return "title";
    if (mode_ == Mode::Won) return "out";
    return "play";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildWorld();
    rules_ = audit();
    art_.bake(sys.vdp, hedge_.data(), ex_, ey_, sx_, sy_);
    sys.vdp.B.enabled = false;
    cx_ = tx_ = sx_;
    cy_ = ty_ = sy_;
    moving_ = false;
    tick_ = 0;
    steps_ = 0;
    routeAt_ = 0;
    nDrawer_ = 0;
    traps_ = 0;
    face_ = 0;
    flip_ = false;
    won_ = false;
    over_ = false;
    left_ = false;
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
            blip(440.f);
        }
    } else if (mode_ == Mode::Play) {
        updateMotion();
        if (!moving_ && !bot_ && sys.pad.pressed(gs::BTN_B)) dropLast();
        if (mode_ == Mode::Play) readMove();
    }
    draw();
}

void Game::buildWorld() {
    for (uint32_t n = 1; n <= 80; n++) {
        rng_ = 0x4D5A5450u ^ (n * 0x01000193u);
        carve();
        if (!placeSpots()) continue;
        if (!buildRoute()) continue;
        if (route_.size() < 12 || route_.size() > 90) continue;
        reason_ = "ok";
        return;
    }
    reason_ = "no route";
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
            int t = order[i];
            order[i] = order[j];
            order[j] = t;
        }
        bool moved = false;
        for (int k = 0; k < 4; k++) {
            int nx = c.x + dir[order[k]][0];
            int ny = c.y + dir[order[k]][1];
            if (nx < 0 || ny < 0 || nx >= COLS || ny >= ROWS) continue;
            if (seen[size_t(ny * COLS + nx)]) continue;
            seen[size_t(ny * COLS + nx)] = 1;
            hedge_[size_t((c.y * 2 + 1 + dir[order[k]][1]) * MW + (c.x * 2 + 1 + dir[order[k]][0]))] = 0;
            hedge_[size_t((ny * 2 + 1) * MW + (nx * 2 + 1))] = 0;
            stack.push_back({nx, ny});
            moved = true;
            break;
        }
        if (!moved) stack.pop_back();
    }
    hedge_[size_t(ey_ * MW + ex_)] = 0;
}

bool Game::placeSpots() {
    int cells[64];
    int n = 0;
    for (int y = 1; y < MH - 1; y++) {
        for (int x = 1; x < MW - 1; x++) {
            if (wall(x, y)) continue;
            if ((x == sx_ && y == sy_) || (x == ex_ && y == ey_)) continue;
            if (openCount(x, y) != 1) continue;
            cells[n++] = y * MW + x;
        }
    }
    if (n < 6) return false;
    for (int i = n - 1; i > 0; --i) {
        int j = rnd(i + 1);
        int t = cells[i];
        cells[i] = cells[j];
        cells[j] = t;
    }
    for (int i = 0; i < 6; i++) {
        spots_[i].x = cells[i] % MW;
        spots_[i].y = cells[i] / MW;
        spots_[i].kind = i;
        spots_[i].taken = false;
    }
    return true;
}

bool Game::path(int sx, int sy, int ex, int ey, std::vector<int>& out) const {
    const int N = MW * MH;
    std::vector<int> parent(size_t(N), -1);
    std::vector<char> vis(size_t(N), 0);
    std::queue<int> q;
    int s = sy * MW + sx;
    int e = ey * MW + ex;
    q.push(s);
    vis[size_t(s)] = 1;
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
            if (wall(nx, ny)) continue;
            if (!(nx == ex && ny == ey) && nx == ex_ && ny == ey_) continue;
            int id = spotAt(nx, ny);
            if (id >= 3) continue;
            int ni = ny * MW + nx;
            if (vis[size_t(ni)]) continue;
            vis[size_t(ni)] = 1;
            parent[size_t(ni)] = cur;
            q.push(ni);
        }
    }
    if (!vis[size_t(e)]) return false;
    std::vector<int> rev;
    for (int c = e; c != s; c = parent[size_t(c)]) {
        if (c < 0) return false;
        rev.push_back(c);
    }
    for (int i = int(rev.size()) - 1; i >= 0; --i) out.push_back(rev[size_t(i)]);
    return true;
}

bool Game::buildRoute() {
    route_.clear();
    int x = sx_, y = sy_;
    int order[4] = {0, 1, 2, -1};
    int tx[4] = {spots_[0].x, spots_[1].x, spots_[2].x, ex_};
    int ty[4] = {spots_[0].y, spots_[1].y, spots_[2].y, ey_};
    for (int i = 0; i < 4; i++) {
        if (!path(x, y, tx[i], ty[i], route_)) return false;
        x = tx[i];
        y = ty[i];
        (void)order;
    }
    return !route_.empty();
}

bool Game::audit() {
    if (std::strcmp(reason_, "ok") != 0) return false;
    if (kindPay(KEY) != 4 || kindPay(BELL) != 6 || kindPay(LAMP) != 8) return false;
    if (kindPay(LOCK) != 4 || kindPay(CHIME) != 6 || kindPay(TORCH) != 8) return false;
    if (twinOf(KEY) != LOCK || twinOf(BELL) != CHIME || twinOf(LAMP) != TORCH) return false;
    for (int i = 0; i < 6; i++) {
        if (wall(spots_[i].x, spots_[i].y)) return false;
        if (spots_[i].kind != i) return false;
        for (int j = i + 1; j < 6; j++)
            if (spots_[i].x == spots_[j].x && spots_[i].y == spots_[j].y) return false;
    }
    return route_.size() >= 12;
}

int Game::rnd(int n) {
    rng_ = rng_ * 1664525u + 1013904223u;
    return int(rng_ % uint32_t(n));
}

bool Game::wall(int x, int y) const {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return hedge_[size_t(y * MW + x)] != 0;
}

bool Game::blocked(int x, int y) const {
    if (wall(x, y)) return true;
    if (x == ex_ && y == ey_ && !matched()) return true;
    return false;
}

int Game::openCount(int x, int y) const {
    int n = 0;
    if (!wall(x + 1, y)) n++;
    if (!wall(x - 1, y)) n++;
    if (!wall(x, y + 1)) n++;
    if (!wall(x, y - 1)) n++;
    return n;
}

int Game::spotAt(int x, int y) const {
    for (int i = 0; i < 6; i++)
        if (!spots_[i].taken && spots_[i].x == x && spots_[i].y == y) return i;
    return -1;
}

bool Game::tryStep(int nx, int ny) {
    if (iabs(nx - cx_) + iabs(ny - cy_) != 1) return false;
    if (blocked(nx, ny)) return false;
    faceDir(nx - cx_, ny - cy_);
    tx_ = nx;
    ty_ = ny;
    moving_ = true;
    tick_ = 0;
    blip(300.f);
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

void Game::arrive() {
    steps_++;
    int id = spotAt(cx_, cy_);
    if (id >= 0 && nDrawer_ < 6) {
        spots_[id].taken = true;
        drawer_[nDrawer_++] = spots_[id].kind;
        if (id >= 3) {
            traps_++;
            thud();
        } else {
            blip(520.f + float(id) * 40.f);
        }
    }
    if (cx_ == ex_ && cy_ == ey_ && matched()) {
        won_ = true;
        left_ = true;
        over_ = true;
        mode_ = Mode::Won;
        chime();
    }
}

void Game::dropLast() {
    if (nDrawer_ <= 0) return;
    int kind = drawer_[--nDrawer_];
    drawer_[nDrawer_] = 0;
    for (int i = 0; i < 6; i++) {
        if (spots_[i].kind == kind && spots_[i].taken) {
            spots_[i].taken = false;
            if (i >= 3 && traps_ > 0) traps_--;
            break;
        }
    }
    blip(180.f);
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
            bump_ = 12;
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
    arrive();
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();
    vdp.A.enabled = mode_ != Mode::Title;
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    tileText(1, 0, "S3 MAZETAPE", 2);
    char buf[40];
    std::snprintf(buf, sizeof buf, "TAPE %s %s %s", kindName(KEY), kindName(BELL), kindName(LAMP));
    tileText(16, 0, buf, 2);
    const char* a = nDrawer_ > 0 ? kindName(drawer_[0]) : "----";
    const char* b = nDrawer_ > 1 ? kindName(drawer_[1]) : "----";
    const char* c = nDrawer_ > 2 ? kindName(drawer_[2]) : "----";
    std::snprintf(buf, sizeof buf, "TILL %s %s %s", a, b, c);
    tileText(1, 1, buf, matched() ? 5 : 2);

    for (int i = 0; i < 6; i++) {
        if (spots_[i].taken) continue;
        int pal = tapeKind(spots_[i].kind) ? 3 : 4;
        put(art_.token[spots_[i].kind], OX + spots_[i].x * CELL + 2, OY + spots_[i].y * CELL + 2, pal, false, false);
    }
    if (!matched()) put(art_.gate, OX + ex_ * CELL + 1, OY + ey_ * CELL + 1, 5, false, false);

    int pixX = cx_ * CELL;
    int pixY = cy_ * CELL;
    if (moving_) {
        pixX += (tx_ - cx_) * CELL * tick_ / kStep;
        pixY += (ty_ - cy_) * CELL * tick_ / kStep;
    }
    int shake = (bump_ > 8) ? ((bump_ & 1) ? 1 : -1) : 0;
    int px = OX + pixX + shake;
    int py = OY + pixY + CELL - 22;
    int fr = (moving_ && (tick_ & 2)) ? 1 : 0;
    put(art_.body[face_][fr], px, py, 1, flip_, false);
    put(art_.shadow, px + 2, py + 18, 1, false, true);

    const gs::Image* msg = &art_.sayFind;
    if (mode_ == Mode::Won) msg = &art_.sayOut;
    else if (matched()) msg = &art_.sayMatch;
    else if (traps_ > 0) msg = &art_.sayWrong;
    center(*msg, gs::SCREEN_H - msg->h - 2, 2);
}

void Game::drawTitle() {
    int y = 8;
    center(art_.titleName, y, 2);
    y += art_.titleName.h + 4;
    center(art_.titleSub, y, 2);
    y += art_.titleSub.h + 6;
    put(art_.titlePic, (gs::SCREEN_W - art_.titlePic.w) / 2, y, 0, false, false);
    y += art_.titlePic.h + 6;
    center(art_.titleRule, y, 2);
    y += art_.titleRule.h + 4;
    center(art_.titleMove, y, 2);
    center(art_.titleGo, gs::SCREEN_H - art_.titleGo.h - 8, 2);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c = (mode_ == Mode::Title) ? (y < 140 ? gs::rgb4(2, 4, 7) : gs::rgb4(1, 3, 2))
                                            : (y < 24 ? gs::rgb4(1, 2, 1) : gs::rgb4(1, 3, 2));
        sys_->vdp.lineBackdrop[y] = c;
    }
}

void Game::tileText(int col, int row, const char* s, int pal) {
    for (int i = 0; s[i]; i++) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c <= 32 || c > 95) continue;
        sys_->vdp.HUD.set(col + i, row, gs::entry(art_.fontBase + (c - 32), pal));
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

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.04f);
    toneLeft_ = 3;
}

void Game::thud() { sys_->apu.noiseBurst(0.14f, 600.f, 0.05f); }

void Game::chime() {
    sys_->apu.tone(0, 523.25f, 0.07f);
    sys_->apu.tone(1, 659.25f, 0.05f);
    sys_->apu.tone(2, 784.f, 0.04f);
    toneLeft_ = 30;
}

void Game::quiet() {
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

}  // namespace mazetape
