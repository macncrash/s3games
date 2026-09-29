#include "game/bedstape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace bedstape {
namespace {

constexpr float kBedY = 108.f;
constexpr float kSpeed = 4.2f;
constexpr int kLife = 96;

}  // namespace

bool Game::matched() const {
    if (filled_ != kTapeN) return false;
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i] || drawer_[i] != kTape[i]) return false;
    return true;
}

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < filled_; i++) {
        int id = drawer_[i];
        if (id >= 0 && id < kHerbs) s += kHerb[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kHerb[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kHerb[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Water) return 1;
    if (mode_ == Mode::Leave) return 2;
    return 3;
}

int Game::nextWant() const { return filled_ < kTapeN ? kTape[filled_] : -1; }

float Game::bedX(int i) const { return 16.f + i * 78.f; }

int Game::nearest() const {
    int best = 0;
    float d = 1e9f;
    for (int i = 0; i < kHerbs; i++) {
        float c = bedX(i) + 32.f;
        float ad = std::fabs(px_ - c);
        if (ad < d) {
            d = ad;
            best = i;
        }
    }
    return best;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 8, 6));
    mode_ = Mode::Title;
    reason_ = "OPEN";
}

void Game::begin() {
    filled_ = 0;
    faults_ = 0;
    offer_ = 0;
    left_ = false;
    over_ = false;
    won_ = false;
    px_ = bedX(0) + 32.f;
    walk_ = 0;
    pouring_ = false;
    face_ = 1;
    for (int i = 0; i < kTapeN; i++) {
        held_[i] = false;
        drawer_[i] = -1;
    }
    reason_ = "WATER";
    mode_ = Mode::Water;
    arm();
}

void Game::arm() {
    life_ = kLife;
    act_ = bot_ ? 8 : 0;
    pouring_ = false;
    if (offer_ >= kDeckN) {
        fail("THE TAPE RAN OUT");
        return;
    }
}

void Game::fail(const char* why) {
    faults_++;
    reason_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    pouring_ = false;
    if (sys_) sys_->apu.tone(0, 110.f, 0.2f);
    beep_ = 10;
}

void Game::fileBed(int bed) {
    if (mode_ != Mode::Water || offer_ >= kDeckN) return;
    if (bed < 0 || bed >= kHerbs) return;
    int ripe = kDeck[offer_];
    if (bed != ripe) {
        fail("WRONG BED");
        return;
    }
    int want = nextWant();
    if (kHerb[bed].decoy || bed != want) {
        fail("NOT ON THE TAPE");
        return;
    }
    drawer_[filled_] = bed;
    held_[filled_] = true;
    filled_++;
    pouring_ = true;
    if (sys_) sys_->apu.tone(0, 660.f + filled_ * 40.f, 0.16f);
    beep_ = 6;
    offer_++;
    if (filled_ == kTapeN && matched()) {
        reason_ = "DRAWER FULL";
        mode_ = Mode::Leave;
        act_ = bot_ ? 16 : 0;
        return;
    }
    arm();
}

void Game::expire() {
    if (mode_ != Mode::Water || offer_ >= kDeckN) return;
    int ripe = kDeck[offer_];
    int want = nextWant();
    if (ripe == want) {
        fail("THE BED DRIED");
        return;
    }
    offer_++;
    if (sys_) sys_->apu.tone(1, 196.f, 0.08f);
    beep_ = 4;
    arm();
}

void Game::depart() {
    if (mode_ != Mode::Leave) return;
    left_ = true;
    won_ = matched() && faults_ == 0 && filled_ == kTapeN;
    over_ = true;
    reason_ = won_ ? "THE DRAWER MATCHES" : "THE DRAWER MISSED";
    mode_ = Mode::Over;
    if (sys_) sys_->apu.tone(1, won_ ? 880.f : 140.f, 0.18f);
    beep_ = 12;
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 18) begin();
        return;
    }
    if (mode_ == Mode::Leave) {
        if (act_ > 0) {
            act_--;
            return;
        }
        depart();
        return;
    }
    if (mode_ != Mode::Water || offer_ >= kDeckN) return;
    if (act_ > 0) {
        act_--;
        return;
    }
    int ripe = kDeck[offer_];
    int want = nextWant();
    if (ripe != want) return;
    float c = bedX(ripe) + 32.f;
    float d = c - px_;
    if (std::fabs(d) > 3.f) {
        px_ += d > 0 ? kSpeed : -kSpeed;
        if ((d > 0 && px_ > c) || (d < 0 && px_ < c)) px_ = c;
        face_ = d > 0 ? 1 : -1;
        walk_ += 1.f;
        return;
    }
    fileBed(ripe);
}

void Game::human() {
    gs::Pad& p = sys_->pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) begin();
        return;
    }
    if (mode_ == Mode::Lose || mode_ == Mode::Over) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            reason_ = "OPEN";
        }
        return;
    }
    if (mode_ == Mode::Leave) {
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START)) depart();
        return;
    }
    float ax = 0;
    if (p.down(gs::BTN_LEFT)) ax -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ax += 1.f;
    if (std::fabs(p.axisX) > 0.25f) ax = p.axisX;
    if (ax > 1.f) ax = 1.f;
    if (ax < -1.f) ax = -1.f;
    if (std::fabs(ax) > 0.15f) {
        px_ += ax * kSpeed;
        face_ = ax > 0 ? 1 : -1;
        walk_ += 1.f;
    }
    if (px_ < 16.f) px_ = 16.f;
    if (px_ > 300.f) px_ = 300.f;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C)) {
        int bed = nearest();
        float c = bedX(bed) + 32.f;
        if (std::fabs(px_ - c) < 24.f) fileBed(bed);
    }
}

void Game::put(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 72) {
            float k = y / 72.f;
            v.lineBackdrop[y] = gs::rgb4(5 + int(k * 4), 8 + int(k * 2), 14 - int(k * 3));
        } else {
            v.lineBackdrop[y] = gs::rgb4(3, 7, 2);
        }
    }

    int ripe = (mode_ == Mode::Water && offer_ < kDeckN) ? kDeck[offer_] : -1;
    for (int i = kHerbs - 1; i >= 0; i--) {
        float x = bedX(i);
        int pal = i == 3 ? PAL_SAGE : PAL_LEAF;
        float bob = (i == ripe) ? std::sin(t_ * 0.25f) * 2.f : 0.f;
        put(art_.herb[i], x + 14, kBedY - 26 + bob, 36, 30, pal);
        put(art_.soil, x + 8, kBedY + 8, 48, 12, i == ripe ? PAL_WATER : PAL_SOIL);
        put(art_.bed, x, kBedY, 64, 28, PAL_WOOD);
    }

    bool pour = pouring_ && beep_ > 0;
    int pose = (pour || (walk_ > 0.5f && (int(walk_) & 4))) ? 1 : 0;
    float manY = 176.f;
    put(art_.shade, px_ - 10, manY - 2, 20, 6, PAL_MAN, false, true);
    put(art_.man[pose], px_ - 13, manY - 34, 26, 34, PAL_MAN, face_ < 0);
    if (pour) put(art_.can, px_ + (face_ < 0 ? -18.f : 8.f), manY - 22, 16, 12, PAL_CAN, face_ < 0);
    if (pour) {
        for (int i = 0; i < 3; i++) put(art_.drop, px_ + (face_ < 0 ? -14.f : 12.f), manY - 8.f + i * 6, 5, 7, PAL_WATER);
    }

    put(art_.drawer, 210, 6, 96, 22, PAL_DRAWER);
    for (int i = 0; i < kTapeN; i++) {
        int pal = held_[i] ? PAL_GOOD : PAL_DIM;
        put(art_.slip, 8.f + i * 34.f, 8, 28, 16, pal);
    }

    if (mode_ == Mode::Title) {
        put(art_.title, 160.f - art_.title.w * 0.5f, 48, float(art_.title.w), float(art_.title.h), PAL_GOLD);
        put(art_.sub, 160.f - art_.sub.w * 0.5f, 86, float(art_.sub.w), float(art_.sub.h), PAL_HUD);
        hudC(22, "ARROWS MOVE", PAL_DIM);
        hudC(23, "A FILES THE RIPE BED", PAL_DIM);
        hudC(25, "START", PAL_GOLD);
    } else if (mode_ == Mode::Lose) {
        put(art_.late, 160.f - art_.late.w * 0.5f, 48, float(art_.late.w), float(art_.late.h), PAL_WARN);
        hudC(22, reason_, PAL_WARN);
    } else if (mode_ == Mode::Over || mode_ == Mode::Leave) {
        put(art_.win, 160.f - art_.win.w * 0.5f, 48, float(art_.win.w), float(art_.win.h), PAL_GOOD);
        hudC(mode_ == Mode::Leave ? 23 : 22, mode_ == Mode::Leave ? "A LEAVES" : reason_, PAL_GOOD);
    }

    if (mode_ != Mode::Title) {
        char line[48];
        std::snprintf(line, sizeof line, "TAPE %s %s %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
        hud(1, 0, line, PAL_GOLD);
        char till[24];
        std::snprintf(till, sizeof till, "TILL %d", drawerScore());
        hud(30, 0, till, PAL_HUD);
        if (mode_ == Mode::Water && ripe >= 0) {
            char now[24];
            std::snprintf(now, sizeof now, "RIPE %s", kHerb[ripe].name);
            hud(1, 26, now, kHerb[ripe].decoy ? PAL_WARN : PAL_GOOD);
            int bars = life_ / 8;
            if (bars < 0) bars = 0;
            if (bars > 12) bars = 12;
            char sun[16];
            int n = 0;
            sun[n++] = 'S';
            sun[n++] = 'U';
            sun[n++] = 'N';
            sun[n++] = ' ';
            for (int i = 0; i < bars; i++) sun[n++] = '#';
            sun[n] = 0;
            hud(20, 26, sun, PAL_SUN);
        }
        for (int i = 0; i < kHerbs; i++) {
            int col = int(bedX(i) / 8.f);
            if (col < 0) col = 0;
            if (col > 34) col = 34;
            int pal = (i == ripe) ? PAL_GOLD : PAL_DIM;
            hud(col, 18, kHerb[i].name, pal);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        pouring_ = false;
    }
    if (bot_) botAct();
    else human();
    if (mode_ == Mode::Water) {
        if (life_ > 0) life_--;
        if (life_ <= 0) expire();
    }
    if (px_ < 12.f) px_ = 12.f;
    if (px_ > 304.f) px_ = 304.f;
    draw();
}

}  // namespace bedstape
