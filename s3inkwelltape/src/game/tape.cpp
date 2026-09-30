#include "game/tape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace inkwelltape {

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
        if (id >= 0 && id < kInks) s += kInk[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kInk[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kInk[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Dip || mode_ == Mode::Splash || mode_ == Mode::Gap) return 1;
    if (mode_ == Mode::Drawer) return 2;
    if (mode_ == Mode::Over && won_) return 3;
    return 4;
}

int Game::nextWant() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return kTape[i];
    return -1;
}

bool Game::prove() {
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) {
        const Ink& b = kInk[kTape[i]];
        if (!b.name || !b.name[0] || b.pay <= 0 || b.decoy) {
            why_ = "TAPE";
            return false;
        }
        for (int j = 0; j < i; j++)
            if (std::strcmp(kInk[kTape[i]].name, kInk[kTape[j]].name) == 0) {
                why_ = "DUP";
                return false;
            }
        sum += b.pay;
    }
    if (sum != 16) {
        why_ = "SUM";
        return false;
    }
    const Ink& decoy = kInk[3];
    if (!decoy.decoy || decoy.pay != kInk[1].pay || std::strcmp(decoy.name, kInk[1].name) == 0) {
        why_ = "DECOY";
        return false;
    }
    int seen[kInks] = {};
    for (int i = 0; i < kDeckN; i++) {
        int id = kDeck[i];
        if (id < 0 || id >= kInks) {
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
    mode_ = Mode::Dip;
    why_ = "DIP";
}

void Game::fileInk() {
    int id = kDeck[offer_];
    int slot = filled_;
    drawer_[slot] = id;
    held_[slot] = true;
    filled_++;
    strikes_++;
    offer_++;
    anim_ = 0;
    mode_ = Mode::Splash;
    why_ = "FILED";
    sys_->apu.tone(0, 392.f + slot * 60.f, 0.08f);
    sys_->apu.tone(1, 620.f, 0.05f);
}

void Game::dip(bool inWindow) {
    int id = kDeck[offer_];
    const Ink& b = kInk[id];
    int want = nextWant();
    if (!inWindow) {
        faults_++;
        fail("MISSED THE DIP");
        return;
    }
    if (b.decoy || id != want) {
        faults_++;
        fail(b.decoy ? "WASH STAYS OUT" : "OUT OF ORDER");
        return;
    }
    fileInk();
}

void Game::expire() {
    int id = kDeck[offer_];
    if (kInk[id].decoy) {
        offer_++;
        anim_ = 0;
        mode_ = Mode::Gap;
        why_ = "WASH DRIED";
        sys_->apu.noiseBurst(0.08f, 80.f, 0.05f);
        return;
    }
    faults_++;
    fail("THE WELL DRIED");
}

void Game::fail(const char* why) {
    why_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    anim_ = 0;
    sys_->apu.tone(0, 110.f, 0.12f);
    sys_->apu.noiseBurst(0.12f, 70.f, 0.08f);
}

void Game::depart() {
    if (!matched() || faults_ != 0 || drawerScore() != 16) {
        fail("DRAWER");
        return;
    }
    left_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    why_ = "the drawer matches the tape";
    anim_ = 0;
    sys_->apu.tone(0, 523.f, 0.1f);
    sys_->apu.tone(1, 659.f, 0.1f);
    sys_->apu.tone(2, 784.f, 0.14f);
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(5, 4, 6);
    uint16_t bot = gs::rgb4(3, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(10, 8, 3);
    if (mode_ == Mode::Lose) mid = gs::rgb4(6, 2, 2);
    if (mode_ == Mode::Drawer) mid = gs::rgb4(9, 7, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float wellY = 148.f;
    const float xs[kInks] = {52.f, 124.f, 196.f, 268.f};
    spr(art_.desk, 160.f, 186.f, float(art_.desk.w), 32.f, PAL_DESK);
    spr(art_.tape, 160.f, 36.f, float(art_.tape.w), float(art_.tape.h), PAL_PAPER);

    int show = -1;
    if ((mode_ == Mode::Dip || mode_ == Mode::Splash) && offer_ < kDeckN) show = kDeck[offer_];
    if (mode_ == Mode::Splash && offer_ > 0) show = kDeck[offer_ - 1];

    for (int i = 0; i < kInks; i++) {
        spr(art_.well, xs[i], wellY, 40.f, 32.f, kInk[i].pal);
        spr(art_.pool, xs[i], wellY - 10.f, 18.f, 8.f, kInk[i].pal);
    }

    float qx = 160.f;
    float lift = 36.f;
    if (show >= 0) qx = xs[show];
    if (mode_ == Mode::Dip) {
        float u = age_ / float(kDieAt);
        lift = 48.f - 42.f * u;
    } else if (mode_ == Mode::Splash) {
        lift = anim_ < 8 ? 4.f : 22.f;
    } else if (mode_ == Mode::Title) {
        qx = 160.f;
        lift = 16.f + float((sys_->frame / 10) % 4);
    } else if (mode_ == Mode::Drawer || (mode_ == Mode::Over && won_)) {
        qx = 160.f;
        lift = 28.f;
    }
    spr(art_.quill, qx, wellY - 46.f - lift, 14.f, 56.f, PAL_QUILL);
    if (mode_ == Mode::Splash && show >= 0) {
        spr(art_.bead, qx - 6.f, wellY - 18.f, 7.f, 7.f, kInk[show].pal);
        spr(art_.bead, qx + 6.f, wellY - 24.f, 5.f, 5.f, kInk[show].pal);
    }

    for (int i = 0; i < kTapeN; i++) {
        float dx = 78.f + i * 82.f;
        spr(art_.drawer, dx, 210.f, 48.f, 22.f, held_[i] ? PAL_DRAWER : PAL_DIM);
        if (held_[i]) spr(art_.bead, dx, 206.f, 8.f, 8.f, kInk[drawer_[i]].pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 INKWELLTAPE", PAL_GOLD);
        hudC(19, "A SHORT INKWELL", PAL_HUD);
        hudC(20, "THE DRAWER HAS TO", PAL_HUD);
        hudC(21, "MATCH THE TAPE", PAL_GOLD);
        hudC(23, "GALL  SEPIA  LAMP", PAL_HUD);
        hudC(24, "WASH PAYS LIKE SEPIA", PAL_DIM);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "THE DRAWER MATCHES", PAL_GOLD);
        hudC(3, "YOU LEAVE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "%s %d  %s %d  %s %d", tapeLabel(0), tapeScore(0), tapeLabel(1), tapeScore(1),
                      tapeLabel(2), tapeScore(2));
        hudC(21, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "STILL AT THE WELL", PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Drawer) {
        hudC(1, "DRAWER FULL", PAL_GOLD);
        hudC(21, "THE TAPE IS IN THE DRAWER", PAL_HUD);
        hudC(23, "LEAVE", PAL_GOLD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START LEAVES", PAL_DIM);
    } else {
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hud(28, 1, buf, PAL_HUD);
        if (mode_ == Mode::Dip && offer_ < kDeckN) {
            int id = kDeck[offer_];
            std::snprintf(buf, sizeof buf, "%s %d", kInk[id].name, kInk[id].pay);
            hudC(22, buf, kInk[id].decoy ? PAL_BAD : PAL_GOLD);
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
            hudC(26, "C DIPS THE WELL", PAL_DIM);
        } else if (mode_ == Mode::Splash) {
            hudC(24, "INTO THE DRAWER", PAL_GOLD);
        } else if (mode_ == Mode::Gap) {
            hudC(24, "WASH STAYS OUT", PAL_DIM);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.55f);
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
    } else if (mode_ == Mode::Dip) {
        bool tap = false;
        int id = kDeck[offer_];
        if (bot_) {
            if (!kInk[id].decoy && id == nextWant()) tap = age_ == (kSweetLo + kSweetHi) / 2;
        } else {
            tap = pad.pressed(gs::BTN_C);
        }
        if (tap) dip(age_ >= kSweetLo && age_ <= kSweetHi);
        else if (++age_ >= kDieAt) expire();
    } else if (mode_ == Mode::Splash) {
        anim_++;
        if (anim_ >= 16) {
            if (matched()) {
                mode_ = Mode::Drawer;
                anim_ = 0;
                why_ = "FULL";
            } else {
                arm();
            }
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 14) arm();
    } else if (mode_ == Mode::Drawer) {
        anim_++;
        if (bot_) {
            if (anim_ >= 20) depart();
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            depart();
        }
    } else if (mode_ == Mode::Lose) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace inkwelltape
