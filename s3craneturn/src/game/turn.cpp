#include "game/turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace craneturn {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float ROAD_HALF = 5.6f;
constexpr float FOCAL = 220.f;
constexpr float CAM_H = 5.4f;
constexpr int HOR = 96;
constexpr float FINISH = 590.f;
constexpr float CLOCK0 = 72.f;
constexpr float STEER = 7.2f;
constexpr int MAP_N = 780;

struct Seg {
    float a, b, curv;
    int id;
};

const Seg kSegs[] = {
    {0, 100, 0, 0},
    {100, 175, 0.0132f, 1},
    {175, 265, 0, 0},
    {265, 340, -0.0132f, 2},
    {340, 430, 0, 0},
    {430, 505, 0.0132f, 3},
    {505, 640, 0, 0},
};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bake();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.clear();
    if (bot_) begin();
    else {
        mode_ = Mode::Title;
        banner_ = "THREE TURNS";
        s_ = 8;
        v_ = 0;
        boom_ = 0.78f;
    }
}

void Game::bake() {
    float h = 0, x = 0, y = 0;
    for (int i = 0; i < MAP_N; i++) {
        wx_[i] = x;
        wy_[i] = y;
        hd_[i] = h;
        float c = curvAt(float(i));
        h += c;
        x += std::sin(h);
        y += std::cos(h);
    }
}

void Game::sample(float s, float& x, float& y, float& h) const {
    s = std::clamp(s, 0.f, float(MAP_N - 2));
    int i = int(s);
    float f = s - float(i);
    x = wx_[i] + (wx_[i + 1] - wx_[i]) * f;
    y = wy_[i] + (wy_[i + 1] - wy_[i]) * f;
    h = hd_[i] + (hd_[i + 1] - hd_[i]) * f;
}

float Game::curvAt(float s) const {
    for (const Seg& g : kSegs)
        if (s >= g.a && s < g.b) return g.curv;
    return 0;
}

int Game::turnExit(float prev, float now) {
    int n = 0;
    for (const Seg& seg : kSegs) {
        if (seg.id && prev < seg.b && now >= seg.b && !got_[seg.id]) {
            if (std::fabs(x_) < 0.96f && std::fabs(lean_) < 0.98f) {
                got_[seg.id] = true;
                n++;
            }
        }
    }
    return n;
}

void Game::begin() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    tipped_ = false;
    turns_ = 0;
    got_[1] = got_[2] = got_[3] = false;
    s_ = 6;
    v_ = 0;
    x_ = 0;
    xvel_ = 0;
    boom_ = 0.78f;
    lean_ = 0;
    clock_ = CLOCK0;
    banner_ = "TUCK THE BOOM";
    why_ = "";
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Run) {
        float stick = 0, thr = 0, brk = 0, tuck = 0;
        if (bot_) {
            float c = curvAt(s_ + 12.f);
            bool turn = std::fabs(c) > 0.004f;
            float want = boom_ > 0.12f ? 5.5f : turn ? 7.6f : 15.4f;
            if (std::fabs(x_) > 0.45f) want = std::min(want, 6.5f);
            if (v_ < want - 0.25f) thr = 1;
            else if (v_ > want + 0.35f) brk = 1;
            float ax = c * v_ * v_ / ROAD_HALF;
            stick = std::clamp((-ax - xvel_ * 2.4f - x_ * 4.2f) / STEER, -1.f, 1.f);
            tuck = boom_ > 0.02f ? 1.f : 0.f;
        } else {
            if (p.down(gs::BTN_LEFT)) stick -= 1;
            if (p.down(gs::BTN_RIGHT)) stick += 1;
            if (std::fabs(p.axisX) > 0.15f) stick = p.axisX;
            if (p.down(gs::BTN_A) || p.accel > 0.1f) thr = p.accel > 0.1f ? p.accel : 1.f;
            if (p.down(gs::BTN_B) || p.brake > 0.1f) brk = p.brake > 0.1f ? p.brake : 1.f;
            if (p.down(gs::BTN_UP)) tuck = 1;
            if (p.down(gs::BTN_DOWN)) tuck = -1;
        }
        physics(DT, stick, thr, brk, tuck);
    } else if (p.pressed(gs::BTN_START) && !bot_) {
        begin();
    }
    draw();
    float hum = mode_ == Mode::Run ? 0.08f + v_ * 0.004f : 0.03f;
    sys.apu.tone(0, 46.f + v_ * 3.1f, hum);
    if (tipped_) sys.apu.tone(1, 70.f, 0.2f);
    else sys.apu.tone(1, 0, 0);
}

void Game::physics(float dt, float stick, float throttle, float brake, float tuck) {
    float prev = s_;
    float c = curvAt(s_);
    if (throttle > 0 && v_ < 18.f) v_ += 8.5f * throttle * dt;
    if (brake > 0) v_ = std::max(0.f, v_ - 16.f * brake * dt);
    v_ = std::max(0.f, v_ - 0.55f * dt);
    s_ += v_ * dt;
    if (s_ > 630.f) s_ = 630.f;

    boom_ = std::clamp(boom_ - tuck * 0.62f * dt, 0.f, 1.f);
    float ax = c * v_ * v_ / ROAD_HALF;
    xvel_ += (ax + stick * STEER) * dt;
    xvel_ *= std::exp(-1.3f * dt);
    x_ += xvel_ * dt;

    float g = c * v_ * v_;
    lean_ = g / 4.1f + boom_ * (0.42f + std::fabs(g) * 0.42f + v_ * 0.012f) + x_ * 0.12f;

    int made = turnExit(prev, s_);
    if (made) {
        turns_ += made;
        banner_ = turns_ >= 3 ? "YARD CLEAR" : "TURN MADE";
    }

    clock_ -= dt;
    if (std::fabs(lean_) > 1.f || std::fabs(x_) > 1.02f) {
        tipped_ = true;
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        why_ = "the crane tipped";
        banner_ = "TIPPED";
        v_ = 0;
        return;
    }
    if (clock_ <= 0.f) {
        clock_ = 0;
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        why_ = "the other crew made the yard";
        banner_ = "CREW WINS";
        return;
    }
    if (s_ >= FINISH && turns_ >= 3) {
        mode_ = Mode::Win;
        over_ = true;
        won_ = true;
        why_ = "three turns clear, the other crew still on the clock";
        banner_ = "THREE TURNS";
        v_ = 0;
    }
}

bool Game::project(float s, float lat, float& sx, float& sy, float& sc) const {
    float cx, cy, ch, px, py, ph;
    sample(s_, cx, cy, ch);
    float rx = std::cos(ch), ry = -std::sin(ch);
    cx += rx * (x_ * ROAD_HALF);
    cy += ry * (x_ * ROAD_HALF);
    sample(s, px, py, ph);
    float prx = std::cos(ph), pry = -std::sin(ph);
    px += prx * lat;
    py += pry * lat;
    float dx = px - cx, dy = py - cy;
    float fwd = dx * std::sin(ch) + dy * std::cos(ch);
    float right = dx * std::cos(ch) - dy * std::sin(ch);
    if (fwd < 5.f || fwd > 120.f) return false;
    sc = FOCAL / fwd;
    sx = 160.f + right * sc;
    sy = float(HOR) + CAM_H * sc;
    return true;
}

void Game::skyRoad() {
    gs::VDP& v = sys_->vdp;
    float camX, camY, ch;
    sample(s_, camX, camY, ch);
    float rx = std::cos(ch), ry = -std::sin(ch);
    camX += rx * (x_ * ROAD_HALF);
    camY += ry * (x_ * ROAD_HALF);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(HOR), 0.f, 1.f);
        int r = 2 + int(6 * u * u);
        int g = 2 + int(3 * u);
        int b = 6 + int(4 * (1.f - u));
        if (y > HOR) {
            r = 2;
            g = 4;
            b = 2;
        }
        if (tipped_ && (int(t_ * 12) & 1)) r = std::min(15, r + 6);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = y <= HOR ? uint8_t((1.f - u) * 4.f) : 0;
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= HOR + 2) continue;
        float fwd = FOCAL * CAM_H / float(y - HOR);
        if (fwd > 110.f || fwd < 4.f) continue;
        float guess = std::clamp(s_ + fwd, 0.f, float(MAP_N - 2));
        float px, py, ph;
        sample(guess, px, py, ph);
        float dx = px - camX, dy = py - camY;
        float f = dx * std::sin(ch) + dy * std::cos(ch);
        if (f < 4.f) continue;
        float right = dx * std::cos(ch) - dy * std::sin(ch);
        L.on = true;
        L.cx = 160.f + right * FOCAL / f;
        L.hw = FOCAL * ROAD_HALF / f;
        L.v = guess * 28.f;
        L.pal = PAL_ROAD;
        L.band = (int(guess) & 1) ? 1 : 0;
        L.style = 1;
        L.left = L.right = gs::GROUND_LAND;
        if (f > 40.f) v.lineFog[y] = uint8_t(std::clamp((f - 40.f) / 8.f, 0.f, 12.f));
    }
}

void Game::blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, bool shadow) {
    if (h < 2.f) return;
    const gs::Image& im = m.pick(h);
    float w = h * (float(im.w) / float(std::max(1, int(im.h))));
    gs::Sprite s;
    s.img = im;
    s.w = int16_t(std::max(1.f, w));
    s.h = int16_t(h);
    s.x = int16_t(cx - w * 0.5f);
    s.y = int16_t(foot - h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    gs::Plane& h = sys_->vdp.HUD;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        if (x < 0 || x > 39 || row < 0 || row > 27) continue;
        unsigned char ch = (unsigned char)s[i];
        if (ch < 32 || ch > 126) ch = '?';
        int t = art_.font[ch - 32];
        h.set(x, row, t ? gs::entry(t, pal) : 0);
    }
}

void Game::hudC(int row, const std::string& s, int pal) {
    int col = 20 - int(s.size()) / 2;
    hud(col, row, s, pal);
}

void Game::draw() {
    skyRoad();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float leanPx = std::clamp(lean_, -1.4f, 1.4f) * 42.f;
    float bob = std::sin(s_ * 0.7f) * 1.2f;
    blit(art_.body, 168.f + leanPx * 0.45f, 196.f + bob, 70.f, PAL_CRANE, false, false);
    blit(art_.wheel, 132.f + leanPx * 0.2f, 200.f + bob, 16.f, PAL_CRANE);
    blit(art_.wheel, 196.f + leanPx * 0.55f, 200.f + bob, 16.f, PAL_CRANE);
    blit(art_.body, 168.f + leanPx * 0.45f, 198.f + bob, 70.f, PAL_CRANE, false, true);
    float boomX = 150.f + leanPx;
    float boomY = 168.f + bob - (1.f - boom_) * 10.f;
    blit(art_.boom, boomX, boomY, 28.f + boom_ * 18.f, PAL_BOOM);
    blit(art_.hook, boomX + 30.f + leanPx * 0.2f, boomY + 8.f + boom_ * 16.f, 14.f + boom_ * 10.f, PAL_BOOM);

    float crewS = (1.f - clock_ / CLOCK0) * FINISH * 1.05f;
    float csx, csy, csc;
    if (project(crewS, 0.f, csx, csy, csc)) blit(art_.crew, csx, csy, 3.2f * csc, PAL_CREW);

    for (const Seg& seg : kSegs) {
        if (!seg.id) continue;
        for (float lat : {-ROAD_HALF - 0.4f, ROAD_HALF + 0.4f}) {
            float sx, sy, sc;
            if (project(seg.a, lat, sx, sy, sc)) blit(art_.pylon, sx, sy, 3.4f * sc, PAL_YARD);
        }
    }
    for (int i = 0; i < 8; i++) {
        float sx, sy, sc;
        if (project(40.f + i * 70.f, (i & 1) ? -8.5f : 8.5f, sx, sy, sc))
            blit(art_.lamp, sx, sy, 4.2f * sc, PAL_YARD);
    }

    char line[48];
    std::snprintf(line, sizeof line, "TURN %d/3", turns_);
    hud(1, 1, line, PAL_INK);
    int sec = int(std::ceil(clock_));
    std::snprintf(line, sizeof line, "CREW %d:%02d", sec / 60, sec % 60);
    hud(28, 1, line, clock_ < 12.f ? PAL_WARN : PAL_INK);
    int leanBar = int(std::fabs(lean_) * 10.f);
    if (leanBar > 10) leanBar = 10;
    std::string bar = "LEAN ";
    for (int i = 0; i < 10; i++) bar += i < leanBar ? '#' : '-';
    hud(1, 26, bar, std::fabs(lean_) > 0.75f ? PAL_WARN : PAL_HUD);
    std::snprintf(line, sizeof line, "BOOM %d", int(boom_ * 100));
    hud(28, 26, line, boom_ > 0.45f ? PAL_WARN : PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(8, "S3 CRANE TURN", PAL_INK);
        hudC(10, "THREE TURNS, NO TIP", PAL_HUD);
        hudC(12, "UP TUCKS THE BOOM", PAL_HUD);
        hudC(14, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(17, "START", PAL_INK);
    } else if (mode_ == Mode::Win) {
        hudC(8, "YARD IS YOURS", PAL_INK);
        hudC(10, "THREE TURNS CLEAR", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(8, banner_, PAL_WARN);
        hudC(10, why_, PAL_HUD);
    } else if (banner_[0]) {
        hudC(8, banner_, PAL_INK);
    }
}

}  // namespace craneturn
