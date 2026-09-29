#include "game/quarry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace qcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 168.f;
constexpr float kReach = 26.f;
constexpr float kHopR = 30.f;
constexpr int kClock = 50 * 60;
constexpr float kPitL = 24.f, kPitR = 308.f, kPitT = 148.f, kPitB = 214.f;
constexpr float kStartX = 190.f, kStartY = 188.f;
constexpr float kHopX = 58.f, kHopY = 178.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {108.f, 164.f, 0}, {168.f, 206.f, 1}, {214.f, 158.f, 2},
    {248.f, 200.f, 3}, {292.f, 168.f, 4}, {132.f, 186.f, 5},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

float scoopNeed(int kind) {
    if (kind == 1) return 0.55f;
    if (kind == 0) return 0.42f;
    if (kind == 3) return 0.38f;
    return 0.28f;
}

const char* kindName(int kind) {
    if (kind == 1) return "BOULDER";
    if (kind == 2) return "SPOIL";
    if (kind == 3) return "ORE";
    if (kind == 4) return "BLOCK";
    if (kind == 5) return "CHUNK";
    return "SLAB";
}

const gs::Mipped& loadArt(const Art& a, int kind) {
    if (kind == 1) return a.boulder;
    if (kind == 2) return a.spoil;
    if (kind == 3) return a.ore;
    if (kind == 4) return a.block;
    if (kind == 5) return a.chunk;
    return a.slab;
}

int loadPal(int kind) {
    if (kind == 3) return PAL_ORE;
    return PAL_ROCK;
}

float loadH(int kind) {
    if (kind == 1) return 22.f;
    if (kind == 0) return 16.f;
    if (kind == 4) return 18.f;
    if (kind == 5) return 14.f;
    return 18.f;
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
    if (dumped_ >= 4) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

void Game::resetPit() {
    for (int i = 0; i < kLoads; i++) {
        load_[i].x = kLay[i].x;
        load_[i].y = kLay[i].y;
        load_[i].kind = kLay[i].kind;
        load_[i].work = 0.f;
        load_[i].gone = false;
    }
    for (Mote& m : mote_) m.life = 0.f;
    scooping_ = false;
    dumping_ = false;
    moving_ = false;
    carrying_ = false;
    carried_ = 0;
    dumped_ = 0;
    focus_ = -1;
    px_ = kStartX;
    py_ = kStartY;
    face_ = -1.f;
    step_ = 0.f;
    shake_ = 0.f;
    belt_ = 0.f;
    clock_ = kClock;
    rng_ = 1;
}

void Game::begin() {
    resetPit();
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
    scooping_ = false;
    dumping_ = false;
    won_ = false;
    over_ = false;
    reason_ = "";
    resetPit();
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

int Game::focusLoad() const {
    if (carrying_) return -1;
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kLoads; i++) {
        if (load_[i].gone) continue;
        float d = dist(px_, py_, load_[i].x, load_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

bool Game::atHopper() const { return dist(px_, py_, kHopX, kHopY) <= kHopR; }

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0.f;
    hold = false;
    float tx = kHopX, ty = kHopY;
    if (!carrying_) {
        int id = -1;
        float best = 1e9f;
        for (int i = 0; i < kLoads; i++) {
            if (load_[i].gone) continue;
            float d = dist(px_, py_, load_[i].x, load_[i].y);
            if (d < best) {
                best = d;
                id = i;
            }
        }
        if (id < 0) return;
        tx = load_[id].x;
        ty = load_[id].y;
    }
    float dx = tx - px_, dy = ty - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    bool close = carrying_ ? d <= kHopR : d <= kReach;
    hold = close;
    if (d < 1.2f) return;
    ix = dx / d;
    iy = dy / d;
    if (close) {
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
    float sp = carrying_ ? kSpeed * 0.78f : kSpeed;
    px_ = std::clamp(px_ + ix * sp * kDt, kPitL, kPitR);
    py_ = std::clamp(py_ + iy * sp * kDt, kPitT, kPitB);
    if (moving_) step_ += kDt * 8.f;
}

void Game::puff(float x, float y) {
    for (int n = 0; n < 4; n++) {
        for (Mote& m : mote_) {
            if (m.life > 0.f) continue;
            m.x = x;
            m.y = y;
            m.vx = (rnd() - 0.5f) * 46.f;
            m.vy = -18.f - rnd() * 28.f;
            m.life = 0.32f + rnd() * 0.22f;
            break;
        }
    }
}

void Game::updateWork(bool hold) {
    focus_ = focusLoad();
    scooping_ = false;
    dumping_ = false;
    if (carrying_) {
        if (hold && atHopper()) {
            dumping_ = true;
            belt_ += kDt;
            if (belt_ >= 0.32f) {
                carrying_ = false;
                dumped_++;
                belt_ = 0.f;
                shake_ = 3.5f;
                puff(kHopX + 8.f, kHopY);
                blip(dumped_ == kLoads ? 523.f : 349.f, 0.12f);
                dumping_ = false;
            }
        } else if (belt_ > 0.f) {
            belt_ = std::max(0.f, belt_ - kDt * 0.4f);
        }
        return;
    }
    if (focus_ < 0) return;
    Load& L = load_[focus_];
    if (hold) {
        scooping_ = true;
        L.work += kDt / scoopNeed(L.kind);
        if (int(L.work * 8.f) != int((L.work - kDt / scoopNeed(L.kind)) * 8.f)) puff(L.x, L.y - 6.f);
        if (L.work >= 1.f) {
            L.gone = true;
            carrying_ = true;
            carried_ = L.kind;
            scooping_ = false;
            focus_ = -1;
            shake_ = 2.f;
            puff(px_, py_ - 8.f);
            blip(220.f, 0.08f);
        }
    } else if (L.work > 0.f) {
        L.work = std::max(0.f, L.work - kDt * 0.4f);
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "GROUND CLEAR";
    blip(659.f, 0.25f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE CLOCK DIED";
    blip(98.f, 0.32f);
}

void Game::updatePlay() {
    float ix, iy;
    bool hold;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);
    move(ix, iy);
    updateWork(hold);
    if (clock_ > 0) clock_--;
    if (dumped_ >= kLoads) win();
    else if (clock_ <= 0) lose();
    if (clock_ > 0 && clock_ < 8 * 60 && (clock_ % 60) == 0) blip(784.f, 0.04f);
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
    if (scooping_ || dumping_ || (mode_ == Mode::Play && moving_)) sys_->apu.noise(scooping_ ? 0.14f : 0.06f, 900.f);
    else sys_->apu.noise(0.f, 400.f);
}

void Game::fadeMotes() {
    for (Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        m.life -= kDt;
        m.x += m.vx * kDt;
        m.y += m.vy * kDt;
        m.vy += 36.f * kDt;
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

void Game::pit() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        uint16_t c;
        if (y < 46) c = mix4(gs::rgb4(6, 9, 12), gs::rgb4(10, 11, 10), y / 46.f);
        else if (y < 118) c = mix4(gs::rgb4(8, 7, 5), gs::rgb4(6, 5, 4), (y - 46) / 72.f);
        else c = mix4(gs::rgb4(7, 6, 4), gs::rgb4(5, 4, 2), std::min(1.f, (y - 118) / 90.f));
        if (y > 140 && ((y / 8) % 2) == 0 && y < 200) c = mix4(c, gs::rgb4(8, 7, 4), 0.25f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = uint8_t(y < 70 ? (70 - y) / 14 : 0);
        v.road[y].on = false;
        (void)t;
    }
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = false;
    v.clearSprites();
}

void Game::scenery() {
    spr(art_.face, 70.f, 78.f, 34.f, PAL_WALL, false, 6, true);
    spr(art_.face, 160.f, 70.f, 40.f, PAL_WALL, false, 5, true);
    spr(art_.face, 250.f, 76.f, 36.f, PAL_WALL, true, 6, true);
    spr(art_.crusher, 52.f, 150.f, 62.f, PAL_STEEL, false, 0, true);
    spr(art_.hopper, kHopX, kHopY + 6.f, 20.f, PAL_STEEL, false, 0, true);
    float bx = 78.f + std::fmod(t_ * 18.f, 10.f);
    spr(art_.belt, bx, 158.f, 10.f, PAL_BELT, false, 0, true);
    spr(art_.belt, bx + 36.f, 162.f, 10.f, PAL_BELT, false, 0, true);
}

void Game::loads() {
    for (int i = 0; i < kLoads; i++) {
        const Load& L = load_[i];
        if (L.gone) continue;
        spr(art_.shadow, L.x, L.y + 2.f, 8.f, PAL_DUST, false, 0, true, true);
        float h = loadH(L.kind) * (1.f - 0.35f * L.work);
        spr(loadArt(art_, L.kind), L.x, L.y, h, loadPal(L.kind), false, 0, true);
        if (i == focus_ && mode_ == Mode::Play) spr(art_.dust, L.x, L.y - h - 4.f, 8.f, PAL_GOLD, false, 0, false);
    }
}

void Game::machine() {
    int step = int(step_) & 1;
    bool flip = face_ < 0.f;
    float bx = px_ + (flip ? -22.f : 22.f);
    float by = py_ - (carrying_ ? 22.f : 10.f);
    spr(art_.shadow, px_, py_ + 2.f, 8.f, PAL_DUST, false, 0, true, true);
    spr(art_.loader[step], px_, py_, 28.f, PAL_LOADER, flip, 0, true);
    spr(art_.bucket[carrying_ ? 1 : 0], bx, by, 16.f, PAL_CAB, flip, 0, true);
    if (carrying_) spr(loadArt(art_, carried_), bx, by - 12.f, loadH(carried_) * 0.7f, loadPal(carried_), false, 0, true);
    for (const Mote& m : mote_) {
        if (m.life <= 0.f) continue;
        spr(art_.dust, m.x, m.y, 6.f + (1.f - m.life) * 4.f, PAL_DUST, false, 0, false);
    }
}

void Game::messages() {
    char buf[48];
    int sec = std::max(0, clock_) / 60;
    if (mode_ == Mode::Title) {
        text("S3 QUARRY CLER", 58.f, 18.f, 2.f, PAL_GOLD);
        text("CLEAR THE GROUND", 70.f, 42.f, 1.5f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 52.f, 58.f, 1.5f, PAL_TEXT);
        if (int(t_ * 2.f) & 1) text("A TO START", 112.f, 96.f, 1.5f, PAL_GOOD);
        return;
    }
    std::snprintf(buf, sizeof(buf), "LEFT %d", kLoads - dumped_);
    text(buf, 8.f, 6.f, 1.5f, PAL_TEXT);
    std::snprintf(buf, sizeof(buf), "%02d", sec);
    text(buf, 280.f, 6.f, 1.5f, sec < 8 ? PAL_ALERT : PAL_GOLD);
    if (mode_ == Mode::Play) {
        if (carrying_) text("DUMP AT THE HOPPER", 70.f, 208.f, 1.f, PAL_GOLD);
        else if (focus_ >= 0) {
            std::snprintf(buf, sizeof(buf), "HOLD  SCOOP %s", kindName(load_[focus_].kind));
            text(buf, 70.f, 208.f, 1.f, PAL_TEXT);
        } else text("DRIVE THE LOADER", 92.f, 208.f, 1.f, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 124.f, 100.f, 2.f, PAL_GOLD);
    } else if (mode_ == Mode::Won) {
        text("GROUND CLEAR", 76.f, 96.f, 2.f, PAL_GOOD);
        text("THE PIT IS YOURS", 82.f, 118.f, 1.5f, PAL_TEXT);
    } else if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 70.f, 96.f, 2.f, PAL_ALERT);
        std::snprintf(buf, sizeof(buf), "%d STILL ON THE FLOOR", kLoads - dumped_);
        text(buf, 58.f, 118.f, 1.5f, PAL_TEXT);
    }
}

void Game::draw() {
    pit();
    scenery();
    loads();
    machine();
    messages();
}

}  // namespace qcler
