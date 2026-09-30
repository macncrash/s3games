#include "game/busturn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace busturn {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZ = 86;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::startRace() {
    corners_[0] = {210.f, 52.f, -1, false};
    corners_[1] = {470.f, 52.f, 1, false};
    corners_[2] = {740.f, 52.f, -1, false};
    turns_ = 0;
    next_ = 0;
    s_ = 0;
    speed_ = 0;
    x_ = 0;
    lean_ = 0;
    steer_ = 0;
    race_ = 0;
    crewLeft_ = kCrew;
    won_ = false;
    over_ = false;
    banner_ = 0;
    mode_ = Mode::Race;
    sys_->apu.tone(0, 70, 0.04f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.clear();
    if (bot_) startRace();
    else mode_ = Mode::Title;
}

float Game::influence(float s, const Corner& c) const {
    float d = std::fabs(s - c.center);
    if (d >= c.half) return 0;
    float u = 1.0f - d / c.half;
    return u * u * (3.0f - 2.0f * u);
}

float Game::bendAt(float s) const {
    float b = 0;
    for (const Corner& c : corners_) b += float(c.dir) * influence(s, c);
    return b;
}

void Game::bot(float& steer, bool& go, bool& stop) const {
    float look = 1.0e9f;
    int dir = 0;
    for (const Corner& c : corners_) {
        if (c.cleared) continue;
        look = c.center - c.half;
        dir = c.dir;
        break;
    }
    float blend = std::fabs(bendAt(s_));
    bool prep = look < 1.0e8f && (look - s_) < 95.0f;
    float target = (prep || blend > 0.08f) ? 14.2f : 29.0f;
    go = speed_ < target - 0.35f;
    stop = speed_ > target + 0.45f;
    float inside = blend > 0.05f ? float(dir) * blend * 0.28f : 0;
    float hold = clampf((inside - x_) * 3.2f, -1.0f, 1.0f);
    if (blend > 0.12f) hold = clampf(hold * 0.35f + float(dir) * 0.92f, -1.0f, 1.0f);
    steer = hold;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 3;
    if (mode_ == Mode::Tip || mode_ == Mode::Late) return 4;
    if (std::fabs(bendAt(s_)) > 0.45f) return 2;
    return 1;
}

void Game::update(float dt) {
    float steer = 0;
    bool go = false, stop = false;
    if (bot_) {
        bot(steer, go, stop);
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) steer -= 1;
        if (p.down(gs::BTN_RIGHT)) steer += 1;
        if (std::fabs(p.axisX) > 0.2f) steer = p.axisX;
        steer = clampf(steer, -1.0f, 1.0f);
        go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.accel > 0.2f;
        stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_X) || p.brake > 0.2f;
    }
    steer_ = steer;
    float acc = 0;
    if (go) acc += 16.5f;
    if (stop) acc -= 32.0f;
    if (!go && !stop) acc -= 3.2f;
    acc -= speed_ * 0.28f;
    speed_ = clampf(speed_ + acc * dt, 0.0f, 33.0f);
    s_ += speed_ * dt;
    float aim = steer * 0.72f;
    x_ += (aim - x_) * std::min(1.0f, dt * (2.0f + speed_ * 0.04f));
    x_ = clampf(x_, -1.25f, 1.25f);

    float bend = bendAt(s_);
    float push = (speed_ / kSafe);
    float centrifugal = -bend * push * push * 0.70f;
    float counter = steer * 0.52f * clampf(speed_ / 9.0f, 0.2f, 1.0f);
    float leanTarget = centrifugal + counter;
    if (std::fabs(x_) > 0.98f) leanTarget += std::copysign((std::fabs(x_) - 0.98f) * speed_ / 14.0f, x_);
    lean_ += (leanTarget - lean_) * std::min(1.0f, dt * 7.0f);

    race_ += dt;
    crewLeft_ = kCrew - race_;

    for (int i = 0; i < 3; i++) {
        Corner& c = corners_[i];
        if (c.cleared) continue;
        if (s_ >= c.center + c.half) {
            c.cleared = true;
            turns_++;
            next_ = turns_;
            sys_->apu.tone(1, 620.0f + float(turns_) * 70.0f, 0.07f);
            banner_ = 0.8f;
        }
    }
    if (banner_ > 0) banner_ -= dt;

    if (std::fabs(lean_) > 1.06f) {
        mode_ = Mode::Tip;
        won_ = false;
        over_ = true;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noiseBurst(0.6f, 500.0f, 0.35f);
        return;
    }
    if (crewLeft_ <= 0 && s_ < kFinish) {
        mode_ = Mode::Late;
        crewLeft_ = 0;
        won_ = false;
        over_ = true;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 140.0f, 0.08f);
        return;
    }
    if (s_ >= kFinish && turns_ >= 3) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 523.0f, 0.09f);
        sys_->apu.tone(2, 784.0f, 0.07f);
    } else {
        sys_->apu.tone(0, 48.0f + speed_ * 3.4f, speed_ > 0.4f ? 0.045f : 0.0f);
    }
}

void Game::sky() {
    const uint16_t top = gs::rgb4(3, 5, 11);
    const uint16_t hor = gs::rgb4(10, 12, 14);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y < HORIZ ? float(y) / float(HORIZ) : 1.0f;
        int r = int(std::lround((1 - u) * ((top >> 8) & 15) + u * ((hor >> 8) & 15)));
        int g = int(std::lround((1 - u) * ((top >> 4) & 15) + u * ((hor >> 4) & 15)));
        int b = int(std::lround((1 - u) * (top & 15) + u * (hor & 15)));
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        int fog = 0;
        if (y > HORIZ - 18 && y < HORIZ + 28) fog = 6 - std::abs(y - HORIZ) / 5;
        sys_->vdp.lineFog[y] = uint8_t(std::max(0, fog));
        sys_->vdp.road[y].on = false;
    }
}

void Game::road() {
    float leanVis = clampf(lean_, -1.1f, 1.1f);
    for (int y = HORIZ; y < gs::SCREEN_H; y++) {
        float p = float(y - HORIZ) / float(gs::SCREEN_H - HORIZ);
        float ahead = s_ + (1.0f - p) * 120.0f + 8.0f;
        float bend = bendAt(ahead);
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = true;
        r.hw = 10.0f + p * 148.0f;
        r.cx = 160.0f + bend * (1.0f - p) * 170.0f - x_ * 108.0f * p - leanVis * 36.0f * p;
        r.v = s_ * 46.0f + (0.04f / std::max(0.03f, p)) * 80.0f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(r.v / 380.0f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = 0;
        r.right = 0;
    }
    sys_->vdp.roadTime = int(s_ * 2.0f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::props() {
    // Farther sprites first so the bus, drawn after, sits on top.
    for (int i = 14; i >= 0; i--) {
        float ps = s_ + 16.0f + float(i) * 18.0f;
        float p = 1.0f - float(i) / 15.0f;
        p = clampf(p, 0.05f, 1.0f);
        float depth = float(i) / 15.0f;
        float y = float(HORIZ) + depth * 118.0f;
        float bend = bendAt(ps);
        float cx = 160.0f + bend * (1.0f - depth) * 150.0f - x_ * 90.0f * depth;
        float side = (i & 1) ? 1.0f : -1.0f;
        float hw = 16.0f + depth * 150.0f;
        float px = cx + side * (hw + 18.0f + float((i * 17) % 11));
        int fog = std::min(12, i / 2);
        bool inBend = std::fabs(bendAt(ps)) > 0.55f;
        if (!inBend && (i % 5 == 0)) spr(art_.tree, px, y, 18.0f + depth * 46.0f, PAL_TOWN, side < 0, fog);
        else if (!inBend) spr(art_.block[(i / 3) % 3], px, y - 6, 22.0f + depth * 58.0f, PAL_TOWN, false, fog);
        else spr(art_.lamp, px, y, 16.0f + depth * 36.0f, PAL_TOWN, false, fog);
    }

    float crewS = (race_ / kCrew) * kFinish;
    float ds = crewS - s_;
    if (ds > 6.0f && ds < 140.0f) {
        float depth = clampf(1.0f - ds / 140.0f, 0.08f, 0.92f);
        float y = float(HORIZ) + (1.0f - depth) * 70.0f + 18.0f;
        float bend = bendAt(crewS);
        float cx = 160.0f + bend * depth * 40.0f - x_ * 40.0f * (1.0f - depth);
        spr(art_.crew, cx, y, 18.0f + (1.0f - depth) * 36.0f, PAL_CREW, false, int(depth * 8));
    }

    if (next_ < 3) {
        const Corner& c = corners_[next_];
        float dist = c.center - s_;
        if (dist > 8.0f && dist < 130.0f && std::fabs(bendAt(s_)) < 0.35f) {
            float depth = clampf(dist / 130.0f, 0.0f, 1.0f);
            float y = float(HORIZ) + (1.0f - depth) * 78.0f + 8.0f;
            float bend = bendAt(c.center - 20.0f);
            float cx = 160.0f + bend * 30.0f + float(c.dir) * (28.0f + depth * 20.0f);
            spr(art_.arrow, cx, y, 16.0f + (1.0f - depth) * 22.0f, PAL_SIGN, c.dir < 0, int(depth * 6));
        }
    }
}

void Game::busSprite() {
    int bank = int(std::lround(clampf(lean_, -1.0f, 1.0f) * 2.0f + 2.0f));
    bank = std::clamp(bank, 0, 4);
    float bob = std::sin(s_ * 0.35f) * 1.4f;
    float cx = 160.0f + lean_ * 26.0f;
    float h = 118.0f;
    spr(art_.bus[bank], cx, 168.0f + bob, h, PAL_BUS, false, 0);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sky();
    if (mode_ == Mode::Title) {
        text("S3 BUSTURN", 160, 48, 1.15f, PAL_HUD, 0);
        text("THREE TURNS", 160, 78, 0.7f, PAL_SIGN, 0);
        text("DO NOT TIP THE BUS", 160, 100, 0.48f, PAL_HUD, 0);
        text("THE CLOCK IS THE OTHER CREW", 160, 118, 0.42f, PAL_CREW, 0);
        spr(art_.bus[2], 160, 168, 100, PAL_BUS, false);
        if (int(t_ * 2) % 2 == 0) hudC(24, "START", PAL_HUD);
        hudC(26, "STEER  THROTTLE  BRAKE", 0);
        return;
    }
    road();
    props();
    busSprite();

    char buf[48];
    std::snprintf(buf, sizeof buf, "TURN %d/3", std::min(3, turns_ + (mode_ == Mode::Race && std::fabs(bendAt(s_)) > 0.2f ? 1 : 0)));
    if (mode_ != Mode::Race) std::snprintf(buf, sizeof buf, "TURN %d/3", turns_);
    hud(1, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "SPD %d", int(std::lround(speed_)));
    hud(1, 2, buf, PAL_HUD);
    int secs = int(std::ceil(std::max(0.0f, crewLeft_)));
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", secs / 60, secs % 60);
    hud(28, 1, buf, crewLeft_ < 12.0f ? PAL_HUD : PAL_HUD);
    // lean bar, 10 cells, centre is calm
    int leanCells = int(std::lround(clampf(lean_, -1.0f, 1.0f) * 5.0f));
    std::string bar = "LEAN ";
    for (int i = -5; i <= 5; i++) bar.push_back(i == 0 ? '|' : (i == leanCells && leanCells != 0 ? '#' : '.'));
    hud(1, 25, bar, std::fabs(lean_) > 0.75f ? PAL_HUD : PAL_HUD);
    // colour the word by poking palette: use red-ish entry 4 when hot
    if (std::fabs(lean_) > 0.75f) hud(1, 25, "LEAN", PAL_HUD);

    if (banner_ > 0 && mode_ == Mode::Race) {
        std::snprintf(buf, sizeof buf, "BEND %d CLEAN", turns_);
        hudC(12, buf, PAL_HUD);
    }
    if (mode_ == Mode::Win) {
        hudC(10, "DEPOT", PAL_HUD);
        hudC(12, "YOU BEAT THE CREW", PAL_HUD);
        std::snprintf(buf, sizeof buf, "CLOCK LEFT %d", secs);
        hudC(14, buf, PAL_HUD);
    } else if (mode_ == Mode::Tip) {
        hudC(11, "TIPPED", PAL_HUD);
        hudC(13, "THE CREW KEEPS THE ROAD", PAL_HUD);
    } else if (mode_ == Mode::Late) {
        hudC(11, "CREW IN", PAL_HUD);
        hudC(13, "THE CLOCK BEAT YOU", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (mode_ == Mode::Title) {
        const gs::Pad& p = sys.pad;
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) startRace();
    } else if (mode_ == Mode::Race) {
        update(DT);
    }
    draw();
}

}  // namespace busturn
