#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace plow {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LEG = 620.f;
constexpr float CREW = 46.f;
constexpr float LANE = 2.45f;
constexpr float SPEED = 17.2f;
constexpr float FOCAL = 240.f;
constexpr float CAM_H = 1.55f;
constexpr int HORIZON = 96;

gs::FMPatch enginePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.35f;
    p.op[0] = {0.5f, 0.7f, 0.05f, 0.4f, 0.75f, 0.3f};
    p.op[1] = {1.f, 0.45f, 0.04f, 0.45f, 0.65f, 0.28f, 1.5f};
    p.op[2] = {2.f, 0.18f, 0.03f, 0.35f, 0.4f, 0.25f};
    p.op[3] = {0.25f, 0.3f, 0.08f, 0.55f, 0.7f, 0.35f};
    p.vol = 0.11f;
    p.drive = 0.35f;
    p.tone = 520.f;
    p.vibRate = 6.f;
    p.vibDepth = 0.008f;
    return p;
}

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.1f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.2f, 0.25f, 0.4f};
    p.op[1] = {2.f, 0.3f, 0.01f, 0.22f, 0.15f, 0.4f};
    p.op[2] = {3.f, 0.18f, 0.01f, 0.2f, 0.1f, 0.35f};
    p.op[3] = {1.f, 0.4f, 0.02f, 0.3f, 0.3f, 0.45f};
    p.vol = 0.16f;
    return p;
}

}  // namespace

float Game::laneAt(float z) const {
    return 2.6f * std::sin(z * 0.015f) + 1.35f * std::sin(z * 0.033f + 0.8f);
}

bool Game::inLane() const { return std::fabs(x_ - laneAt(z_)) <= LANE; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.HUD.resize(64, 32);
    sys.apu.setMaster(0.8f);
    sys.apu.setPatch(0, enginePatch());
    sys.apu.setPatch(1, chimePatch());
    resetLeg();
    if (bot_) mode_ = Mode::Drive;
}

void Game::resetLeg() {
    z_ = 0;
    x_ = laneAt(0);
    latV_ = 0;
    time_ = 0;
    off_ = 0;
    crew_ = CREW;
    over_ = false;
    won_ = false;
    engineOn_ = false;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) mode_ = Mode::Drive;
    } else if (mode_ == Mode::Drive) {
        update(DT);
    } else if (sys.pad.pressed(gs::BTN_START) && !bot_) {
        resetLeg();
        mode_ = Mode::Title;
    }
    draw();
}

void Game::update(float dt) {
    float steer = 0;
    if (bot_) {
        float aim = laneAt(z_ + 20.f);
        float err = aim - x_;
        steer = std::clamp(err * 0.85f - latV_ * 0.12f, -1.f, 1.f);
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) steer -= 1.f;
        if (p.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(p.axisX) > 0.15f) steer = p.axisX;
        steer = std::clamp(steer, -1.f, 1.f);
    }
    latV_ += steer * 34.f * dt;
    latV_ *= std::exp(-5.5f * dt);
    x_ += latV_ * dt;
    z_ += SPEED * dt;
    time_ += dt;
    if (inLane()) off_ = 0;
    else off_ += dt;

    if (!engineOn_) {
        sys_->apu.keyOn(0, 46.f, 0.12f);
        engineOn_ = true;
    }
    sys_->apu.setFreq(0, 42.f + 8.f * std::fabs(latV_));

    if (off_ > 0.4f) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        sys_->apu.keyOff(0);
        sys_->apu.keyOn(1, 180.f, 0.2f);
        return;
    }
    if (time_ >= crew_) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        sys_->apu.keyOff(0);
        sys_->apu.keyOn(1, 160.f, 0.2f);
        return;
    }
    if (z_ >= LEG) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = inLane();
        sys_->apu.keyOff(0);
        sys_->apu.keyOn(1, won_ ? 523.f : 196.f, 0.22f);
    }
}

void Game::blit(const gs::Mipped& m, float cx, float footY, float h, int pal, bool flip, int fog) {
    if (m.h <= 0 || h < 2.f) return;
    float sc = h / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.w = int(std::lround(w));
    sp.h = int(std::lround(h));
    sp.x = int(std::lround(cx - w * 0.5f));
    sp.y = int(std::lround(footY - h));
    sp.pal = uint8_t(pal);
    sp.fog = uint8_t(std::clamp(fog, 0, 16));
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::ui(const gs::Image& im, float x, float y, int pal) {
    if (im.w == 0) return;
    gs::Sprite sp;
    sp.img = im;
    sp.x = int(std::lround(x));
    sp.y = int(std::lround(y));
    sp.w = im.w;
    sp.h = im.h;
    sp.pal = uint8_t(pal);
    sys_->vdp.sprite(sp);
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    hor_ = HORIZON;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(hor_), 0.f, 1.f);
        int r, g, b;
        if (y <= hor_) {
            r = int(6 + 8 * u);
            g = int(4 + 6 * u);
            b = int(2 + 3 * u);
        } else {
            r = 4;
            g = 5;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        if (y <= hor_) v.lineFog[y] = uint8_t((1.f - u) * 3.f);
        else {
            float dz = FOCAL * CAM_H / float(std::max(1, y - hor_));
            v.lineFog[y] = uint8_t(std::clamp(dz / 28.f, 0.f, 12.f));
        }
    }
    v.A.enabled = false;
    v.B.enabled = false;
    v.roadTime = int(z_ * 4.f);
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 180.f || dz < 1.2f) continue;
        float half = 3.15f;
        float hw = FOCAL * half / dz;
        float z = z_ + dz;
        float worldCx = laneAt(z);
        L.on = true;
        L.cx = 160.f + (worldCx - x_) * hw / half;
        L.hw = hw;
        L.v = z * 14.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor(z * 0.35f)) & 1) ? 1 : 0;
        L.style = gs::ROAD_RUTS;
        L.left = L.right = gs::GROUND_LAND;
    }
}

struct Proj {
    float x, y, z, hw, fog;
    bool ok;
};

static Proj project(float camZ, float camX, float z, float lateral, int hor) {
    Proj p{};
    float dz = z - camZ;
    if (dz < 1.3f || dz > 170.f) return p;
    p.z = dz;
    p.hw = FOCAL * 3.15f / dz;
    p.x = 160.f + (lateral - camX) * p.hw / 3.15f;
    p.y = float(hor) + FOCAL * CAM_H / dz;
    p.fog = std::clamp(dz / 18.f, 0.f, 14.f);
    p.ok = true;
    return p;
}

void Game::world() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    float lean = latV_ * 1.6f;
    blit(art_.plow, 160.f + lean, 198.f, 78.f, PAL_PLOW, lean < 0, 0);
    blit(art_.share, 128.f + lean * 0.4f, 206.f, 22.f, PAL_PLOW, false, 0);
    blit(art_.share, 192.f + lean * 0.4f, 206.f, 22.f, PAL_PLOW, true, 0);

    if (mode_ == Mode::Title) {
        ui(art_.title, (320 - art_.title.w) * 0.5f, 18, PAL_TITLE);
        ui(art_.sub, (320 - art_.sub.w) * 0.5f, 18.f + art_.title.h + 6, PAL_TITLE);
        ui(art_.tag, (320 - art_.tag.w) * 0.5f, 18.f + art_.title.h + art_.sub.h + 14, PAL_TITLE);
    } else if (mode_ == Mode::Over) {
        const gs::Image& im = won_ ? art_.won : art_.lost;
        ui(im, (320 - im.w) * 0.5f, 36, PAL_TITLE);
    }

    float gateZ = LEG;
    Proj g = project(z_, x_, gateZ, laneAt(gateZ), hor_);
    if (g.ok && mode_ != Mode::Title) {
        float h = g.hw * 0.55f;
        blit(art_.gate, g.x, g.y - FOCAL * 0.4f / g.z, h, PAL_GATE, false, int(g.fog));
    }

    float near = std::ceil((z_ + 6.f) / 8.f) * 8.f;
    for (float z = near; z < z_ + 140.f; z += 8.f) {
        float c = laneAt(z);
        Proj left = project(z_, x_, z, c - 3.5f, hor_);
        Proj right = project(z_, x_, z, c + 3.5f, hor_);
        if (left.ok) blit(art_.post, left.x, left.y, FOCAL * 1.5f / left.z, PAL_POST, false, int(left.fog));
        if (right.ok) blit(art_.post, right.x, right.y, FOCAL * 1.5f / right.z, PAL_POST, true, int(right.fog));
    }
    float bz = std::ceil((z_ + 10.f) / 22.f) * 22.f;
    for (float z = bz; z < z_ + 150.f; z += 22.f) {
        float c = laneAt(z);
        Proj left = project(z_, x_, z, c - 6.2f, hor_);
        Proj right = project(z_, x_, z, c + 6.4f, hor_);
        if (left.ok) blit(art_.bale, left.x, left.y, FOCAL * 1.1f / left.z, PAL_FIELD, false, int(left.fog));
        if (right.ok) blit(art_.bale, right.x, right.y, FOCAL * 1.1f / right.z, PAL_FIELD, true, int(right.fog));
    }
}

void Game::hudText(int col, int row, const char* s, int pal) {
    gs::Plane& h = sys_->vdp.HUD;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        if (x < 0 || x > 39 || row < 0 || row > 27) continue;
        unsigned char ch = (unsigned char)s[i];
        if (ch < 32 || ch > 126) ch = '?';
        int t = art_.font[ch];
        h.set(x, row, t ? gs::entry(t, pal) : 0);
    }
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    h.clear();
    h.enabled = true;
    if (mode_ == Mode::Title) {
        hudText(8, 24, "START  TAKE THE PLOW", PAL_HUD);
        return;
    }
    char buf[48];
    float left = std::max(0.f, crew_ - time_);
    float dist = std::max(0.f, LEG - z_);
    std::snprintf(buf, sizeof buf, "LEG %3.0fM", dist);
    hudText(1, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "CREW %4.1f", left);
    hudText(26, 1, buf, left < 8.f ? PAL_WARN : PAL_HUD);
    if (mode_ == Mode::Drive) {
        float err = std::fabs(x_ - laneAt(z_));
        if (err > LANE * 0.72f) hudText(14, 25, "EDGE", PAL_WARN);
        else hudText(13, 25, "IN LANE", PAL_GOOD);
    } else if (won_) {
        hudText(6, 22, "THE FURROW HELD", PAL_GOOD);
    } else if (off_ > 0.4f) {
        hudText(8, 22, "OUT OF THE LANE", PAL_WARN);
    } else {
        hudText(7, 22, "THE CREW BEAT YOU", PAL_WARN);
    }
}

void Game::draw() {
    sky();
    road();
    world();
    hud();
}

}  // namespace plow
