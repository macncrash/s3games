#include "game/drumtape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace drumtape {

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
        if (id >= 0 && id < kParts) s += kPart[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kPart[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kPart[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Rise || mode_ == Mode::Hit || mode_ == Mode::Rest) return 1;
    if (mode_ == Mode::Full) return 2;
    if (mode_ == Mode::Done && won_) return 3;
    return 4;
}

int Game::nextWant() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return kTape[i];
    return -1;
}

const gs::Image& Game::partImg(int id) const {
    if (id == 1) return art_.head;
    if (id == 2) return art_.mallet;
    if (id == 3) return art_.crack;
    return art_.shell;
}

bool Game::prove() {
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) {
        const Part& p = kPart[kTape[i]];
        if (!p.name || !p.name[0] || p.pay <= 0 || p.decoy) {
            why_ = "TAPE";
            return false;
        }
        for (int j = 0; j < i; j++)
            if (std::strcmp(kPart[kTape[i]].name, kPart[kTape[j]].name) == 0) {
                why_ = "DUP";
                return false;
            }
        sum += p.pay;
    }
    if (sum != 18) {
        why_ = "SUM";
        return false;
    }
    const Part& decoy = kPart[3];
    if (!decoy.decoy || decoy.pay != kPart[1].pay || std::strcmp(decoy.name, kPart[1].name) == 0) {
        why_ = "DECOY";
        return false;
    }
    int seen[kParts] = {};
    for (int i = 0; i < kDeckN; i++) {
        int id = kDeck[i];
        if (id < 0 || id >= kParts) {
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
    return true;
}

void Game::begin() {
    filled_ = faults_ = strikes_ = offer_ = age_ = anim_ = 0;
    over_ = won_ = left_ = false;
    for (int i = 0; i < kTapeN; i++) {
        held_[i] = false;
        drawer_[i] = -1;
    }
    why_ = rules_ ? "OPEN" : why_;
    sys_->apu.silence();
    if (rules_) arm();
    else fail("RULES");
}

void Game::arm() {
    if (offer_ >= kDeckN) {
        fail("SHORT");
        return;
    }
    age_ = anim_ = 0;
    mode_ = Mode::Rise;
    why_ = "RISE";
}

void Game::filePart() {
    int id = kDeck[offer_];
    int slot = filled_;
    drawer_[slot] = id;
    held_[slot] = true;
    filled_++;
    strikes_++;
    offer_++;
    anim_ = 0;
    mode_ = Mode::Hit;
    why_ = "FILED";
    sys_->apu.keyOn(0, 180.f + slot * 55.f, 0.32f);
    sys_->apu.noiseBurst(0.16f, 180.f, 0.07f);
}

void Game::strike(bool inWindow) {
    int id = kDeck[offer_];
    const Part& p = kPart[id];
    int want = nextWant();
    if (!inWindow) {
        faults_++;
        fail("MISSED THE HEAD");
        return;
    }
    if (p.decoy || id != want) {
        faults_++;
        fail(p.decoy ? "CRACK STAYS OUT" : "OUT OF ORDER");
        return;
    }
    filePart();
}

void Game::expire() {
    int id = kDeck[offer_];
    if (kPart[id].decoy) {
        offer_++;
        anim_ = 0;
        mode_ = Mode::Rest;
        why_ = "CRACK FELL";
        sys_->apu.noiseBurst(0.08f, 70.f, 0.05f);
        return;
    }
    faults_++;
    fail("THE PART DIED");
}

void Game::fail(const char* why) {
    why_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    anim_ = 0;
    sys_->apu.noiseBurst(0.22f, 48.f, 0.14f);
}

void Game::depart() {
    if (!matched() || faults_ != 0 || drawerScore() != 18) {
        fail("DRAWER");
        return;
    }
    left_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Done;
    why_ = "the drawer matches the tape";
    anim_ = 0;
    sys_->apu.keyOn(0, 330.f, 0.28f);
    sys_->apu.keyOn(1, 495.f, 0.2f);
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
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t mid = gs::rgb4(4, 2, 6);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (mode_ == Mode::Done && won_) mid = gs::rgb4(3, 6, 4);
    if (mode_ == Mode::Fail) mid = gs::rgb4(6, 1, 2);
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

    const float dx = 168.f;
    const float dy = 128.f;
    spr(art_.drum, dx, dy, PAL_SHELL);
    spr(art_.skin, dx, dy - 18.f, PAL_SKIN);

    int show = -1;
    if (mode_ == Mode::Rise || mode_ == Mode::Hit) show = kDeck[offer_ < kDeckN ? offer_ : kDeckN - 1];
    if (mode_ == Mode::Hit && offer_ > 0) show = kDeck[offer_ - 1];
    if (show >= 0 && mode_ != Mode::Done) {
        float py = dy - 46.f;
        if (mode_ == Mode::Hit) py = dy - 46.f - anim_ * 1.6f;
        spr(partImg(show), dx - 46.f, py, kPart[show].pal);
    }

    float sy = 36.f;
    if (mode_ == Mode::Rise) {
        float u = age_ / float(kDieAt);
        sy = 28.f + 86.f * u;
    } else if (mode_ == Mode::Hit) {
        sy = 118.f - anim_ * 1.1f;
    }
    spr(art_.stick, dx + 22.f, sy, PAL_WOOD);

    for (int i = 0; i < kTapeN; i++) {
        float x = 64.f + i * 64.f;
        spr(art_.drawer, x, 200.f, held_[i] ? PAL_PAPER : PAL_DIM);
        if (held_[i]) spr(partImg(drawer_[i]), x, 194.f, kPart[drawer_[i]].pal);
    }

    for (int i = 0; i < kTapeN; i++) {
        int pal = held_[i] ? PAL_GOLD : PAL_DIM;
        spr(art_.lamp, 22.f + i * 14.f, 16.f, pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 DRUMTAPE", PAL_GOLD);
        hudC(17, "STRIKE THE DRUM", PAL_HUD);
        hudC(19, "UNTIL THE DRAWER", PAL_HUD);
        hudC(20, "MATCHES THE TAPE", PAL_GOLD);
        hudC(22, "SHELL  HEAD  MALLET", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Done && won_) {
        hudC(2, "THE DRAWER MATCHES", PAL_GOLD);
        hudC(4, "YOU LEAVE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "%s %d   %s %d   %s %d", tapeLabel(0), tapeScore(0), tapeLabel(1),
                      tapeScore(1), tapeLabel(2), tapeScore(2));
        hudC(21, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(2, "THE DRUM FAILED", PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Full) {
        hudC(2, "DRAWER FULL", PAL_GOLD);
        hudC(21, "THE TAPE IS IN THE DRAWER", PAL_HUD);
        hudC(23, "LEAVE", PAL_GOLD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START LEAVES", PAL_DIM);
    } else {
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hud(30, 1, buf, PAL_HUD);
        if (mode_ == Mode::Rise && offer_ < kDeckN) {
            int id = kDeck[offer_];
            std::snprintf(buf, sizeof buf, "%s %d", kPart[id].name, kPart[id].pay);
            hudC(22, buf, kPart[id].decoy ? PAL_BAD : PAL_GOLD);
            int mark = age_ * 20 / kDieAt;
            if (mark > 19) mark = 19;
            char bar[24];
            for (int i = 0; i < 20; i++) {
                int at = i * kDieAt / 20;
                bool sweet = at >= kSweetLo && at <= kSweetHi;
                bar[i] = (i == mark) ? '|' : (sweet ? '=' : '-');
            }
            bar[20] = 0;
            hudC(24, bar, PAL_GOLD);
            hudC(26, "C STRIKES THE HEAD", PAL_DIM);
        } else if (mode_ == Mode::Hit) {
            hudC(24, "INTO THE DRAWER", PAL_GOLD);
        } else if (mode_ == Mode::Rest) {
            hudC(24, "CRACK STAYS OUT", PAL_DIM);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    rules_ = prove();
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Rise) {
        bool tap = false;
        int id = kDeck[offer_];
        if (bot_) {
            if (!kPart[id].decoy && id == nextWant()) tap = age_ == (kSweetLo + kSweetHi) / 2;
        } else {
            tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
        }
        if (tap) {
            strike(age_ >= kSweetLo && age_ <= kSweetHi);
        } else if (++age_ >= kDieAt) {
            expire();
        }
    } else if (mode_ == Mode::Hit) {
        anim_++;
        if (anim_ >= 26) {
            if (matched()) {
                mode_ = Mode::Full;
                anim_ = 0;
                why_ = "FULL";
            } else {
                arm();
            }
        }
    } else if (mode_ == Mode::Rest) {
        anim_++;
        if (anim_ >= 16) arm();
    } else if (mode_ == Mode::Full) {
        anim_++;
        if (bot_) {
            if (anim_ >= 20) depart();
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
            depart();
        }
    } else if (mode_ == Mode::Fail) {
        anim_++;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Done) {
        anim_++;
    }

    draw();
}

}  // namespace drumtape
