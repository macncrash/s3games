#include "game/boccetape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace boccetape {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kRollTime = 0.7f;

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += kRing[i].pay;
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kRing[i].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kRing[i].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Aim || mode_ == Mode::Pause) return 1;
    if (mode_ == Mode::Roll) return 2;
    if (mode_ == Mode::Pocket) return 3;
    if (mode_ == Mode::Leave || mode_ == Mode::Over) return 4;
    if (mode_ == Mode::Lose) return 5;
    return 6;
}

int Game::nextOpen() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return i;
    return -1;
}

int Game::ringAt(float x, float y) const {
    int hit = -1;
    float best = 1e9f;
    for (int i = 0; i < 4; i++) {
        float dx = x - kRing[i].x, dy = y - kRing[i].y;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d <= kRing[i].r && d < best) {
            best = d;
            hit = i;
        }
    }
    return hit;
}

void Game::solve(int ring, float& ang, float& pow) const {
    float dx = kRing[ring].x - kHandX;
    float dy = kRing[ring].y - kHandY;
    float dist = std::sqrt(dx * dx + dy * dy);
    ang = std::atan2(dx, -dy);
    pow = (dist - kDist0) / (kDist1 - kDist0);
}

bool Game::prove() {
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) {
        if (!kRing[i].name || !kRing[i].name[0] || kRing[i].pay <= 0 || kRing[i].decoy) {
            reason_ = "TAPE";
            return false;
        }
        for (int j = 0; j < i; j++)
            if (std::strcmp(kRing[i].name, kRing[j].name) == 0) {
                reason_ = "DUP";
                return false;
            }
        sum += kRing[i].pay;
        float ang = 0, pow = 0;
        solve(i, ang, pow);
        if (pow < 0.05f || pow > 0.98f) {
            reason_ = "RANGE";
            return false;
        }
        float dist = kDist0 + pow * (kDist1 - kDist0);
        float x = kHandX + std::sin(ang) * dist;
        float y = kHandY - std::cos(ang) * dist;
        if (ringAt(x, y) != i) {
            reason_ = "MISS";
            return false;
        }
    }
    if (sum != 14) {
        reason_ = "SUM";
        return false;
    }
    if (!kRing[3].decoy || kRing[3].pay != kRing[0].pay || std::strcmp(kRing[3].name, kRing[0].name) == 0) {
        reason_ = "DECOY";
        return false;
    }
    float ang = 0, pow = 0;
    solve(3, ang, pow);
    float dist = kDist0 + pow * (kDist1 - kDist0);
    float x = kHandX + std::sin(ang) * dist;
    float y = kHandY - std::cos(ang) * dist;
    if (ringAt(x, y) != 3) {
        reason_ = "DECOY MISS";
        return false;
    }
    reason_ = "OPEN";
    return true;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    left_ = false;
    bowls_ = 0;
    throws_ = 0;
    traps_ = 0;
    fly_ = -1;
    shake_ = 0;
    aim_ = 0;
    power_ = 0.55f;
    clock_ = 0;
    walk_ = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    reason_ = rules_ ? "OPEN" : reason_;
    ballX_ = kHandX;
    ballY_ = kHandY;
}

void Game::begin() {
    bowls_ = 0;
    throws_ = 0;
    traps_ = 0;
    fly_ = -1;
    left_ = false;
    won_ = false;
    over_ = false;
    walk_ = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    beginAim();
}

void Game::beginAim() {
    mode_ = Mode::Aim;
    aimT_ = 0;
    rollT_ = 0;
    int n = nextOpen();
    if (bot_ && n >= 0) solve(n, aim_, power_);
    ballX_ = kHandX;
    ballY_ = kHandY;
    reason_ = "BOWL";
}

void Game::throwBowl() {
    float dist = kDist0 + clampf(power_, 0.f, 1.f) * (kDist1 - kDist0);
    float ang = clampf(aim_, -1.15f, 1.15f);
    landX_ = kHandX + std::sin(ang) * dist;
    landY_ = kHandY - std::cos(ang) * dist;
    throws_++;
    rollT_ = 0;
    mode_ = Mode::Roll;
    reason_ = "ROLL";
    blip(180.f, 0.05f, 0.08f);
}

void Game::take(int line) {
    held_[line] = true;
    bowls_++;
    fly_ = line;
    flyT_ = 0;
    shake_ = 0;
    mode_ = Mode::Pocket;
    reason_ = kRing[line].name;
    blip(520.f, 0.06f, 0.12f);
}

void Game::miss(const char* why) {
    reason_ = why;
    shake_ = 8;
    judgeT_ = 0;
    mode_ = Mode::Judge;
    blip(90.f, 0.05f, 0.1f);
}

void Game::beginLeave() {
    mode_ = Mode::Leave;
    leaveT_ = 0;
    left_ = true;
    reason_ = "LEAVE";
    blip(660.f, 0.05f, 0.16f);
}

void Game::beginLose() {
    mode_ = Mode::Lose;
    leaveT_ = 0;
    won_ = false;
    reason_ = traps_ ? "TRAP" : "OPEN";
    blip(70.f, 0.06f, 0.2f);
}

void Game::finishRoll() {
    int hit = ringAt(landX_, landY_);
    int need = nextOpen();
    if (hit == 3) {
        traps_++;
        beginLose();
        return;
    }
    if (need >= 0 && hit == need) {
        take(need);
        return;
    }
    if (throws_ >= kMaxBowls) {
        beginLose();
        return;
    }
    miss(hit >= 0 ? "NOT THE TAPE" : "SHORT");
}

void Game::blip(float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    toneT_ = hold;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layCourt();
    rules_ = prove();
    toTitle();
}

void Game::layCourt() {
    if (!sys_) return;
    gs::Plane& a = sys_->vdp.A;
    a.resize(64, 32);
    a.clear();
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            bool court = x >= 2 && x <= 24 && y >= 3 && y <= 24;
            bool edge = court && (x == 2 || x == 24 || y == 3 || y == 24);
            int tile = edge ? art_.rail : (court ? art_.dirt : art_.grass);
            int pal = edge ? PAL_WOOD : (court ? PAL_DIRT : PAL_TREE);
            a.set(x, y, gs::entry(tile, pal));
        }
    }
    a.enabled = true;
    sys_->vdp.B.enabled = false;
    sys_->vdp.HUD.enabled = true;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    clock_ += kDt;
    if (toneT_ > 0) {
        toneT_ -= kDt;
        if (toneT_ <= 0) sys.apu.tone(0, 0, 0);
    }
    if (shake_ > 0) shake_--;

    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || (bot_ && clock_ > 0.45f)) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = heldMode_;
    } else if (mode_ == Mode::Aim) {
        if (p.pressed(gs::BTN_START) && !bot_) {
            heldMode_ = Mode::Aim;
            mode_ = Mode::Pause;
        } else {
            aimT_ += kDt;
            if (bot_) {
                if (aimT_ > 0.18f) throwBowl();
            } else {
                if (p.down(gs::BTN_LEFT)) aim_ -= 1.3f * kDt;
                if (p.down(gs::BTN_RIGHT)) aim_ += 1.3f * kDt;
                if (p.down(gs::BTN_UP)) power_ += 0.7f * kDt;
                if (p.down(gs::BTN_DOWN)) power_ -= 0.7f * kDt;
                aim_ = clampf(aim_, -1.05f, 1.05f);
                power_ = clampf(power_, 0.08f, 1.f);
                if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) throwBowl();
            }
        }
    } else if (mode_ == Mode::Roll) {
        rollT_ += kDt;
        float u = clampf(rollT_ / kRollTime, 0.f, 1.f);
        float e = 1.f - (1.f - u) * (1.f - u);
        ballX_ = kHandX + (landX_ - kHandX) * e;
        ballY_ = kHandY + (landY_ - kHandY) * e;
        if (rollT_ >= kRollTime) finishRoll();
    } else if (mode_ == Mode::Pocket) {
        flyT_ += kDt;
        if (flyT_ > 0.55f) {
            if (matched()) beginLeave();
            else beginAim();
        }
    } else if (mode_ == Mode::Judge) {
        judgeT_ += kDt;
        if (judgeT_ > 0.45f) beginAim();
    } else if (mode_ == Mode::Leave) {
        leaveT_ += kDt;
        walk_ += 70.f * kDt;
        if (leaveT_ > 1.15f) {
            won_ = matched() && left_ && bowls_ == kTapeN && drawerScore() == 14 && traps_ == 0 && rules_;
            over_ = true;
            mode_ = Mode::Over;
            reason_ = won_ ? "LEAVE" : "DOES NOT MATCH";
            if (won_) sys.setLight(220, 180, 70);
        }
    } else if (mode_ == Mode::Lose) {
        leaveT_ += kDt;
        if (leaveT_ > 1.3f) {
            over_ = true;
            won_ = false;
            mode_ = Mode::Over;
        }
        if (!bot_ && p.pressed(gs::BTN_START)) toTitle();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && p.pressed(gs::BTN_START)) toTitle();
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(std::max(1, std::min(2000, int(std::lround(w)))));
    s.h = int16_t(std::max(1, std::min(2000, int(std::lround(h)))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::backdrop() {
    if (!sys_) return;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.lineBackdrop[y] = y < 28 ? gs::rgb4(6, 10, 14) : gs::rgb4(2, 7, 3);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    float jx = (shake_ > 0 && (int(clock_ * 60.f) & 1)) ? 2.f : 0.f;

    float px = 108.f + walk_;
    float py = 208.f;
    if (mode_ != Mode::Over) spr(art_.player, px, py, 26.f, PAL_PLAYER);

    if (mode_ == Mode::Pocket && fly_ >= 0 && fly_ < kTapeN) {
        float u = clampf(flyT_ / 0.5f, 0.f, 1.f);
        float sx = 230.f + float(fly_) * 28.f;
        float x = landX_ + (sx - landX_) * u;
        float y = landY_ + (160.f - landY_) * u - std::sin(u * 3.1416f) * 28.f;
        spr(art_.slip, x, y, 18.f, fly_ == 0 ? PAL_PALE : PAL_RED);
        spr(art_.bowl, landX_, landY_, fly_ == 0 ? 12.f : 18.f, fly_ == 0 ? PAL_PALE : PAL_RED);
    }

    if (mode_ == Mode::Roll || mode_ == Mode::Aim || mode_ == Mode::Pause || mode_ == Mode::Judge ||
        mode_ == Mode::Title) {
        float bx = ballX_, by = ballY_;
        if (mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Pause) {
            bx = kHandX;
            by = kHandY - 8.f;
            float dist = kDist0 + power_ * (kDist1 - kDist0);
            float gx = kHandX + std::sin(aim_) * dist;
            float gy = kHandY - std::cos(aim_) * dist;
            spr(art_.ring, gx, gy, 12.f, PAL_AIM);
            spr(art_.shadow, gx + 2.f, gy + 3.f, 8.f, PAL_SHADE, true);
        }
        int which = nextOpen();
        spr(art_.bowl, bx + jx, by, which == 0 ? 12.f : 18.f, which == 0 ? PAL_PALE : PAL_RED);
        spr(art_.shadow, bx + 2.f + jx, by + 4.f, 10.f, PAL_SHADE, true);
    }

    for (int i = 0; i < kTapeN; i++) {
        float sx = 230.f + float(i) * 28.f;
        if (held_[i] && !(mode_ == Mode::Pocket && fly_ == i))
            spr(art_.slip, sx, 160.f, 16.f, i == 0 ? PAL_PALE : PAL_RED);
        spr(art_.slot, sx, 168.f, 12.f, PAL_SLOT);
    }

    for (int i = 0; i < 4; i++) {
        float h = kRing[i].r * 2.15f;
        int pal = i == 3 ? PAL_GREEN : (i < kTapeN && held_[i] ? PAL_GOLD : PAL_AIM);
        spr(art_.ring, kRing[i].x, kRing[i].y, h, pal);
    }
    spr(art_.pallino, kRing[0].x, kRing[0].y, 10.f, PAL_PALE);
    spr(art_.tree, 18.f, 40.f, 42.f, PAL_TREE);
    spr(art_.pine, 196.f, 36.f, 22.f, PAL_TREE);
    spr(art_.tree, 20.f, 168.f, 36.f, PAL_TREE);

    hud(27, 1, "TAPE", PAL_GOLD);
    for (int i = 0; i < kTapeN; i++) {
        char line[24];
        std::snprintf(line, sizeof(line), "%d %s %s", kRing[i].pay, kRing[i].name, held_[i] ? "IN" : "--");
        hud(26, 3 + i * 2, line, held_[i] ? PAL_GOLD : PAL_HUD);
    }
    char till[16];
    std::snprintf(till, sizeof(till), "TILL %d", drawerScore());
    hud(27, 10, till, PAL_PAPER);
    hud(26, 12, "CLUSTER 7 OUT", PAL_GREEN);

    if (mode_ == Mode::Title) {
        hudC(8, "S3 BOCCETAPE", PAL_GOLD);
        hudC(10, "MATCH THE TAPE", PAL_HUD);
        hudC(12, "THEN LEAVE", PAL_HUD);
        hudC(16, "START", PAL_PAPER);
    } else if (mode_ == Mode::Pause) {
        hudC(14, "PAUSE", PAL_GOLD);
    } else if (mode_ == Mode::Aim) {
        hudC(26, "AIM  A BOWL", PAL_HUD);
    } else if (mode_ == Mode::Lose || (mode_ == Mode::Over && !won_)) {
        hudC(14, "STILL OPEN", PAL_RED);
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(14, "DRAWER MATCHES", PAL_GOLD);
        hudC(16, "YOU LEAVE", PAL_PAPER);
    } else if (reason_) {
        hudC(26, reason_, PAL_HUD);
    }
}

}  // namespace boccetape
