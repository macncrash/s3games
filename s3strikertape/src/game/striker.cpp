#include "game/striker.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace strikertape {
namespace {

constexpr float kDt = 1.f / 60.f;

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += kBand[i].pay;
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kBand[i].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kBand[i].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Ready || mode_ == Mode::Pause) return 1;
    if (mode_ == Mode::Rise) return 2;
    if (mode_ == Mode::Call && matched()) return 3;
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) return 4;
    return 5;
}

int Game::nextOpen() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return i;
    return -1;
}

int Game::bandAt(float p) const {
    for (int i = 0; i < kBandN; i++)
        if (p >= kBand[i].p0 && p <= kBand[i].p1) return i;
    return -1;
}

float Game::meter() const {
    float s = std::sin(meterT_ * 2.15f);
    return 0.08f + 0.84f * (0.5f + 0.5f * s);
}

float Game::puckY(float p) const {
    return kTowerBot - 16.f - clampf(p, 0.f, 1.f) * (kTowerBot - kTowerTop - 36.f);
}

bool Game::prove() {
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) {
        if (!kBand[i].name || !kBand[i].name[0] || kBand[i].pay <= 0 || kBand[i].line != i) {
            reason_ = "TAPE";
            return false;
        }
        for (int j = 0; j < i; j++)
            if (std::strcmp(kBand[i].name, kBand[j].name) == 0) {
                reason_ = "DUP";
                return false;
            }
        sum += kBand[i].pay;
        float mid = bandMid(i);
        if (bandAt(mid) != i) {
            reason_ = "WINDOW";
            return false;
        }
        if (kBand[i].p1 - kBand[i].p0 < 0.06f) {
            reason_ = "THIN";
            return false;
        }
    }
    if (sum != 21 || tapeSum() != 21) {
        reason_ = "SUM";
        return false;
    }
    const Band& tin = kBand[3];
    if (tin.line != -1 || tin.pay != kBand[0].pay || std::strcmp(tin.name, kBand[0].name) == 0) {
        reason_ = "DECOY";
        return false;
    }
    if (bandAt(bandMid(3)) != 3) {
        reason_ = "DECOY MISS";
        return false;
    }
    for (int i = 0; i < kBandN; i++) {
        for (int j = i + 1; j < kBandN; j++) {
            bool over = !(kBand[i].p1 < kBand[j].p0 || kBand[j].p1 < kBand[i].p0);
            if (over) {
                reason_ = "OVERLAP";
                return false;
            }
        }
    }
    reason_ = "OPEN";
    return true;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    left_ = false;
    swings_ = 0;
    traps_ = 0;
    hit_ = -1;
    power_ = 0;
    shown_ = 0;
    meterT_ = 0;
    clock_ = 0;
    walk_ = 0;
    shake_ = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    reason_ = rules_ ? "OPEN" : reason_;
}

void Game::begin() {
    swings_ = 0;
    traps_ = 0;
    left_ = false;
    won_ = false;
    over_ = false;
    walk_ = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    ready();
}

void Game::ready() {
    mode_ = Mode::Ready;
    riseT_ = 0;
    callT_ = 0;
    shown_ = 0;
    hit_ = -1;
    reason_ = "STRIKE";
}

void Game::strike() {
    power_ = clampf(meter(), 0.f, 1.f);
    swings_++;
    riseT_ = 0;
    shown_ = 0;
    mode_ = Mode::Rise;
    reason_ = "UP";
    blip(140.f, 0.07f, 0.08f);
}

void Game::take(int line) {
    held_[line] = true;
    hit_ = line;
    callT_ = 0;
    mode_ = Mode::Call;
    reason_ = kBand[line].name;
    blip(line == 0 ? 740.f : (line == 1 ? 520.f : 360.f), 0.07f, 0.16f);
}

void Game::miss(const char* why) {
    reason_ = why;
    shake_ = 8;
    callT_ = 0;
    hit_ = -1;
    mode_ = Mode::Call;
    blip(90.f, 0.05f, 0.1f);
}

void Game::trap() {
    traps_++;
    beginLose("TIN");
}

void Game::beginLeave() {
    mode_ = Mode::Leave;
    leaveT_ = 0;
    left_ = true;
    reason_ = "LEAVE";
    blip(660.f, 0.06f, 0.18f);
}

void Game::beginLose(const char* why) {
    mode_ = Mode::Lose;
    leaveT_ = 0;
    won_ = false;
    reason_ = why;
    blip(70.f, 0.06f, 0.2f);
}

void Game::settle() {
    int hit = bandAt(power_);
    int need = nextOpen();
    if (hit == 3) {
        trap();
        return;
    }
    if (need >= 0 && hit == need) {
        take(need);
        return;
    }
    if (swings_ >= kMaxSwings) {
        beginLose("OPEN");
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
    layYard();
    rules_ = prove();
    toTitle();
}

void Game::layYard() {
    if (!sys_) return;
    gs::Plane& a = sys_->vdp.A;
    a.resize(64, 32);
    a.clear();
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            bool ground = y >= 22;
            int tile = ground ? art_.dirt : art_.sky;
            int pal = ground ? PAL_WOOD : PAL_NIGHT;
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
    meterT_ += kDt;
    if (toneT_ > 0) {
        toneT_ -= kDt;
        if (toneT_ <= 0) sys.apu.tone(0, 0, 0);
    }
    if (shake_ > 0) shake_--;

    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || (bot_ && clock_ > 0.4f)) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = heldMode_;
    } else if (mode_ == Mode::Ready) {
        if (p.pressed(gs::BTN_START) && !bot_) {
            heldMode_ = Mode::Ready;
            mode_ = Mode::Pause;
        } else if (bot_) {
            int n = nextOpen();
            float m = meter();
            if (n >= 0 && m > kBand[n].p0 + 0.012f && m < kBand[n].p1 - 0.012f) strike();
        } else if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) {
            strike();
        }
    } else if (mode_ == Mode::Rise) {
        riseT_ += kDt;
        float u = clampf(riseT_ / 0.48f, 0.f, 1.f);
        float e = 1.f - (1.f - u) * (1.f - u);
        shown_ = power_ * e;
        if (riseT_ >= 0.48f) {
            shown_ = power_;
            settle();
        }
    } else if (mode_ == Mode::Call) {
        callT_ += kDt;
        if (callT_ > 0.42f) {
            if (matched()) beginLeave();
            else ready();
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_ += kDt;
        walk_ += 78.f * kDt;
        if (leaveT_ > 1.05f) {
            won_ = matched() && left_ && swings_ == kTapeN && drawerScore() == tapeSum() && traps_ == 0 && rules_;
            over_ = true;
            mode_ = Mode::Over;
            reason_ = won_ ? "LEAVE" : "DOES NOT MATCH";
            if (won_) sys.setLight(220, 170, 60);
        }
    } else if (mode_ == Mode::Lose) {
        leaveT_ += kDt;
        if (leaveT_ > 1.1f) {
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
        int n = y < 150 ? 1 : 2;
        sys_->vdp.lineBackdrop[y] = n == 1 ? gs::rgb4(1, 1, 4) : gs::rgb4(3, 2, 1);
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

    float jx = (shake_ > 0) ? float((shake_ & 1) ? 1 : -1) : 0.f;
    float mx = kManX + walk_;
    float my = kManY;
    bool swinging = mode_ == Mode::Rise && riseT_ < 0.2f;
    spr(art_.mallet, mx + (swinging ? 22.f : 16.f), my - (swinging ? 6.f : 18.f), swinging ? 16.f : 14.f, PAL_WOOD);
    spr(art_.man, mx, my, 52.f, PAL_MAN);

    spr(art_.tower, kTowerX, 108.f, 168.f, PAL_WOOD);
    spr(art_.bell, kTowerX, 22.f, 22.f, PAL_BRASS);
    spr(art_.lamp, kTowerX - 28.f, 36.f, 16.f, PAL_GOLD);
    spr(art_.lamp, kTowerX + 28.f, 36.f, 16.f, PAL_GOLD);

    float py = puckY(mode_ == Mode::Ready ? 0.f : shown_);
    spr(art_.puck, kTowerX + jx, py, 12.f, PAL_PUCK);

    for (int i = 0; i < kTapeN; i++) {
        float sx = 248.f;
        float sy = 78.f + float(i) * 28.f;
        spr(art_.slot, sx, sy + 6.f, 14.f, PAL_PAPER);
        if (held_[i]) spr(art_.slip, sx, sy, 16.f, i == 0 ? PAL_BELL : (i == 1 ? PAL_GOLD : PAL_DING));
    }

    int cols = 18;
    float m = (mode_ == Mode::Ready) ? meter() : power_;
    int filled = int(clampf(m, 0.f, 1.f) * float(cols));
    char bar[24];
    for (int i = 0; i < cols; i++) bar[i] = i < filled ? '#' : '.';
    bar[cols] = 0;
    hud(1, 24, bar, PAL_RED);

    hud(28, 1, "TAPE", PAL_GOLD);
    for (int i = 0; i < kTapeN; i++) {
        char line[24];
        std::snprintf(line, sizeof(line), "%d %s %s", kBand[i].pay, kBand[i].name, held_[i] ? "IN" : "--");
        hud(26, 3 + i * 2, line, held_[i] ? PAL_GOLD : PAL_HUD);
    }
    char till[20];
    std::snprintf(till, sizeof(till), "TILL %d", drawerScore());
    hud(26, 10, till, PAL_PAPER);
    hud(26, 12, "TIN 10 OUT", PAL_TIN);

    if (mode_ == Mode::Title) {
        hudC(8, "S3 STRIKERTAPE", PAL_GOLD);
        hudC(10, "MATCH THE TAPE", PAL_HUD);
        hudC(12, "THEN LEAVE", PAL_HUD);
        hudC(16, "START", PAL_PAPER);
    } else if (mode_ == Mode::Pause) {
        hudC(14, "PAUSE", PAL_GOLD);
    } else if (mode_ == Mode::Ready) {
        hudC(26, "A STRIKE", PAL_HUD);
    } else if (mode_ == Mode::Lose || (mode_ == Mode::Over && !won_)) {
        hudC(14, "STILL OPEN", PAL_RED);
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(14, "DRAWER MATCHES", PAL_GOLD);
        hudC(16, "YOU LEAVE", PAL_PAPER);
    } else if (reason_) {
        hudC(26, reason_, PAL_HUD);
    }
}

}  // namespace strikertape
