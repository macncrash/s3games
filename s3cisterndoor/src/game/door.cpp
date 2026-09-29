#include "game/door.h"

#include <cmath>
#include <cstdio>

namespace cistern {

namespace {

constexpr int kHoldFrames = 180 * 60;

struct Beat {
    int frames;
    int dir;  // -1 lean left, 1 lean right, 2 shoulder
};

// The cistern does not shove at random. It keeps a tide.
const Beat kBeats[] = {
    {160, -1}, {150, 1}, {130, 2}, {170, -1}, {110, 1}, {150, 2},
    {140, 1},  {180, -1}, {120, 2}, {160, 1}, {100, -1}, {190, 2},
};
constexpr int kBeatN = int(sizeof(kBeats) / sizeof(kBeats[0]));

int cycleLen() {
    int n = 0;
    for (int i = 0; i < kBeatN; i++) n += kBeats[i].frames;
    return n;
}

void beatAt(int t, int& index, int& dir, int& into) {
    int c = cycleLen();
    int u = t % c;
    int acc = 0;
    for (int i = 0; i < kBeatN; i++) {
        if (u < acc + kBeats[i].frames) {
            index = i;
            dir = kBeats[i].dir;
            into = u - acc;
            return;
        }
        acc += kBeats[i].frames;
    }
    index = 0;
    dir = kBeats[0].dir;
    into = 0;
}

}  // namespace

int Game::seal() const {
    int s = int(std::lround((1.f - open_) * 100.f));
    if (s < 0) s = 0;
    if (s > 100) s = 100;
    return s;
}

int Game::heldSec() const {
    if (phase_ == Phase::Title) return 0;
    int f = play_;
    if (f > kHoldFrames) f = kHoldFrames;
    return f / 60;
}

int Game::marker() const {
    if (phase_ == Phase::Title) return 0;
    if (phase_ == Phase::Play) return 1;
    return 2;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 2, 2));
    phase_ = Phase::Title;
    age_ = 0;
    play_ = 0;
    open_ = 0.28f;
    beat_ = -1;
}

int Game::braceWanted() const {
    int index, dir, into;
    beatAt(play_, index, dir, into);
    return dir;
}

int Game::braceHeld() const {
    if (bot_) return braceWanted();
    const gs::Pad& p = sys_->pad;
    bool left = p.down(gs::BTN_LEFT);
    bool right = p.down(gs::BTN_RIGHT);
    bool shoulder = p.down(gs::BTN_A) || p.down(gs::BTN_DOWN) || p.down(gs::BTN_B);
    if (shoulder && !left && !right) return 2;
    if (left && !right) return -1;
    if (right && !left) return 1;
    if (shoulder) return 2;
    return 0;
}

void Game::spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip) {
    if (w < 1 || h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, int x, int y, int pal) {
    for (char ch : s) {
        unsigned c = unsigned(ch);
        if (c < 32 || c > 127) {
            x += 6;
            continue;
        }
        const gs::Image& g = art_.glyph[c - 32];
        spr(g, float(x), float(y), g.w, g.h, pal);
        x += g.w + 1;
    }
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H);
        int r = int(1 + (1 - t) * 1);
        int g = int(2 + (1 - t) * 2);
        int b = int(2 + (1 - t) * 3);
        if (phase_ == Phase::Lose && y > 140) {
            r = 1;
            g = 4;
            b = 5;
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    int want = 0;
    int into = 0;
    if (phase_ != Phase::Title) {
        int index;
        beatAt(play_, index, want, into);
    }
    float bob = std::sin(age_ * 0.08f) * 2.f;
    float gap = open_;

    float archX = 46, archY = 28;
    spr(art_.water, 84, 78 + bob * 0.3f, 92, 70, PAL_WATER);
    spr(art_.arch, archX, archY, 168, 150, PAL_STONE);

    float doorW = 78.f * (1.f - gap * 0.62f);
    float doorH = 112.f;
    float doorX = 78.f + gap * 10.f;
    float doorY = 48.f;
    spr(art_.door, doorX, doorY, doorW, doorH, PAL_WOOD);
    if (gap < 0.45f) spr(art_.iron, doorX + doorW - 10, doorY + 18, 8, 40, PAL_WOOD);

    for (int i = 0; i < 4; i++) {
        float dy = std::fmod(age_ * 0.7f + i * 28.f, 90.f);
        spr(art_.drip, 70 + i * 22, 36 + dy, 3, 8, PAL_WATER);
    }

    int held = (phase_ == Phase::Play) ? braceHeld() : 0;
    float px = 168;
    float py = 96;
    const gs::Image* who = &art_.body;
    if (held == -1) px = 154;
    if (held == 1) px = 186;
    if (held == 2) {
        px = 160;
        py = 108;
        who = &art_.shoulder;
    }
    spr(*who, px, py, who->w, who->h, PAL_BODY);

    if (phase_ == Phase::Play) {
        bool flash = (into < 18) && ((age_ / 4) & 1);
        int ap = flash ? PAL_ALARM : PAL_HUD;
        if (want == -1) spr(art_.arrow, 18, 100, 22, 16, ap, false);
        if (want == 1) spr(art_.arrow, 280, 100, 22, 16, ap, true);
        if (want == 2) spr(art_.arrow, 150, 196, 22, 16, ap, false);
    }

    int secLeft = 180;
    if (phase_ != Phase::Title) {
        int left = kHoldFrames - play_;
        if (left < 0) left = 0;
        secLeft = left / 60;
    }
    char clock[16];
    std::snprintf(clock, sizeof(clock), "%d:%02d", secLeft / 60, secLeft % 60);
    text(clock, 8, 6, PAL_HUD);
    text("SEAL", 250, 6, PAL_HUD);
    int sw = seal();
    spr(art_.bar, 250, 16, float(sw) * 0.62f, 6, sw < 30 ? PAL_ALARM : PAL_HUD);

    if (phase_ == Phase::Title) {
        spr(art_.word, 70, 78, art_.word.w, art_.word.h, PAL_HUD);
        if ((age_ / 30) & 1) text("START  LEAN WITH THE TIDE", 58, 120, PAL_HUD);
        text("LEFT   RIGHT   A SHOULDER", 70, 136, PAL_STONE);
        text("THREE MINUTES AND THE LATCH TAKES", 46, 168, PAL_HUD);
    } else if (phase_ == Phase::Play) {
        const char* hint = "MATCH THE SHOVE";
        if (want == -1) hint = "LEAN LEFT";
        else if (want == 1) hint = "LEAN RIGHT";
        else hint = "SHOULDER IN";
        text(hint, 118, 208, (into < 18) ? PAL_ALARM : PAL_HUD);
    } else if (phase_ == Phase::Win) {
        text("IT IS DONE", 118, 188, PAL_HUD);
        text("THE DOOR HOLDS", 108, 202, PAL_STONE);
    } else {
        text("THE DOOR GAVE", 110, 188, PAL_ALARM);
        text("THE CISTERN TAKES IT", 90, 202, PAL_WATER);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;

    if (phase_ == Phase::Title) {
        bool go = bot_ ? age_ > 20 : (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A));
        if (go) {
            phase_ = Phase::Play;
            play_ = 0;
            open_ = 0.28f;
            beat_ = -1;
        }
    } else if (phase_ == Phase::Play) {
        int index, dir, into;
        beatAt(play_, index, dir, into);
        if (index != beat_) {
            beat_ = index;
            sys.apu.tone(0, dir == 2 ? 92.f : 140.f, 0.08f);
            sys.apu.tone(1, 0, 0);
        } else if (into == 8) {
            sys.apu.tone(0, 0, 0);
        }

        int held = braceHeld();
        if (held == dir) {
            open_ -= 0.0075f;
            if (open_ < 0.06f) open_ = 0.06f;
        } else {
            open_ += 0.0032f;
        }
        play_++;
        if (open_ >= 1.f) {
            open_ = 1.f;
            phase_ = Phase::Lose;
            sys.apu.tone(0, 55.f, 0.16f);
            sys.apu.noiseBurst(0.3f, 900.f, 0.4f);
        } else if (play_ >= kHoldFrames) {
            open_ = 0.04f;
            phase_ = Phase::Win;
            sys.apu.tone(0, 220.f, 0.12f);
            sys.apu.tone(1, 330.f, 0.08f);
        }
    } else if (phase_ == Phase::Lose || phase_ == Phase::Win) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
            phase_ = Phase::Title;
            age_ = 0;
            open_ = 0.28f;
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }

    paint();
}

}  // namespace cistern
