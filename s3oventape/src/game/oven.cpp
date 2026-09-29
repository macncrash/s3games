#include "game/oven.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace oventape {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kRise = 0.48f;
constexpr float kDone0 = 0.54f;
constexpr float kDone1 = 0.80f;
constexpr int kQueueN = 5;

struct Dough {
    const char* name;
    int pay;
    int tape;
};

const Dough kQueue[kQueueN] = {
    {"CRUST", 6, 0}, {"CHAR", 0, -1}, {"GOLD", 9, 1}, {"SOOT", 0, -1}, {"MILK", 4, 2},
};

const char* kTapeName[kTapeN] = {"CRUST", "GOLD", "MILK"};
const int kTapePay[kTapeN] = {6, 9, 4};
}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += kTapePay[i];
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kTapeName[i];
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kTapePay[i];
}

const char* Game::modeName() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Bake: return "bake";
    case Mode::Leave: return "leave";
    case Mode::Lose: return "lose";
    }
    return "?";
}

bool Game::audit() const {
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) {
        if (!kTapeName[i] || !kTapeName[i][0] || kTapePay[i] <= 0) return false;
        for (int j = 0; j < i; j++)
            if (std::strcmp(kTapeName[i], kTapeName[j]) == 0) return false;
        sum += kTapePay[i];
        bool seen = false;
        for (int q = 0; q < kQueueN; q++) {
            if (kQueue[q].tape == i) {
                if (seen) return false;
                seen = true;
                if (std::strcmp(kQueue[q].name, kTapeName[i]) != 0) return false;
                if (kQueue[q].pay != kTapePay[i]) return false;
            }
        }
        if (!seen) return false;
    }
    if (sum != 19) return false;
    for (int q = 0; q < kQueueN; q++) {
        if (kQueue[q].tape >= 0) continue;
        for (int i = 0; i < kTapeN; i++)
            if (std::strcmp(kQueue[q].name, kTapeName[i]) == 0) return false;
        if (kQueue[q].pay != 0) return false;
    }
    return true;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    left_ = false;
    heat_ = 0;
    say_[0] = 0;
}

void Game::begin() {
    mode_ = Mode::Bake;
    over_ = false;
    won_ = false;
    left_ = false;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    idx_ = 0;
    pulls_ = 0;
    traps_ = 0;
    heat_ = 0;
    sayT_ = 0;
    leaveT_ = 0;
    std::snprintf(say_, sizeof say_, "MATCH THE TAPE");
    std::snprintf(reason_, sizeof reason_, "baking");
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::snprintf(reason_, sizeof reason_, "tape audit failed");
    clock_ = 0;
    if (bot_) begin();
    else toTitle();
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.22f);
    sys_->apu.tone(1, freq * 0.5f, 0.08f);
}

void Game::advance() {
    heat_ = 0;
    idx_++;
    if (idx_ < kQueueN) return;
    if (matched() && traps_ == 0 && drawerScore() == 19) openDoor();
    else {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        std::snprintf(say_, sizeof say_, "DRAWER WRONG");
        std::snprintf(reason_, sizeof reason_, "drawer missed the tape");
        blip(120.f);
    }
}

void Game::openDoor() {
    mode_ = Mode::Leave;
    std::snprintf(say_, sizeof say_, "DRAWER MATCHES");
    std::snprintf(reason_, sizeof reason_, "drawer matches");
    blip(740.f);
}

void Game::walkOut() {
    left_ = true;
    won_ = true;
    over_ = true;
    std::snprintf(say_, sizeof say_, "YOU LEAVE");
    blip(880.f);
}

void Game::pull() {
    if (idx_ < 0 || idx_ >= kQueueN) return;
    const Dough& d = kQueue[idx_];
    pulls_++;
    sayT_ = 0.65f;
    if (heat_ >= kDone0 && heat_ < kDone1 && d.tape >= 0 && !held_[d.tape]) {
        held_[d.tape] = true;
        std::snprintf(say_, sizeof say_, "%s  +%d", d.name, d.pay);
        blip(d.tape == 1 ? 660.f : 480.f);
        advance();
        return;
    }
    traps_++;
    if (heat_ < kDone0) std::snprintf(say_, sizeof say_, "TOO SOON");
    else if (d.tape < 0) std::snprintf(say_, sizeof say_, "%s  SPOILS", d.name);
    else std::snprintf(say_, sizeof say_, "BURNED");
    blip(140.f);
    advance();
}

void Game::skip() {
    if (idx_ < 0 || idx_ >= kQueueN) return;
    sayT_ = 0.4f;
    std::snprintf(say_, sizeof say_, "PASS %s", kQueue[idx_].name);
    blip(220.f);
    advance();
}

void Game::burn() {
    traps_++;
    sayT_ = 0.55f;
    std::snprintf(say_, sizeof say_, "BURNED");
    blip(100.f);
    advance();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    const gs::Pad& pad = sys.pad;
    const bool start = pad.pressed(gs::BTN_START);
    const bool action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
    const bool pass = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X);

    if (mode_ == Mode::Title) {
        heat_ = 0.45f + 0.2f * std::sin(clock_ * 1.7f);
        if (bot_ || start || action) begin();
    } else if (mode_ == Mode::Bake) {
        heat_ += kRise * kDt;
        bool take = false;
        bool drop = false;
        if (bot_ && idx_ < kQueueN) {
            const Dough& d = kQueue[idx_];
            if (d.tape >= 0 && !held_[d.tape]) take = heat_ >= 0.64f && heat_ < 0.74f;
            else drop = heat_ >= 0.28f;
        }
        if (!bot_ && action) take = true;
        if (!bot_ && pass) drop = true;
        if (take) pull();
        else if (drop && mode_ == Mode::Bake) skip();
        else if (mode_ == Mode::Bake && heat_ >= 1.f) burn();
        if (sayT_ > 0) sayT_ -= kDt;
        if (!bot_ && start) toTitle();
    } else if (mode_ == Mode::Leave) {
        leaveT_ += kDt;
        if (bot_ && leaveT_ > 0.35f) walkOut();
        else if (!bot_ && (action || start)) walkOut();
    } else if (!bot_ && (start || action)) {
        toTitle();
    }

    if (mode_ != Mode::Bake) sys.apu.tone(0, 0, 0);
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float cy, int pal) {
    if (!sys_ || img.w < 1) return;
    gs::Sprite s;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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
    const int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 86) {
            float u = y / 86.f;
            v.lineBackdrop[y] = gs::rgb4(3 + int(u * 3), 2, 1);
        } else if (y < 168) {
            v.lineBackdrop[y] = gs::rgb4(6, 4, 2);
        } else {
            int g = 3 + ((y / 8) & 1);
            v.lineBackdrop[y] = gs::rgb4(g + 3, g + 1, 1);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) {
        word(art_.title, 100.f, 28.f, PAL_TITLE);
        word(art_.tapeWord, 220.f, 28.f, PAL_TAPE);
    }
    if (mode_ == Mode::Leave || (mode_ == Mode::Lose && left_)) word(art_.leave, 160.f, 40.f, PAL_GOOD);
    if (won_) word(art_.leave, 160.f, 36.f, PAL_GOOD);

    const float mouth = 92.f + (1.f - std::min(heat_, 1.f)) * 18.f;
    float fh = 8.f + std::min(heat_, 1.f) * 30.f;
    if (mode_ == Mode::Title) fh = 14.f + 6.f * std::sin(clock_ * 6.f);
    spr(art_.flame, 108.f, mouth, std::max(8.f, fh), PAL_FLAME);

    if (mode_ == Mode::Bake && idx_ < kQueueN) {
        const Dough& d = kQueue[idx_];
        int pal = d.tape == 1 ? PAL_GOLD : (d.tape < 0 ? PAL_BAD : PAL_LOAF);
        spr(art_.loaf, 108.f, 100.f, 22.f, pal);
    }
    if (mode_ == Mode::Leave || won_) spr(art_.loaf, 108.f, 88.f, 24.f, PAL_GOLD);

    spr(art_.oven, 108.f, 118.f, 128.f, PAL_OVEN);
    spr(art_.head, 36.f, 150.f + std::sin(clock_ * 2.4f) * 1.2f, 42.f, PAL_BAKER);
    spr(art_.drawer, 230.f, 168.f, 36.f, PAL_WOOD);

    for (int i = 0; i < kTapeN; i++) {
        float x = 196.f + float(i) * 22.f;
        spr(art_.slip, x, held_[i] ? 150.f : 118.f, 12.f, held_[i] ? PAL_GOOD : PAL_TAPE);
    }

    char line[48];
    std::snprintf(line, sizeof line, "TAPE  %s %d  %s %d  %s %d", kTapeName[0], kTapePay[0], kTapeName[1], kTapePay[1],
                  kTapeName[2], kTapePay[2]);
    hud(1, 1, line, PAL_TAPE);
    std::snprintf(line, sizeof line, "TILL %d", drawerScore());
    hud(1, 25, line, PAL_GOOD);
    if (mode_ == Mode::Bake && idx_ < kQueueN) {
        std::snprintf(line, sizeof line, "NOW %s", kQueue[idx_].name);
        hud(28, 25, line, kQueue[idx_].tape < 0 ? PAL_BAD : PAL_LOAF);
    }
    if (say_[0] && (sayT_ > 0.f || mode_ != Mode::Bake)) hudC(3, say_, mode_ == Mode::Lose ? PAL_BAD : PAL_TITLE);
    if (mode_ == Mode::Title) hudC(22, "A BAKES   B PASSES", PAL_INK);
    hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_INK);
}

}  // namespace oventape
