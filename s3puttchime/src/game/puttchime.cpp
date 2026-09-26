#include "game/puttchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace puttchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr int kFpc = 6;
constexpr int kGraceSec = 24;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 120;
constexpr int kInto = 3;

constexpr float kStartX = 150.f;
constexpr float kStartY = 186.f;
constexpr float kCupX = 168.f;
constexpr float kCupY = 122.f;
constexpr float kCupR = 8.5f;
constexpr float kDropV = 128.f;
constexpr float kStopV = 7.f;
constexpr float kAx = 36.f;
constexpr float kAy = -16.f;
constexpr float kMu = 62.f;
constexpr float kWallY = 108.f;
constexpr float kWallL = 126.f;
constexpr float kWallR = 210.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 6;
    p.fb = 0.18f;
    p.op[0] = {1.0f, 1.0f, 0.004f, 0.52f, 0.14f, 0.46f};
    p.op[1] = {2.6f, 0.28f, 0.003f, 0.34f, 0.0f, 0.26f};
    p.op[2] = {4.2f, 0.14f, 0.003f, 0.18f, 0.0f, 0.16f};
    p.op[3] = {1.5f, 0.22f, 0.005f, 0.58f, 0.06f, 0.34f};
    p.vol = 0.28f;
    p.echo = 0.28f;
    p.tone = 1600.f;
    return p;
}

float baseAim() { return std::atan2(kCupY - kStartY, kCupX - kStartX); }

}  // namespace

Game::Halt Game::stepLie(Lie& b, float* arrive, int* lips) {
    if (b.rest) return Halt::Rest;
    b.vx += kAx * kDt;
    b.vy += kAy * kDt;
    float sp = std::hypot(b.vx, b.vy);
    bool fringe = b.x < 46.f || b.x > 274.f || b.y > 202.f;
    float drop = (fringe ? kMu * 3.2f : kMu) * kDt;
    if (sp <= drop) {
        b.vx = b.vy = 0;
    } else {
        float s = (sp - drop) / sp;
        b.vx *= s;
        b.vy *= s;
    }
    b.x += b.vx * kDt;
    b.y += b.vy * kDt;

    float dx = b.x - kCupX;
    float dy = b.y - kCupY;
    float d = std::hypot(dx, dy);
    sp = std::hypot(b.vx, b.vy);
    if (d < kCupR) {
        if (sp <= kDropV) {
            if (arrive) *arrive = sp;
            b.x = kCupX;
            b.y = kCupY;
            b.vx = b.vy = 0;
            b.rest = true;
            return Halt::Drop;
        }
        if (lips) (*lips)++;
        float nx = d > 0.05f ? dx / d : 0.f;
        float ny = d > 0.05f ? dy / d : 1.f;
        b.x = kCupX + nx * (kCupR + 1.4f);
        b.y = kCupY + ny * (kCupR + 1.4f);
        float vn = b.vx * nx + b.vy * ny;
        if (vn < 0.f) {
            b.vx -= 1.7f * vn * nx;
            b.vy -= 1.7f * vn * ny;
        }
        b.vx *= 0.6f;
        b.vy *= 0.6f;
    }

    if (b.y < kWallY && b.x > kWallL && b.x < kWallR) {
        b.y = kWallY;
        b.vx = b.vy = 0;
        b.rest = true;
        return Halt::Rest;
    }
    if (b.x < 24.f || b.x > 296.f || b.y > 220.f || b.y < 72.f) {
        b.vx = b.vy = 0;
        b.rest = true;
        return Halt::Rest;
    }
    sp = std::hypot(b.vx, b.vy);
    if (sp < kStopV) {
        b.vx = b.vy = 0;
        b.rest = true;
        return Halt::Rest;
    }
    return Halt::Fly;
}

int Game::clockSec() const {
    int t = kStartSec + playFrames_ / kFpc;
    return t < 0 ? 0 : t;
}

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
        s = (titleFrames_ / kFpc) % 60;
        return;
    }
    split(h, m, s);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(8, 12, 15));
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.18f, 0.28f, 0.14f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPan(0, 0.02f);
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    titleFrames_ = 0;
    playFrames_ = 0;
    over_ = false;
    won_ = false;
    holed_ = false;
    swinging_ = false;
    botReady_ = false;
    solved_ = false;
    strokes_ = 0;
    lastSec_ = -1;
    tickLeft_ = 0;
    strikes_ = 0;
    bellAmp_ = 0.12f;
    reason_ = "";
    ball_ = Lie{};
    ball_.x = kStartX;
    ball_.y = kStartY;
    ball_.rest = true;
    aim_ = baseAim();
    silenceTicks();
    sys_->apu.keyOff(0);
}

void Game::newGame() {
    over_ = false;
    won_ = false;
    holed_ = false;
    strokes_ = 0;
    playFrames_ = 0;
    rollFrames_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    strikes_ = 0;
    lastSec_ = -1;
    reason_ = "";
    botReady_ = false;
    solved_ = false;
    if (bot_) {
        float a = baseAim(), v = 160.f;
        int travel = 0;
        if (solve(a, v, travel)) {
            botAng_ = a;
            botSpd_ = v;
            botTravel_ = travel;
            botReady_ = true;
            solved_ = true;
            aim_ = a;
        } else {
            reason_ = "NO LINE";
        }
    }
    beginAim();
    sys_->setLight(40, 110, 60);
}

void Game::beginAim() {
    ball_ = Lie{};
    ball_.x = kStartX;
    ball_.y = kStartY;
    ball_.rest = true;
    if (!bot_) aim_ = baseAim();
    else if (botReady_) aim_ = botAng_;
    meter_ = 0;
    meterDir_ = 1.f;
    swinging_ = false;
    rollFrames_ = 0;
    earlyFrames_ = 0;
    missFrames_ = 0;
    holed_ = false;
    mode_ = Mode::Aim;
}

void Game::putt(float ang, float spd) {
    aim_ = ang;
    ball_.x = kStartX;
    ball_.y = kStartY;
    ball_.vx = std::cos(ang) * spd;
    ball_.vy = std::sin(ang) * spd;
    ball_.rest = false;
    swinging_ = false;
    rollFrames_ = 0;
    strokes_++;
    mode_ = Mode::Roll;
    if (!sys_) return;
    sys_->apu.noiseBurst(0.2f, 1600.f, 0.05f);
    blip(196.f, 0.06f);
    sys_->rumble(0.12f, 0.22f, 40);
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    holed_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    bellAmp_ = 1.f;
    ball_.x = kCupX;
    ball_.y = kCupY;
    ball_.vx = ball_.vy = 0;
    ball_.rest = true;
    if (!sys_) return;
    sys_->setLight(255, 196, 48);
    sys_->rumble(0.4f, 0.8f, 180);
    strikeBell(0);
    strikes_ = 1;
}

void Game::beginEarly() {
    holed_ = true;
    ball_.x = kCupX;
    ball_.y = kCupY;
    ball_.vx = ball_.vy = 0;
    ball_.rest = true;
    if (strokes_ >= 3 || pastHour()) {
        beginFail(pastHour() ? "LATE" : "SHORT");
        return;
    }
    reason_ = "SHORT";
    mode_ = Mode::Early;
    earlyFrames_ = 0;
    blip(120.f, 0.06f);
    if (sys_) sys_->setLight(150, 70, 30);
}

void Game::beginMiss() {
    if (strokes_ >= 3 || pastHour()) {
        beginFail(pastHour() ? "LATE" : "OPEN");
        return;
    }
    reason_ = "OPEN";
    mode_ = Mode::Miss;
    missFrames_ = 0;
    blip(90.f, 0.05f);
    if (sys_) sys_->setLight(120, 50, 30);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    blip(78.f, 0.07f);
    if (sys_) sys_->setLight(140, 24, 24);
}

bool Game::solve(float& ang, float& spd, int& travel) {
    float base = baseAim();
    bool any = false;
    float best = 1e9f;
    float bestA = base;
    float bestV = 160.f;
    int bestN = 0;
    for (float da = -0.72f; da <= 0.42f; da += 0.015f) {
        float a = base + da;
        for (float v = 108.f; v <= 228.f; v += 3.f) {
            Lie b;
            b.x = kStartX;
            b.y = kStartY;
            b.vx = std::cos(a) * v;
            b.vy = std::sin(a) * v;
            b.rest = false;
            int steps = 0;
            int lips = 0;
            float arrive = 0;
            Halt h = Halt::Fly;
            while (steps < 420 && h == Halt::Fly) {
                h = stepLie(b, &arrive, &lips);
                steps++;
            }
            if (h != Halt::Drop || steps < 36) continue;
            float score = std::fabs(da) * 5.f + std::fabs(v - 165.f) * 0.03f + std::fabs(arrive - 62.f) * 0.04f +
                          std::fabs(float(steps) - 78.f) * 0.12f + lips * 40.f;
            if (score < best) {
                best = score;
                bestA = a;
                bestV = v;
                bestN = steps;
                any = true;
            }
        }
    }
    ang = bestA;
    spd = bestV;
    travel = bestN;
    return any && bestN > kInto + 12;
}

void Game::blip(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    tickLeft_ = 7;
}

void Game::strikeBell(int n) {
    if (!sys_) return;
    static const float peal[] = {523.25f, 659.25f, 783.99f, 1046.5f};
    sys_->apu.keyOn(0, peal[n % 4]);
    sys_->apu.tone(1, peal[n % 4] * 0.5f, 0.045f);
    tickLeft_ = 8;
    bellAmp_ = 1.f;
}

void Game::silenceTicks() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::tickClock() {
    int sec = (mode_ == Mode::Title) ? (titleFrames_ / kFpc) : clockSec();
    if (sec == lastSec_) return;
    int prev = lastSec_;
    lastSec_ = sec;
    if (prev < 0) return;
    if (mode_ == Mode::Chime || mode_ == Mode::Over || mode_ == Mode::Fail) return;
    int until = framesUntilHour();
    if (mode_ != Mode::Title && until > 0 && until <= kFpc * 15) blip((sec & 1) ? 880.f : 660.f, 0.05f);
    else blip(220.f, 0.035f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow, bool feet) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::image(const gs::Image& img, float cx, float cy, float h, int pal) {
    if (!sys_ || h < 1.f || img.h == 0) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::clockAt(float cx, float cy, float size) {
    int h, m, s;
    faceTime(h, m, s);
    int hi = ((h % 12) * 5 + m / 12) % 60;
    bool noon = mode_ == Mode::Chime || (mode_ == Mode::Over && won_) || (mode_ != Mode::Title && onHour());
    if (noon) image(art_.ring, cx, cy, size * 1.08f, PAL_GOLD);
    image(art_.cap, cx, cy, size * 0.16f, PAL_HAND);
    image(art_.hand[2][s % 60], cx, cy, size, PAL_HAND);
    image(art_.hand[1][m % 60], cx, cy, size, PAL_HAND);
    image(art_.hand[0][hi], cx, cy, size, PAL_HAND);
    if (!noon) image(art_.ring, cx, cy, size * 1.06f, PAL_FACE);
    image(art_.face, cx, cy, size, PAL_FACE);
}

void Game::aimLine(float ang, float spd) {
    Lie b;
    b.x = kStartX;
    b.y = kStartY;
    b.vx = std::cos(ang) * spd;
    b.vy = std::sin(ang) * spd;
    b.rest = false;
    int shown = 0;
    for (int i = 0; i < 240 && shown < 10; i++) {
        Halt h = stepLie(b);
        if ((i % 6) == 5) {
            spr(art_.dot, b.x, b.y, 4.f, PAL_AIM);
            shown++;
        }
        if (h != Halt::Fly) break;
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::lawn() {
    gs::VDP& v = sys_->vdp;
    bool win = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    bool dead = mode_ == Mode::Fail || (mode_ == Mode::Over && !won_ && reason_[0]);
    uint16_t top = win ? gs::rgb4(8, 8, 12) : dead ? gs::rgb4(4, 2, 4) : gs::rgb4(5, 9, 14);
    uint16_t mid = win ? gs::rgb4(13, 10, 6) : dead ? gs::rgb4(7, 3, 3) : gs::rgb4(9, 13, 15);
    uint16_t hor = win ? gs::rgb4(15, 12, 6) : dead ? gs::rgb4(5, 2, 2) : gs::rgb4(12, 14, 12);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = clampf(y / 78.f, 0.f, 1.f);
        v.lineBackdrop[y] = u < 0.65f ? mix(top, mid, u / 0.65f) : mix(mid, hor, (u - 0.65f) / 0.35f);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = true;
    v.hudEnabled = true;
    lawn();

    bool sunk = (mode_ == Mode::Chime || (mode_ == Mode::Over && won_) || mode_ == Mode::Early) && holed_;
    int pose = 0;
    if (mode_ == Mode::Roll && rollFrames_ < 18) pose = 1;
    else if ((mode_ == Mode::Chime || (mode_ == Mode::Over && won_))) pose = 2;
    int spin = (rollFrames_ / 3) & 1;
    int flagF = ((playFrames_ + titleFrames_) / 8) & 1;
    float swing = std::sin(bellPh_) * 7.f * bellAmp_;

    bool showLine = mode_ == Mode::Title || mode_ == Mode::Aim || (mode_ == Mode::Pause && held_ == Mode::Aim);
    if (showLine) {
        float spd = 155.f;
        float ang = aim_;
        if (bot_ && botReady_) {
            ang = botAng_;
            spd = botSpd_;
        } else if (swinging_) {
            spd = 96.f + meter_ * 132.f;
        }
        aimLine(ang, spd);
    }

    spr(art_.golfer[pose], kStartX - 16.f, kStartY + 10.f, 40.f, PAL_PLAYER);
    float ballH = sunk ? 7.f : 11.f;
    float ballY = sunk ? kCupY + 2.f : ball_.y;
    float ballX = sunk ? kCupX : ball_.x;
    spr(art_.ball[spin], ballX, ballY, ballH, PAL_BALL);
    spr(art_.shadow, ballX + 2.f, ballY + 5.f, 6.f, PAL_BALL, false, true);
    spr(art_.flag[flagF], kCupX - 2.f, kCupY + 2.f, 34.f, PAL_FLAG, false, false, true);
    spr(art_.cup, kCupX, kCupY + 2.f, 16.f, PAL_CUP);

    const float flowers[][2] = {{48, 150}, {58, 176}, {270, 146}, {262, 178}, {78, 132}};
    for (int i = 0; i < 5; i++) spr(art_.flower, flowers[i][0], flowers[i][1], 12.f, PAL_FLAG);

    float bdx = kAx, bdy = kAy;
    float bl = std::hypot(bdx, bdy);
    if (bl > 1.f) {
        bdx /= bl;
        bdy /= bl;
        for (int i = 0; i < 4; i++) {
            float x = 118.f + bdx * (10.f + i * 16.f);
            float y = 156.f + bdy * (6.f + i * 10.f);
            bool flip = bdx < 0.f;
            spr(art_.borrow, x, y, 8.f, PAL_AIM, flip);
        }
    }

    clockAt(160.f, 40.f, 58.f);
    spr(art_.bell, 128.f + swing, 74.f, 16.f, PAL_BELL);
    spr(art_.bell, 192.f - swing, 74.f, 16.f, PAL_BELL, true);
    spr(art_.tower, 160.f, 78.f, 108.f, PAL_TOWER);

    spr(art_.tree, 26.f, 108.f, 64.f, PAL_TREE);
    spr(art_.tree, 294.f, 104.f, 58.f, PAL_TREE, true);
    spr(art_.tree, 64.f, 92.f, 40.f, PAL_TREE);
    spr(art_.cloud, 54.f + std::sin(cloud_) * 6.f, 16.f, 16.f, PAL_SKY);
    spr(art_.cloud, 236.f + std::sin(cloud_ * 0.8f) * 4.f, 12.f, 12.f, PAL_SKY);
    spr(art_.sun, 292.f, 22.f, 16.f, PAL_GOLD);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(16, "A SHORT PUTT", PAL_GOLD);
        hudC(18, "HOLE IT AS THE HOUR CHIMES", PAL_HUD);
        hudC(20, "EARLY DOES NOT COUNT", PAL_GREEN);
        if ((titleFrames_ / 30) & 1) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "LEFT RIGHT AIM   HOLD Z", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME   ESC TITLE", PAL_HUD);
    } else if ((mode_ == Mode::Over && won_) || mode_ == Mode::Chime) {
        int h, m, s;
        split(h, m, s);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
        hud(1, 0, buf, PAL_GOLD);
        hudC(11, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "PUTT %d", strokes_);
        hudC(13, buf, PAL_HUD);
        if (mode_ == Mode::Over && !bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        hudC(11, pastHour() || std::strcmp(reason_, "LATE") == 0 ? "THE HOUR PASSED" : "NO CHIME", PAL_ALERT);
        hudC(13, reason_[0] ? reason_ : "OPEN", PAL_HUD);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else if (mode_ == Mode::Early) {
        hudC(11, "SHORT OF THE HOUR", PAL_ALERT);
        hudC(13, "THE CUP GIVES IT BACK", PAL_GOLD);
    } else if (mode_ == Mode::Miss) {
        hudC(11, "THE CUP IS OPEN", PAL_ALERT);
        hudC(13, strokes_ >= 2 ? "ONE PUTT LEFT" : "STILL SHORT OF THE HOUR", PAL_GOLD);
    } else {
        int h, m, s;
        split(h, m, s);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
        int until = framesUntilHour();
        bool hot = onHour() || (until > 0 && until <= kFpc * 10);
        hud(1, 0, buf, hot ? PAL_GOLD : PAL_HUD);
        int showing = mode_ == Mode::Roll ? strokes_ : strokes_ + 1;
        std::snprintf(buf, sizeof buf, "PUTT %d/3", showing);
        hud(31, 0, buf, strokes_ >= 2 ? PAL_ALERT : PAL_GOLD);
        if (until > 0) {
            int show = (until + kFpc - 1) / kFpc;
            std::snprintf(buf, sizeof buf, "HOUR IN %d", show);
            hud(1, 1, buf, hot ? PAL_GOLD : PAL_HUD);
        } else if (onHour()) {
            hud(1, 1, "ON THE HOUR", PAL_GOLD);
        } else {
            hud(1, 1, "TOO LATE", PAL_ALERT);
        }
        if (mode_ == Mode::Roll) {
            hud(1, 2, "ROLLING", PAL_GOLD);
        } else if (swinging_) {
            int n = int(std::lround(clampf(meter_, 0.f, 1.f) * 10.f));
            std::string bar = "PACE ";
            for (int i = 0; i < 10; i++) bar += (i < n) ? ((i >= 5) ? '#' : '=') : ((i == 5) ? '|' : '.');
            hud(1, 2, bar, PAL_GOLD);
        } else {
            hud(1, 2, "HOLD Z", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    cloud_ += kDt * 0.35f;
    bellPh_ += kDt * (mode_ == Mode::Chime || (mode_ == Mode::Over && won_) ? 11.f : 1.6f);
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) bellAmp_ = std::max(0.35f, bellAmp_ - kDt * 0.35f);
    else bellAmp_ = 0.12f;

    bool ticking = mode_ == Mode::Aim || mode_ == Mode::Roll || mode_ == Mode::Early || mode_ == Mode::Miss;
    if (ticking) playFrames_++;
    if (mode_ == Mode::Title) titleFrames_++;
    tickClock();
    if (tickLeft_ > 0 && mode_ != Mode::Chime) {
        if (--tickLeft_ == 0) silenceTicks();
    }

    const gs::Pad& pad = sys.pad;
    bool action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    if (bot_) {
        start = false;
        back = false;
        action = false;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) showTitle();
        draw();
        return;
    }
    if (back && mode_ == Mode::Title) {
        sys.quit();
        draw();
        return;
    }
    if (back && mode_ != Mode::Chime && mode_ != Mode::Over && mode_ != Mode::Fail) {
        showTitle();
        draw();
        return;
    }
    if (start && mode_ != Mode::Title && mode_ != Mode::Over && mode_ != Mode::Fail && mode_ != Mode::Chime &&
        mode_ != Mode::Early && mode_ != Mode::Miss) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Title) {
        if (bot_) {
            if (titleFrames_ >= 40) newGame();
        } else if (start || action) {
            newGame();
        }
    } else if (mode_ == Mode::Over || mode_ == Mode::Fail) {
        if (mode_ == Mode::Fail && !over_ && ++failFrames_ >= 72) over_ = true;
        if (!bot_ && (start || action)) newGame();
    } else if (mode_ == Mode::Aim) {
        if (pastHour()) {
            beginFail("LATE");
        } else if (bot_) {
            if (botReady_) aim_ = botAng_;
            int until = framesUntilHour();
            int want = botTravel_ - kInto;
            int land = until - botTravel_;
            bool inWindow = land <= 0 && land > -(kGraceSec * kFpc);
            if (botReady_ && (until == want || (until < want && inWindow))) putt(botAng_, botSpd_);
        } else {
            float rate = 1.25f;
            if (std::fabs(pad.axisX) > 0.2f) aim_ += pad.axisX * rate * kDt;
            else {
                if (pad.down(gs::BTN_LEFT)) aim_ -= rate * kDt;
                if (pad.down(gs::BTN_RIGHT)) aim_ += rate * kDt;
            }
            float base = baseAim();
            float da = aim_ - base;
            while (da > kPi) da -= kPi * 2.f;
            while (da < -kPi) da += kPi * 2.f;
            aim_ = base + clampf(da, -0.9f, 0.6f);
            bool hold = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
            if (hold && !swinging_) {
                swinging_ = true;
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            if (swinging_ && hold) {
                meter_ += meterDir_ * kDt / 0.85f;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                }
                if (meter_ <= 0.f) {
                    meter_ = 0.f;
                    meterDir_ = 1.f;
                }
            }
            if (swinging_ && !hold) putt(aim_, 96.f + meter_ * 132.f);
        }
    } else if (mode_ == Mode::Roll) {
        rollFrames_++;
        sys.apu.noise(0.028f, 2400.f, true);
        Halt h = stepLie(ball_);
        if (h == Halt::Drop) {
            sys.apu.noise(0, 1000);
            if (onHour()) beginChime();
            else if (pastHour()) beginFail("LATE");
            else beginEarly();
        } else if (h == Halt::Rest || rollFrames_ > 420) {
            sys.apu.noise(0, 1000);
            beginMiss();
        }
    } else if (mode_ == Mode::Early) {
        if (++earlyFrames_ > 48) {
            if (pastHour() || strokes_ >= 3) beginFail(pastHour() ? "LATE" : "SHORT");
            else beginAim();
        }
    } else if (mode_ == Mode::Miss) {
        if (++missFrames_ > 48) {
            if (pastHour() || strokes_ >= 3) beginFail(pastHour() ? "LATE" : "OPEN");
            else beginAim();
        }
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (strikes_ < 12 && (chimeFrames_ % 5) == 0) {
            strikeBell(strikes_);
            strikes_++;
        }
        if (chimeFrames_ >= 12 * 5 + 36) {
            mode_ = Mode::Over;
            over_ = true;
            sys.apu.keyOff(0);
            silenceTicks();
        }
    }

    if (mode_ != Mode::Roll) sys.apu.noise(0, 1000);
    int until = framesUntilHour();
    if (mode_ == Mode::Aim || mode_ == Mode::Roll) {
        if (onHour()) sys.setLight(240, 190, 50);
        else if (until > 0 && until < kFpc * 12) sys.setLight(180, 140, 40);
        else sys.setLight(36, 120, 56);
    }
    draw();
}

}  // namespace puttchime
