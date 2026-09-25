#include "parade.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace parade {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float SPEED = 112.f;
constexpr float STEP = SPEED * DT;
constexpr float PW = 7.f;
constexpr float PH = 8.f;
constexpr float BOT_PAD = 3.f;
constexpr float X_MIN = 56.f;
constexpr float X_MAX = 264.f;
constexpr float START_X = 176.f;
constexpr float START_Y = 760.f;
constexpr float SQ_X = 176.f;
constexpr float SQ_Y = 58.f;
constexpr int NLANE = 7;

enum Kind { Horse = 0, Wagon = 1, Drum = 2, Baton = 3 };

enum What { W_MAJOR = 0, W_HORSE, W_WAGON, W_DRUM, W_BATON, W_PERSON, W_FLAG, W_SQUARE, W_RING, W_CONFETTI, W_PUFF };

struct Lane {
    int kind;
    float y;
    float speed;
    float loop;
    float phase;
    int count;
    int look;
    const char* name;
};

// Bottom of the avenue first. Spacing leaves a gutter between hit bands.
const Lane LANES[NLANE] = {
    {Horse, 640, 52, 520, 30, 2, 0, "HORSES"},
    {Drum, 560, -64, 480, 110, 3, 0, "DRUMS"},
    {Horse, 480, 78, 500, 180, 2, 1, "HORSES"},
    {Wagon, 400, -46, 560, 40, 2, 0, "FLOATS"},
    {Baton, 320, 96, 480, 90, 3, 0, "BATONS"},
    {Wagon, 240, 60, 540, 200, 2, 1, "FLOATS"},
    {Baton, 160, 116, 480, 150, 3, 1, "BATONS"},
};

float wrapf(float a, float m) {
    float r = std::fmod(a, m);
    if (r < 0.f) r += m;
    return r;
}

float hitW(int kind) {
    switch (kind) {
        case Horse: return 48.f;
        case Wagon: return 90.f;
        case Drum: return 28.f;
        case Baton: return 30.f;
        default: return 32.f;
    }
}

float hitH(int kind) {
    switch (kind) {
        case Horse: return 22.f;
        case Wagon: return 30.f;
        case Drum: return 28.f;
        case Baton: return 12.f;
        default: return 20.f;
    }
}

float drawH(int kind) {
    switch (kind) {
        case Horse: return 34.f;
        case Wagon: return 46.f;
        case Drum: return 34.f;
        case Baton: return 18.f;
        default: return 32.f;
    }
}

int whatOf(int kind) {
    switch (kind) {
        case Horse: return W_HORSE;
        case Wagon: return W_WAGON;
        case Drum: return W_DRUM;
        default: return W_BATON;
    }
}

gs::FMPatch tubaPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.22f;
    p.op[0] = {1, 1, 0.02f, 0.2f, 0.62f, 0.16f};
    p.op[1] = {2, 0.42f, 0.02f, 0.24f, 0.4f, 0.18f};
    p.op[2] = {3, 0.22f, 0.03f, 0.28f, 0.28f, 0.2f};
    p.op[3] = {0.5f, 0.5f, 0.02f, 0.22f, 0.7f, 0.18f};
    p.vol = 0.18f;
    p.drive = 0.08f;
    p.tone = 900;
    return p;
}

gs::FMPatch trumpetPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.4f;
    p.op[0] = {1, 1, 0.012f, 0.14f, 0.55f, 0.1f};
    p.op[1] = {2, 0.52f, 0.012f, 0.16f, 0.38f, 0.1f};
    p.op[2] = {3, 0.28f, 0.016f, 0.2f, 0.24f, 0.12f};
    p.op[3] = {4.0f, 0.16f, 0.02f, 0.2f, 0.18f, 0.12f};
    p.vol = 0.13f;
    p.drive = 0.18f;
    p.tone = 2400;
    return p;
}

gs::FMPatch blipPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.op[0] = {1, 1, 0.005f, 0.07f, 0.f, 0.05f};
    p.op[1] = {2, 0.35f, 0.005f, 0.08f, 0.f, 0.06f};
    p.op[2] = {3, 0.18f, 0.01f, 0.09f, 0.f, 0.07f};
    p.op[3] = {5, 0.12f, 0.01f, 0.09f, 0.f, 0.07f};
    p.vol = 0.11f;
    return p;
}

float bodyX(const Lane& L, int i, float t) {
    float spacing = L.loop / float(L.count);
    return wrapf(L.phase + i * spacing + L.speed * t, L.loop) - 100.f;
}

}  // namespace

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.f / 16777216.f);
}

float Game::yReach(int kind) const { return hitH(kind) * 0.5f + PH; }

bool Game::overlaps(float x, float y, float t, float pad) const {
    for (int li = 0; li < NLANE; li++) {
        const Lane& L = LANES[li];
        float hw = hitW(L.kind) * 0.5f + PW + pad;
        float hh = yReach(L.kind) + pad;
        for (int i = 0; i < L.count; i++) {
            float bx = bodyX(L, i, t);
            if (std::fabs(x - bx) < hw && std::fabs(y - L.y) < hh) return true;
        }
    }
    return false;
}

bool Game::inBand(float y) const {
    for (int i = 0; i < NLANE; i++)
        if (std::fabs(y - LANES[i].y) < yReach(LANES[i].kind)) return true;
    return false;
}

bool Game::pathClear(float x, float y0, float y1) const {
    float y = y0;
    float tt = t_;
    int guard = 0;
    while (y > y1 + 0.01f && guard++ < 600) {
        float ny = std::max(y1, y - STEP);
        if (overlaps(x, ny, tt, BOT_PAD)) return false;
        y = ny;
        tt += DT;
    }
    return true;
}

int Game::nextLane() const {
    int best = -1;
    float bestY = -1.f;
    for (int i = 0; i < NLANE; i++) {
        float gate = LANES[i].y - yReach(LANES[i].kind) - 4.f;
        if (py_ > gate && LANES[i].y > bestY) {
            bestY = LANES[i].y;
            best = i;
        }
    }
    return best;
}

float Game::landY(int idx) const {
    const Lane& L = LANES[idx];
    float above = L.y - yReach(L.kind) - 8.f;
    int next = -1;
    float nextY = -1.f;
    for (int i = 0; i < NLANE; i++) {
        if (LANES[i].y < L.y - 1.f && LANES[i].y > nextY) {
            nextY = LANES[i].y;
            next = i;
        }
    }
    if (next < 0) return above;
    float belowNext = nextY + yReach(LANES[next].kind) + 8.f;
    if (above > belowNext + 4.f) return (above + belowNext) * 0.5f;
    return above;
}

bool Game::confirm() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

void Game::begin() {
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    px_ = START_X;
    py_ = START_Y;
    safeX_ = px_;
    safeY_ = py_;
    lives_ = 5;
    score_ = 0;
    rows_ = 0;
    scored_ = 0;
    inv_ = 0.7f;
    t_ = 0;
    commit_ = false;
    moving_ = false;
    faceLeft_ = false;
    wait_ = 0;
    shake_ = 0;
    hurtFlash_ = 0;
    fan_ = -1;
    slide_ = 1;
    bits_.clear();
}

void Game::countRows() {
    int n = 0;
    for (int i = 0; i < NLANE; i++) {
        float top = LANES[i].y - yReach(LANES[i].kind);
        if (py_ < top) n++;
    }
    rows_ = n;
}

void Game::awardRows() {
    if (rows_ > scored_) {
        score_ += 200 * (rows_ - scored_);
        scored_ = rows_;
        sys_->apu.keyOn(2, 720.f + rows_ * 30.f, 0.12f);
    }
}

void Game::burst(float x, float y, int n) {
    for (int i = 0; i < n; i++) {
        if (bits_.size() > 72) bits_.erase(bits_.begin());
        float a = rnd() * 6.2831853f;
        float s = 20.f + rnd() * 70.f;
        bits_.push_back({x, y, std::cos(a) * s, std::sin(a) * s - 20.f, 0.45f + rnd() * 0.5f});
    }
}

void Game::victory() {
    if (mode_ == Mode::Win) return;
    px_ = SQ_X;
    py_ = SQ_Y;
    countRows();
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    score_ += 2500;
    commit_ = false;
    fan_ = 0;
    burst(SQ_X, SQ_Y, 42);
    sys_->setLight(40, 210, 70);
    sys_->rumble(0.25f, 0.5f, 180);
}

void Game::hurt() {
    burst(px_, py_, 16);
    lives_--;
    hurtFlash_ = 0.8f;
    shake_ = 7.f;
    commit_ = false;
    wait_ = 0;
    sys_->apu.noiseBurst(0.5f, 1600.f, 0.22f);
    sys_->rumble(0.7f, 0.35f, 140);
    sys_->setLight(220, 40, 30);
    if (lives_ <= 0) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        return;
    }
    inv_ = 1.35f;
    px_ = safeX_;
    py_ = safeY_;
}

void Game::botMove() {
    if (commit_) {
        px_ = commitX_;
        py_ = std::max(commitY_, py_ - STEP);
        if (py_ <= commitY_ + 0.01f) commit_ = false;
        return;
    }
    int n = nextLane();
    if (n < 0) {
        float nx = px_ + std::clamp(SQ_X - px_, -STEP, STEP);
        float ny = py_ + std::clamp(SQ_Y - py_, -STEP, STEP);
        nx = std::clamp(nx, X_MIN, X_MAX);
        if (!overlaps(nx, ny, t_, 0.f)) {
            px_ = nx;
            py_ = ny;
        }
        return;
    }
    const Lane& L = LANES[n];
    float edge = L.y + yReach(L.kind) + 12.f;
    if (py_ > edge + 0.5f) {
        float ny = std::max(edge, py_ - STEP);
        if (!overlaps(px_, ny, t_, 0.f)) py_ = ny;
        else {
            float nx = std::clamp(px_ + float(slide_) * STEP, X_MIN, X_MAX);
            if (!overlaps(nx, py_, t_, 0.f)) px_ = nx;
            else slide_ = -slide_;
        }
        return;
    }
    float land = landY(n);
    if (pathClear(px_, py_, land)) {
        wait_ = 0;
        commit_ = true;
        commitX_ = px_;
        commitY_ = land;
        py_ = std::max(land, py_ - STEP);
        if (py_ <= land + 0.01f) commit_ = false;
        return;
    }
    wait_ += DT;
    if (wait_ > 2.4f) {
        float nx = std::clamp(px_ + float(slide_) * 32.f, X_MIN, X_MAX);
        if (std::fabs(nx - px_) < 1.f) slide_ = -slide_;
        else if (!overlaps(nx, py_, t_, 0.f)) {
            px_ = nx;
            wait_ = 0;
        } else slide_ = -slide_;
    }
}

void Game::humanMove() {
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
    float hurry = (p.down(gs::BTN_C) || p.down(gs::BTN_TURBO)) ? 1.45f : 1.f;
    px_ += x * STEP * hurry;
    py_ += y * STEP * hurry;
}

void Game::tickBits() {
    for (Bit& a : ambient_) {
        a.y += a.vy * DT;
        a.x += std::sin(t_ * 1.7f + a.vx) * 14.f * DT;
        if (a.y > 900.f) a.y = 30.f;
        if (a.x < 40.f) a.x = 40.f;
        if (a.x > 280.f) a.x = 280.f;
    }
    for (int i = int(bits_.size()) - 1; i >= 0; --i) {
        Bit& b = bits_[i];
        b.x += b.vx * DT;
        b.y += b.vy * DT;
        b.vy += 30.f * DT;
        b.life -= DT;
        if (b.life <= 0.f) bits_.erase(bits_.begin() + i);
    }
}

void Game::updatePlay() {
    t_ += DT;
    float ox = px_;
    float oy = py_;
    if (bot_) botMove();
    else humanMove();
    px_ = std::clamp(px_, X_MIN, X_MAX);
    py_ = std::clamp(py_, 36.f, START_Y);
    moving_ = std::fabs(px_ - ox) > 0.01f || std::fabs(py_ - oy) > 0.01f;
    if (px_ < ox - 0.01f) faceLeft_ = true;
    else if (px_ > ox + 0.01f) faceLeft_ = false;

    countRows();
    awardRows();
    if (std::fabs(px_ - SQ_X) <= 24.f && std::fabs(py_ - SQ_Y) <= 24.f) {
        victory();
        tickBits();
        return;
    }
    if (inv_ > 0.f) inv_ -= DT;
    else if (overlaps(px_, py_, t_, 0.f)) hurt();
    else if (!inBand(py_)) {
        safeX_ = px_;
        safeY_ = py_;
    }
    tickBits();
}

void Game::music() {
    static const float mel[16] = {523.25f, 659.25f, 783.99f, 659.25f, 587.33f, 659.25f, 523.25f, 392.00f,
                                   523.25f, 659.25f, 783.99f, 1046.5f, 783.99f, 659.25f, 523.25f, 392.00f};
    static const float bass[4] = {130.81f, 98.00f, 116.54f, 130.81f};
    static const float fan[8] = {523.25f, 659.25f, 783.99f, 1046.5f, 1318.5f, 1046.5f, 783.99f, 1046.5f};
    int eighth = int(sys_->frame / 14);
    if (eighth == lastStep_) return;
    lastStep_ = eighth;
    if (mode_ == Mode::Pause) return;
    if (fan_ >= 0) {
        if (fan_ < 8) {
            sys_->apu.keyOn(1, fan[fan_], 0.2f);
            sys_->apu.keyOn(0, fan[fan_] * 0.5f, 0.16f);
            fan_++;
        }
        return;
    }
    if (mode_ == Mode::Over) return;
    sys_->apu.keyOn(1, mel[eighth & 15], 0.12f);
    if ((eighth & 3) == 0) sys_->apu.keyOn(0, bass[(eighth >> 2) & 3], 0.16f);
    if (eighth & 1) sys_->apu.noiseBurst(0.07f, 4800.f, 0.05f);
}

void Game::spr(const gs::Mipped& m, float x, float y, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(x - s.w * 0.5f));
    s.y = int16_t(std::lround(y - s.h * 0.5f));
    if (s.x >= gs::SCREEN_W || s.y >= gs::SCREEN_H || s.x + s.w <= 0 || s.y + s.h <= 0) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::shade(float x, float y, float w) {
    if (w < 4.f || art_.shadow.h < 1) return;
    float h = w * float(art_.shadow.h) / float(art_.shadow.w);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::max(1L, long(std::lround(h))));
    s.x = int16_t(std::lround(x - s.w * 0.5f));
    s.y = int16_t(std::lround(y - s.h * 0.5f));
    if (s.x >= gs::SCREEN_W || s.y >= gs::SCREEN_H || s.x + s.w <= 0 || s.y + s.h <= 0) return;
    s.img = art_.shadow.pick(h);
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::queueSpr(int what, float x, float y, float h, int pal, int frame, bool flip, int fog, float bias) {
    Spr s;
    s.key = y + bias;
    s.what = what;
    s.x = x;
    s.y = y;
    s.h = h;
    s.pal = pal;
    s.frame = frame;
    s.fog = fog;
    s.flip = flip;
    queue_.push_back(s);
}

void Game::flushQueue() {
    std::sort(queue_.begin(), queue_.end(), [](const Spr& a, const Spr& b) { return a.key > b.key; });
    for (const Spr& s : queue_) {
        float sx = s.x;
        float sy = s.y - cam_;
        if (sy < -80.f || sy > gs::SCREEN_H + 80.f) continue;
        if (sx < -120.f || sx > gs::SCREEN_W + 120.f) continue;
        switch (s.what) {
            case W_MAJOR:
                shade(sx, sy + 16.f, 22.f);
                spr(art_.major[s.frame & 1], sx, sy, s.h, s.pal, s.flip, s.fog);
                break;
            case W_HORSE:
                shade(sx, sy + 12.f, s.h * 1.3f);
                spr(art_.horse[s.frame & 1], sx, sy, s.h, PAL_HORSE, s.flip, s.fog);
                break;
            case W_WAGON:
                shade(sx, sy + 16.f, s.h * 1.6f);
                spr(art_.wagon[s.frame & 1], sx, sy, s.h, PAL_WAGON, s.flip, s.fog);
                break;
            case W_DRUM:
                shade(sx, sy + 10.f, s.h * 0.8f);
                spr(art_.drum[s.frame & 1], sx, sy, s.h, PAL_DRUM, false, s.fog);
                break;
            case W_BATON:
                spr(art_.baton[s.frame & 1], sx, sy, s.h, PAL_BATON, s.flip, s.fog);
                break;
            case W_PERSON: {
                float bob = std::sin(t_ * 3.f + s.x) * 1.2f;
                spr(art_.person[s.frame % 3], sx, sy + bob, s.h, PAL_CROWD, s.flip, s.fog);
                break;
            }
            case W_FLAG: {
                float wave = std::sin(t_ * 4.f + s.y * 0.05f) * 1.5f;
                spr(art_.pennant, sx, sy + wave, s.h, PAL_PENNANT, s.flip, s.fog);
                break;
            }
            case W_SQUARE:
                spr(art_.square, sx, sy, s.h, PAL_SQUARE, false, 0);
                break;
            case W_RING:
                spr(art_.ring, sx, sy, s.h, PAL_SQUARE, false, 0);
                break;
            case W_CONFETTI:
                spr(art_.confetti, sx, sy, s.h, PAL_CONFETTI, s.flip, s.fog);
                break;
            case W_PUFF:
                spr(art_.puff, sx, sy, s.h, PAL_FX, false, s.fog);
                break;
            default:
                break;
        }
    }
    queue_.clear();
}

void Game::textLine(const std::string& s, float x, float y, float h, int pal) {
    float width[96];
    int n = 0;
    float total = 0.f;
    for (unsigned char ch : s) {
        if (n >= 96) break;
        float w;
        if (ch == ' ') w = h * 0.40f;
        else {
            int gi = std::clamp(int(ch), 32, 127) - 32;
            const gs::Mipped& g = art_.glyph[gi];
            w = g.h > 0 ? h * float(g.w) / float(g.h) : h * 0.45f;
        }
        width[n++] = w;
        total += w + h * 0.05f;
    }
    float pen = x - total * 0.5f;
    int i = 0;
    for (unsigned char ch : s) {
        if (i >= n) break;
        float w = width[i++];
        if (ch != ' ') {
            int gi = std::clamp(int(ch), 32, 127) - 32;
            spr(art_.glyph[gi], pen + w * 0.5f, y, h, pal, false, 0);
        }
        pen += w + h * 0.05f;
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::drawUi() {
    char buf[40];
    std::snprintf(buf, sizeof buf, "SCORE %05d", score_);
    hud(1, 0, "S3 PARADE", PAL_WHITE);
    hud(39 - int(std::strlen(buf)), 0, buf, PAL_GOLD);

    if (mode_ == Mode::Title) {
        hud(1, 1, "ARROWS MOVE  C HURRIES  ENTER GO", PAL_WHITE);
        textLine("S3 PARADE", 160, 48, 30, PAL_GOLD);
        textLine("REACH THE SQUARE", 160, 80, 16, PAL_WHITE);
        textLine("DONT GET HIT", 160, 102, 16, PAL_RED);
        if ((sys_->frame / 20) % 2 == 0) textLine("ENTER MARCHES", 160, 208, 14, PAL_GOLD);
        textLine("MARCH UP", 160, 132, 14, PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hud(1, 1, "PAUSED", PAL_GOLD);
        textLine("PAUSED", 160, 108, 28, PAL_GOLD);
        textLine("ENTER MARCHES", 160, 140, 14, PAL_WHITE);
    } else if (mode_ == Mode::Win) {
        hud(1, 1, "THE SQUARE", PAL_GREEN);
        textLine("THE SQUARE", 160, 118, 26, PAL_GOLD);
        textLine("YOU MADE THE REVIEW", 160, 148, 14, PAL_WHITE);
    } else if (mode_ == Mode::Over) {
        hud(1, 1, "CUT FROM THE LINE", PAL_RED);
        textLine("CUT FROM THE LINE", 160, 112, 16, PAL_RED);
        textLine("ENTER RETRIES", 160, 140, 14, PAL_WHITE);
    } else {
        int lifePal = hurtFlash_ > 0.f ? PAL_RED : PAL_WHITE;
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hud(1, 1, buf, lifePal);
        std::snprintf(buf, sizeof buf, "ROW %d/%d", rows_, NLANE);
        hud(12, 1, buf, PAL_GOLD);
        int n = nextLane();
        const char* name = n < 0 ? "SQUARE" : LANES[n].name;
        hud(24, 1, name, PAL_WHITE);
        float sqScreen = SQ_Y - cam_;
        if (sqScreen < 8.f) {
            float bob = std::sin(sys_->frame * 0.22f) * 3.f;
            spr(art_.chevron, 160, 28 + bob, 14, PAL_GOLD, false, 0);
        }
    }
}

void Game::draw() {
    float cam = py_ - 156.f;
    if (shake_ > 0.15f) cam += std::sin(float(sys_->frame) * 1.8f) * shake_;
    cam_ = std::clamp(cam, 0.f, 1024.f - float(gs::SCREEN_H));
    sys_->vdp.B.scroll(0, int(std::lround(cam_)));
    int fog = mode_ == Mode::Title ? 6 : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / 223.f;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(int(2 + u * 5), int(2 + u * 2), int(6 - u * 2));
        sys_->vdp.lineFog[y] = uint8_t(fog);
    }
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    int space = art_.font[0];
    for (int r = 0; r < 2; r++)
        for (int c = 0; c < 40; c++) sys_->vdp.HUD.set(c, r, gs::entry(space, PAL_WHITE));

    drawUi();

    int paradeFog = mode_ == Mode::Title ? 5 : 0;
    float pulse = std::sin(sys_->frame * 0.12f);
    queueSpr(W_RING, SQ_X, SQ_Y, 70.f + pulse * 4.f, PAL_SQUARE, 0, false, 0, 0.f);
    queueSpr(W_SQUARE, SQ_X, SQ_Y, 52.f + pulse * 2.f, PAL_SQUARE, 0, false, 0, 2.f);

    for (int li = 0; li < NLANE; li++) {
        const Lane& L = LANES[li];
        bool flip = L.speed < 0.f;
        for (int i = 0; i < L.count; i++) {
            int fr = ((int(t_ * 8.f) + i) & 1);
            if (L.kind == Wagon) fr = L.look & 1;
            queueSpr(whatOf(L.kind), bodyX(L, i, t_), L.y, drawH(L.kind), 0, fr, flip, paradeFog, 0.f);
        }
    }

    for (const Watcher& w : crowd_) {
        bool right = w.x > 160.f;
        queueSpr(W_PERSON, w.x, w.y, 26.f, PAL_CROWD, w.kind, right, paradeFog, -1.f);
    }
    for (const Watcher& w : flags_) {
        queueSpr(W_FLAG, w.x, w.y, 18.f, PAL_PENNANT, 0, w.x > 200.f, paradeFog, -2.f);
    }
    for (const Bit& a : ambient_) queueSpr(W_CONFETTI, a.x, a.y, 9.f, PAL_CONFETTI, 0, false, paradeFog, -3.f);
    for (const Bit& b : bits_) {
        float h = 8.f + b.life * 10.f;
        queueSpr(W_CONFETTI, b.x, b.y, h, PAL_CONFETTI, 0, b.vx < 0.f, 0, 6.f);
        if (b.life > 0.35f) queueSpr(W_PUFF, b.x, b.y, 16.f + b.life * 8.f, PAL_FX, 0, false, 0, 7.f);
    }

    bool blink = inv_ > 0.f && ((sys_->frame / 4) & 1);
    if (!blink) {
        int step = moving_ ? int((sys_->frame / 7) & 1) : 0;
        queueSpr(W_MAJOR, px_, py_, 40.f, PAL_MAJOR, step, faceLeft_, 0, 8.f);
    }
    flushQueue();

    if (mode_ == Mode::Win) sys_->setLight(40, 210, 70);
    else if (mode_ == Mode::Over) sys_->setLight(200, 30, 24);
    else if (mode_ == Mode::Play) sys_->setLight(210, 150, 40);
    else sys_->setLight(90, 40, 140);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    mode_ = Mode::Title;
    px_ = START_X;
    py_ = START_Y;
    safeX_ = px_;
    safeY_ = py_;
    lives_ = 5;
    score_ = 0;
    rows_ = 0;
    scored_ = 0;
    over_ = false;
    won_ = false;
    fan_ = -1;
    t_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.resize(64, 128);
    float rows[NLANE];
    for (int i = 0; i < NLANE; i++) rows[i] = LANES[i].y;
    buildArt(sys.vdp, art_, rows, NLANE);
    sys.vdp.B.scroll(0, int(START_Y - 156.f));

    crowd_.clear();
    flags_.clear();
    for (int i = 0; i < 16; i++) {
        float y = 150.f + i * 42.f;
        crowd_.push_back({26.f, y, i % 3});
        crowd_.push_back({296.f, y + 18.f, (i + 1) % 3});
    }
    for (int i = 0; i < 12; i++) {
        float y = 170.f + i * 54.f;
        flags_.push_back({14.f, y, 0});
        flags_.push_back({306.f, y + 14.f, 0});
    }
    for (int i = 0; i < 8; i++) flags_.push_back({52.f + i * 28.f, 30.f, 0});

    ambient_.clear();
    for (int i = 0; i < 16; i++) {
        Bit a;
        a.x = 64.f + rnd() * 190.f;
        a.y = 90.f + rnd() * 680.f;
        a.vx = rnd() * 6.28f;
        a.vy = 16.f + rnd() * 26.f;
        a.life = -1.f;
        ambient_.push_back(a);
    }
    bits_.clear();

    sys.apu.setMaster(0.72f);
    sys.apu.setPatch(0, tubaPatch());
    sys.apu.setPatch(1, trumpetPatch());
    sys.apu.setPatch(2, blipPatch());
    sys.apu.setPan(0, -0.35f);
    sys.apu.setPan(1, 0.28f);
    sys.setLight(90, 40, 140);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    bool go = confirm();
    bool started = false;
    if (mode_ == Mode::Title && (bot_ || go)) {
        begin();
        started = true;
    } else if ((mode_ == Mode::Win || mode_ == Mode::Over) && !bot_ && go) {
        begin();
        started = true;
    }

    if (mode_ == Mode::Play && !started && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        sys.apu.keyOff(0);
        sys.apu.keyOff(1);
        sys.apu.keyOff(2);
    } else if (mode_ == Mode::Pause && go) {
        mode_ = Mode::Play;
    }

    if (mode_ == Mode::Play) updatePlay();
    else if (mode_ != Mode::Pause) {
        t_ += DT;
        tickBits();
    }
    if (hurtFlash_ > 0.f) hurtFlash_ -= DT;
    if (shake_ > 0.f) shake_ *= 0.84f;
    music();
    draw();
}

}  // namespace parade
