#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace sbann {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kSpeed = 70.f;
constexpr float kJumpV = -220.f;
constexpr float kGrav = 620.f;
constexpr float kWind = 16.f;
constexpr float kWatch = 48.f;
constexpr float kTake = 11.f;
constexpr float kChain = 286.f;
constexpr float kChainHit = 11.f;
constexpr float kSentry = 178.f;
constexpr float kSenHit = 8.f;
constexpr float kBanner0 = 308.f;
constexpr float kHome = 48.f;
constexpr float kHomeA = 36.f;
constexpr float kHomeB = 64.f;
constexpr float kSpawn = 72.f;
constexpr int kSegs = 3;
constexpr float kSegL[kSegs] = {24.f, 128.f, 250.f};
constexpr float kSegR[kSegs] = {108.f, 230.f, 316.f};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (over_) return 4;
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return 0;
    if (carrying_ && px_ > 240.f) return 2;
    if (carrying_) return 3;
    return 1;
}

float Game::clock() const {
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return playT_;
    return t_;
}

float Game::windNow() const { return kWind * std::sin(clock() * 0.65f); }

float Game::lowness() const {
    float ph = clock() * 1.15f + 0.4f;
    return 0.5f + 0.5f * std::sin(ph);
}

bool Game::chainOpen() const {
    float ph = clock() * 1.15f + 0.4f;
    float s = 0.5f + 0.5f * std::sin(ph);
    return s < 0.40f && std::cos(ph) < 0.f;
}

bool Game::solid(float x) const {
    for (int i = 0; i < kSegs; i++)
        if (x >= kSegL[i] && x <= kSegR[i]) return true;
    return false;
}

int Game::segAt(float x) const {
    for (int i = 0; i < kSegs; i++)
        if (x >= kSegL[i] && x <= kSegR[i]) return i;
    return -1;
}

float Game::deckY(float x) const {
    float u = std::clamp((x - 24.f) / (316.f - 24.f), 0.f, 1.f);
    return 126.f + std::sin(u * kPi) * 14.f;
}

float Game::cableY(float x) const {
    float u = std::clamp((x - 48.f) / 250.f, 0.f, 1.f);
    return 34.f + std::sin(u * kPi) * 30.f;
}

float Game::snap(float x) const {
    if (!solid(x)) {
        float best = kSpawn;
        float bestD = 1e9f;
        for (int i = 0; i < kSegs; i++) {
            float c = std::clamp(x, kSegL[i], kSegR[i]);
            float d = std::fabs(c - x);
            if (d < bestD) {
                bestD = d;
                best = c;
            }
        }
        x = best;
    }
    int s = segAt(x);
    if (s >= 0) {
        if (x < kSegL[s] + 5.f) x = kSegL[s] + 5.f;
        if (x > kSegR[s] - 5.f) x = kSegR[s] - 5.f;
    }
    if (x > kSentry - 16.f && x < kSentry + 16.f) x = kSentry - 18.f;
    if (std::fabs(x - kChain) < 16.f) x = (x < kChain) ? kChain - 22.f : kChain + 18.f;
    if (!solid(x)) x = kSpawn;
    return x;
}

bool Game::gapJump(int way) const {
    int s = segAt(px_);
    if (s < 0 || !onGround_ || way == 0) return false;
    if (way > 0 && s == kSegs - 1) return false;
    if (way < 0 && s == 0) return false;
    float dist = way > 0 ? kSegR[s] - px_ : px_ - kSegL[s];
    return dist >= 0.f && dist <= 18.f;
}

bool Game::sentryJump(int way) const {
    if (!onGround_ || segAt(px_) != 1) return false;
    float goal = carrying_ ? kHome : bx_;
    if (way > 0) return px_ >= 152.f && px_ <= 166.f && goal > 200.f;
    if (way < 0) return px_ >= 198.f && px_ <= 214.f && goal < 160.f;
    return false;
}

bool Game::chainWait(int way) const {
    if (way > 0) return px_ > 248.f && px_ < kChain - 14.f && !chainOpen();
    if (way < 0) return px_ > kChain + 14.f && px_ < 320.f && !chainOpen();
    return false;
}

void Game::resetWorld() {
    carrying_ = false;
    planted_ = false;
    onGround_ = true;
    face_ = 1;
    px_ = kSpawn;
    py_ = deckY(kSpawn);
    vx_ = vy_ = 0;
    bx_ = kBanner0;
    by_ = deckY(kBanner0);
    lastSolid_ = kSpawn;
    stun_ = coyote_ = jumpBuf_ = 0;
    step_ = 0;
    shake_ = 0;
    blipT_ = 0;
    splash_ = 0;
    playT_ = 0;
    watch_ = kWatch;
    lastSec_ = 99;
    puffs_.clear();
}

void Game::boot() {
    resetWorld();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "THE WATCH IS OVER";
}

void Game::begin() {
    resetWorld();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "THE WATCH IS OVER";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 5));
    t_ = 0;
    boot();
}

void Game::blip(float freq) {
    blipT_ = 0.07f;
    blipF_ = freq;
}

void Game::grab() {
    if (carrying_) return;
    carrying_ = true;
    face_ = 1;
    blip(660.f);
    shake_ = 0.4f;
}

void Game::plant() {
    win();
}

void Game::dropBanner(float x) {
    if (!carrying_) return;
    carrying_ = false;
    bx_ = snap(x);
    by_ = deckY(bx_);
}

void Game::knock(float fromX) {
    dropBanner(solid(px_) ? px_ : lastSolid_);
    float dir = px_ < fromX ? -1.f : 1.f;
    vx_ = dir * 100.f;
    vy_ = -150.f;
    onGround_ = false;
    stun_ = 0.55f;
    shake_ = 2.2f;
    coyote_ = 0;
    blip(90.f);
    sys_->apu.noiseBurst(0.35f, 180.f, 0.16f);
    if (!bot_) sys_->rumble(0.4f, 0.2f, 80);
}

void Game::fall() {
    float sx = px_;
    float sy = std::min(py_, 176.f);
    dropBanner(lastSolid_);
    px_ = kSpawn;
    py_ = deckY(kSpawn);
    vx_ = vy_ = 0;
    onGround_ = true;
    stun_ = 0.5f;
    shake_ = 2.8f;
    splash_ = 0.45f;
    face_ = 1;
    puffs_.push_back({sx, sy, 0.4f, 1});
    sys_->apu.noiseBurst(0.4f, 240.f, 0.22f);
    blip(70.f);
    if (!bot_) sys_->rumble(0.55f, 0.25f, 120);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    planted_ = true;
    carrying_ = false;
    bx_ = kHome;
    by_ = deckY(kHome);
    reason_ = "THE BANNER IS BACK";
    shake_ = 0.6f;
    if (!bot_) sys_->rumble(0.25f, 0.1f, 140);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    shake_ = 1.6f;
    sys_->apu.noiseBurst(0.3f, 90.f, 0.3f);
}

void Game::step(float dir, bool jump, bool act) {
    if (stun_ > 0) stun_ -= kDt;
    if (coyote_ > 0) coyote_ -= kDt;
    if (jumpBuf_ > 0) jumpBuf_ -= kDt;
    if (jump && stun_ <= 0) jumpBuf_ = 0.12f;
    if (dir > 0.25f) face_ = 1;
    else if (dir < -0.25f) face_ = -1;

    bool canJump = stun_ <= 0.f && (onGround_ || coyote_ > 0.f);
    if (jumpBuf_ > 0.f && canJump) {
        float d = std::fabs(dir) > 0.25f ? (dir > 0.f ? 1.f : -1.f) : float(face_);
        vx_ = d * kSpeed;
        vy_ = kJumpV;
        onGround_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        blip(480.f);
    } else if (onGround_) {
        vx_ = stun_ > 0.f ? 0.f : dir * kSpeed + windNow();
    }

    float prevY = py_;
    if (onGround_) {
        float nx = px_ + vx_ * kDt;
        if (solid(nx)) {
            px_ = nx;
            lastSolid_ = px_;
        } else {
            vx_ = 0;
        }
        py_ = deckY(px_);
        vy_ = 0;
        if (std::fabs(vx_) > 12.f) step_ += std::fabs(vx_) * kDt * 0.18f;
    } else {
        px_ += vx_ * kDt;
        vy_ += kGrav * kDt;
        py_ += vy_ * kDt;
        if (vy_ >= 0.f && solid(px_)) {
            float d = deckY(px_);
            if (prevY <= d + 6.f && py_ >= d - 2.f) {
                onGround_ = true;
                py_ = d;
                vy_ = 0;
                vx_ = 0;
                lastSolid_ = px_;
                coyote_ = 0;
                puffs_.push_back({px_, py_, 0.25f, 0});
            }
        } else if (!solid(px_) && py_ > deckY(px_) + 14.f) {
            fall();
        }
    }
    if (mode_ == Mode::Play && py_ > 188.f) fall();

    if (mode_ == Mode::Play && stun_ <= 0.f && onGround_) {
        bool chain = lowness() > 0.62f && std::fabs(px_ - kChain) < kChainHit;
        bool guard = std::fabs(px_ - kSentry) < kSenHit && segAt(px_) == 1;
        if (chain) knock(kChain);
        else if (guard) knock(kSentry);
    }

    if (mode_ == Mode::Play && act && stun_ <= 0.f && onGround_) {
        if (!carrying_ && std::fabs(px_ - bx_) <= kTake) grab();
        else if (carrying_ && px_ >= kHomeA && px_ <= kHomeB) plant();
    }

    if (mode_ != Mode::Play) return;
    playT_ += kDt;
    watch_ -= kDt;
    if (watch_ <= 0.f) lose("THE WATCH IS OVER");

    int sec = std::max(0, (int)std::ceil(watch_));
    if (sec != lastSec_ && sec <= 10 && sec > 0) blip(sec <= 5 ? 880.f : 520.f);
    lastSec_ = sec;

    if (shake_ > 0.05f) shake_ *= 0.90f;
    for (Puff& p : puffs_) p.t -= kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.f; }), puffs_.end());
    if (puffs_.size() > 12) puffs_.erase(puffs_.begin(), puffs_.end() - 12);
    if (splash_ > 0.f) splash_ -= kDt;
}

void Game::bot(float& dir, bool& jump, bool& act) const {
    dir = 0;
    jump = false;
    act = false;
    if (stun_ > 0.f) return;

    float goal = carrying_ ? kHome : bx_;
    int way = 0;
    if (goal > px_ + 2.f) way = 1;
    else if (goal < px_ - 2.f) way = -1;

    if (carrying_ && onGround_ && px_ >= kHomeA && px_ <= kHomeB) {
        act = true;
        return;
    }
    if (!carrying_ && onGround_ && std::fabs(px_ - bx_) <= kTake) {
        act = true;
        return;
    }

    dir = float(way);
    if (way != 0 && sentryJump(way)) jump = true;
    if (way != 0 && gapJump(way)) jump = true;
    if (!jump && chainWait(way)) {
        float hold = way > 0 ? kChain - 22.f : kChain + 18.f;
        float corr = std::clamp((hold - px_) * 0.18f, -0.8f, 0.8f);
        dir = std::clamp(corr - windNow() / kSpeed, -1.f, 1.f);
        jump = false;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;

    float dir = 0;
    bool jump = false;
    bool act = false;
    bool go = false;
    if (bot_) {
        if (mode_ == Mode::Title) go = true;
        else if (mode_ == Mode::Play) bot(dir, jump, act);
    } else {
        const gs::Pad& p = sys.pad;
        if (p.down(gs::BTN_LEFT)) dir -= 1.f;
        if (p.down(gs::BTN_RIGHT)) dir += 1.f;
        if (dir == 0.f && std::fabs(p.axisX) > 0.28f) dir = std::clamp(p.axisX, -1.f, 1.f);
        jump = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_UP) || p.pressed(gs::BTN_TURBO);
        act = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_DOWN) || p.pressed(gs::BTN_Y);
        go = p.pressed(gs::BTN_START);
        if (p.pressed(gs::BTN_MODE) && mode_ == Mode::Title && sys.hasHome()) sys.eject();
    }

    if (go) {
        if (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Fail) begin();
        else if (mode_ == Mode::Play) mode_ = Mode::Pause;
        else if (mode_ == Mode::Pause) mode_ = Mode::Play;
    }
    if (mode_ == Mode::Play) step(dir, jump, act);

    draw();
    audio();

    if (mode_ == Mode::Victory) sys.setLight(40, 160, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 30, 24);
    else if (carrying_) sys.setLight(160, 40, 36);
    else sys.setLight(28, 36, 72);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 24 || s.y + s.h < -24) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();

    const uint16_t sky0 = gs::rgb4(1, 1, 4);
    const uint16_t sky1 = gs::rgb4(3, 3, 8);
    const uint16_t rust = gs::rgb4(9, 4, 3);
    const uint16_t gorge = gs::rgb4(2, 2, 3);
    const uint16_t water0 = gs::rgb4(1, 4, 6);
    const uint16_t water1 = gs::rgb4(0, 1, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) c = lerpC(sky0, sky1, y / 78.f);
        else if (y < 118) c = lerpC(sky1, rust, (y - 78) / 40.f);
        else if (y < 168) c = lerpC(rust, gorge, (y - 118) / 50.f);
        else c = lerpC(water0, water1, (y - 168) / 56.f);
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
    }

    float jig = shake_;
    float ox = std::sin(t_ * 90.f) * jig;
    float oy = std::cos(t_ * 70.f) * jig * 0.4f;
    int flap = int(clock() * 6.f) & 1;
    int pace = int(step_) & 1;

    if (mode_ == Mode::Title) spr(art_.word[0], 160 + ox, 20, 36, PAL_TITLE);
    else if (mode_ == Mode::Victory) spr(art_.word[1], 160 + ox, 20, 36, PAL_GOOD);
    else if (mode_ == Mode::Fail) spr(art_.word[2], 160 + ox, 20, 36, PAL_ALERT);
    else if (mode_ == Mode::Pause) spr(art_.word[3], 160 + ox, 20, 34, PAL_AMBER);

    float wind = windNow();
    if (std::fabs(wind) > 8.f && (mode_ == Mode::Play || mode_ == Mode::Title)) {
        for (int i = 0; i < 4; i++) {
            float wy = 70.f + float(i) * 16.f;
            float wx = std::fmod(t_ * wind * 3.f + float(i) * 80.f, 360.f) - 20.f;
            spr(art_.wind, wx, wy, 3.5f, PAL_FX, wind < 0, 6);
        }
    }

    bool blink = stun_ > 0.f && (int(t_ * 14.f) & 1);
    if (!blink) {
        bool air = !onGround_ && mode_ == Mode::Play;
        const gs::Mipped& body = air ? art_.air : art_.runner[pace];
        spr(body, px_ + ox, py_ + oy, air ? 32.f : 36.f, PAL_YOU, face_ < 0, 0, true);
    }

    if (planted_) {
        spr(art_.banner[flap], kHome + ox, deckY(kHome) + oy, 42.f, PAL_BANNER, false, 0, true);
    } else if (carrying_) {
        float oxb = face_ > 0 ? -16.f : 16.f;
        spr(art_.banner[flap], px_ + oxb + ox, py_ - 4.f + oy, 40.f, PAL_BANNER, face_ > 0, 0, true);
    } else {
        spr(art_.banner[flap], bx_ + ox, deckY(bx_) + oy, 42.f, PAL_BANNER, false, 0, true);
    }

    int gf = int(clock() * 3.f) & 1;
    spr(art_.guard[gf], kSentry + ox, deckY(kSentry) + oy, 30.f, PAL_FOE, false, 0, true);

    float low = lowness();
    float hookY = deckY(kChain) - 8.f - (1.f - low) * 36.f;
    float top = cableY(kChain);
    spr(art_.hook, kChain + ox, hookY + oy, 16.f, PAL_IRON);
    if (low > 0.5f) spr(art_.chev, kChain + ox, top - 8.f + oy, low > 0.62f ? 12.f : 8.f, PAL_ALERT);
    int links = 8;
    for (int i = 0; i < links; i++) {
        float u = float(i) / float(links - 1);
        float y = top + (hookY - top) * u;
        spr(art_.link, kChain + ox, y + oy, 6.f, PAL_IRON);
    }

    for (const Puff& p : puffs_) {
        float h = p.kind ? 10.f + (0.4f - p.t) * 16.f : 7.f;
        spr(p.kind ? art_.splash : art_.dust, p.x + ox, p.y + oy, h, PAL_FX);
    }

    spr(art_.post, kHome + ox, deckY(kHome) + oy, 22.f, PAL_IRON, false, 0, true);
    if (!carrying_ && !planted_) spr(art_.post, kBanner0 + ox, deckY(kBanner0) + oy, 22.f, PAL_IRON, false, 0, true);

    auto planks = [&](int i) {
        float a = kSegL[i];
        float b = kSegR[i];
        for (float x = a + 8.f; x < b - 4.f; x += 14.f) {
            float h = 11.f;
            spr(art_.plank, x + ox, deckY(x) + h * 0.15f + oy, h, PAL_WOOD, false, 0, true);
        }
        spr(art_.lip, a + ox, deckY(a) + 2.f + oy, 12.f, PAL_WOOD, true, 0, true);
        spr(art_.lip, b + ox, deckY(b) + 2.f + oy, 12.f, PAL_WOOD, false, 0, true);
    };
    for (int i = 0; i < kSegs; i++) planks(i);

    for (int i = 0; i <= 28; i++) {
        float u = float(i) / 28.f;
        float x = 48.f + 250.f * u;
        spr(art_.link, x + ox, cableY(x) + oy, 6.f, PAL_IRON);
    }
    for (int i = 0; i < kSegs; i++) {
        for (float x = kSegL[i] + 10.f; x < kSegR[i] - 6.f; x += 18.f) {
            float y0 = cableY(x);
            float y1 = deckY(x);
            float h = std::max(8.f, y1 - y0);
            spr(art_.link, x + ox, (y0 + y1) * 0.5f + oy, std::min(h, 10.f), PAL_IRON);
        }
    }

    int lamp = int(t_ * 8.f) & 1;
    spr(art_.lamp, 52 + ox, 58 + oy + (lamp ? 0.f : 1.f), lamp ? 16.f : 15.f, PAL_STONE);
    spr(art_.lamp, 300 + ox, 62 + oy, lamp ? 15.f : 16.f, PAL_STONE);

    spr(art_.tower, 52 + ox, 196 + oy, 150.f, PAL_STONE, false, 0, true);
    spr(art_.tower, 300 + ox, 198 + oy, 150.f, PAL_STONE, true, 0, true);
    spr(art_.rock, 28 + ox, 230 + oy, 120.f, PAL_ROCK, false, 2, true);
    spr(art_.rock, 300 + ox, 230 + oy, 120.f, PAL_ROCK, true, 2, true);

    for (int i = 0; i < 5; i++) {
        float x = std::fmod(40.f + t_ * (18.f + float(i) * 4.f) + float(i) * 70.f, 380.f) - 30.f;
        float y = 30.f + float(i) * 8.f + std::sin(t_ * 2.f + float(i)) * 3.f;
        int bf = int(t_ * 8.f + float(i)) & 1;
        spr(art_.bat[bf], x, y, 7.f + float(i % 2), PAL_FX, i & 1, 4);
    }
    spr(art_.moon, 246, 30, 22.f, PAL_FX, false, 1);
    spr(art_.cloud, std::fmod(20.f + t_ * 6.f, 400.f) - 40.f, 42.f, 16.f, PAL_FX, false, 5);
    spr(art_.cloud, std::fmod(180.f + t_ * 4.f, 440.f) - 50.f, 22.f, 18.f, PAL_FX, false, 6);
    for (int i = 0; i < 7; i++) {
        float sx = 18.f + float(i) * 28.f + std::sin(t_ * 1.4f + float(i)) * 6.f;
        float sy = 186.f + float(i % 3) * 8.f + std::sin(t_ * 2.2f + float(i)) * 2.f;
        spr(art_.star, sx, 16.f + float((i * 17) % 40), (i % 3) == 0 ? 4.f : 3.f, PAL_FX, false, 2);
        spr(art_.star, sx, sy, 3.f, PAL_FX, false, 8);
    }

    if (mode_ == Mode::Title) {
        hudC(16, "BRING THE BANNER BACK", PAL_AMBER);
        hudC(17, "MISS THAT AND THE WATCH IS OVER", PAL_HUD);
        hudC(19, "ARROWS MOVE ALONG THE SPAN", PAL_HUD);
        hudC(20, "Z OR UP JUMPS", PAL_HUD);
        hudC(21, "X OR DOWN TAKES AND PLANTS", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(23, "ENTER TO CROSS", PAL_GOOD);
        hud(1, 27, S3_VERSION_STRING, PAL_HUD);
        hud(34, 27, "S3-16", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hudC(16, "THE BANNER IS BACK", PAL_GOOD);
        hudC(17, "THE SPAN IS YOURS", PAL_AMBER);
        hudC(23, "ENTER TO CROSS AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(16, "THE WATCH IS OVER", PAL_ALERT);
        hudC(17, "THE BANNER IS NOT BACK", PAL_HUD);
        hudC(23, "ENTER TO TRY THE SPAN AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(23, "ENTER TO GO ON", PAL_HUD);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 SPAN BANN", PAL_TITLE);
        int sec = std::max(0, (int)std::ceil(std::max(0.f, watch_)));
        std::string w = "WATCH " + std::to_string(sec);
        int wp = sec <= 10 ? PAL_ALERT : PAL_AMBER;
        hud(40 - int(w.size()), 0, w, wp);

        std::string line;
        int lp = PAL_HUD;
        if (carrying_) {
            line = "BANNER IN HAND";
            lp = PAL_GOOD;
        } else if (bx_ > 280.f) {
            line = "BANNER ON THE FAR POST";
        } else {
            line = "BANNER IS DOWN";
            lp = PAL_AMBER;
        }
        hud(1, 1, line, lp);

        std::string hint = "BRING IT BACK TO THE NEAR POST";
        int hp = PAL_HUD;
        if (std::fabs(px_ - kChain) < 40.f && lowness() > 0.55f) {
            hint = "CHAIN IS LOW";
            hp = PAL_ALERT;
        } else if (std::fabs(px_ - kChain) < 40.f) {
            hint = "CHAIN IS UP";
            hp = PAL_GOOD;
        } else if (std::fabs(px_ - kSentry) < 36.f && !carrying_) {
            hint = "JUMP THE GUARD";
            hp = PAL_AMBER;
        } else if (carrying_ && px_ < 120.f) {
            hint = "PLANT IT ON THE NEAR POST";
            hp = PAL_GOOD;
        }
        hudC(26, hint, hp);
        hud(1, 27, "ARROWS MOVE   Z JUMPS   X PLANTS", PAL_HUD);
    }
}

void Game::audio() {
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Victory) {
        static const float notes[] = {349.f, 440.f, 523.f, 698.f, 523.f, 698.f, 880.f, 698.f};
        int step = int(t_ * 7.f) % 8;
        a.tone(0, notes[step], 0.055f);
        a.tone(1, notes[(step + 2) % 8] * 0.5f, 0.03f);
        a.tone(2, 0, 0);
        a.noise(0, 0, false);
        return;
    }
    if (mode_ == Mode::Fail) {
        static const float notes[] = {220.f, 174.f, 146.f, 110.f};
        int step = std::min(3, int(t_ * 3.f));
        a.tone(0, notes[step], 0.04f);
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
        a.noise(0, 0, false);
        return;
    }
    if (blipT_ > 0.f) {
        blipT_ -= kDt;
        a.tone(0, blipF_, 0.05f);
    } else if (mode_ == Mode::Play && onGround_ && std::fabs(vx_) > 20.f && (int(step_ * 2.f) & 1)) {
        a.tone(0, 140.f, 0.02f);
    } else {
        a.tone(0, 0, 0);
    }
    if (mode_ == Mode::Play) {
        float w = std::fabs(windNow());
        a.tone(1, 48.f + w, 0.012f + w * 0.001f);
        if (carrying_) a.tone(2, 196.f, 0.03f);
        else a.tone(2, 0, 0);
        a.noise(0.02f + w * 0.002f, 320.f + w * 8.f, false);
    } else {
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
        a.noise(0.01f, 180.f, false);
    }
}

}  // namespace sbann
