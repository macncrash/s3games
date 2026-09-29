#include "game/choir.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace choir {
namespace {

// Six notes. `at` is the frame the beat lands, measured from the phrase start.
struct Beat {
    int who;
    int at;
};
constexpr Beat PHRASE[] = {{0, 50}, {2, 98}, {4, 146}, {1, 194}, {3, 242}, {2, 290}};
constexpr int NBEAT = 6;
constexpr int WINDOW = 9;
constexpr int PHRASE_END = 330;

const float PITCH[] = {261.6f, 293.7f, 329.6f, 349.2f, 392.0f, 440.0f};

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.15f;
    p.op[0] = {1, 1, 0.002f, 0.45f, 0.15f, 1.4f};
    p.op[1] = {2.76f, 0.45f, 0.002f, 0.3f, 0.0f, 0.9f};
    p.op[2] = {5.2f, 0.25f, 0.004f, 0.2f, 0.0f, 0.5f};
    p.op[3] = {1, 0.35f, 0.002f, 0.6f, 0.2f, 1.6f};
    p.vol = 0.28f;
    p.echo = 0.45f;
    return p;
}

gs::FMPatch voicePatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1, 1, 0.02f, 0.12f, 0.7f, 0.18f};
    p.op[1] = {2, 0.4f, 0.02f, 0.15f, 0.4f, 0.2f};
    p.op[2] = {3, 0.2f, 0.03f, 0.2f, 0.3f, 0.2f};
    p.op[3] = {1, 0.3f, 0.02f, 0.12f, 0.5f, 0.18f};
    p.vol = 0.16f;
    p.echo = 0.25f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (over_) return won_ ? 3 : 4;
    if (mode_ == Mode::Ring) return 2;
    if (mode_ == Mode::Phrase) return 1;
    return 0;
}

void Game::chime(float freq) {
    sys_->apu.keyOn(1, freq, 0.18f);
    hold_ = 8;
}

void Game::bellVoice() {
    sys_->apu.keyOn(0, 196.0f, 0.34f);
    sys_->apu.keyOn(2, 392.0f, 0.16f);
    sys_->apu.tone(0, 784.0f, 0.05f);
}

void Game::missVoice() { sys_->apu.noiseBurst(0.35f, 240.0f, 0.18f); }

void Game::beginPhrase() {
    mode_ = Mode::Phrase;
    phrase_ = 0;
    next_ = 0;
    std::memset(struck_, 0, sizeof(struck_));
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.28f, 0.35f, 0.25f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPatch(1, voicePatch());
    sys.apu.setPatch(2, bellPatch());
    tries_ = 3;
    age_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    swing_ = 0;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 2) return;
    float s = h / float(m.h);
    float w = m.w * s;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.x = int16_t(cx - w * 0.5f);
    sp.y = int16_t(cy - h * 0.5f);
    sp.w = int16_t(w);
    sp.h = int16_t(h);
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    float w = 0;
    for (const char* p = s; *p; p++) w += 12.f * scale;
    float cx = x;
    if (align == 1) cx = x - w * 0.5f;
    if (align == 2) cx = x - w;
    for (const char* p = s; *p; p++) {
        int g = (*p >= 32 && *p < 128) ? *p - 32 : 0;
        const gs::Mipped& m = art_.glyph[g];
        float h = 14.f * scale;
        spr(m, cx + 6.f * scale, y, h, pal);
        cx += 12.f * scale;
    }
}

void Game::update() {
    age_++;
    if (hold_ > 0 && --hold_ == 0) {
        sys_->apu.keyOff(1);
        sys_->apu.tone(0, 0, 0);
    }
    bool tap = sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_B) || sys_->pad.pressed(gs::BTN_C) ||
               sys_->pad.pressed(gs::BTN_START);
    if (bot_) {
        if (mode_ == Mode::Title && age_ > 24) tap = true;
        if (mode_ == Mode::Phrase) {
            for (int i = 0; i < NBEAT; i++) {
                if (!struck_[i] && phrase_ == PHRASE[i].at) tap = true;
            }
        }
    }

    if (mode_ == Mode::Title) {
        if (tap) {
            tries_ = 3;
            beginPhrase();
        }
        return;
    }
    if (mode_ == Mode::Ring) {
        swing_ = std::sin(age_ * 0.18f) * 18.f;
        if (age_ > 150) {
            mode_ = Mode::Over;
            over_ = true;
            won_ = true;
            sys_->apu.keyOff(0);
            sys_->apu.keyOff(2);
        }
        return;
    }
    if (mode_ == Mode::Fail) {
        if (age_ > 90) {
            mode_ = Mode::Over;
            over_ = true;
            won_ = false;
        }
        return;
    }
    if (mode_ != Mode::Phrase) return;

    phrase_++;
    int open = -1;
    for (int i = 0; i < NBEAT; i++) {
        if (struck_[i]) continue;
        if (std::abs(phrase_ - PHRASE[i].at) <= WINDOW) open = i;
        if (phrase_ == PHRASE[i].at + WINDOW + 1) {
            struck_[i] = true;
            tries_--;
            missVoice();
            mouth_ = 0;
            if (tries_ <= 0) {
                mode_ = Mode::Fail;
                age_ = 0;
                return;
            }
            beginPhrase();
            return;
        }
    }
    if (tap) {
        if (open >= 0 && !struck_[open]) {
            struck_[open] = true;
            next_++;
            mouth_ = PHRASE[open].who;
            chime(PITCH[open]);
            if (next_ >= NBEAT) {
                mode_ = Mode::Ring;
                age_ = 0;
                won_ = true;
                bellVoice();
                return;
            }
        } else {
            tries_--;
            missVoice();
            if (tries_ <= 0) {
                mode_ = Mode::Fail;
                age_ = 0;
                return;
            }
            beginPhrase();
            return;
        }
    }
    if (phrase_ > PHRASE_END && next_ >= NBEAT) {
        mode_ = Mode::Ring;
        age_ = 0;
        won_ = true;
        bellVoice();
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();

    const float xs[5] = {46, 104, 160, 216, 274};
    bool singing = mode_ == Mode::Phrase || mode_ == Mode::Ring;

    text("S3 CHOIRBELL", 160, 16, 1.15f, PAL_HUD, 1);
    if (mode_ == Mode::Title) {
        text(bot_ ? "THE CHOIR STANDS" : "A  TO BEGIN", 160, 196, 1.f, PAL_HUD, 1);
        text("RING THE BELL", 160, 34, 0.85f, PAL_HUD, 1);
    } else if (mode_ == Mode::Ring || (mode_ == Mode::Over && won_)) {
        text("THE BELL RINGS", 160, 196, 1.f, PAL_HUD, 1);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        text("THE THIRD TRY DIED", 160, 196, 0.9f, PAL_HUD, 1);
    } else {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "TRIES %d", tries_);
        text(buf, 8, 196, 0.9f, PAL_HUD, 0);
        text("A ON THE NOTE", 312, 196, 0.8f, PAL_HUD, 2);
    }

    float bellX = 160 + ((mode_ == Mode::Ring || (over_ && won_)) ? swing_ : std::sin(age_ * 0.04f) * 1.5f);
    spr(art_.bell, bellX, 58, 36, PAL_BELL);
    spr(art_.clapper, bellX + swing_ * 0.15f, 78, 12, PAL_BELL);

    for (int i = 0; i < 5; i++) {
        float bob = (singing && mouth_ == i) ? std::sin(age_ * 0.5f) * 2.f : 0;
        const gs::Mipped& robe = (i % 2) ? art_.robeAlt : art_.robe;
        int pal = (i % 2) ? PAL_ROBE2 : PAL_ROBE;
        spr(robe, xs[i], 148 + bob, 70, pal);
        spr(art_.head, xs[i], 108 + bob, 22, PAL_SKIN);
        spr(art_.book, xs[i], 156 + bob, 10, PAL_NOTE);
        if (i == 0 || i == 4) spr(art_.candle, xs[i] + (i == 0 ? -22 : 22), 132, 22, PAL_BELL);
    }

    if (mode_ == Mode::Phrase) {
        for (int i = 0; i < NBEAT; i++) {
            if (struck_[i]) continue;
            int dt = phrase_ - PHRASE[i].at;
            if (dt < -28 || dt > WINDOW) continue;
            float y = 92 - (PHRASE[i].at - phrase_) * 0.35f;
            spr(art_.note, xs[PHRASE[i].who], y, dt > -WINDOW ? 26 : 18, PAL_NOTE);
            break;
        }
    }

    spr(art_.pew, 160, 186, 26, PAL_STONE);
    spr(art_.floor, 160, 208, 32, PAL_STONE);
    spr(art_.column, 18, 110, 130, PAL_STONE);
    spr(art_.column, 302, 110, 130, PAL_STONE);
    spr(art_.arch, 160, 78, 120, PAL_STONE);
    spr(art_.glass, 160, 86, 64, PAL_GLASS);
    spr(art_.glass, 78, 96, 48, PAL_GLASS);
    spr(art_.glass, 242, 96, 48, PAL_GLASS);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update();
    draw();
}

}  // namespace choir
