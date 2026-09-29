#include "game/loom.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace loomtape {

bool Game::audit() {
    if (std::strcmp(tapeName(0), "WARP") != 0 || tapePay(0) != 5) return false;
    if (std::strcmp(tapeName(1), "WEFT") != 0 || tapePay(1) != 8) return false;
    if (std::strcmp(tapeName(2), "REED") != 0 || tapePay(2) != 3) return false;
    int onTape = 0, twins = 0, sum = 0;
    for (int i = 0; i < kSlips; i++) {
        const Slip& s = slipAt(i);
        if (s.tape >= 0) {
            onTape++;
            sum += s.pay;
            if (s.twin) return false;
            if (std::strcmp(s.name, tapeName(s.tape)) != 0) return false;
        } else if (s.twin) {
            twins++;
            bool same = false;
            for (int t = 0; t < kTapeN; t++)
                if (tapePay(t) == s.pay) same = true;
            if (!same) return false;
        } else {
            return false;
        }
    }
    if (onTape != kTapeN || twins != kTapeN || sum != 16) return false;
    // The tape order appears once each in a single pass of the shed.
    int seen = -1;
    for (int i = 0; i < kSlips; i++) {
        if (slipAt(i).tape < 0) continue;
        if (slipAt(i).tape != seen + 1) return false;
        seen = slipAt(i).tape;
    }
    if (seen != kTapeN - 1) return false;
    std::snprintf(reason_, sizeof reason_, "rules hold");
    return true;
}

const char* Game::tapeLabel(int i) const { return tapeName(i); }

int Game::tapeScore(int i) const { return tapePay(i); }

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += tapePay(i);
    return s;
}

const char* Game::modeName() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Offer: return "offer";
    case Mode::Call: return "call";
    case Mode::Leave: return "leave";
    case Mode::Win: return "win";
    case Mode::Lose: return "lose";
    }
    return "?";
}

void Game::blip(int ch, float hz, float vol) { sys_->apu.keyOn(ch, hz, vol); }

void Game::begin() {
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    next_ = throws_ = traps_ = slot_ = slotAge_ = anim_ = 0;
    dir_ = 1;
    over_ = won_ = left_ = false;
    call_[0] = 0;
    mode_ = Mode::Offer;
    std::snprintf(reason_, sizeof reason_, "open");
    sys_->apu.silence();
}

void Game::cast() {
    const Slip& s = slipAt(slot_);
    throws_++;
    bool take = s.tape == next_ && next_ < kTapeN;
    if (take) {
        held_[next_] = true;
        std::snprintf(call_, sizeof call_, "%s IN THE DRAWER", s.name);
        std::snprintf(reason_, sizeof reason_, "%s held", s.name);
        next_++;
        blip(0, 520.f + next_ * 80.f, 0.22f);
    } else if (s.twin) {
        traps_++;
        std::snprintf(call_, sizeof call_, "%s STAYS OUT", s.name);
        std::snprintf(reason_, sizeof reason_, "twin");
        blip(1, 160.f, 0.16f);
    } else {
        std::snprintf(call_, sizeof call_, "%s IS OPEN", s.name);
        std::snprintf(reason_, sizeof reason_, "out of order");
        blip(1, 110.f, 0.14f);
    }
    dir_ = -dir_;
    anim_ = 0;
    mode_ = Mode::Call;
    sys_->apu.noiseBurst(take ? 0.08f : 0.14f, take ? 1800.f : 500.f, 0.05f);
}

void Game::afterCall() {
    if (next_ >= kTapeN) {
        anim_ = 0;
        left_ = false;
        mode_ = Mode::Leave;
        std::snprintf(reason_, sizeof reason_, "drawer matches");
        blip(0, 659.f, 0.24f);
        blip(1, 880.f, 0.16f);
        return;
    }
    if (throws_ >= kBeats) {
        won_ = false;
        over_ = true;
        left_ = false;
        mode_ = Mode::Lose;
        std::snprintf(reason_, sizeof reason_, "beats spent");
        return;
    }
    slotAge_ = 0;
    slot_ = (slot_ + 1) % kSlips;
    mode_ = Mode::Offer;
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, bool hflip) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = hflip;
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
    uint16_t top = gs::rgb4(2, 2, 5);
    uint16_t mid = gs::rgb4(6, 4, 5);
    uint16_t bot = gs::rgb4(2, 1, 2);
    if (mode_ == Mode::Win || (mode_ == Mode::Leave && anim_ > 20)) mid = gs::rgb4(8, 6, 2);
    if (mode_ == Mode::Lose) mid = gs::rgb4(5, 1, 2);
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

    const float loomX = 132.f;
    const float loomY = 128.f;
    spr(art_.frame, loomX, loomY, PAL_WOOD);
    spr(art_.beam, loomX, 62.f, PAL_WOOD);
    spr(art_.drawer, 40.f, 128.f, PAL_DRAWER);

    float shed = (mode_ == Mode::Offer && (slotAge_ & 8)) ? 4.f : -4.f;
    const float warp0 = 96.f;
    const float gap = 12.f;
    for (int c = 0; c < 7; c++) {
        float x = warp0 + c * gap;
        spr(art_.heddle, x, 108.f + ((c & 1) ? shed : -shed), PAL_WOOD);
        spr(art_.warp, x, 118.f, PAL_WARP);
    }
    for (int r = 0; r < next_ && r < kTapeN; r++) {
        int pal = PAL_CLOTH;
        float y = 168.f - r * 8.f;
        for (int c = 0; c < 6; c++) spr(art_.pick, warp0 + gap * 0.5f + c * gap, y, pal);
    }

    float reedY = 148.f;
    if (mode_ == Mode::Offer) reedY = 140.f + (slotAge_ / float(kHold)) * 16.f;
    if (mode_ == Mode::Call) reedY = 156.f;
    spr(art_.reed, loomX, reedY, PAL_WOOD);

    float t = mode_ == Mode::Offer ? slotAge_ / float(kHold) : (mode_ == Mode::Call ? 1.f : 0.5f);
    if (dir_ < 0) t = 1.f - t;
    float sx = 90.f + t * 90.f;
    const Slip& cur = slipAt(slot_);
    bool sweet = mode_ == Mode::Offer && cur.tape == next_;
    spr(art_.shuttle, sx, 132.f, sweet ? PAL_CLOTH : PAL_SHUTTLE, dir_ < 0);

    for (int i = 0; i < kTapeN; i++) {
        int pal = held_[i] ? PAL_GOLD : PAL_DIM;
        spr(art_.spool, 40.f, 104.f + i * 22.f, pal);
    }

    char buf[80];
    int f = sys_ ? int(sys_->frame) : 0;
    if (mode_ == Mode::Title) {
        hudC(2, "S3 LOOMTAPE", PAL_GOLD);
        hudC(18, "A SHORT LOOM", PAL_INK);
        hudC(20, "THE DRAWER HAS TO", PAL_GOLD);
        hudC(21, "MATCH THE TAPE", PAL_INK);
        std::snprintf(buf, sizeof buf, "TAPE %s %d  %s %d  %s %d", tapeName(0), tapePay(0), tapeName(1), tapePay(1),
                      tapeName(2), tapePay(2));
        hudC(23, buf, PAL_DIM);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Win || (mode_ == Mode::Leave && anim_ > 24)) {
        hudC(2, "THE DRAWER MATCHES", PAL_GOLD);
        hudC(20, "THE TAPE", PAL_INK);
        std::snprintf(buf, sizeof buf, "%s %d  %s %d  %s %d", tapeName(0), tapePay(0), tapeName(1), tapePay(1),
                      tapeName(2), tapePay(2));
        hudC(22, buf, PAL_GOLD);
        hudC(24, "YOU LEAVE", PAL_INK);
    } else if (mode_ == Mode::Lose) {
        hudC(2, "STILL OPEN", PAL_BAD);
        hudC(21, reason_, PAL_INK);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeName(0), tapeName(1), tapeName(2));
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hud(30, 1, buf, PAL_INK);
        if (mode_ == Mode::Offer) {
            std::snprintf(buf, sizeof buf, "SHED %s %d", cur.name, cur.pay);
            hudC(24, buf, sweet ? PAL_GOLD : PAL_INK);
            hudC(26, "C THROWS THE SHUTTLE", PAL_DIM);
        } else if (mode_ == Mode::Call) {
            hudC(24, call_, cur.tape == next_ - 1 && next_ > 0 ? PAL_GOLD : PAL_BAD);
        } else if (mode_ == Mode::Leave) {
            hudC(24, "THE CLOTH IS IN", PAL_GOLD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) {
        std::snprintf(reason_, sizeof reason_, "rules failed");
        std::fprintf(stderr, "s3loomtape rules failed\n");
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Offer) {
        bool tap = false;
        if (bot_) tap = slotAge_ == 10 && slipAt(slot_).tape == next_;
        else tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
        if (tap) cast();
        else if (++slotAge_ >= kHold) {
            slotAge_ = 0;
            slot_ = (slot_ + 1) % kSlips;
        }
    } else if (mode_ == Mode::Call) {
        if (++anim_ >= 16) afterCall();
    } else if (mode_ == Mode::Leave) {
        anim_++;
        if (anim_ == 12) blip(1, 988.f, 0.16f);
        if (anim_ >= 36) {
            left_ = true;
            won_ = matched() && traps_ == 0 && throws_ == kTapeN;
            over_ = true;
            mode_ = won_ ? Mode::Win : Mode::Lose;
            std::snprintf(reason_, sizeof reason_, won_ ? "the drawer matches the tape" : "left early");
        }
    } else if (mode_ == Mode::Lose) {
        anim_++;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Win) {
        anim_++;
    }

    draw();
}

}  // namespace loomtape
