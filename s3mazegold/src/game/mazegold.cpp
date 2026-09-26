#include "game/mazegold.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <queue>

namespace mazegold {
namespace {
constexpr int kStep = 6;
constexpr int kMask = 32;

const char* kMap[MH] = {
    "###############", "#S..#.....#C..#", "###.#.###.###.#", "#...#.#.#.....#", "#.###.#.#####.#", "#.#G..#...#...#",
    "#.###.#.#.#.###", "#.#...#.#C#.#G#", "#.#.###.###.#.#", "#...#G.......E#", "###############",
};

const int kStarX[] = {8, 22, 12, 28, 292, 306, 284, 300};
const int kStarY[] = {36, 78, 120, 168, 40, 88, 132, 176};
}  // namespace

void Game::tally(int mask, int& g, int& c) const {
    g = c = 0;
    for (int i = 0; i < kCoins; i++) {
        if ((mask & (1 << i)) == 0) continue;
        if (coins_[i].gold) g++;
        else c++;
    }
}

int Game::nextMask(int mask, int x, int y) const {
    for (int i = 0; i < kCoins; i++) {
        if (coins_[i].x != x || coins_[i].y != y) continue;
        if (mask & (1 << i)) return mask;
        if (coins_[i].gold) return mask | (1 << i);
        int g = 0, c = 0;
        tally(mask, g, c);
        if (g * 2 + c + 1 < kLine) return mask | (1 << i);
        return mask;
    }
    return mask;
}

bool Game::canLeave() const {
    return finisherGold_ && gold_ >= 1 && score() >= kLine && gold_ + cream_ < kLine;
}

void Game::recount() {
    tally(got_, gold_, cream_);
}

bool Game::blocked(int x, int y) const {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return hedge_[size_t(y * MW + x)] != 0;
}

void Game::buildWorld() {
    hedge_.assign(size_t(MW * MH), 1);
    int n = 0;
    sx_ = 1;
    sy_ = 1;
    ex_ = MW - 2;
    ey_ = MH - 2;
    for (int y = 0; y < MH; y++) {
        for (int x = 0; x < MW; x++) {
            char ch = kMap[y][x];
            if (ch == '#') continue;
            hedge_[size_t(y * MW + x)] = 0;
            if (ch == 'S') {
                sx_ = x;
                sy_ = y;
            } else if (ch == 'E') {
                ex_ = x;
                ey_ = y;
            } else if ((ch == 'G' || ch == 'C') && n < kCoins) {
                coins_[n].x = x;
                coins_[n].y = y;
                coins_[n].gold = ch == 'G';
                n++;
            }
        }
    }
    if (!planRoute()) say_ = "NO PATH";
}

bool Game::planRoute() {
    route_.clear();
    const int N = MW * MH * kMask;
    std::vector<int> parent(size_t(N), -1);
    std::vector<char> seen(size_t(N), 0);
    auto key = [](int x, int y, int mask) { return (y * MW + x) * kMask + mask; };
    auto winning = [&](int mask) {
        int g = 0, c = 0;
        tally(mask, g, c);
        return g >= 1 && c >= 1 && g * 2 + c >= kLine && g + c < kLine;
    };
    const int start = key(sx_, sy_, 0);
    std::queue<int> q;
    q.push(start);
    seen[size_t(start)] = 1;
    parent[size_t(start)] = start;
    int goal = -1;
    const int dir[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!q.empty()) {
        int k = q.front();
        q.pop();
        int mask = k % kMask;
        int cell = k / kMask;
        int x = cell % MW;
        int y = cell / MW;
        if (x == ex_ && y == ey_ && winning(mask)) {
            goal = k;
            break;
        }
        for (int d = 0; d < 4; d++) {
            int nx = x + dir[d][0];
            int ny = y + dir[d][1];
            if (blocked(nx, ny)) continue;
            int nmask = nextMask(mask, nx, ny);
            int nk = key(nx, ny, nmask);
            if (seen[size_t(nk)]) continue;
            seen[size_t(nk)] = 1;
            parent[size_t(nk)] = k;
            q.push(nk);
        }
    }
    if (goal < 0) return false;
    int guard = 0;
    for (int k = goal; k != start && guard < N; k = parent[size_t(k)], guard++) {
        if (k < 0) {
            route_.clear();
            return false;
        }
        route_.push_back(k / kMask);
    }
    if (guard >= N) {
        route_.clear();
        return false;
    }
    std::reverse(route_.begin(), route_.end());

    int mask = 0;
    int g = 0, c = 0;
    bool fin = false;
    int x = sx_, y = sy_;
    for (int cell : route_) {
        int nx = cell % MW;
        int ny = cell / MW;
        if (std::abs(nx - x) + std::abs(ny - y) != 1) {
            route_.clear();
            return false;
        }
        int sc0 = g * 2 + c;
        int nm = nextMask(mask, nx, ny);
        if (nm != mask) {
            bool tookGold = false;
            int bit = nm ^ mask;
            for (int i = 0; i < kCoins; i++)
                if (bit & (1 << i)) tookGold = coins_[i].gold;
            mask = nm;
            tally(mask, g, c);
            if (tookGold && sc0 < kLine && g * 2 + c >= kLine) fin = true;
        }
        x = nx;
        y = ny;
    }
    if (!(x == ex_ && y == ey_ && fin && g >= 1 && c >= 1 && g * 2 + c >= kLine && g + c < kLine)) {
        route_.clear();
        return false;
    }
    return true;
}

void Game::clearRun() {
    cx_ = tx_ = sx_;
    cy_ = ty_ = sy_;
    moving_ = false;
    tick_ = 0;
    steps_ = 0;
    bump_ = 0;
    face_ = 0;
    flip_ = false;
    gold_ = cream_ = got_ = 0;
    finisherGold_ = false;
    won_ = left_ = over_ = false;
    routeAt_ = 0;
    say_ = "TOLL 6. GOLD COUNTS 2";
    toneLeft_ = 0;
}

void Game::resetRun() {
    clearRun();
    mode_ = Mode::Play;
    hush();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildWorld();
    int coinX[kCoins], coinY[kCoins];
    for (int i = 0; i < kCoins; i++) {
        coinX[i] = coins_[i].x;
        coinY[i] = coins_[i].y;
    }
    art_.bake(sys.vdp, hedge_.data(), sx_, sy_, ex_, ey_, coinX, coinY, kCoins);
    sys.vdp.B.enabled = false;
    sys.vdp.A.scroll(0, 0);
    sys.vdp.HUD.scroll(0, 0);
    clearRun();
    mode_ = Mode::Title;
    say_ = "ONLY THE GOLD COUNTS DOUBLE";
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (toneLeft_ > 0 && --toneLeft_ == 0) hush();
    if (bump_ > 0) --bump_;
    if (mode_ == Mode::Title) {
        bool go = bot_ ? sys.frame >= 16 : sys.pad.anyPressed();
        if (go) {
            mode_ = Mode::Play;
            say_ = "TOLL 6. GOLD COUNTS 2";
            blip(440.f);
        }
    }
    if (mode_ == Mode::Play) {
        updateMotion();
        readMove();
    } else if (mode_ == Mode::Won) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) resetRun();
    }
    draw();
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
    if (std::abs(dx) + std::abs(dy) != 1) return false;
    if (blocked(nx, ny)) return false;
    faceDir(dx, dy);
    tx_ = nx;
    ty_ = ny;
    moving_ = true;
    tick_ = 0;
    blip(196.f);
    return true;
}

void Game::readMove() {
    if (moving_ || mode_ != Mode::Play) return;
    if (bot_) {
        if (routeAt_ >= int(route_.size())) return;
        int n = route_[size_t(routeAt_)];
        if (tryStep(n % MW, n / MW)) routeAt_++;
        return;
    }
    const gs::Pad& pad = sys_->pad;
    int dx = 0, dy = 0;
    if (pad.down(gs::BTN_LEFT) || pad.axisX < -0.45f) dx = -1;
    else if (pad.down(gs::BTN_RIGHT) || pad.axisX > 0.45f) dx = 1;
    else if (pad.down(gs::BTN_UP) || pad.axisY > 0.45f) dy = -1;
    else if (pad.down(gs::BTN_DOWN) || pad.axisY < -0.45f) dy = 1;
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

void Game::arrive() {
    steps_++;
    int before = score();
    int mask = nextMask(got_, cx_, cy_);
    if (mask != got_) {
        bool tookGold = false;
        int bit = mask ^ got_;
        for (int i = 0; i < kCoins; i++)
            if (bit & (1 << i)) tookGold = coins_[i].gold;
        got_ = mask;
        recount();
        if (tookGold) {
            if (before < kLine && score() >= kLine) finisherGold_ = true;
            say_ = canLeave() ? "GATE OPEN. STEP OUT" : "GOLD COUNTS 2";
            chord(523.f, 659.f, 784.f, 12);
            if (sys_) sys_->setLight(255, 190, 40);
        } else {
            say_ = canLeave() ? "GATE OPEN. STEP OUT" : "CREAM COUNTS 1";
            blip(392.f);
        }
    } else {
        for (int i = 0; i < kCoins; i++) {
            if (coins_[i].x != cx_ || coins_[i].y != cy_) continue;
            if (got_ & (1 << i)) break;
            if (!coins_[i].gold) {
                say_ = "CREAM DOES NOT BUY IT";
                thud();
            }
            break;
        }
    }
    if (cx_ == ex_ && cy_ == ey_) {
        if (canLeave()) {
            won_ = true;
            left_ = true;
            over_ = true;
            mode_ = Mode::Won;
            say_ = "LEFT. ONLY THE GOLD COUNTED DOUBLE";
            chord(392.f, 523.f, 784.f, 36);
            if (sys_) {
                sys_->rumble(0.4f, 0.75f, 180);
                sys_->setLight(255, 200, 60);
            }
        } else {
            say_ = "STAY. THE DOUBLE IS NOT PAID";
            thud();
        }
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();
    glow();
    vdp.A.enabled = mode_ != Mode::Title;
    if (mode_ == Mode::Title) drawTitle();
    else drawPlay();
}

void Game::drawTitle() {
    center(art_.titleName, 6, 6);
    center(art_.titleRule, 28, 6);
    center(art_.titleLeave, 42, 6);
    put(art_.titlePic, (gs::SCREEN_W - art_.titlePic.w) / 2, 56, 5, false);
    center(art_.titleMove, gs::SCREEN_H - art_.titleMove.h - 10, 6);
    put(art_.moon, 292, 8, 8, false);
    for (int i = 0; i < 8; i++) {
        if (((sys_->frame / 8) + i) % 4 == 0) continue;
        put(art_.star, kStarX[i], kStarY[i], 8, false);
    }
}

void Game::drawPlay() {
    tileText(1, 0, "S3 MAZE GOLD");
    char buf[48];
    std::snprintf(buf, sizeof buf, "STEPS %d", steps_ > 999 ? 999 : steps_);
    int len = int(std::strlen(buf));
    tileText(39 - len, 0, buf);
    std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d  TOLL %d", gold_, cream_, score(), kLine);
    tileText(1, 1, buf);
    tileText(1, 26, say_);
    if (mode_ == Mode::Won && !bot_) tileText(1, 27, "ENTER AGAIN");

    int pixX = cx_ * CELL;
    int pixY = cy_ * CELL;
    if (moving_) {
        pixX += (tx_ - cx_) * CELL * tick_ / kStep;
        pixY += (ty_ - cy_) * CELL * tick_ / kStep;
    }
    int bob = (moving_ && (tick_ & 2)) ? -1 : 0;
    int shake = (bump_ > 8) ? ((bump_ & 1) ? 1 : -1) : 0;
    int px = OX + pixX + shake;
    int py = OY + pixY + CELL - 24 + bob;
    int fr = (moving_ && (tick_ & 2)) ? 1 : 0;
    put(art_.body[face_][fr], px, py, 1, flip_);
    put(art_.shadow, px + 2, py + 20, 1, false, true);

    for (int i = 0; i < kCoins; i++) {
        if (got_ & (1 << i)) continue;
        int hop = int((sys_->frame / 10 + i) & 1);
        int x = OX + coins_[i].x * CELL + 2;
        int y = OY + coins_[i].y * CELL + 2 - hop;
        put(coins_[i].gold ? art_.goldCoin : art_.creamCoin, x, y, coins_[i].gold ? 2 : 3, false);
    }
    if (!canLeave()) {
        put(art_.bars, OX + ex_ * CELL, OY + ey_ * CELL - 4, 4, false);
    } else {
        int lift = int((sys_->frame / 8) & 1);
        put(art_.lamp, OX + ex_ * CELL + 2, OY + ey_ * CELL - 8 - lift, 2, false);
    }
    put(art_.moon, 294, 32, 8, false);
    for (int i = 0; i < 8; i++) {
        if (((sys_->frame / 10) + i) % 5 == 0) continue;
        put(art_.star, kStarX[i], kStarY[i], 8, false);
    }
}

void Game::backdrop() {
    bool open = canLeave();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (mode_ == Mode::Title) c = y < 140 ? gs::rgb4(1, 1, 4) : gs::rgb4(1, 2, 2);
        else if (y < 24) c = gs::rgb4(1, 1, 3);
        else if (y >= 200) c = open ? gs::rgb4(5, 4, 1) : gs::rgb4(1, 1, 2);
        else c = gs::rgb4(0, 1, 2);
        sys_->vdp.lineBackdrop[y] = c;
    }
}

void Game::glow() {
    gs::VDP& vdp = sys_->vdp;
    bool open = canLeave();
    int t = int((sys_->frame / (open ? 4 : 10)) & 3);
    int g = open ? 12 + t : 6 + t;
    vdp.setColor(11, gs::rgb4(15, g, 3));
    vdp.setColor(12, gs::rgb4(15, 15, open ? 12 : 6));
    int flame = 11 + int((sys_->frame / 3) & 3);
    vdp.setColor(1 * 16 + 7, gs::rgb4(15, flame, 4));
    int glint = 12 + int((sys_->frame / 6) & 3);
    vdp.setColor(2 * 16 + 3, gs::rgb4(15, glint, 5));
}

void Game::tileText(int col, int row, const char* s) {
    if (col < 0) col = 0;
    for (int i = 0; s[i]; i++) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if (c <= 32 || c > 95) continue;
        sys_->vdp.HUD.set(col + i, row, gs::entry(art_.fontBase + (c - 32), 7));
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

void Game::center(const gs::Image& img, int y, int pal) { put(img, (gs::SCREEN_W - int(img.w)) / 2, y, pal, false); }

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.05f);
    if (toneLeft_ < 4) toneLeft_ = 4;
}

void Game::chord(float a, float b, float c, int hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.08f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    if (toneLeft_ < hold) toneLeft_ = hold;
}

void Game::thud() {
    if (!sys_) return;
    sys_->apu.noiseBurst(0.14f, 640.f, 0.05f);
}

void Game::hush() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

}  // namespace mazegold
