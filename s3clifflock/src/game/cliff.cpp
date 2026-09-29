#include "cliff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace clifflock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FINISH = 248.f;
constexpr float CREW_LIMIT = 46.f;
constexpr float LO_X = 112.f;
constexpr float HI_X = 162.f;
constexpr float ZOOM = 7.2f;
constexpr float TRUCK_H = 2.15f;
constexpr float TRUCK_HALF_W = 0.72f;
constexpr float MOUTH = 1.05f;
constexpr float SHELF = 1.65f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::roadY(float wx) const {
    if (wx < 78.f) return 16.f;
    if (wx < 100.f) return 16.f + (6.f - 16.f) * ((wx - 78.f) / 22.f);
    if (wx < 176.f) return 6.f;
    if (wx < 200.f) return 6.f + (20.f - 6.f) * ((wx - 176.f) / 24.f);
    return 20.f;
}

float Game::sx(float wx) const { return (wx - camX_) * ZOOM + 78.f; }

float Game::sy(float wy) const { return 168.f - (wy - 6.f) * ZOOM; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 5;
    if (x_ < LO_X - 6.f) return 1;
    if (x_ < LO_X + 8.f) return 2;
    if (x_ < HI_X + 4.f) return 3;
    return 4;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    note_ = "";
    noteT_ = 0;
}

void Game::startRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    why_ = "";
    t_ = 0;
    legT_ = 0;
    x_ = 6.f;
    lat_ = 0;
    speed_ = 0;
    lo_ = Gate{};
    hi_ = Gate{};
    lo_.x = LO_X;
    hi_.x = HI_X;
    note_ = "THE OTHER CREW IS ALREADY MOVING";
    noteT_ = 2.4f;
    chime_ = -1;
    sys_->apu.silence();
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Win) return;
    why_ = why;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    note_ = why;
    noteT_ = 6.f;
    sys_->apu.noiseBurst(0.35f, 900.f, 0.25f);
}

void Game::win() {
    if (mode_ == Mode::Win) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "";
    note_ = "LOCK CLEAR";
    noteT_ = 6.f;
    chime_ = 0;
    chimeT_ = 0;
}

void Game::pilot(float& throttle, float& brake, float& steer) const {
    throttle = 0;
    brake = 0;
    steer = 0;
    if (!bot_) return;
    // Hold the middle of the shelf and keep a pace the gates are open for.
    steer = clampf(-lat_ * 3.2f, -1.f, 1.f);
    float want = 9.2f;
    const float clearNeed = 2.25f;
    auto blocked = [&](const Gate& g) {
        float clear = 0.28f + g.open * 3.5f;
        return x_ < g.x - 1.2f && x_ > g.x - 14.f && clear < clearNeed;
    };
    if (blocked(lo_) || blocked(hi_)) want = 1.4f;
    if (speed_ < want - 0.3f) throttle = 1.f;
    else if (speed_ > want + 0.4f) brake = 1.f;
}

void Game::physics(float throttle, float brake, float steer) {
    float acc = -1.1f;
    if (throttle > 0) acc = 5.4f;
    if (brake > 0) acc = -9.f;
    // The climb out of the chamber costs speed.
    if (x_ > 176.f && x_ < 200.f && throttle > 0) acc = 3.2f;
    speed_ = clampf(speed_ + acc * DT, 0.f, 12.4f);
    x_ += speed_ * DT;
    lat_ = clampf(lat_ + steer * 2.1f * DT, -2.4f, 2.4f);
}

void Game::gates(float dt) {
    // Lower leaf lifts on its own once the leg is rolling.
    if (t_ > 0.6f) lo_.open = std::min(1.f, lo_.open + dt * 0.22f);
    // Upper leaf waits until the truck is in the chamber, then lifts.
    if (x_ > LO_X + 4.f) hi_.open = std::min(1.f, hi_.open + dt * 0.42f);
}

void Game::checkGate(Gate& g) {
    const float halfL = 1.8f;
    bool overlap = std::fabs(x_ - g.x) < halfL;
    float clear = 0.28f + g.open * 3.5f;
    bool tight = std::fabs(lat_) + TRUCK_HALF_W > MOUTH;
    bool low = clear < TRUCK_H;
    if (overlap && (tight || low)) {
        fail("scraped a gate");
        return;
    }
    if (!g.passed && x_ > g.x + halfL) {
        g.passed = true;
        note_ = &g == &lo_ ? "LOWER GATE" : "UPPER GATE";
        noteT_ = 1.3f;
        sys_->apu.tone(1, 520.f, 0.12f);
    }
}

void Game::finishLine() {
    if (x_ >= FINISH && lo_.passed && hi_.passed) win();
}

void Game::audio() {
    if (mode_ == Mode::Run) {
        float f = 48.f + speed_ * 7.5f;
        sys_->apu.tone(0, f, speed_ > 0.4f ? 0.05f : 0.f);
    } else {
        sys_->apu.tone(0, 0, 0);
    }
    if (chime_ >= 0) {
        chimeT_ += DT;
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int step = int(chimeT_ / 0.16f);
        if (step != chime_) {
            chime_ = step;
            if (step < 4) sys_->apu.tone(2, notes[step], 0.14f);
            else sys_->apu.tone(2, 0, 0);
        }
        if (step >= 4) chime_ = -1;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip) {
    if (ht < 1.5f || m.h < 1) return;
    const gs::Image& im = m.pick(ht);
    gs::Sprite s;
    s.img = im;
    s.h = int(ht);
    s.w = std::max(1, int(ht * (float(m.w) / float(m.h)) + 0.5f));
    s.x = int(std::lround(cx - s.w * 0.5f));
    s.y = int(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    if (s.x > gs::SCREEN_W || s.y > gs::SCREEN_H || s.x + s.w < 0 || s.y + s.h < 0) return;
    sys_->vdp.sprite(s);
}

void Game::prop(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip) {
    spr(m, sx(wx), sy(wy), worldH * ZOOM, pal, flip);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    float pen = x;
    float ht = 7.f * scale;
    for (const char* p = s; *p; ++p) {
        int ci = int(uint8_t(*p)) - 32;
        if (ci < 0 || ci > 95) ci = 0;
        const gs::Mipped& g = art_.glyph[ci];
        float w = ht * (g.h > 0 ? float(g.w) / float(g.h) : 0.6f);
        spr(g, pen + w * 0.5f, y, ht, pal, false);
        pen += w + scale;
    }
}

void Game::hud(int col, int row, const char* s, int pal) { text(s, 8.f + col * 8.f, 12.f + row * 10.f, 1.f, pal); }

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    for (const char* p = s; *p; ++p) n++;
    float w = n * 6.f;
    text(s, 160.f - w * 0.5f, 18.f + row * 12.f, 1.f, pal);
}

void Game::sky() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) {
            int r = 2 + y / 20;
            int g = 3 + y / 16;
            int b = 8 + y / 30;
            c = gs::rgb4(r, g, b);
        } else if (y < 150) {
            c = gs::rgb4(7, 6, 5);
        } else {
            int band = (y / 3 + int(t_ * 4.f)) & 1;
            c = band ? gs::rgb4(2, 5, 8) : gs::rgb4(3, 6, 9);
        }
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::draw() {
    sky();
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.enabled = false;
    vdp.B.enabled = false;

    // Far cliff and the shelf, then the lock, then the trucks. Earlier sprites sit on top,
    // so the HUD is submitted last.
    float x0 = camX_ - 8.f;
    float x1 = camX_ + 48.f;
    for (float wx = std::floor(x0 / 4.f) * 4.f; wx < x1; wx += 4.f) {
        float y = roadY(wx + 2.f);
        float top = sy(y);
        prop(art_.cliff, wx + 2.f, y - 3.3f, 6.6f, PAL_ROCK);
        // Shelf lip.
        spr(art_.slab, sx(wx + 2.f), top, 10.f, PAL_ROCK);
        (void)top;
    }

    auto drawGate = [&](const Gate& g) {
        float base = roadY(g.x);
        float leafH = (1.f - g.open) * 4.2f + 0.35f;
        prop(art_.post, g.x - 1.15f, base + 2.6f, 5.4f, PAL_GATE);
        prop(art_.post, g.x + 1.15f, base + 2.6f, 5.4f, PAL_GATE);
        float leafTop = base + leafH;
        prop(art_.leaf, g.x, (base + leafTop) * 0.5f, std::max(0.4f, leafH), PAL_GATE);
    };
    drawGate(hi_);
    drawGate(lo_);

    float gy = FINISH;
    float ghostX = gy * clampf(legT_ / CREW_LIMIT, 0.f, 1.f);
    prop(art_.truck, ghostX, roadY(ghostX) + 0.85f, 1.7f, PAL_CREW, false);

    float py = roadY(x_) + 0.95f;
    // Lateral offset reads as a small vertical lean so the shelf edge stays readable.
    prop(art_.truck, x_, py + lat_ * 0.15f, 2.05f, PAL_TRUCK, false);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(2, "S3 CLIFF LOCK", PAL_INK);
        hudC(4, "TAKE THE CLIFF", PAL_HUD);
        hudC(5, "PASS THE LOCK", PAL_HUD);
        hudC(6, "DO NOT SCRAPE A GATE", PAL_WARN);
        hudC(8, "THE CLOCK IS THE OTHER CREW", PAL_INK);
        hudC(11, "A THROTTLE   B BRAKE", PAL_HUD);
        hudC(12, "LEFT RIGHT  HOLD THE SHELF", PAL_HUD);
        hudC(14, "START", PAL_INK);
    } else {
        std::snprintf(buf, sizeof(buf), "SPD %4.1f", speed_);
        hud(0, 0, buf, PAL_HUD);
        float left = std::max(0.f, CREW_LIMIT - legT_);
        std::snprintf(buf, sizeof(buf), "CREW %4.1f", left);
        hud(22, 0, buf, left < 8.f ? PAL_WARN : PAL_INK);
        std::snprintf(buf, sizeof(buf), "SHELF %+4.1f", lat_);
        hud(0, 1, buf, std::fabs(lat_) > 1.2f ? PAL_WARN : PAL_HUD);
        const char* leg = "APPROACH";
        if (x_ >= FINISH) leg = "MARK";
        else if (x_ > HI_X) leg = "CLIMB";
        else if (x_ > LO_X) leg = "CHAMBER";
        else if (x_ > LO_X - 16.f) leg = "LOWER GATE";
        hud(22, 1, leg, PAL_INK);
        if (noteT_ > 0 && note_) hudC(4, note_, PAL_WARN);
        if (mode_ == Mode::Pause) hudC(8, "PAUSED", PAL_INK);
        if (mode_ == Mode::Fail) {
            hudC(7, "LEG LOST", PAL_WARN);
            hudC(9, why_, PAL_HUD);
            hudC(12, "A TO RUN AGAIN", PAL_INK);
        }
        if (mode_ == Mode::Win) {
            hudC(7, "CLIFF TAKEN", PAL_INK);
            hudC(9, "LOCK PASSED CLEAN", PAL_HUD);
            std::snprintf(buf, sizeof(buf), "%.1f SECONDS", legT_);
            hudC(11, buf, PAL_HUD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    showTitle();
    if (bot_) startRun();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START))) startRun();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        float throttle = 0, brake = 0, steer = 0;
        if (bot_) {
            pilot(throttle, brake, steer);
        } else {
            if (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP)) throttle = 1.f;
            if (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN)) brake = 1.f;
            if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
            if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
            if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
            if (pad.accel > 0.15f) throttle = std::max(throttle, pad.accel);
            if (pad.brake > 0.15f) brake = std::max(brake, pad.brake);
        }
        physics(throttle, brake, steer);
        t_ += DT;
        legT_ += DT;
        if (noteT_ > 0) noteT_ -= DT;
        gates(DT);
        if (std::fabs(lat_) > SHELF) fail("left the cliff");
        if (mode_ == Mode::Run) checkGate(lo_);
        if (mode_ == Mode::Run) checkGate(hi_);
        if (mode_ == Mode::Run && legT_ >= CREW_LIMIT && x_ < FINISH) fail("the other crew took the cliff");
        if (mode_ == Mode::Run) finishLine();
        camX_ = x_ - 6.f;
    }
    if (mode_ != Mode::Run) {
        // Keep the title framed on the approach.
        if (mode_ == Mode::Title) camX_ = 4.f;
    }
    audio();
    // Drop the one-shot gate blip after a short hold.
    if (mode_ == Mode::Run && noteT_ < 1.1f) sys.apu.tone(1, 0, 0);
    draw();
}

}  // namespace clifflock
