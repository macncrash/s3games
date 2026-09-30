#include "game/lenstape.h"

#include "version.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace lenstape {

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kGlassSpec[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kGlassSpec[kTape[i]].pay;
}

int Game::drawerScore() const {
    int n = 0;
    for (int i = 0; i < filled_ && i < kTapeN; i++) {
        int id = drawer_[i];
        if (id >= 0 && id < kGlass) n += kGlassSpec[id].pay;
    }
    return n;
}

int Game::nextWant() const {
    if (filled_ < 0 || filled_ >= kTapeN) return -1;
    return kTape[filled_];
}

bool Game::matched() const {
    if (filled_ != kTapeN || faults_ != 0 || strikes_ != kTapeN) return false;
    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i] || drawer_[i] != kTape[i]) return false;
    }
    return drawerScore() == 17;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Pocket) return 2;
    if (mode_ == Mode::Over && won_) return 3;
    return 1;
}

const gs::Image& Game::glassImg(int id) const {
    if (id == 1) return art_.flint;
    if (id == 2) return art_.meniscus;
    if (id == 3) return art_.prism;
    return art_.crown;
}

bool Game::prove() {
    why_ = "RULES";
    if (kTapeN != 3 || kGlass != 4) return false;
    if (std::strcmp(kGlassSpec[0].name, "CROWN") != 0 || kGlassSpec[0].pay != 4 || kGlassSpec[0].decoy) return false;
    if (std::strcmp(kGlassSpec[1].name, "FLINT") != 0 || kGlassSpec[1].pay != 7 || kGlassSpec[1].decoy) return false;
    if (std::strcmp(kGlassSpec[2].name, "MENISCUS") != 0 || kGlassSpec[2].pay != 6 || kGlassSpec[2].decoy) return false;
    if (std::strcmp(kGlassSpec[3].name, "PRISM") != 0 || kGlassSpec[3].pay != 7 || !kGlassSpec[3].decoy) return false;
    if (kTape[0] != 0 || kTape[1] != 1 || kTape[2] != 2) return false;
    if (kPassAt <= kGateHi || kGateLo < 1 || kGateLo > kGateHi) return false;
    int seen[kGlass] = {};
    for (int i = 0; i < kDeckN; i++) {
        int id = kDeck[i];
        if (id < 0 || id >= kGlass) {
            why_ = "DECK";
            return false;
        }
        seen[id]++;
    }
    if (seen[0] != 1 || seen[1] != 1 || seen[2] != 1 || seen[3] != 2) {
        why_ = "DECK";
        return false;
    }
    why_ = "OPEN";
    rules_ = true;
    return true;
}

void Game::begin() {
    filled_ = faults_ = strikes_ = offer_ = age_ = anim_ = 0;
    over_ = won_ = left_ = false;
    for (int i = 0; i < kTapeN; i++) {
        held_[i] = false;
        drawer_[i] = -1;
    }
    sys_->apu.silence();
    if (!rules_ && !prove()) {
        fail("RULES");
        return;
    }
    arm();
}

void Game::arm() {
    if (offer_ >= kDeckN) {
        fail("SHORT");
        return;
    }
    age_ = anim_ = 0;
    mode_ = Mode::Bench;
    why_ = "BENCH";
}

void Game::seat() {
    int id = kDeck[offer_];
    drawer_[filled_] = id;
    held_[filled_] = true;
    filled_++;
    strikes_++;
    offer_++;
    anim_ = 0;
    mode_ = Mode::Seat;
    why_ = "FILED";
    sys_->apu.keyOn(0, 330.f + filled_ * 55.f, 0.26f);
}

void Game::fileLens(bool inGate) {
    int id = kDeck[offer_];
    const Glass& g = kGlassSpec[id];
    if (!inGate) {
        faults_++;
        fail("MISSED THE GATE");
        return;
    }
    if (g.decoy || id != nextWant()) {
        faults_++;
        fail(g.decoy ? "PRISM STAYS OUT" : "OUT OF ORDER");
        return;
    }
    seat();
}

void Game::passBy() {
    int id = kDeck[offer_];
    if (kGlassSpec[id].decoy) {
        offer_++;
        anim_ = 0;
        mode_ = Mode::Gap;
        why_ = "PRISM PASSED";
        sys_->apu.noiseBurst(0.08f, 90.f, 0.05f);
        return;
    }
    faults_++;
    fail("THE LENS PASSED");
}

void Game::fail(const char* why) {
    why_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    anim_ = 0;
    sys_->apu.noiseBurst(0.18f, 48.f, 0.12f);
}

void Game::depart() {
    if (!matched() || faults_ != 0 || drawerScore() != 17) {
        fail("DRAWER");
        return;
    }
    left_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    why_ = "the drawer matches the tape";
    anim_ = 0;
    sys_->apu.keyOn(0, 440.f, 0.28f);
    sys_->apu.keyOn(1, 660.f, 0.2f);
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 2, 4);
    uint16_t mid = gs::rgb4(2, 4, 7);
    uint16_t bot = gs::rgb4(1, 2, 3);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(3, 6, 8);
    if (mode_ == Mode::Lose) mid = gs::rgb4(6, 1, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.5f ? mix(top, mid, u / 0.5f) : mix(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float benchY = 128.f;
    spr(art_.bench, 160.f, benchY, PAL_BENCH);
    spr(art_.gate, 160.f, benchY - 6.f, PAL_GOLD);

    int show = -1;
    float lx = 40.f;
    if (mode_ == Mode::Bench && offer_ < kDeckN) {
        show = kDeck[offer_];
        lx = 28.f + age_ * (264.f / float(kPassAt));
    } else if (mode_ == Mode::Seat && offer_ > 0) {
        show = kDeck[offer_ - 1];
        lx = 160.f;
    }
    if (show >= 0 && mode_ != Mode::Over && mode_ != Mode::Pocket) {
        float ly = benchY - 22.f;
        if (mode_ == Mode::Seat) ly = benchY - 22.f - anim_ * 1.1f;
        spr(glassImg(show), lx, ly, kGlassSpec[show].pal);
    }

    for (int i = 0; i < kTapeN; i++) {
        float dx = 78.f + i * 72.f;
        spr(art_.tray, dx, 196.f, held_[i] ? PAL_PAPER : PAL_DIM);
        if (held_[i]) spr(glassImg(drawer_[i]), dx, 188.f, kGlassSpec[drawer_[i]].pal);
    }
    for (int i = 0; i < kTapeN; i++) {
        spr(art_.lamp, 28.f + i * 16.f, 20.f, held_[i] ? PAL_GOLD : PAL_DIM);
    }

    char buf[96];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 LENSTAPE", PAL_GOLD);
        hudC(16, "THE DRAWER HAS TO", PAL_HUD);
        hudC(18, "MATCH THE TAPE", PAL_GOLD);
        hudC(21, "CROWN   FLINT   MENISCUS", PAL_HUD);
        hudC(23, "LET THE PRISM PASS", PAL_DIM);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE DRAWER MATCHES", PAL_GOLD);
        hudC(4, "YOU LEAVE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "%s %d  %s %d  %s %d", tapeLabel(0), tapeScore(0), tapeLabel(1), tapeScore(1),
                      tapeLabel(2), tapeScore(2));
        hudC(21, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "DRAWER %d", drawerScore());
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Lose) {
        hudC(2, "THE BENCH FAILED", PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pocket) {
        hudC(2, "DRAWER FULL", PAL_GOLD);
        hudC(21, "THE TAPE IS IN THE DRAWER", PAL_HUD);
        hudC(23, "LEAVE", PAL_GOLD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START LEAVES", PAL_DIM);
    } else {
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "DRAWER %d", drawerScore());
        hud(28, 1, buf, PAL_HUD);
        if (mode_ == Mode::Bench && offer_ < kDeckN) {
            int id = kDeck[offer_];
            std::snprintf(buf, sizeof buf, "%s %d", kGlassSpec[id].name, kGlassSpec[id].pay);
            hudC(22, buf, kGlassSpec[id].decoy ? PAL_BAD : PAL_GOLD);
            int mark = age_ * 20 / kPassAt;
            if (mark > 19) mark = 19;
            char bar[24];
            for (int i = 0; i < 20; i++) {
                int at = i * kPassAt / 20;
                bool gate = at >= kGateLo && at <= kGateHi;
                bar[i] = (i == mark) ? '|' : (gate ? '=' : '-');
            }
            bar[20] = 0;
            hudC(24, bar, PAL_GOLD);
            hudC(26, "A FILES THE GATE", PAL_DIM);
        } else if (mode_ == Mode::Seat) {
            hudC(24, "INTO THE DRAWER", PAL_GOLD);
        } else if (mode_ == Mode::Gap) {
            hudC(24, "PRISM STAYS OUT", PAL_DIM);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    rules_ = prove();
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Bench) {
        bool tap = false;
        int id = kDeck[offer_];
        if (bot_) {
            if (!kGlassSpec[id].decoy && id == nextWant()) tap = age_ == (kGateLo + kGateHi) / 2;
        } else {
            tap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
        }
        if (tap) fileLens(age_ >= kGateLo && age_ <= kGateHi);
        else if (++age_ >= kPassAt) passBy();
    } else if (mode_ == Mode::Seat) {
        anim_++;
        if (anim_ >= 26) {
            if (matched()) {
                mode_ = Mode::Pocket;
                anim_ = 0;
                why_ = "FULL";
            } else {
                arm();
            }
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 16) arm();
    } else if (mode_ == Mode::Pocket) {
        anim_++;
        if (bot_) {
            if (anim_ >= 20) depart();
        } else if (pad.pressed(gs::BTN_START)) {
            depart();
        }
    } else if (mode_ == Mode::Lose) {
        anim_++;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Over) {
        anim_++;
    }

    draw();
}

}  // namespace lenstape
