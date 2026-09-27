#include "game/mazeseven.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>

namespace mazeseven {
namespace {

constexpr char kMap[MH][MW + 1] = {
    "###########", "#Y.1.#.3.T#", "#.#.#.#.#.#", "#2..3..1..#", "#.#.###.#.#",
    "#..######.#", "#.#.#.#.#.#", "#3..2..1..#", "###########",
};

static_assert(kMap[1][1] == 'Y' && kMap[1][9] == 'T', "the two starts");
static_assert(kMap[3][4] == '3' && kMap[3][1] == '2' && kMap[7][4] == '2', "your lamps");
static_assert(kMap[1][7] == '3' && kMap[7][1] == '3', "their lamps");
static_assert(3 + 2 + 2 == 7 && 3 + 3 == 6, "first to seven, them still short");

const char* valueName(int v) {
    if (v == 3) return "THREE";
    if (v == 2) return "TWO";
    return "ONE";
}

}  // namespace

bool Game::rules() const {
    return booksOk_ && won_ && over_ && sawShort_ && you_ == 7 && them_ == 6 && nYou_ == 3 && nThem_ == 2 &&
           youTake_[0] == 3 && youTake_[1] == 2 && youTake_[2] == 2 && themTake_[0] == 3 && themTake_[1] == 3 &&
           steps_ >= 20;
}

void Game::buildMap() {
    hedge_.assign(size_t(MW * MH), 1);
    lampN_ = 0;
    yx_ = 1;
    yy_ = 1;
    tx_ = 9;
    ty_ = 1;
    for (int y = 0; y < MH; y++) {
        for (int x = 0; x < MW; x++) {
            char c = kMap[y][x];
            if (c == '#') continue;
            hedge_[size_t(y * MW + x)] = 0;
            if (c == 'Y') {
                yx_ = x;
                yy_ = y;
            } else if (c == 'T') {
                tx_ = x;
                ty_ = y;
            } else if (c >= '1' && c <= '3' && lampN_ < 8) {
                lamps_[lampN_++] = Lamp{x, y, c - '0', true};
            }
        }
    }
    auto id = [&](int x, int y) {
        for (int i = 0; i < lampN_; i++)
            if (lamps_[i].x == x && lamps_[i].y == y) return i;
        return -1;
    };
    youBook_[0] = id(4, 3);
    youBook_[1] = id(1, 3);
    youBook_[2] = id(4, 7);
    themBook_[0] = id(7, 1);
    themBook_[1] = id(1, 7);
    booksOk_ = lampN_ == 8 && yx_ == 1 && yy_ == 1 && tx_ == 9 && ty_ == 1 && youBook_[0] >= 0 &&
               lamps_[youBook_[0]].val == 3 && youBook_[1] >= 0 && lamps_[youBook_[1]].val == 2 &&
               youBook_[2] >= 0 && lamps_[youBook_[2]].val == 2 && themBook_[0] >= 0 &&
               lamps_[themBook_[0]].val == 3 && themBook_[1] >= 0 && lamps_[themBook_[1]].val == 3;
    if (!booksOk_) std::fprintf(stderr, "S3 MAZE SEVEN  map lamps do not match the book\n");
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildMap();
    std::vector<uint8_t> glow(size_t(MW * MH), 0);
    for (int i = 0; i < lampN_; i++) glow[size_t(lamps_[i].y * MW + lamps_[i].x)] = uint8_t(lamps_[i].val);
    art_.bake(sys.vdp, hedge_.data(), glow.data(), yx_, yy_, tx_, ty_);
    sys.vdp.B.enabled = false;
    if (bot_) beginMatch();
    else {
        clearMatch();
        mode_ = Mode::Title;
    }
}

void Game::clearMatch() {
    you_ = them_ = 0;
    nYou_ = nThem_ = 0;
    sawShort_ = false;
    won_ = false;
    over_ = false;
    steps_ = 0;
    hold_ = 0;
    bump_ = 0;
    who_ = Who::You;
    target_ = -1;
    route_.clear();
    routeAt_ = 0;
    for (int i = 0; i < 8; i++) youTake_[i] = themTake_[i] = 0;
    for (int i = 0; i < lampN_; i++) lamps_[i].live = true;
    self_ = Walker{};
    rival_ = Walker{};
    self_.x = self_.tx = yx_;
    self_.y = self_.ty = yy_;
    self_.face = 2;
    rival_.x = rival_.tx = tx_;
    rival_.y = rival_.ty = ty_;
    rival_.face = 2;
    rival_.flip = true;
}

void Game::beginMatch() {
    clearMatch();
    mode_ = Mode::Play;
    beginActor();
    blip(440.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    hush();
    if (bump_ > 0) --bump_;
    if ((mode_ == Mode::Play || mode_ == Mode::Show) && them_ >= 6 && you_ < 7) sawShort_ = true;

    if (mode_ == Mode::Title) {
        const gs::Pad& pad = sys.pad;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) beginMatch();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) beginMatch();
    } else if (mode_ == Mode::Show) {
        if (--hold_ <= 0) {
            who_ = who_ == Who::You ? Who::Them : Who::You;
            mode_ = Mode::Play;
            beginActor();
        }
    } else {
        play();
    }
    draw();
}

bool Game::blocked(int x, int y) const {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return hedge_[size_t(y * MW + x)] != 0;
}

bool Game::pathTo(int sx, int sy, int gx, int gy) {
    route_.clear();
    routeAt_ = 0;
    if (sx == gx && sy == gy) return true;
    const int N = MW * MH;
    std::vector<int> parent(size_t(N), -1);
    std::queue<int> q;
    int s = sy * MW + sx;
    int g = gy * MW + gx;
    q.push(s);
    parent[size_t(s)] = s;
    const int dir[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!q.empty()) {
        int cur = q.front();
        q.pop();
        if (cur == g) break;
        int x = cur % MW;
        int y = cur / MW;
        for (int k = 0; k < 4; k++) {
            int nx = x + dir[k][0];
            int ny = y + dir[k][1];
            if (blocked(nx, ny)) continue;
            int ni = ny * MW + nx;
            if (parent[size_t(ni)] != -1) continue;
            parent[size_t(ni)] = cur;
            q.push(ni);
        }
    }
    if (parent[size_t(g)] == -1) return false;
    std::vector<int> rev;
    for (int c = g; c != s; c = parent[size_t(c)]) rev.push_back(c);
    for (int i = int(rev.size()) - 1; i >= 0; --i) route_.push_back(rev[size_t(i)]);
    return !route_.empty();
}

int Game::nextTarget(bool yours) const {
    const int* book = yours ? youBook_ : themBook_;
    int n = yours ? 3 : 2;
    for (int i = 0; i < n; i++) {
        int id = book[i];
        if (id >= 0 && lamps_[id].live) return id;
    }
    const Walker& w = yours ? self_ : rival_;
    int best = -1;
    int bestV = -1;
    int bestD = 9999;
    for (int i = 0; i < lampN_; i++) {
        if (!lamps_[i].live) continue;
        int d = std::abs(lamps_[i].x - w.x) + std::abs(lamps_[i].y - w.y);
        if (lamps_[i].val > bestV || (lamps_[i].val == bestV && d < bestD)) {
            best = i;
            bestV = lamps_[i].val;
            bestD = d;
        }
    }
    return best;
}

void Game::beginActor() {
    route_.clear();
    routeAt_ = 0;
    target_ = -1;
    if (who_ == Who::You && !bot_) return;
    int id = nextTarget(who_ == Who::You);
    if (id < 0) return;
    const Walker& w = who_ == Who::You ? self_ : rival_;
    if (!pathTo(w.x, w.y, lamps_[id].x, lamps_[id].y)) return;
    target_ = id;
}

int Game::lampAt(int x, int y) const {
    for (int i = 0; i < lampN_; i++)
        if (lamps_[i].live && lamps_[i].x == x && lamps_[i].y == y) return i;
    return -1;
}

void Game::face(Walker& w, int dx, int dy) {
    if (dx < 0) {
        w.face = 2;
        w.flip = true;
    } else if (dx > 0) {
        w.face = 2;
        w.flip = false;
    } else if (dy < 0) {
        w.face = 1;
        w.flip = false;
    } else if (dy > 0) {
        w.face = 0;
        w.flip = false;
    }
}

bool Game::startStep(Walker& w, int nx, int ny) {
    if (std::abs(nx - w.x) + std::abs(ny - w.y) != 1) return false;
    if (blocked(nx, ny)) return false;
    face(w, nx - w.x, ny - w.y);
    w.tx = nx;
    w.ty = ny;
    w.moving = true;
    w.tick = 0;
    blip(who_ == Who::You ? 330.f : 247.f);
    return true;
}

void Game::stepWalker(Walker& w) {
    w.tick++;
    if (w.tick < pace()) return;
    w.x = w.tx;
    w.y = w.ty;
    w.moving = false;
    w.tick = 0;
    steps_++;
}

void Game::play() {
    Walker& w = active();
    if (w.moving) {
        stepWalker(w);
        if (w.moving) return;
    }
    if (who_ == Who::You && !bot_) {
        readHuman();
        return;
    }
    if (routeAt_ < int(route_.size())) {
        int n = route_[size_t(routeAt_)];
        if (startStep(w, n % MW, n / MW)) routeAt_++;
        return;
    }
    if (target_ >= 0 && lamps_[target_].live && lamps_[target_].x == w.x && lamps_[target_].y == w.y) claim(target_);
}

void Game::readHuman() {
    if (self_.moving) return;
    const gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z)) {
        int id = lampAt(self_.x, self_.y);
        if (id >= 0) claim(id);
        else if (bump_ == 0) {
            sys_->apu.noiseBurst(0.12f, 500.f, 0.04f);
            bump_ = 10;
        }
        return;
    }
    int dx = 0;
    int dy = 0;
    if (pad.down(gs::BTN_LEFT)) dx = -1;
    else if (pad.down(gs::BTN_RIGHT)) dx = 1;
    else if (pad.down(gs::BTN_UP)) dy = -1;
    else if (pad.down(gs::BTN_DOWN)) dy = 1;
    if (!dx && !dy) return;
    if (!startStep(self_, self_.x + dx, self_.y + dy)) {
        face(self_, dx, dy);
        if (bump_ == 0) {
            sys_->apu.noiseBurst(0.14f, 700.f, 0.04f);
            bump_ = 12;
        }
    }
}

void Game::claim(int id) {
    if (mode_ != Mode::Play || id < 0 || id >= lampN_ || !lamps_[id].live) return;
    lamps_[id].live = false;
    int v = lamps_[id].val;
    if (who_ == Who::You) {
        you_ += v;
        if (nYou_ < 8) youTake_[nYou_++] = v;
    } else {
        them_ += v;
        if (nThem_ < 8) themTake_[nThem_++] = v;
    }
    if (you_ < 7 && them_ >= 6) sawShort_ = true;
    sys_->apu.tone(0, who_ == Who::You ? 523.25f : 349.23f, 0.06f);
    sys_->apu.tone(1, who_ == Who::You ? 659.25f : 440.f, 0.05f);
    toneLeft_ = 12;
    if (you_ >= 7 && them_ < 7) finish(true);
    else if (them_ >= 7) finish(false);
    else {
        mode_ = Mode::Show;
        hold_ = bot_ ? 8 : 28;
    }
}

void Game::finish(bool win) {
    won_ = win && you_ >= 7 && them_ < 7;
    over_ = true;
    mode_ = won_ ? Mode::Win : Mode::Lose;
    if (won_) {
        sys_->apu.tone(0, 523.25f, 0.07f);
        sys_->apu.tone(1, 659.25f, 0.06f);
        sys_->apu.tone(2, 783.99f, 0.05f);
        toneLeft_ = 40;
    } else {
        sys_->apu.tone(0, 196.f, 0.06f);
        sys_->apu.noiseBurst(0.16f, 240.f, 0.08f);
        toneLeft_ = 24;
    }
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.035f);
    toneLeft_ = 3;
}

void Game::hush() {
    if (!sys_ || toneLeft_ <= 0) return;
    if (--toneLeft_ == 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
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
    s.w = img.w;
    s.h = img.h;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::center(const gs::Image& img, int y, int pal) { put(img, (gs::SCREEN_W - img.w) / 2, y, pal, false); }

void Game::walker(const Walker& w, int pal) {
    int px = w.x * CELL;
    int py = w.y * CELL;
    if (w.moving) {
        int p = pace();
        px += (w.tx - w.x) * CELL * w.tick / p;
        py += (w.ty - w.y) * CELL * w.tick / p;
    }
    int bob = (w.moving && (w.tick & 2)) ? -1 : 0;
    int x = OX + px;
    int y = OY + py + CELL - 24 + bob;
    int fr = (w.moving && (w.tick & 4)) ? 1 : 0;
    put(art_.shadow, x + 2, y + 19, PAL_SHADE, false, true);
    put(art_.body[w.face][fr], x, y, pal, w.flip, false);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (mode_ == Mode::Title) {
            v.lineBackdrop[y] = y < 150 ? gs::rgb4(1, 1, 5) : gs::rgb4(1, 3, 2);
        } else if (y < 30) {
            v.lineBackdrop[y] = gs::rgb4(1, 2, 6);
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 3, 2);
        }
    }
}

void Game::flicker() {
    int hot = (sys_->frame / 8) & 1;
    sys_->vdp.setColor(PAL_LAMP * 16 + 4, gs::rgb4(15, hot ? 12 : 8, 1));
    sys_->vdp.setColor(PAL_LAMP * 16 + 5, gs::rgb4(15, 15, hot ? 12 : 6));
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    flicker();
    v.A.enabled = mode_ != Mode::Title;
    v.B.enabled = false;
    if (mode_ == Mode::Title) drawTitle();
    else drawPlay();
}

void Game::drawTitle() {
    const int stars[][2] = {{12, 8}, {28, 18}, {300, 10}, {286, 22}, {48, 12}};
    for (auto s : stars) put(art_.star, s[0], s[1], PAL_STAR);
    put(art_.moon, 292, 6, PAL_MOON);
    int y = 4;
    center(art_.wordTitle, y, PAL_GOLD);
    y += art_.wordTitle.h + 2;
    center(art_.wordFirst, y, PAL_INK);
    y += art_.wordFirst.h + 3;
    put(art_.poster, (gs::SCREEN_W - art_.poster.w) / 2, y, PAL_TITLE);
    y += art_.poster.h + 4;
    center(art_.wordOne, y, PAL_GOLD);
    y += art_.wordOne.h + 2;
    center(art_.wordSix, y, PAL_INK);
    y += art_.wordSix.h + 2;
    center(art_.wordMove, y, PAL_INK);
    y += art_.wordMove.h + 1;
    center(art_.wordTake, y, PAL_GREEN);
    center(art_.wordEnter, gs::SCREEN_H - art_.wordEnter.h - 4, PAL_GOLD);
}

void Game::drawPlay() {
    const int stars[][2] = {{8, 6}, {304, 8}, {292, 18}};
    for (auto s : stars) put(art_.star, s[0], s[1], PAL_STAR);

    if (who_ == Who::You) {
        walker(self_, PAL_YOU);
        walker(rival_, PAL_THEM);
    } else {
        walker(rival_, PAL_THEM);
        walker(self_, PAL_YOU);
    }

    int glow = -1;
    if (mode_ == Mode::Play) glow = (who_ == Who::You && !bot_) ? nextTarget(true) : target_;
    int bob = ((sys_->frame / 8) & 1) ? -1 : 0;
    for (int i = 0; i < lampN_; i++) {
        if (!lamps_[i].live) continue;
        int ly = OY + lamps_[i].y * CELL + (i == glow ? bob : 0);
        put(art_.lamp[lamps_[i].val - 1], OX + lamps_[i].x * CELL, ly, PAL_LAMP);
    }

    for (int i = 0; i < 7; i++) {
        put(art_.pip, 28, 48 + i * 16, i < you_ ? PAL_GOLD : PAL_DIM);
        put(art_.pip, 284, 48 + i * 16, i < them_ ? PAL_RED : PAL_DIM);
    }
    put(art_.wordYou, 26, 36, PAL_GOLD);
    put(art_.wordThem, 274, 36, PAL_RED);

    const gs::Image* line = nullptr;
    int linePal = PAL_INK;
    if (mode_ == Mode::Win) {
        line = &art_.sayWin;
        linePal = PAL_GOLD;
    } else if (mode_ == Mode::Lose) {
        line = &art_.sayLose;
        linePal = PAL_RED;
    } else if (you_ < 7 && (you_ >= 6 || them_ >= 6)) {
        line = &art_.sayShort;
        linePal = PAL_GOLD;
    } else if (you_ == 0 && them_ == 0) {
        line = &art_.sayPiece;
        linePal = PAL_INK;
    }
    if (line) center(*line, 184, linePal);

    char buf[40];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(1, 0, "S3 MAZE SEVEN", PAL_GOLD);
    hud(1, 1, buf, PAL_GREEN);
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(39 - int(std::strlen(buf)), 1, buf, PAL_RED);
    hudC(2, "FIRST TO SEVEN", you_ >= 7 ? PAL_GOLD : PAL_INK);

    if (mode_ == Mode::Play && who_ == Who::You) {
        int id = nextTarget(true);
        if (id >= 0) {
            int v = lamps_[id].val;
            if (you_ < 7 && you_ + v >= 7) std::snprintf(buf, sizeof buf, "TAKE THE LAST %s", valueName(v));
            else std::snprintf(buf, sizeof buf, "TAKE THE %s", valueName(v));
            hudC(25, buf, PAL_GOLD);
        }
    } else if (mode_ == Mode::Play && who_ == Who::Them) {
        hudC(25, "THEIR LAMP", PAL_RED);
    } else if (mode_ == Mode::Win) {
        hudC(25, "ENTER AGAIN", PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        hudC(25, "ENTER AGAIN", PAL_RED);
    }
    if (!bot_) hudC(26, "ARROWS WALK   Z TAKES", PAL_INK);
}

}  // namespace mazeseven
