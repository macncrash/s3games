#include "game/curlchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace curlchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSub = 1.f / 120.f;
constexpr int kSubs = 2;
constexpr int kCap = 360;
constexpr int kFpc = 6;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 120;
constexpr int kGraceFrames = kGraceSec * kFpc - 1;
constexpr int kStones = 3;
constexpr float kDrag = 5.6f;
constexpr float kCurl = 0.72f;
constexpr float kStop = 0.14f;
constexpr float kSweepDrag = 0.26f;
constexpr float kSweepCurl = 0.50f;
constexpr float kVxLo = -3.0f;
constexpr float kVxHi = 3.0f;
constexpr float kVyLo = 7.6f;
constexpr float kVyHi = 16.0f;
constexpr float kStoneDraw = 20.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool coversButton(float x, float y, bool dead) {
    if (dead) return false;
    if (y < kHog + kStoneR) return false;
    if (std::fabs(x) > kSide) return false;
    return std::hypot(x, y - kTee) <= kButton;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 6;
    p.fb = 0.18f;
    p.op[0] = {1.0f, 1.0f, 0.004f, 0.55f, 0.18f, 0.48f};
    p.op[1] = {2.4f, 0.28f, 0.003f, 0.40f, 0.0f, 0.30f};
    p.op[2] = {3.8f, 0.14f, 0.003f, 0.22f, 0.0f, 0.20f};
    p.op[3] = {1.5f, 0.22f, 0.005f, 0.70f, 0.10f, 0.40f};
    p.vol = 0.28f;
    p.echo = 0.34f;
    p.tone = 1600.f;
    return p;
}

}  // namespace

bool Game::onButton() const { return won_ && coversButton(stone_.x, stone_.y, stone_.dead); }

void Game::shove(Stone& r, float sweep, float dt) const {
    float sp = std::hypot(r.vx, r.vy);
    if (r.dead || sp < kStop) {
        r.vx = r.vy = 0;
        return;
    }
    float drag = kDrag * (1.f - kSweepDrag * sweep);
    if (drag < 1.2f) drag = 1.2f;
    float curl = kCurl * (1.f - kSweepCurl * sweep) * float(r.handle);
    float inv = 1.f / sp;
    float ax = -drag * r.vx * inv + curl * r.vy * inv;
    float ay = -drag * r.vy * inv - curl * r.vx * inv;
    float nx = r.vx + ax * dt;
    float ny = r.vy + ay * dt;
    if (nx * r.vx + ny * r.vy <= 0.f || std::hypot(nx, ny) < kStop) {
        float t = sp / drag;
        if (t > dt) t = dt;
        r.x += r.vx * t;
        r.y += r.vy * t;
        r.vx = r.vy = 0;
    } else {
        r.vx = nx;
        r.vy = ny;
        r.x += r.vx * dt;
        r.y += r.vy * dt;
    }
    if (std::fabs(r.x) > kSide || r.y > kBack + 0.08f) {
        r.dead = true;
        r.vx = r.vy = 0;
    }
}

bool Game::sliding() const {
    return mode_ == Mode::Slide && slideFrames_ > 6 && runFrames_ > 12 && slideFrames_ < runFrames_ - 4;
}

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return (secLeft - 1) * kFpc + (kFpc - sub);
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    h = t / 3600;
    m = (t / 60) % 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    split(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    split(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    split(h, m, s);
    return s;
}

void Game::faceTime(int& h, int& m, int& s) const {
    if (mode_ == Mode::Title) {
        h = 11;
        m = 58;
        s = (titleFrames_ / 4) % 60;
        return;
    }
    split(h, m, s);
}

const char* Game::lieName(const Stone& s) const {
    if (s.dead) return s.y > kTee ? "HEAVY" : "WIDE";
    if (s.y < kHog + kStoneR) return "HOG";
    if (coversButton(s.x, s.y, false)) return "BUTTON";
    float d = std::hypot(s.x, s.y - kTee);
    if (d > kHouse) return s.y < kTee ? "SHORT" : "HEAVY";
    if (s.y < kTee - 0.35f) return "LIGHT";
    if (s.y > kTee + 0.35f) return "HEAVY";
    return "OFF";
}

Game::End Game::coast(float vx, float vy, int handle, float sweep, bool record) {
    Stone r;
    r.x = 0.f;
    r.y = kHack;
    r.vx = vx;
    r.vy = vy;
    r.handle = handle;
    if (record) pathN_ = 0;
    int frames = 0;
    while (frames < kCap && !r.dead && std::hypot(r.vx, r.vy) >= kStop) {
        frames++;
        for (int s = 0; s < kSubs; s++) {
            if (r.dead || std::hypot(r.vx, r.vy) < kStop) break;
            shove(r, sweep, kSub);
        }
        if (record && pathN_ < 16 && (frames % 7) == 3) {
            pathX_[pathN_] = r.x;
            pathY_[pathN_] = r.y;
            pathN_++;
        }
    }
    if (std::hypot(r.vx, r.vy) < kStop) r.vx = r.vy = 0;
    End e;
    e.x = r.x;
    e.y = r.y;
    e.frames = frames;
    e.dead = r.dead || std::hypot(r.vx, r.vy) >= kStop;
    e.hog = !r.dead && r.y < kHog + kStoneR;
    e.button = coversButton(r.x, r.y, e.dead);
    return e;
}

void Game::solve() {
    float bestRank = 1e9f;
    float bestD = 99.f;
    float bx = 0.f, by = 12.f;
    int bh = 0;
    auto take = [&](float vx, float vy, int h) {
        End e = coast(vx, vy, h, 0.f, false);
        if (e.frames < 24 || e.frames >= kCap || e.dead) return;
        float d = std::hypot(e.x, e.y - kTee);
        float rank = (d <= kButton ? 0.f : 1000.f) + (h != 0 ? 0.f : 10.f) + d;
        if (rank < bestRank) {
            bestRank = rank;
            bestD = d;
            bx = vx;
            by = vy;
            bh = h;
        }
    };

    float lo = 8.2f, hi = 15.2f;
    for (int i = 0; i < 16; i++) {
        float mid = (lo + hi) * 0.5f;
        End e = coast(0.f, mid, 0, 0.f, false);
        if (e.dead || e.y > kTee) hi = mid;
        else lo = mid;
    }
    for (float dvy = -0.40f; dvy <= 0.401f; dvy += 0.05f) take(0.f, lo + dvy, 0);

    for (int h : {1, -1}) {
        for (float vy = lo - 0.20f; vy <= lo + 1.55f; vy += 0.07f) {
            for (float vx = -2.1f; vx <= 2.1f; vx += 0.07f) take(vx, vy, h);
            if (bestD <= 0.08f && bh != 0) break;
        }
        if (bestD <= 0.08f && bh != 0) break;
    }
    for (float dvy = -0.10f; dvy <= 0.101f; dvy += 0.02f)
        for (float dvx = -0.10f; dvx <= 0.101f; dvx += 0.02f) take(bx + dvx, by + dvy, bh);

    solVx_ = bx;
    solVy_ = by;
    solHandle_ = bh;
    preview_ = coast(bx, by, bh, 0.f, true);
    solDist_ = std::hypot(preview_.x, preview_.y - kTee);
    runFrames_ = preview_.frames;
    solved_ = preview_.button && runFrames_ > 24 && runFrames_ < kCap;
    if (!solved_)
        std::fprintf(stderr, "s3curlchime no draw  dist %.3f  frames %d\n", solDist_, runFrames_);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.22f, 0.30f, 0.18f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPan(0, 0.15f);
    solve();
    showTitle();
    draw();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    titleFrames_ = 0;
    playFrames_ = 0;
    over_ = false;
    won_ = false;
    thrown_ = 0;
    reason_ = "";
    stone_ = {};
    sweep_ = 0;
    wasWindow_ = false;
    lastSec_ = -1;
    strikes_ = 0;
    handle_ = solHandle_;
    aimVx_ = solVx_;
    aimVy_ = solVy_;
    for (auto& p : puff_) p.life = 0;
    sys_->setLight(70, 110, 150);
    sys_->apu.keyOff(0);
    sys_->apu.noise(0.f, 800.f, false);
}

void Game::newGame() {
    over_ = false;
    won_ = false;
    thrown_ = 0;
    playFrames_ = 0;
    slideFrames_ = 0;
    againFrames_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    strikes_ = 0;
    beep_ = 0;
    lastSec_ = -1;
    reason_ = "";
    wasWindow_ = false;
    stone_ = {};
    sweep_ = 0;
    handle_ = solHandle_;
    aimVx_ = solVx_;
    aimVy_ = solVy_;
    for (auto& p : puff_) p.life = 0;
    mode_ = Mode::Aim;
    sys_->setLight(60, 120, 170);
    sys_->apu.keyOff(0);
}

void Game::launch() {
    stone_ = {};
    stone_.x = 0.f;
    stone_.y = kHack;
    stone_.vx = aimVx_;
    stone_.vy = aimVy_;
    stone_.handle = handle_;
    stone_.live = true;
    thrown_++;
    sweep_ = 0.f;
    slideFrames_ = 0;
    mode_ = Mode::Slide;
    blip(150.f);
    sys_->rumble(0.12f, 0.22f, 50);
    sys_->apu.noiseBurst(0.12f, 900.f, 0.05f);
}

void Game::beginAgain(const char* why) {
    reason_ = why;
    lieX_ = stone_.x;
    lieY_ = stone_.y;
    stone_.live = false;
    stone_.vx = stone_.vy = 0;
    mode_ = Mode::Again;
    againFrames_ = 0;
    sweep_ = 0.f;
    sys_->setLight(150, 50, 40);
    blip(110.f);
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    stone_.vx = stone_.vy = 0;
    stone_.live = true;
    sys_->setLight(255, 190, 60);
    sys_->rumble(0.4f, 0.8f, 180);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    lieX_ = stone_.x;
    lieY_ = stone_.y;
    stone_.live = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    sweep_ = 0.f;
    blip(90.f);
    sys_->setLight(140, 24, 24);
}

void Game::settle() {
    stone_.vx = stone_.vy = 0;
    bool button = coversButton(stone_.x, stone_.y, stone_.dead);
    if (button && onHour()) {
        beginChime();
        return;
    }
    const char* why = lieName(stone_);
    if (button && !pastHour()) why = "EARLY";
    if (pastHour()) beginFail(button ? "LATE" : why);
    else if (thrown_ >= kStones) beginFail(button ? "EARLY" : why);
    else beginAgain(why);
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(2, freq, 0.05f);
    beep_ = 6;
}

void Game::strikeBell() {
    static const float peal[] = {392.0f, 494.0f, 587.3f, 784.0f};
    sys_->apu.keyOn(0, peal[strikes_ % 4], 0.32f);
    sys_->apu.tone(1, peal[strikes_ % 4] * 0.5f, 0.04f);
    strikes_++;
    beep_ = 8;
}

void Game::tickSound() {
    int sec = clockSec();
    if (sec == lastSec_) return;
    int prev = lastSec_;
    lastSec_ = sec;
    if (prev < 0) return;
    int until = framesUntilHour();
    if (until > 0 && until <= kFpc * 18) blip(until <= kFpc * 6 ? 880.f : 620.f);
    else if (sec % 10 == 0) blip(196.f);
}

void Game::decayAudio() {
    if (!sys_) return;
    if (beep_ > 0 && --beep_ == 0 && mode_ != Mode::Chime && !(mode_ == Mode::Over && won_)) {
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Slide)
        sys_->apu.noise(sweep_ > 0.5f ? 0.045f : 0.014f, sweep_ > 0.5f ? 2100.f : 640.f, false);
    else sys_->apu.noise(0.f, 800.f, false);
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 8 || s.y > gs::SCREEN_H + 8 || s.x + s.w < -8 || s.y + s.h < -8) return;
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * float(m.w) / float(m.h);
    spr(m.pick(h), cx, cy, w, h, pal, flip, false);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
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
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::clockAt(float cx, float cy, float size, bool live) {
    int h, m, s;
    faceTime(h, m, s);
    int hi = ((h % 12) * 5 + m / 12) % 60;
    bool hot = mode_ == Mode::Chime || (mode_ == Mode::Over && won_) || (live && onHour());
    int until = framesUntilHour();
    if (live && until > 0 && until <= kFpc * 12) hot = true;
    float swing = std::sin(bellPh_) * (hot ? 7.f : 2.2f);
    if (hot) spr(art_.ring, cx, cy, size * 1.18f, size * 1.18f, PAL_GOLD);
    spr(art_.cap, cx, cy, size * 0.16f, size * 0.16f, PAL_HAND);
    spr(art_.hand[2][s % 60], cx, cy, size, size, PAL_HAND);
    spr(art_.hand[1][m % 60], cx, cy, size, size, PAL_HAND);
    spr(art_.hand[0][hi], cx, cy, size, size, PAL_HAND);
    spr(art_.face, cx, cy, size, size, PAL_FACE);
    float by = cy + size * 0.62f;
    spr(art_.clapper, cx - 16.f + swing, by + 4.f, 5.f, 8.f, PAL_BELL);
    spr(art_.bell, cx - 16.f + swing * 0.6f, by, 16.f, 20.f, PAL_BELL);
    spr(art_.clapper, cx + 16.f - swing, by + 4.f, 5.f, 8.f, PAL_BELL, true);
    spr(art_.bell, cx + 16.f - swing * 0.6f, by, 16.f, 20.f, PAL_BELL, true);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = true;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.lineBackdrop[y] = gs::rgb4(2, 1, 1);
    }

    const Mode view = mode_ == Mode::Pause ? held_ : mode_;
    const bool victory = view == Mode::Chime || (view == Mode::Over && won_);
    const bool aiming = view == Mode::Title || view == Mode::Aim;
    const bool showPath = view == Mode::Title || view == Mode::Aim;

    clockAt(278.f, 74.f, 58.f, view != Mode::Title);

    if (victory) {
        float pulse = 10.f + float(chimeFrames_ % 20);
        for (int i = 0; i < 8; i++) {
            float a = bellPh_ + float(i) * 0.785f;
            spr(art_.dot, screenX(0.f) + std::cos(a) * pulse, screenY(kTee) + std::sin(a) * pulse * 0.65f, 4.f, 4.f,
                PAL_WIN);
        }
    }

    if (showPath) {
        int pal = preview_.button ? PAL_GREEN : PAL_DIM;
        for (int i = 0; i < pathN_; i++) spr(art_.dot, screenX(pathX_[i]), screenY(pathY_[i]), 3.f, 3.f, pal);
    }

    auto shadowAt = [&](float wx, float wy) {
        spr(art_.shadow, screenX(wx) + 1.f, screenY(wy) + 3.f, kStoneDraw * 0.9f, kStoneDraw * 0.32f, PAL_INK, false,
            true);
    };
    auto stoneAt = [&](float wx, float wy, int pal, int handle) {
        sprM(art_.stone, screenX(wx), screenY(wy), kStoneDraw, pal, handle < 0);
    };

    float rockX = 0.f, rockY = kHack;
    bool haveRock = false;
    if (stone_.live) {
        rockX = stone_.x;
        rockY = stone_.y;
        haveRock = true;
    } else if (view == Mode::Again || view == Mode::Fail) {
        rockX = lieX_;
        rockY = lieY_;
        haveRock = true;
    }

    if (aiming) {
        int pal = preview_.button ? PAL_YEL : PAL_GHOST;
        stoneAt(preview_.x, preview_.y, pal, handle_);
        shadowAt(preview_.x, preview_.y);
        stoneAt(0.f, kHack, PAL_RED, handle_);
        shadowAt(0.f, kHack);
    }
    if (haveRock && !aiming) {
        int pal = victory ? PAL_YEL : PAL_RED;
        stoneAt(rockX, rockY, pal, stone_.live ? stone_.handle : handle_);
        shadowAt(rockX, rockY);
    } else if (aiming) {
        rockX = 0.f;
        rockY = kHack;
    }

    float skipX = (haveRock || aiming ? rockX : 0.f) + 1.35f;
    float skipY = (view == Mode::Slide ? rockY - 1.05f : (aiming ? kHack : rockY - 0.4f));
    if (skipY < kHack - 0.2f) skipY = kHack - 0.2f;
    if (skipX > kSide - 0.35f) skipX = rockX - 1.35f;
    float bob = std::sin(anim_ * 0.35f) * 1.4f;
    int pose = (view == Mode::Slide && slideFrames_ < 22) ? 1 : 0;
    float sh = pose ? 22.f : 30.f;
    sprM(art_.skip[pose], screenX(skipX), screenY(skipY) + bob, sh, PAL_SKIP, skipX < rockX);

    bool sweeping = view == Mode::Slide && sweep_ > 0.5f;
    float broomX = sweeping ? rockX + 0.45f : skipX + 0.2f;
    float broomY = sweeping ? rockY : skipY + 0.15f;
    float wag = sweeping ? std::sin(anim_ * 0.9f) * 3.f : 0.f;
    spr(art_.broom, screenX(broomX) + wag, screenY(broomY) - 8.f, float(art_.broom.w), float(art_.broom.h), PAL_BROOM,
        handle_ < 0);
    for (const auto& pf : puff_) {
        if (pf.life <= 0.f) continue;
        float h = 4.f + pf.life * 12.f;
        spr(art_.puff, screenX(pf.x), screenY(pf.y), h, h * 0.8f, PAL_PUFF);
    }

    if (view == Mode::Aim && preview_.button) {
        float g = 5.f + std::sin(anim_ * 0.25f) * 1.5f;
        spr(art_.dot, screenX(0.f), screenY(kTee), g, g, PAL_GOLD);
    }

    char buf[48];
    if (view == Mode::Title) {
        hudC(0, "S3 CURLCHIME", PAL_GOLD);
        hudC(1, "THE HOUR HAS TO CHIME", PAL_INK);
        hud(1, 5, "DRAW", PAL_GOLD);
        hud(1, 6, "THE", PAL_INK);
        hud(1, 7, "BUTTON", PAL_GOLD);
        hud(1, 9, "ON", PAL_INK);
        hud(1, 10, "TWELVE", PAL_WIN);
        hud(1, 13, "3 STONES", PAL_INK);
        hud(1, 15, "EARLY", PAL_ALERT);
        hud(1, 16, "IS LIFTED", PAL_DIM);
        hudC(26, "ARROWS LINE   X HANDLE   Z THROW", PAL_INK);
        hudC(27, "HOLD Z TO SWEEP     START", PAL_DIM);
        return;
    }

    int h, m, s;
    split(h, m, s);
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    hudC(1, buf, onHour() || victory ? PAL_WIN : PAL_GOLD);

    if (victory) {
        hudC(0, "THE HOUR CHIMES", PAL_WIN);
        std::snprintf(buf, sizeof buf, "STONE %d", thrown_);
        hud(1, 4, buf, PAL_GOLD);
        hud(1, 6, "BUTTON", PAL_GREEN);
        hudC(26, "TWELVE BELLS", PAL_GOLD);
        if (view == Mode::Over && !bot_) hudC(27, "START", PAL_DIM);
        return;
    }
    if (view == Mode::Fail || (view == Mode::Over && !won_)) {
        hudC(0, "THE HOUR DID NOT CHIME", PAL_ALERT);
        hudC(26, reason_[0] ? reason_ : "LATE", PAL_ALERT);
        if (!bot_) hudC(27, "START RETRY", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME    ESC TITLE", PAL_INK);
        return;
    }

    hud(1, 0, "S3 CURLCHIME", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "STONE %d", std::min(thrown_ + (view == Mode::Slide ? 0 : 1), kStones));
    hud(1, 4, buf, PAL_INK);
    hud(1, 6, handle_ > 0 ? "CURL R" : handle_ < 0 ? "CURL L" : "NO CURL", PAL_GOLD);

    int until = framesUntilHour();
    bool window = preview_.button && until <= preview_.frames && until >= preview_.frames - kGraceFrames;
    const char* call = "LINE";
    int cpal = PAL_INK;
    if (view == Mode::Slide) {
        call = sweep_ > 0.5f ? "SWEEP" : "LET IT";
        cpal = sweep_ > 0.5f ? PAL_WIN : PAL_DIM;
    } else if (view == Mode::Again) {
        call = reason_;
        cpal = PAL_ALERT;
    } else if (window) {
        call = "THROW";
        cpal = PAL_WIN;
    } else if (preview_.button) {
        call = "WAIT";
        cpal = PAL_GOLD;
    } else if (preview_.hog) {
        call = "HOG";
        cpal = PAL_ALERT;
    } else if (preview_.dead) {
        call = preview_.y > kTee ? "HEAVY" : "WIDE";
        cpal = PAL_ALERT;
    } else {
        call = preview_.y < kTee ? "LIGHT" : "HEAVY";
        cpal = PAL_DIM;
    }
    hud(1, 8, call, cpal);

    int wt = int(clampf((aimVy_ - kVyLo) / (kVyHi - kVyLo), 0.f, 1.f) * 6.f + 0.5f);
    int ln = int(clampf((aimVx_ - kVxLo) / (kVxHi - kVxLo), 0.f, 1.f) * 6.f + 0.5f);
    std::snprintf(buf, sizeof buf, "WT %.*s", wt, "######");
    hud(1, 11, buf, PAL_DIM);
    std::snprintf(buf, sizeof buf, "LN %.*s", ln, "######");
    hud(1, 12, buf, PAL_DIM);

    if (view == Mode::Aim) hud(1, 15, window ? "Z NOW" : "Z THROW", window ? PAL_WIN : PAL_DIM);
    else if (view == Mode::Slide) hud(1, 15, "HOLD Z", PAL_DIM);
    else hud(1, 15, "LIFTED", PAL_ALERT);
    hudC(26, "ARROWS LINE   X HANDLE", PAL_DIM);
    hudC(27, "BUTTON ON TWELVE", onHour() ? PAL_WIN : PAL_INK);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += 1.f;
    bellPh_ += (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) ? 0.45f : 0.05f;
    decayAudio();

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
        else if (back) showTitle();
        draw();
        return;
    }

    if (mode_ == Mode::Title) titleFrames_++;
    if (mode_ == Mode::Aim || mode_ == Mode::Slide) {
        playFrames_++;
        tickSound();
    }

    if (back && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        else if (!bot_) sys.quit();
        draw();
        return;
    }
    if (back && !bot_ && mode_ != Mode::Chime && mode_ != Mode::Over && mode_ != Mode::Fail) {
        showTitle();
        draw();
        return;
    }
    if (start && !bot_ && mode_ != Mode::Title && mode_ != Mode::Over && mode_ != Mode::Fail && mode_ != Mode::Chime &&
        mode_ != Mode::Again) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Title) {
        if (bot_) {
            if (titleFrames_ >= 40) newGame();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Aim) {
        if (!bot_) {
            if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X)) {
                if (handle_ > 0) handle_ = 0;
                else if (handle_ == 0) handle_ = -1;
                else handle_ = 1;
            }
            float mx = 0.f, my = 0.f;
            if (pad.down(gs::BTN_LEFT)) mx -= 1.f;
            if (pad.down(gs::BTN_RIGHT)) mx += 1.f;
            if (pad.down(gs::BTN_UP)) my += 1.f;
            if (pad.down(gs::BTN_DOWN)) my -= 1.f;
            if (std::fabs(pad.axisX) > 0.25f) mx = pad.axisX;
            if (std::fabs(pad.axisY) > 0.25f) my = pad.axisY;
            aimVx_ = clampf(aimVx_ + mx * 1.35f * kDt, kVxLo, kVxHi);
            aimVy_ = clampf(aimVy_ + my * 2.1f * kDt, kVyLo, kVyHi);
        } else {
            aimVx_ = solVx_;
            aimVy_ = solVy_;
            handle_ = solHandle_;
        }
        preview_ = coast(aimVx_, aimVy_, handle_, 0.f, true);
        int until = framesUntilHour();
        bool window = preview_.button && until <= preview_.frames && until >= preview_.frames - kGraceFrames;
        if (window && !wasWindow_) blip(990.f);
        wasWindow_ = window;
        sys.setLight(window ? 220 : 60, window ? 170 : 120, window ? 50 : 170);
        if (pastHour()) beginFail("LATE");
        else if (bot_) {
            if (preview_.button && until == preview_.frames) launch();
        } else if (fire) launch();
    } else if (mode_ == Mode::Slide) {
        slideFrames_++;
        bool held = !bot_ && (pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO));
        sweep_ = held ? 1.f : 0.f;
        for (int s = 0; s < kSubs; s++) {
            if (stone_.dead || std::hypot(stone_.vx, stone_.vy) < kStop) break;
            shove(stone_, sweep_, kSub);
        }
        if (sweep_ > 0.5f && !stone_.dead) {
            for (auto& pf : puff_) {
                if (pf.life > 0.f) continue;
                pf.x = stone_.x + (handle_ > 0 ? 0.35f : -0.35f);
                pf.y = stone_.y;
                pf.life = 0.28f;
                break;
            }
        }
        for (auto& pf : puff_)
            if (pf.life > 0.f) pf.life -= kDt;
        if (stone_.dead || std::hypot(stone_.vx, stone_.vy) < kStop) settle();
    } else if (mode_ == Mode::Again) {
        if (++againFrames_ >= 36) {
            if (pastHour()) beginFail("LATE");
            else {
                mode_ = Mode::Aim;
                wasWindow_ = false;
            }
        }
    } else if (mode_ == Mode::Chime) {
        if (chimeFrames_ % 6 == 1 && strikes_ < 12) strikeBell();
        if (++chimeFrames_ >= 84) {
            mode_ = Mode::Over;
            over_ = true;
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ >= 48) {
            mode_ = Mode::Over;
            over_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (start || fire)) newGame();
    }

    draw();
}

}  // namespace curlchime
