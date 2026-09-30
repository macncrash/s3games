#include "game/wharf.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace wcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 170.f;
constexpr float kReach = 26.f;
constexpr int kClock = 36 * 60;
constexpr float kPierL = 24.f, kPierR = 304.f, kPierT = 108.f, kPierB = 188.f;
constexpr float kStartX = 48.f, kStartY = 150.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {78.f, 128.f, 0},  {128.f, 172.f, 1}, {168.f, 124.f, 2},
    {210.f, 176.f, 3}, {248.f, 132.f, 4}, {286.f, 168.f, 5},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float heaveNeed(int kind) {
    if (kind == 2) return 0.42f;
    if (kind == 4) return 0.36f;
    if (kind == 0) return 0.32f;
    if (kind == 5) return 0.28f;
    if (kind == 1) return 0.26f;
    return 0.22f;
}

const char* kindName(int kind) {
    if (kind == 1) return "NET";
    if (kind == 2) return "BARREL";
    if (kind == 3) return "ROPE";
    if (kind == 4) return "TIMBER";
    if (kind == 5) return "BUOY";
    return "CRATE";
}

const gs::Mipped& heapArt(const Art& a, int kind) {
    if (kind == 1) return a.net;
    if (kind == 2) return a.barrel;
    if (kind == 3) return a.coil;
    if (kind == 4) return a.timber;
    if (kind == 5) return a.buoy;
    return a.crate;
}

int heapPal(int kind) {
    if (kind == 1 || kind == 3) return PAL_ROPE;
    if (kind == 2) return PAL_BARREL;
    if (kind == 4) return PAL_TIMBER;
    if (kind == 5) return PAL_ALERT;
    return PAL_CRATE;
}

float heapH(int kind) {
    if (kind == 4) return 14.f;
    if (kind == 5) return 28.f;
    if (kind == 2) return 26.f;
    if (kind == 1) return 18.f;
    return 22.f;
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

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

void Game::resetPier() {
    for (int i = 0; i < kHeaps; i++) {
        heap_[i].x = kLay[i].x;
        heap_[i].y = kLay[i].y;
        heap_[i].kind = kLay[i].kind;
        heap_[i].work = 0.f;
        heap_[i].gone = false;
    }
    for (Mote& m : mote_) m.life = 0.f;
    heaving_ = false;
    moving_ = false;
    focus_ = -1;
    cleared_ = 0;
    px_ = kStartX;
    py_ = kStartY;
    face_ = 1.f;
    step_ = 0.f;
    shake_ = 0.f;
    swing_ = 0.f;
    clock_ = kClock;
    rng_ = 7;
}

void Game::begin() {
    resetPier();
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
    heaving_ = false;
    won_ = false;
    over_ = false;
    reason_ = "";
    resetPier();
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
    float bd = kReach + 0.01f;
    for (int i = 0; i < kHeaps; i++) {
        if (heap_[i].gone) continue;
        float d = dist(px_, py_, heap_[i].x, heap_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0.f;
    hold = false;
    int id = 0;
    float best = 1e9f;
    for (int i = 0; i < kHeaps; i++) {
        if (heap_[i].gone) continue;
        float d = dist(px_, py_, heap_[i].x, heap_[i].y);
        if (d < best) {
            best = d;
            id = i;
        }
    }
    if (heap_[id].gone) return;
    float dx = heap_[id].x - px_, dy = heap_[id].y - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    hold = d <= kReach;
    if (d < 1.2f) return;
    ix = dx / d;
    iy = dy / d;
    if (hold) {
        ix *= 0.12f;
        iy *= 0.12f;
    }
}

void Game::humanInput(float& ix, float& iy, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = iy = 0.f;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (p.down(gs::BTN_UP)) iy -= 1.f;
    if (p.down(gs::BTN_DOWN)) iy += 1.f;
    if (std::fabs(p.axisX) + std::fabs(p.axisY) > 0.2f) {
        ix = p.axisX;
        iy = -p.axisY;
    }
    float m = std::sqrt(ix * ix + iy * iy);
    if (m > 1.f) {
        ix /= m;
        iy /= m;
    }
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X);
}

void Game::move(float ix, float iy) {
    moving_ = std::fabs(ix) + std::fabs(iy) > 0.05f;
    if (std::fabs(ix) > 0.15f) face_ = ix < 0.f ? -1.f : 1.f;
    px_ = std::clamp(px_ + ix * kSpeed * kDt, kPierL, kPierR);
    py_ = std::clamp(py_ + iy * kSpeed * kDt, kPierT, kPierB);
    if (moving_) step_ += kDt * 8.f;
}

void Game::splash(float x, float y) {
    for (int n = 0; n < 5; n++) {
        for (Mote& m : mote_) {
            if (m.life > 0.f) continue;
            m.x = x;
            m.y = y;
            m.vx = (rnd() - 0.5f) * 50.f;
            m.vy = -18.f - rnd() * 36.f;
            m.life = 0.3f + rnd() * 0.25f;
            break;
        }
    }
}

void Game::updateWork(bool hold) {
    focus_ = focusHeap();
    heaving_ = false;
    if (focus_ < 0) return;
    Heap& h = heap_[focus_];
    if (hold) {
        heaving_ = true;
        swing_ += kDt * 10.f;
        h.work += kDt / heaveNeed(h.kind);
        if (h.work >= 1.f) {
            h.gone = true;
            cleared_++;
            shake_ = 2.5f;
            splash(h.x, h.y + 6.f);
            blip(cleared_ == kHeaps ? 523.f : 349.f, 0.1f);
            focus_ = -1;
            heaving_ = false;
        }
    } else if (h.work > 0.f) {
        h.work = std::max(0.f, h.work - kDt * 0.4f);
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "THE GROUND IS CLEAR";
    blip(659.f, 0.22f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE WATCH IS OVER";
    blip(98.f, 0.28f);
}

void Game::updatePlay() {
    float ix, iy;
    bool hold;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);
    move(ix, iy);
    updateWork(hold);
    if (clock_ > 0) clock_--;
    if (cleared_ >= kHeaps) win();
    else if (clock_ <= 0) lose();
    if (clock_ < 8 * 60 && (clock_ % 60) == 0) blip(740.f, 0.04f);
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
    float tide = 0.04f + 0.02f * std::sin(t_ * 1.3f);
    sys_->apu.noise(heaving_ ? 0.14f : tide, heaving_ ? 900.f : 220.f);
}

void Game::fadeMotes() {
    for (Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        m.life -= kDt;
        m.x += m.vx * kDt;
        m.y += m.vy * kDt;
        m.vy += 50.f * kDt;
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
    s.w = int(std::lround(h * (float(m.w) / float(m.h))));
    s.h = int(std::lround(h));
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        uint16_t c;
        if (y < 96) {
            c = mix4(gs::rgb4(1, 2, 5), gs::rgb4(4, 6, 9), y / 96.f);
        } else if (y < 118) {
            c = mix4(gs::rgb4(3, 5, 7), gs::rgb4(6, 7, 5), (y - 96) / 22.f);
        } else {
            float w = (y - 118) / 106.f;
            c = mix4(gs::rgb4(2, 5, 8), gs::rgb4(1, 2, 5), w);
            int band = (int(y * 0.5f + t_ * 18.f) & 7) == 0 ? 1 : 0;
            if (band && y > 190) c = mix4(c, gs::rgb4(5, 9, 12), 0.35f);
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        (void)t;
    }
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = false;
}

void Game::pier() {
    spr(art_.shed, 168.f, 92.f, 52.f, PAL_WOOD, false, 2, true);
    spr(art_.lamp, 36.f, 78.f, 40.f, PAL_GOLD, false, 0, true);
    spr(art_.lamp, 300.f, 80.f, 38.f, PAL_GOLD, false, 0, true);
    for (int i = 0; i < 7; i++) {
        float x = 28.f + i * 44.f;
        spr(art_.piling, x, 214.f, 36.f, PAL_WOOD, false, 1, true);
    }
    float gx = 40.f + std::fmod(t_ * 18.f, 90.f);
    spr(art_.gull, gx, 36.f + std::sin(t_ * 2.f) * 3.f, 8.f, PAL_TEXT, false, 0, false);
    spr(art_.gull, 220.f - std::fmod(t_ * 12.f, 70.f), 48.f, 7.f, PAL_TEXT, true, 1, false);
}

void Game::heaps() {
    int order[kHeaps];
    for (int i = 0; i < kHeaps; i++) order[i] = i;
    std::sort(order, order + kHeaps, [&](int a, int b) { return heap_[a].y < heap_[b].y; });
    bool drew = false;
    for (int i : order) {
        if (!drew && heap_[i].y > py_) {
            docker();
            drew = true;
        }
        const Heap& h = heap_[i];
        if (h.gone) {
            spr(art_.mark, h.x, h.y, 6.f, PAL_WOOD, false, 3, true);
            continue;
        }
        spr(art_.shadow, h.x, h.y + 2.f, 7.f, PAL_NIGHT, false, 0, true, true);
        float bob = (h.work > 0.f) ? std::sin(swing_ * 7.f) * 2.f : 0.f;
        float scale = heapH(h.kind) * (1.f - 0.15f * h.work);
        spr(heapArt(art_, h.kind), h.x, h.y + bob, scale, heapPal(h.kind), false, 0, true);
    }
    if (!drew) docker();
}

void Game::docker() {
    int fr = moving_ ? (int(step_) & 1) : 0;
    spr(art_.shadow, px_, py_ + 2.f, 7.f, PAL_NIGHT, false, 0, true, true);
    spr(art_.crew[fr], px_, py_, 46.f, PAL_CREW, face_ < 0.f, 0, true);
    float hx = px_ + face_ * 12.f;
    float hy = py_ - 16.f + (heaving_ ? std::sin(swing_ * 9.f) * 5.f : 0.f);
    spr(art_.hook, hx, hy, heaving_ ? 18.f : 14.f, PAL_GOLD, face_ < 0.f, 0, true);
}

void Game::messages() {
    char buf[48];
    if (mode_ == Mode::Title) {
        text("S3 WHARF CLER", 74.f, 18.f, 2.f, PAL_GOLD);
        text("CLEAR THE GROUND", 80.f, 42.f, 1.5f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 64.f, 58.f, 1.5f, PAL_ALERT);
        if (int(t_ * 2.f) & 1) text("PRESS START", 106.f, 78.f, 1.5f, PAL_GOOD);
        text("ARROWS MOVE   HOLD A TO HEAVE", 46.f, 204.f, 1.f, PAL_TEXT);
        return;
    }
    int sec = std::max(0, clock_) / 60;
    std::snprintf(buf, sizeof(buf), "CLOCK %02d", sec);
    text(buf, 8.f, 6.f, 2.f, sec < 8 ? PAL_ALERT : PAL_GOLD);
    std::snprintf(buf, sizeof(buf), "LEFT %d", kHeaps - cleared_);
    text(buf, 220.f, 8.f, 1.5f, PAL_TEXT);
    if (mode_ == Mode::Play && focus_ >= 0) {
        const Heap& h = heap_[focus_];
        std::snprintf(buf, sizeof(buf), "HEAVE %s", kindName(h.kind));
        text(buf, 8.f, 28.f, 1.f, PAL_GOOD);
        int bars = int(h.work * 8.f);
        char bar[12];
        for (int i = 0; i < 8; i++) bar[i] = i < bars ? '#' : '-';
        bar[8] = 0;
        text(bar, 8.f, 40.f, 1.f, PAL_GOLD);
    }
    if (mode_ == Mode::Pause) text("PAUSED", 122.f, 90.f, 2.f, PAL_TEXT);
    if (mode_ == Mode::Won) {
        text("GROUND CLEAR", 80.f, 70.f, 2.f, PAL_GOOD);
        text("THE WATCH HOLDS", 88.f, 92.f, 1.5f, PAL_GOLD);
    }
    if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 60.f, 70.f, 2.f, PAL_ALERT);
        text("THE GROUND STAYS FOUL", 64.f, 94.f, 1.f, PAL_TEXT);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sky();
    pier();
    for (const Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        spr(art_.splash, m.x, m.y, 6.f + (0.35f - m.life) * 6.f, PAL_FX, false, 0, true);
    }
    if (mode_ == Mode::Title) {
        spr(art_.shadow, 160.f, 168.f, 7.f, PAL_NIGHT, false, 0, true, true);
        spr(art_.crew[int(t_ * 3.f) & 1], 160.f, 166.f, 50.f, PAL_CREW, false, 0, true);
        spr(art_.hook, 176.f, 146.f, 16.f, PAL_GOLD, false, 0, true);
        for (int i = 0; i < kHeaps; i++)
            spr(heapArt(art_, heap_[i].kind), heap_[i].x, heap_[i].y, heapH(heap_[i].kind), heapPal(heap_[i].kind),
                false, 1, true);
    } else {
        heaps();
    }
    messages();
}

}  // namespace wcler
