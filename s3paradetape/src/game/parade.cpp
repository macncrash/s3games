#include "game/parade.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace paradetape {
namespace {

constexpr float START_Y = 198.f;
constexpr float GOAL_Y = 36.f;
constexpr float SPD = 2.1f;
constexpr float X_MIN = 28.f;
constexpr float X_MAX = 292.f;
constexpr float kColX[kTapeN] = {72.f, 160.f, 248.f};
constexpr int kPay[kTapeN] = {3, 5, 4};
const char* kName[kTapeN] = {"RED", "GOLD", "BLUE"};
constexpr int kPal[kTapeN] = {PAL_RED, PAL_GOLD, PAL_BLUE};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += kPay[i];
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kName[i];
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kPay[i];
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Leave) return 4;
    if (crossings_ > 0) return 3;
    return 1;
}

int Game::nextSlip() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return i;
    return kTapeN;
}

int Game::columnAt(float x) const {
    int best = 0;
    float d = 1e9f;
    for (int i = 0; i < kTapeN; i++) {
        float a = std::fabs(x - kColX[i]);
        if (a < d) {
            d = a;
            best = i;
        }
    }
    return best;
}

void Game::checkRules() {
    rules_ = false;
    reason_ = "OPEN";
    if (kTapeN != 3) {
        reason_ = "tape length";
        return;
    }
    if (std::strcmp(kName[0], "RED") || std::strcmp(kName[1], "GOLD") || std::strcmp(kName[2], "BLUE")) {
        reason_ = "tape names";
        return;
    }
    if (kPay[0] != 3 || kPay[1] != 5 || kPay[2] != 4) {
        reason_ = "tape scores";
        return;
    }
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) sum += kPay[i];
    if (sum != 12) {
        reason_ = "tape total";
        return;
    }
    rules_ = true;
    reason_ = "RULES";
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    left_ = false;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    crossings_ = 0;
    traps_ = 0;
    committed_ = false;
    fan_ = -1;
    px_ = 160.f;
    py_ = START_Y;
    face_ = 1.f;
    aim_ = 0;
    age_ = 0;
    inv_ = 0;
    flash_ = 0;
    if (rules_) reason_ = "OPEN";
}

void Game::begin() {
    toTitle();
    mode_ = Mode::March;
    frame_ = 0;
    inv_ = 8;
    px_ = kColX[0];
    aim_ = 0;
    sys_->setLight(180, 40, 50);
    reason_ = "MARCH";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(5, 5, 7));
    lanes_[0] = {168.f, 0.46f, 214.f, 46.f, 12.f};
    lanes_[1] = {118.f, -0.55f, 228.f, 44.f, 80.f};
    lanes_[2] = {70.f, 0.41f, 206.f, 42.f, 40.f};
    checkRules();
    toTitle();
}

bool Game::danger(float x, float y, int fr) const {
    const float hw = 7.f;
    const float hh = 8.f;
    for (const Lane& L : lanes_) {
        if (std::fabs(y - L.y) > 11.f + hh) continue;
        float origin = L.phase + float(fr) * L.speed;
        float first = std::fmod(origin, L.spacing);
        if (first < 0.f) first += L.spacing;
        for (float cx = first - L.spacing; cx < 360.f; cx += L.spacing) {
            if (std::fabs(x - cx) < L.width * 0.5f + hw) return true;
        }
    }
    return false;
}

bool Game::climbClear(float x, float y, int fr) const {
    float dist = y - GOAL_Y;
    if (dist < 1.f) return !danger(x, GOAL_Y, fr);
    int frames = int(dist / SPD) + 3;
    for (int a = 0; a <= frames; a++) {
        float yy = y - a * SPD;
        if (yy < GOAL_Y) yy = GOAL_Y;
        if (danger(x, yy, fr + a)) return false;
        if (yy <= GOAL_Y) break;
    }
    return true;
}

void Game::bump() {
    flash_ = 16;
    inv_ = 18;
    committed_ = false;
    py_ = START_Y;
    px_ = kColX[columnAt(px_)];
    sys_->apu.noiseBurst(0.35f, 900.f, 0.12f);
    sys_->rumble(0.4f, 0.15f, 70);
    reason_ = "HIT";
}

void Game::arrive() {
    committed_ = false;
    int col = columnAt(px_);
    int want = nextSlip();
    py_ = START_Y;
    if (want < kTapeN && col == want && std::fabs(px_ - kColX[col]) < 18.f) {
        held_[col] = true;
        crossings_++;
        px_ = kColX[std::min(col + 1, kTapeN - 1)];
        aim_ = std::min(col + 1, kTapeN - 1);
        sys_->apu.keyOn(2, 620.f + col * 80.f, 0.16f);
        sys_->setLight(col == 0 ? 180 : 40, col == 1 ? 160 : 40, col == 2 ? 180 : 40);
        reason_ = "FILED";
        if (matched()) {
            left_ = true;
            won_ = rules_ && traps_ == 0 && drawerScore() == 12 && crossings_ == kTapeN;
            mode_ = Mode::Leave;
            over_ = true;
            fan_ = 0;
            reason_ = won_ ? "LEAVE" : "SHORT";
            sys_->rumble(0.2f, 0.45f, 160);
        }
    } else {
        traps_++;
        px_ = kColX[want < kTapeN ? want : 0];
        aim_ = want < kTapeN ? want : 0;
        sys_->apu.keyOn(3, 180.f, 0.14f);
        reason_ = "TRAP";
    }
    inv_ = 14;
}

void Game::human() {
    const gs::Pad& p = sys_->pad;
    float x = 0.f, y = 0.f;
    if (p.down(gs::BTN_LEFT)) x -= 1.f;
    if (p.down(gs::BTN_RIGHT)) x += 1.f;
    if (p.down(gs::BTN_UP)) y -= 1.f;
    if (p.down(gs::BTN_DOWN)) y += 1.f;
    float m = std::sqrt(x * x + y * y);
    if (m > 1.f) {
        x /= m;
        y /= m;
    }
    if (x < -0.1f) face_ = -1.f;
    else if (x > 0.1f) face_ = 1.f;
    float hurry = (p.down(gs::BTN_C) || p.down(gs::BTN_TURBO)) ? 1.25f : 1.f;
    float nx = clampf(px_ + x * SPD * hurry, X_MIN, X_MAX);
    float ny = clampf(py_ + y * SPD * hurry, GOAL_Y, START_Y);
    if (py_ > 186.f && ny > 186.f) {
        px_ = nx;
        py_ = ny;
        return;
    }
    if (!danger(nx, py_, frame_)) px_ = nx;
    if (!danger(px_, ny, frame_)) py_ = ny;
}

void Game::bot() {
    int want = nextSlip();
    if (want >= kTapeN) return;
    float home = kColX[want];
    if (committed_) {
        face_ = 1.f;
        py_ -= SPD;
        if (py_ <= GOAL_Y) {
            py_ = GOAL_Y;
            arrive();
        }
        return;
    }
    if (std::fabs(px_ - home) > 1.4f) {
        float nx = px_ + clampf(home - px_, -SPD, SPD);
        face_ = nx < px_ ? -1.f : 1.f;
        if (!danger(nx, START_Y, frame_)) px_ = nx;
        py_ = START_Y;
        return;
    }
    px_ = home;
    py_ = START_Y;
    if (climbClear(home, START_Y, frame_)) {
        committed_ = true;
        reason_ = "MARCH";
    }
}

void Game::logic() {
    if (mode_ != Mode::March) return;
    frame_++;
    if (!bot_) human();
    else bot();
    px_ = clampf(px_, X_MIN, X_MAX);
    py_ = clampf(py_, GOAL_Y, START_Y);
    if (inv_ > 0) {
        inv_--;
        if (flash_ > 0) flash_--;
        return;
    }
    if (flash_ > 0) flash_--;
    if (danger(px_, py_, frame_)) {
        bump();
        return;
    }
    if (!bot_ && py_ <= GOAL_Y + 1.f) arrive();
}

void Game::audio() {
    if (!sys_) return;
    int eighth = int(sys_->frame / 15);
    if (eighth == lastBeat_) return;
    lastBeat_ = eighth;
    if (mode_ == Mode::Title) return;
    static const float fan[8] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f, 659.25f, 880.f, 1046.5f};
    if (fan_ >= 0) {
        if (fan_ < 8) sys_->apu.keyOn(1, fan[fan_++], 0.18f);
        return;
    }
    static const float mel[8] = {392.f, 440.f, 523.25f, 392.f, 349.23f, 440.f, 523.25f, 329.63f};
    sys_->apu.keyOn(1, mel[eighth & 7], 0.08f);
    if ((eighth & 3) == 0) sys_->apu.noiseBurst(0.04f, 4800.f, 0.03f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);
    if (mode_ == Mode::Title) {
        age_++;
        if (start || (bot_ && age_ > 10)) begin();
    }
    if (mode_ == Mode::March || mode_ == Mode::Leave) age_++;
    logic();
    audio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x >= gs::SCREEN_W || s.y >= gs::SCREEN_H || s.x + s.w <= 0 || s.y + s.h <= 0) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float cy, int pal) {
    if (!sys_ || img.w < 1) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 28) v.lineBackdrop[y] = gs::rgb4(2, 5, 3);
        else if (y < 188) {
            int stripe = ((y / 10) & 1) ? 0 : 1;
            v.lineBackdrop[y] = gs::rgb4(4 + stripe, 4 + stripe, 5);
        } else v.lineBackdrop[y] = gs::rgb4(6, 5, 4);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    bool blink = flash_ > 0 && ((flash_ / 3) & 1);
    if (!blink && mode_ != Mode::Title) {
        spr(art_.you, px_, py_, 26.f, PAL_YOU, face_ < 0.f);
        spr(art_.shadow, px_, py_ + 14.f, 8.f, PAL_INK, false, true);
    }

    for (int i = 0; i < 6; i++) {
        spr(art_.flag, 18.f + i * 56.f, 16.f, 16.f, (i % 3 == 0) ? PAL_RED : (i % 3 == 1) ? PAL_GOLD : PAL_BLUE, i & 1);
        spr(art_.drum, 36.f + i * 48.f, 208.f, 12.f, PAL_CROWD);
    }
    for (int i = 0; i < 7; i++) spr(art_.curb, 24.f + i * 46.f, 24.f, 12.f, PAL_CURB);
    for (int i = 0; i < 7; i++) spr(art_.curb, 24.f + i * 46.f, 210.f, 12.f, PAL_CURB);

    for (int li = 0; li < 3; li++) {
        const Lane& L = lanes_[li];
        float origin = L.phase + float(frame_) * L.speed;
        float first = std::fmod(origin, L.spacing);
        if (first < 0.f) first += L.spacing;
        int pal = kPal[li];
        for (float cx = first - L.spacing; cx < 360.f; cx += L.spacing) {
            spr(art_.shadow, cx, L.y + 10.f, 7.f, PAL_INK, false, true);
            spr(art_.float_, cx, L.y, 18.f, pal, L.speed < 0.f);
        }
    }

    if (mode_ == Mode::Leave || crossings_ > 0) {
        for (int i = 0; i < 10; i++) {
            float t = age_ * 0.5f + i * 37.f;
            float x = std::fmod(12.f + i * 31.f + t * 0.3f, 320.f);
            float y = std::fmod(6.f + i * 7.f, 30.f);
            spr(art_.confetti, x, y, 5.f, kPal[i % 3]);
        }
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        word(art_.logo, 160.f, 58.f, PAL_GOLD);
        hudC(14, "PLAY PARADE", PAL_INK);
        hudC(16, "DRAWER MATCHES THE TAPE", PAL_GOLD);
        hudC(18, "RED THEN GOLD THEN BLUE", PAL_INK);
        hudC(24, bot_ ? "MARCHING" : "START", PAL_GOLD);
    } else if (mode_ == Mode::Leave) {
        word(art_.done, 160.f, 40.f, PAL_GOLD);
        hudC(12, won_ ? "YOU LEAVE" : "STILL OPEN", won_ ? PAL_YOU : PAL_RED);
    }

    if (mode_ != Mode::Title) {
        hud(1, 1, "TAPE", PAL_INK);
        int col = 6;
        for (int i = 0; i < kTapeN; i++) {
            std::snprintf(buf, sizeof buf, "%s %d", kName[i], kPay[i]);
            hud(col, 1, buf, held_[i] ? kPal[i] : PAL_INK);
            col += int(std::strlen(buf)) + 1;
        }
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hud(1, 26, buf, drawerScore() == 12 ? PAL_GOLD : PAL_INK);
        int want = nextSlip();
        if (mode_ == Mode::March && want < kTapeN) {
            std::snprintf(buf, sizeof buf, "NEXT %s", kName[want]);
            hud(28, 26, buf, kPal[want]);
        } else if (mode_ == Mode::Leave) {
            hud(30, 26, "LEAVE", PAL_GOLD);
        }
        for (int i = 0; i < kTapeN; i++) {
            spr(art_.flag, kColX[i], 196.f, held_[i] ? 14.f : 10.f, kPal[i]);
        }
    }
}

}  // namespace paradetape
