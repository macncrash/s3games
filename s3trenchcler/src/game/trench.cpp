#include "game/trench.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeedX = 1.45f;
constexpr float kSpeedZ = 0.78f;
constexpr float kReach = 0.12f;
constexpr int kClock = 48 * 60;
constexpr float kXMin = -0.72f, kXMax = 0.72f;
constexpr float kZMin = 0.10f, kZMax = 0.90f;

struct Lay {
    float x, z;
    int kind;
};

constexpr Lay kLay[6] = {
    {-0.32f, 0.24f, 0}, {0.38f, 0.36f, 1}, {-0.18f, 0.48f, 2},
    {0.30f, 0.60f, 3},  {-0.42f, 0.72f, 4}, {0.12f, 0.84f, 5},
};

float digNeed(int kind) {
    if (kind == 0) return 0.48f;
    if (kind == 2) return 0.42f;
    if (kind == 1) return 0.36f;
    if (kind == 3) return 0.32f;
    if (kind == 4) return 0.28f;
    return 0.22f;
}

const char* kindName(int kind) {
    if (kind == 1) return "CRATE";
    if (kind == 2) return "WIRE";
    if (kind == 3) return "BAGS";
    if (kind == 4) return "PLANK";
    if (kind == 5) return "SHELL";
    return "SPOIL";
}

const gs::Mipped& heapArt(const Art& a, int kind) {
    if (kind == 1) return a.crate;
    if (kind == 2) return a.coil;
    if (kind == 3) return a.bags;
    if (kind == 4) return a.plank;
    if (kind == 5) return a.shell;
    return a.spoil;
}

int heapPal(int kind) {
    if (kind == 1 || kind == 5) return PAL_STEEL;
    if (kind == 2) return PAL_WIRE;
    if (kind == 3) return PAL_BAG;
    if (kind == 4) return PAL_WOOD;
    return PAL_MUD;
}

float heapH(int kind) {
    if (kind == 5) return 14.f;
    if (kind == 4) return 12.f;
    if (kind == 1) return 22.f;
    return 20.f;
}

void project(float wx, float wz, float& sx, float& sy, float& sc) {
    float z = std::clamp(wz, 0.02f, 0.98f);
    float ny = 1.f - z;
    sy = 78.f + ny * 140.f;
    float hw = 28.f + ny * ny * 120.f;
    sx = 160.f + wx * hw;
    sc = 0.32f + ny * 0.95f;
}

uint16_t mix4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ch[3];
    for (int i = 0; i < 3; i++) {
        int s = 8 - 4 * i;
        int ca = (a >> s) & 15;
        int cb = (b >> s) & 15;
        ch[i] = int(std::lround(ca + (cb - ca) * t));
    }
    return gs::rgb4(ch[0], ch[1], ch[2]);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (cleared_ >= 4) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

void Game::resetTrench() {
    for (int i = 0; i < kHeaps; i++) {
        heap_[i].x = kLay[i].x;
        heap_[i].z = kLay[i].z;
        heap_[i].kind = kLay[i].kind;
        heap_[i].work = 0.f;
        heap_[i].gone = false;
    }
    for (Mote& m : mote_) m.life = 0.f;
    digging_ = false;
    moving_ = false;
    focus_ = -1;
    cleared_ = 0;
    px_ = 0.f;
    pz_ = 0.14f;
    face_ = 1.f;
    step_ = 0.f;
    shake_ = 0.f;
    swing_ = 0.f;
    clock_ = kClock;
    rng_ = 1;
}

void Game::begin() {
    resetTrench();
    won_ = false;
    over_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    blip(294.f, 0.1f);
}

void Game::toTitle() {
    if (sys_) {
        sys_->apu.silence();
        sys_->apu.noise(0.f, 400.f);
    }
    blip_ = 0.f;
    tick_ = 0.f;
    digging_ = false;
    won_ = false;
    over_ = false;
    reason_ = "";
    resetTrench();
    mode_ = Mode::Title;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    toTitle();
    if (bot_) begin();
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

int Game::focusHeap() const {
    int best = -1;
    float bd = kReach;
    for (int i = 0; i < kHeaps; i++) {
        if (heap_[i].gone) continue;
        float dx = heap_[i].x - px_, dz = heap_[i].z - pz_;
        float d = std::sqrt(dx * dx + dz * dz);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, float& iz, bool& hold) {
    ix = iz = 0.f;
    hold = false;
    int id = -1;
    float best = 1e9f;
    for (int i = 0; i < kHeaps; i++) {
        if (heap_[i].gone) continue;
        float dx = heap_[i].x - px_, dz = heap_[i].z - pz_;
        float d = dx * dx + dz * dz;
        if (d < best) {
            best = d;
            id = i;
        }
    }
    if (id < 0) return;
    float dx = heap_[id].x - px_, dz = heap_[id].z - pz_;
    float d = std::sqrt(dx * dx + dz * dz);
    hold = d <= kReach;
    if (d < 0.02f) return;
    ix = dx / d;
    iz = dz / d;
    if (hold) {
        ix *= 0.2f;
        iz *= 0.2f;
    }
}

void Game::humanInput(float& ix, float& iz, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = iz = 0.f;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (p.down(gs::BTN_UP)) iz += 1.f;
    if (p.down(gs::BTN_DOWN)) iz -= 1.f;
    if (std::fabs(p.axisX) + std::fabs(p.axisY) > 0.2f) {
        ix = p.axisX;
        iz = p.axisY;
    }
    float m = std::sqrt(ix * ix + iz * iz);
    if (m > 1.f) {
        ix /= m;
        iz /= m;
    }
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X);
}

void Game::move(float ix, float iz) {
    moving_ = std::fabs(ix) + std::fabs(iz) > 0.05f;
    if (std::fabs(ix) > 0.15f) face_ = ix < 0.f ? -1.f : 1.f;
    px_ = std::clamp(px_ + ix * kSpeedX * kDt, kXMin, kXMax);
    pz_ = std::clamp(pz_ + iz * kSpeedZ * kDt, kZMin, kZMax);
    if (moving_) step_ += kDt * 8.f;
}

void Game::puff(float x, float z) {
    for (int n = 0; n < 4; n++) {
        for (Mote& m : mote_) {
            if (m.life > 0.f) continue;
            m.x = x;
            m.z = z;
            m.vx = (rnd() - 0.5f) * 0.5f;
            m.vz = -0.05f - rnd() * 0.08f;
            m.life = 0.3f + rnd() * 0.25f;
            break;
        }
    }
}

void Game::updateWork(bool hold) {
    focus_ = focusHeap();
    digging_ = false;
    if (focus_ < 0) return;
    Heap& h = heap_[focus_];
    if (hold) {
        digging_ = true;
        swing_ += kDt * 9.f;
        h.work += kDt / digNeed(h.kind);
        if (int(swing_ * 2.f) != int((swing_ - kDt * 9.f) * 2.f)) puff(h.x, h.z);
        if (h.work >= 1.f) {
            h.gone = true;
            cleared_++;
            shake_ = 3.f;
            puff(h.x, h.z);
            blip(cleared_ == kHeaps ? 523.f : 349.f, 0.12f);
            focus_ = -1;
            digging_ = false;
        }
    } else if (h.work > 0.f) {
        h.work = std::max(0.f, h.work - kDt * 0.35f);
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "IT IS DONE";
    blip(659.f, 0.28f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE CLOCK DIED";
    blip(98.f, 0.32f);
}

void Game::updatePlay() {
    float ix, iz;
    bool hold;
    if (bot_) botInput(ix, iz, hold);
    else humanInput(ix, iz, hold);
    move(ix, iz);
    updateWork(hold);
    if (clock_ > 0) clock_--;
    if (cleared_ >= kHeaps) win();
    else if (clock_ <= 0) lose();
    if (clock_ < 10 * 60 && clock_ > 0 && (clock_ % 60) == 0) blip(740.f, 0.04f);
}

void Game::blip(float freq, float hold) {
    blip_ = hold;
    tick_ = freq;
}

void Game::serviceAudio() {
    if (!sys_) return;
    if (blip_ > 0.f) {
        sys_->apu.tone(0, tick_, 0.16f);
        blip_ -= kDt;
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (digging_) sys_->apu.noise(0.1f, 1400.f);
    else sys_->apu.noise(0.f, 400.f);
}

void Game::fadeMotes() {
    for (Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        m.life -= kDt;
        m.x += m.vx * kDt;
        m.z += m.vz * kDt;
    }
    if (shake_ > 0.f) shake_ -= kDt * 8.f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (startPressed()) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updatePlay();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (startPressed() && !bot_) {
        toTitle();
    }
    fadeMotes();
    serviceAudio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.img = m.pick(h);
    float sc = h / float(m.h);
    s.w = std::max(1, int(std::lround(m.w * sc)));
    s.h = std::max(1, int(std::lround(h)));
    float sh = shake_ > 0.f ? (rnd() - 0.5f) * shake_ : 0.f;
    s.x = int(std::lround(cx - s.w * 0.5f + sh));
    s.y = int(std::lround((feet ? cy - s.h : cy) + sh * 0.2f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float cx = x;
    for (; *s; ++s) {
        unsigned char ch = (unsigned char)*s;
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        int gi = (ch >= 32 && ch < 128) ? ch - 32 : 0;
        float h = 7.f * scale;
        float w = 8.f * scale;
        spr(art_.glyph[gi], cx + w * 0.5f, y, h, pal, false, 0, false);
        cx += 6.f * scale;
    }
}

void Game::trench() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float skyT = std::clamp(y / 70.f, 0.f, 1.f);
        uint16_t sky = mix4(gs::rgb4(2, 2, 4), gs::rgb4(6, 5, 4), skyT);
        if (y > 58) sky = mix4(gs::rgb4(5, 4, 2), gs::rgb4(3, 2, 1), std::clamp((y - 58) / 24.f, 0.f, 1.f));
        v.lineBackdrop[y] = sky;
        gs::RoadLine& r = v.road[y];
        if (y < 78) {
            r.on = false;
            v.lineFog[y] = uint8_t(std::clamp((70 - y) / 12, 0, 4));
            continue;
        }
        float ny = std::clamp((y - 78) / 140.f, 0.f, 1.f);
        r.on = true;
        r.cx = 160.f;
        r.hw = 28.f + ny * ny * 120.f;
        r.v = 30.f + (1.f - ny) * 1400.f;
        r.pal = PAL_ROAD;
        r.band = (int(r.v / 48.f) & 1) ? 1 : 0;
        r.style = gs::ROAD_MUD;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
        v.lineFog[y] = uint8_t(std::clamp((1.f - ny) * 9.f, 0.f, 9.f));
    }
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = false;
}

void Game::heapsAndMan() {
    struct Bit {
        float z;
        int kind;  // 0 heap, 1 man, 2 post L, 3 post R, 4 mote
        int id;
    };
    Bit bits[40];
    int n = 0;
    auto push = [&](float z, int kind, int id) {
        if (n < 40) bits[n++] = Bit{z, kind, id};
    };
    for (int i = 0; i < kHeaps; i++) push(heap_[i].z, 0, i);
    push(pz_, 1, 0);
    const float posts[] = {0.22f, 0.42f, 0.62f, 0.82f};
    for (float z : posts) {
        push(z, 2, 0);
        push(z, 3, 0);
    }
    for (int i = 0; i < 20; i++)
        if (mote_[i].life > 0.f) push(mote_[i].z, 4, i);
    std::sort(bits, bits + n, [](const Bit& a, const Bit& b) { return a.z > b.z; });

    for (int i = 0; i < n; i++) {
        const Bit& b = bits[i];
        float sx, sy, sc;
        if (b.kind == 0) {
            const Heap& h = heap_[b.id];
            project(h.x, h.z, sx, sy, sc);
            int fog = int((h.z) * 8.f);
            if (h.gone) {
                spr(art_.scrape, sx, sy, 8.f * sc, PAL_MUD, false, fog, true);
                continue;
            }
            spr(art_.shadow, sx, sy + 2.f, 8.f * sc, PAL_LIP, false, 0, true, true);
            float bob = (h.work > 0.f) ? std::sin(swing_ * 6.f) * 2.f : 0.f;
            spr(heapArt(art_, h.kind), sx, sy + bob, heapH(h.kind) * sc, heapPal(h.kind), false, fog, true);
        } else if (b.kind == 1) {
            project(px_, pz_, sx, sy, sc);
            int fr = moving_ ? (int(step_) & 1) : 0;
            spr(art_.shadow, sx, sy + 2.f, 9.f * sc, PAL_LIP, false, 0, true, true);
            spr(art_.digger[fr], sx, sy, 46.f * sc, PAL_MAN, face_ < 0.f, int(pz_ * 6.f), true);
            float ox = face_ * 10.f * sc;
            float oy = (digging_ ? std::sin(swing_ * 8.f) * 5.f : 0.f);
            spr(art_.shovel, sx + ox, sy - 16.f * sc + oy, 22.f * sc, PAL_STEEL, face_ < 0.f, 0, true);
        } else if (b.kind == 2 || b.kind == 3) {
            float wx = b.kind == 2 ? -0.92f : 0.92f;
            project(wx, b.z, sx, sy, sc);
            spr(art_.post, sx, sy, 36.f * sc, PAL_WOOD, b.kind == 3, int(b.z * 7.f), true);
        } else {
            const Mote& m = mote_[b.id];
            project(m.x, m.z, sx, sy, sc);
            spr(art_.dust, sx, sy, (6.f + (0.45f - m.life) * 10.f) * sc, PAL_FX, false, 0, true);
        }
    }
}

void Game::messages() {
    char buf[48];
    if (mode_ == Mode::Title) {
        text("S3 TRENCH CLER", 68.f, 18.f, 2.f, PAL_GOLD);
        text("ONE TRENCH", 104.f, 40.f, 1.5f, PAL_TEXT);
        text("CLEAR THE GROUND", 78.f, 168.f, 1.5f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 58.f, 184.f, 1.5f, PAL_ALERT);
        if (int(t_ * 2.f) & 1) text("PRESS START", 104.f, 204.f, 1.5f, PAL_GOOD);
        return;
    }
    int sec = std::max(0, clock_) / 60;
    std::snprintf(buf, sizeof(buf), "CLOCK %02d", sec);
    text(buf, 8.f, 6.f, 2.f, sec < 10 ? PAL_ALERT : PAL_GOLD);
    std::snprintf(buf, sizeof(buf), "LEFT %d", kHeaps - cleared_);
    text(buf, 226.f, 8.f, 1.5f, PAL_TEXT);
    if (mode_ == Mode::Play && focus_ >= 0) {
        const Heap& h = heap_[focus_];
        std::snprintf(buf, sizeof(buf), "SHOVEL %s", kindName(h.kind));
        text(buf, 8.f, 28.f, 1.f, PAL_GOOD);
        int bars = int(h.work * 8.f);
        char bar[12];
        for (int i = 0; i < 8; i++) bar[i] = i < bars ? '#' : '-';
        bar[8] = 0;
        text(bar, 8.f, 40.f, 1.f, PAL_GOLD);
    }
    if (mode_ == Mode::Pause) text("PAUSED", 120.f, 96.f, 2.f, PAL_TEXT);
    if (mode_ == Mode::Won) {
        text("IT IS DONE", 92.f, 88.f, 2.f, PAL_GOOD);
        text("THE GROUND IS CLEAR", 70.f, 112.f, 1.5f, PAL_GOLD);
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 74.f, 88.f, 2.f, PAL_ALERT);
        text("THE GROUND STAYS FOUL", 62.f, 112.f, 1.f, PAL_TEXT);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    trench();
    for (int i = 0; i < 7; i++) {
        float x = 16.f + i * 46.f;
        spr(art_.lip, x, 74.f, 16.f, PAL_LIP, (i & 1) != 0, 1, true);
    }
    heapsAndMan();
    messages();
    if (mode_ == Mode::Title) {
        text("ARROWS WALK  HOLD A", 62.f, 148.f, 1.f, PAL_TEXT);
    }
}

}  // namespace tcler
