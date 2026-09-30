#include "purs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace purs {
namespace {

constexpr int PACK = 4;
constexpr float SPAWN_T[PACK] = {0.7f, 6.2f, 11.7f, 17.2f};
constexpr float SPAWN_LANE[PACK] = {0.28f, 0.72f, 0.36f, 0.64f};
constexpr float APPROACH = 0.15f;
constexpr float STEER = 0.7f;
constexpr float BEAM = 0.13f;
constexpr float HURT = 0.7f;

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap carArt(bool roofLamp) {
    gs::Bitmap b(40, 28);
    b.rect(6, 8, 28, 14, 1);
    b.rect(10, 4, 20, 8, 2);
    b.rect(14, 6, 6, 4, 3);
    b.rect(22, 6, 6, 4, 3);
    b.rect(4, 12, 4, 6, 4);
    b.rect(32, 12, 4, 6, 4);
    b.rect(8, 20, 8, 4, 5);
    b.rect(24, 20, 8, 4, 5);
    if (roofLamp) {
        b.rect(17, 1, 6, 4, 6);
        b.rect(18, 0, 4, 2, 7);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap deadArt() {
    gs::Bitmap b(40, 22);
    b.poly({{4, 16}, {10, 8}, {30, 10}, {36, 18}}, 1);
    b.rect(12, 6, 14, 6, 2);
    b.rect(6, 14, 6, 4, 3);
    b.rect(28, 15, 6, 4, 3);
    b.ellipse(20, 4, 5, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(64, 48);
    for (int y = 0; y < 48; y++) {
        float t = float(y) / 47.f;
        int half = 4 + int(t * 28);
        int c = y < 16 ? 2 : 1;
        b.rect(float(32 - half), float(y), float(half * 2), 1, c);
    }
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 4, 4, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 36);
    b.rect(4, 4, 2, 32, 1);
    b.rect(2, 2, 6, 5, 2);
    b.rect(1, 32, 8, 4, 3);
    return b;
}

}  // namespace

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    gs::TileAlloc tiles(vdp, 1);
    static const uint16_t hud[] = {0, gs::rgb4(15, 14, 10), gs::rgb4(5, 5, 8)};
    static const uint16_t alert[] = {0, gs::rgb4(15, 4, 3)};
    static const uint16_t ok[] = {0, gs::rgb4(6, 15, 8)};
    static const uint16_t you[] = {0, gs::rgb4(3, 4, 8), gs::rgb4(6, 8, 13), gs::rgb4(12, 14, 15),
                                   gs::rgb4(15, 12, 3), gs::rgb4(1, 1, 2), gs::rgb4(12, 8, 2),
                                   gs::rgb4(15, 14, 4), gs::rgb4(0, 0, 1)};
    static const uint16_t rival[] = {0, gs::rgb4(5, 2, 2), gs::rgb4(9, 3, 3), gs::rgb4(14, 10, 8),
                                     gs::rgb4(15, 4, 2), gs::rgb4(2, 1, 1), gs::rgb4(8, 6, 2),
                                     gs::rgb4(12, 10, 4), gs::rgb4(1, 0, 0)};
    static const uint16_t dead[] = {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(8, 4, 2),
                                    gs::rgb4(10, 10, 11), gs::rgb4(1, 1, 2)};
    static const uint16_t beam[] = {0, gs::rgb4(12, 9, 2), gs::rgb4(15, 14, 5)};
    static const uint16_t post[] = {0, gs::rgb4(6, 6, 7), gs::rgb4(15, 12, 3), gs::rgb4(3, 3, 4)};
    static const uint16_t fx[] = {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_OK, ok, 2);
    setPal(vdp, PAL_YOU, you, 9);
    setPal(vdp, PAL_RIVAL, rival, 9);
    setPal(vdp, PAL_DEAD, dead, 6);
    setPal(vdp, PAL_BEAM, beam, 3);
    setPal(vdp, PAL_POST, post, 4);
    setPal(vdp, PAL_FX, fx, 3);

    static const uint16_t road[16] = {
        0,
        gs::rgb4(1, 2, 2), gs::rgb4(2, 3, 2), gs::rgb4(3, 4, 3),
        gs::rgb4(4, 4, 3), gs::rgb4(3, 3, 2),
        gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4),
        gs::rgb4(5, 5, 6), gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 5),
        gs::rgb4(1, 2, 4), gs::rgb4(2, 3, 6), gs::rgb4(4, 6, 8),
        gs::rgb4(14, 12, 3), gs::rgb4(7, 7, 8)};
    setPal(vdp, 12, road, 16);
    vdp.setFogColor(gs::rgb4(1, 1, 3));

    for (int i = 0; i < 96; i++) {
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        gs::Bitmap g = gs::textBitmap(std::string(1, char(32 + i)), st);
        gs::Bitmap tile(8, 8);
        for (int y = 0; y < g.h && y < 8; y++)
            for (int x = 0; x < g.w && x < 8; x++)
                if (g.get(x, y)) tile.set(x, y, 1);
        art_.font[i] = tiles.shared(tile.px.data());
        art_.glyph[i] = gs::uploadMipped(vdp, g.w > 0 ? g : tile);
    }
    art_.you = gs::uploadMipped(vdp, carArt(true));
    art_.rival = gs::uploadMipped(vdp, carArt(false));
    art_.dead = gs::uploadMipped(vdp, deadArt());
    art_.beam = gs::uploadMipped(vdp, beamArt());
    art_.lamp = gs::uploadMipped(vdp, lampArt());
    art_.post = gs::uploadMipped(vdp, postArt());
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    mode_ = Mode::Title;
    over_ = won_ = false;
    if (bot_) beginRun();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (stallMark_ > 0) return 2;
    return 1;
}

void Game::beginRun() {
    mode_ = Mode::Run;
    t_ = 0;
    px_ = 0.5f;
    heat_ = 0;
    grace_ = 0;
    rail_ = 0;
    burn_ = false;
    stalled_ = spawned_ = score_ = 0;
    stallMark_ = flash_ = 0;
    over_ = won_ = false;
    reason_ = "THE BEACON FAILED";
    for (int i = 0; i < PACK; i++) pack_[i] = Rival{};
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Run) return;
    reason_ = why;
    mode_ = Mode::Over;
    over_ = true;
    won_ = false;
    burn_ = false;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 90, 0.35f);
}

void Game::winRun() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    score_ += 400;
    burn_ = false;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 520, 0.3f);
}

void Game::botSteer() {
    int threat = -1;
    float best = 9.f;
    for (int i = 0; i < PACK; i++) {
        if (!pack_[i].live) continue;
        if (pack_[i].z < best) {
            best = pack_[i].z;
            threat = i;
        }
    }
    if (threat < 0) {
        burn_ = false;
        float home = 0.5f - px_;
        px_ += std::clamp(home, -STEER / 60.f, STEER / 60.f);
        return;
    }
    float lane = pack_[threat].lane;
    float d = lane - px_;
    px_ += std::clamp(d, -STEER / 60.f, STEER / 60.f);
    burn_ = std::fabs(lane - px_) < BEAM + 0.02f && pack_[threat].z < 1.02f;
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) beginRun();
        return;
    }
    if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            over_ = false;
            mode_ = Mode::Title;
        }
        return;
    }

    if (bot_) botSteer();
    else {
        float dir = 0;
        if (pad.down(gs::BTN_LEFT)) dir -= 1;
        if (pad.down(gs::BTN_RIGHT)) dir += 1;
        if (std::fabs(pad.axisX) > 0.25f) dir = pad.axisX;
        px_ += dir * STEER * dt;
        burn_ = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.accel > 0.4f;
        if (px_ <= 0.08f && dir < 0) rail_ += dt;
        else if (px_ >= 0.92f && dir > 0) rail_ += dt;
        else rail_ = std::max(0.f, rail_ - dt);
        if (rail_ > 0.45f) {
            lose("THE BEACON LEFT THE ROAD");
            return;
        }
    }
    px_ = std::clamp(px_, 0.08f, 0.92f);

    if (burn_) heat_ += 18.f * dt;
    else heat_ = std::max(0.f, heat_ - 34.f * dt);
    if (heat_ >= 100.f) {
        lose("YOUR MACHINE STALLED");
        return;
    }

    for (int i = spawned_; i < PACK; i++) {
        if (t_ + dt < SPAWN_T[i]) break;
        pack_[i].live = true;
        pack_[i].z = 1.08f;
        pack_[i].lane = SPAWN_LANE[i];
        pack_[i].life = 1.f;
        spawned_ = i + 1;
    }

    bool any = false;
    for (int i = 0; i < PACK; i++) {
        Rival& r = pack_[i];
        if (r.done) {
            r.age++;
            continue;
        }
        if (!r.live) continue;
        any = true;
        r.z -= APPROACH * dt;
        r.age++;
        if (burn_ && r.z < 1.02f && std::fabs(r.lane - px_) < BEAM) r.life -= HURT * dt;
        if (r.life <= 0.f) {
            r.live = false;
            r.done = true;
            r.life = 0;
            stalled_++;
            score_ += 250;
            stallMark_ = 50;
            flash_ = 8;
            sys_->apu.tone(2, 180, 0.4f);
        } else if (r.z <= 0.07f) {
            lose("A MACHINE TOOK THE BEACON");
            return;
        }
    }

    if (spawned_ == PACK && !any) {
        grace_ += dt;
        if (grace_ > 0.7f) {
            winRun();
            return;
        }
    } else grace_ = 0;

    t_ += dt;
    if (stallMark_ > 0) stallMark_--;
    if (flash_ > 0) flash_--;
    if (burn_) sys_->apu.tone(0, 90.f + heat_ * 1.4f, 0.18f);
    else sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::project(float lane, float z, float& x, float& y, float& scale) const {
    float zz = std::clamp(z, 0.02f, 1.2f);
    float ny = (1.08f - zz) / 1.06f;
    ny = std::clamp(ny, 0.f, 1.f);
    y = 86.f + ny * 118.f;
    float half = 22.f + ny * 118.f;
    x = 160.f + (lane - 0.5f) * half * 2.f;
    scale = 10.f + ny * 38.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false, 0);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(t_ * 60.f);
    const float horizon = 78.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 78 ? (78 - y) / 18 : 0;
        v.lineBackdrop[y] = gs::rgb4(1, 1, std::clamp(3 + sky, 1, 8));
        v.lineFog[y] = uint8_t(y < 100 ? (100 - y) / 8 : 0);
        gs::RoadLine& r = v.road[y];
        if (y < int(horizon)) {
            r.on = false;
            continue;
        }
        float ny = (float(y) - horizon) / (210.f - horizon);
        r.on = true;
        r.cx = 160.f;
        r.hw = 16.f + ny * 130.f;
        r.v = (1.f - ny) * 2400.f + t_ * 380.f;
        r.pal = 12;
        r.band = (int(r.v / 180.f) & 1) ? 1 : 0;
        r.style = 1;
        r.left = r.right = gs::GROUND_LAND;
    }

    if (mode_ == Mode::Title) {
        text("BEACON", 160, 40, 1.35f, PAL_BEAM);
        text("PURSUIT", 160, 64, 1.1f, PAL_HUD);
        text("BE THE LAST MACHINE", 160, 96, 0.55f, PAL_OK);
        hudC(22, "ARROWS STEER   A BURNS THE BEACON", PAL_HUD);
        hudC(24, "START", PAL_OK);
        spr(art_.you, 160, 168, 46, PAL_YOU);
        spr(art_.lamp, 160, 132, 16, PAL_FX);
        return;
    }

    if (burn_ && mode_ == Mode::Run) {
        float bx, by, sc;
        project(px_, 0.42f, bx, by, sc);
        float x0, y0, s0;
        project(px_, 0.05f, x0, y0, s0);
        (void)sc;
        (void)s0;
        float bh = std::max(12.f, y0 - by);
        spr(art_.beam, (bx + x0) * 0.5f, (by + y0) * 0.5f, bh, PAL_BEAM, false, 6);
    }

    for (int side = -1; side <= 1; side += 2) {
        for (int k = 0; k < 5; k++) {
            float z = std::fmod(0.95f - t_ * 0.22f + k * 0.22f, 1.05f);
            if (z < 0.08f) continue;
            float x, y, sc;
            project(side < 0 ? 0.02f : 0.98f, z, x, y, sc);
            spr(art_.post, x, y, sc * 0.85f, PAL_POST, false, int((1.f - z) * 4));
        }
    }

    for (int i = 0; i < PACK; i++) {
        const Rival& r = pack_[i];
        if (!r.live && !r.done) continue;
        if (r.done && r.age > 90) continue;
        float x, y, sc;
        float z = r.done ? std::max(0.12f, r.z) : r.z;
        project(r.lane, z, x, y, sc);
        int fog = int(std::clamp(z, 0.f, 1.f) * 10.f);
        if (r.done) spr(art_.dead, x, y, sc * 0.8f, PAL_DEAD, false, fog);
        else spr(art_.rival, x, y, sc, PAL_RIVAL, r.lane < px_, fog);
    }

    float x, y, sc;
    project(px_, 0.02f, x, y, sc);
    spr(art_.you, x, y + 8.f, 44.f, PAL_YOU);
    if (burn_ && (flash_ || (int(t_ * 12.f) & 1))) spr(art_.lamp, x, y - 16.f, 18.f, PAL_FX);

    char line[48];
    std::snprintf(line, sizeof line, "RUNNING %d", PACK - stalled_ + (mode_ == Mode::Run ? 1 : 0));
    hud(1, 1, line, PAL_HUD);
    std::snprintf(line, sizeof line, "HEAT %d", int(heat_));
    hud(28, 1, line, heat_ > 70.f ? PAL_ALERT : PAL_HUD);
    if (burn_) hud(1, 2, "BEACON", PAL_BEAM);

    if (mode_ == Mode::Victory) {
        text("LAST MACHINE", 160, 36, 0.85f, PAL_OK);
        hudC(24, "STILL RUNNING", PAL_OK);
    } else if (mode_ == Mode::Over) {
        text("LOST", 160, 36, 1.2f, PAL_ALERT);
        hudC(24, reason_, PAL_ALERT);
    } else {
        hudC(26, "LAST MACHINE STILL RUNNING", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.f / 60.f);
    draw();
}

}  // namespace purs
