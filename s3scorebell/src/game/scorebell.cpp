#include "game/scorebell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace scorebell {
namespace {

const float kFreq[4] = {294.f, 349.f, 440.f, 523.f};
const float kLaneY[4] = {128.f, 116.f, 104.f, 92.f};
const float kBeats[6] = {0.85f, 1.45f, 2.05f, 2.75f, 3.40f, 4.15f};
const int kLanes[6] = {0, 2, 1, 3, 2, 1};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.4f);
    for (int i = 0; i < kNotes; i++) {
        notes_[i].lane = kLanes[i];
        notes_[i].beat = kBeats[i];
        notes_[i].state = 0;
    }
    mode_ = Mode::Title;
    why_ = "OPEN";
    draw();
}

void Game::freshPhrase() {
    song_ = 0;
    struck_ = 0;
    for (int i = 0; i < kNotes; i++) notes_[i].state = 0;
}

void Game::begin() {
    freshPhrase();
    mode_ = Mode::Play;
    t_ = 0;
    why_ = "PLAY THE BAR";
    blip(392.f);
}

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.07f); }

void Game::strike(Note& n) {
    if (n.state != 0) return;
    n.state = 1;
    struck_++;
    blip(kFreq[n.lane]);
    if (struck_ >= kNotes) ring();
}

void Game::dieTry(const char* why) {
    if (mode_ != Mode::Play) return;
    dead_++;
    why_ = why;
    sys_->apu.tone(0, 110.f, 0.08f);
    sys_->apu.noiseBurst(0.1f, 400.f, 0.18f);
    sys_->rumble(0.35f, 0.15f, 60);
    for (int i = 0; i < kNotes; i++) {
        if (notes_[i].state == 0) notes_[i].state = 2;
    }
    if (dead_ >= 3) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        rung_ = false;
        why_ = "the third try died";
        return;
    }
    freshPhrase();
}

void Game::ring() {
    mode_ = Mode::Ring;
    rung_ = true;
    t_ = 0;
    swing_ = 1.f;
    chime_ = 0;
    chimeWait_ = 0;
    why_ = "bell rang before the third try died";
    sys_->setLight(220, 170, 40);
}

void Game::leave() {
    mode_ = Mode::Leave;
    over_ = true;
    won_ = rung_ && dead_ < 3 && struck_ >= kNotes;
    if (!won_) why_ = "left before the bell";
}

float Game::noteX(const Note& n) const { return kLine + (n.beat - song_) * kSpeed; }

int Game::lanePressed() const {
    const gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_LEFT) || pad.pressed(gs::BTN_A)) return 0;
    if (pad.pressed(gs::BTN_DOWN) || pad.pressed(gs::BTN_B)) return 1;
    if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_C)) return 2;
    if (pad.pressed(gs::BTN_RIGHT) || pad.pressed(gs::BTN_X)) return 3;
    return -1;
}

void Game::update(float dt) {
    age_++;
    t_ += dt;
    if (swing_ > 0.02f) swing_ *= 0.985f;
    else swing_ = 0;

    if (mode_ == Mode::Title) {
        if (bot_ ? t_ > 0.35f : (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) begin();
        return;
    }
    if (mode_ == Mode::Lose || mode_ == Mode::Leave) {
        if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
            dead_ = 0;
            rung_ = false;
            over_ = false;
            won_ = false;
            chime_ = -1;
            swing_ = 0;
            begin();
        }
        return;
    }
    if (mode_ == Mode::Ring) {
        if (t_ > 1.4f) leave();
        return;
    }

    song_ += dt;
    for (int i = 0; i < kNotes; i++) {
        Note& n = notes_[i];
        if (n.state != 0) continue;
        if (noteX(n) < kLine - 16.f) {
            dieTry("a note slipped the bar");
            return;
        }
    }

    if (bot_) {
        for (int i = 0; i < kNotes; i++) {
            Note& n = notes_[i];
            if (n.state != 0) continue;
            float x = noteX(n);
            if (x <= kLine + 6.f && x >= kLine - 10.f) {
                strike(n);
                break;
            }
        }
        return;
    }

    int lane = lanePressed();
    if (lane < 0) return;
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < kNotes; i++) {
        if (notes_[i].state != 0) continue;
        float d = std::fabs(noteX(notes_[i]) - kLine);
        if (d < 18.f && d < bestD) {
            bestD = d;
            best = i;
        }
    }
    if (best < 0) return;
    if (notes_[best].lane == lane) strike(notes_[best]);
    else dieTry("wrong lane on the bar");
}

void Game::audio() {
    if (mode_ != Mode::Play && mode_ != Mode::Ring && sys_->frame % 8 == 0) sys_->apu.tone(0, 0, 0);
    if (chime_ < 0) return;
    if (chimeWait_ > 0) {
        chimeWait_--;
        return;
    }
    static const float mel[] = {523.f, 659.f, 784.f, 1046.f, 784.f};
    if (chime_ < 5) {
        sys_->apu.tone(1, mel[chime_], 0.09f);
        sys_->apu.tone(2, mel[chime_] * 0.5f, 0.04f);
        chimeWait_ = 10;
        chime_++;
        swing_ = 1.f;
    } else {
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
        chime_ = -1;
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = 2 + int((1.f - u) * 3);
        int g = 2 + int((1.f - u) * 2);
        int b = 4 + int(u * 3);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::hud(int col, int row, const char* s) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.5f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    backdrop();

    spr(art_.stand, 286.f, 86.f, 78.f, PAL_WOOD);
    float sway = std::sin(t_ * 14.f) * swing_ * 10.f;
    spr(art_.bell, 286.f + sway, 46.f, 34.f, PAL_BRASS);
    spr(art_.clapper, 286.f + sway * 1.4f, 62.f, 12.f, PAL_BRASS);
    spr(art_.desk, 150.f, 186.f, 46.f, PAL_WOOD);
    spr(art_.sheet, 150.f, 112.f, 86.f, PAL_PAPER);
    spr(art_.clef, 48.f, 112.f, 46.f, PAL_INK);
    spr(art_.bar, kLine, 112.f, 48.f, PAL_INK);

    for (int i = 0; i < kNotes; i++) {
        const Note& n = notes_[i];
        float x = (mode_ == Mode::Title) ? (100.f + i * 22.f) : noteX(n);
        if (x < 36.f || x > 300.f) continue;
        int pal = PAL_INK;
        float h = 24.f;
        if (n.state == 1) {
            pal = PAL_BRASS;
            h = 28.f;
        } else if (n.state == 2) {
            pal = PAL_DEAD;
        }
        spr(art_.note, x, kLaneY[n.lane], h, pal);
    }

    hud(12, 1, "S3 SCORE BELL");
    if (mode_ == Mode::Title) {
        hud(6, 3, "PLAY UNTIL THE BELL");
        hud(2, 24, "L D U R ON THE BAR   START");
        hud(3, 26, "BELL BEFORE THE THIRD TRY");
    } else {
        char line[40];
        std::snprintf(line, sizeof(line), "DEAD %d/3", dead_);
        hud(1, 1, line);
        std::snprintf(line, sizeof(line), "NOTES %d", struck_);
        hud(30, 1, line);
        if (mode_ == Mode::Play) hud(1, 26, "HIT THE LANE ON THE BAR");
        if (mode_ == Mode::Ring || mode_ == Mode::Leave) {
            hud(6, 24, "BELL RANG");
            hud(4, 26, "LEAVE BEFORE THE THIRD TRY");
        }
        if (mode_ == Mode::Lose) {
            hud(8, 24, "THIRD TRY DIED");
            hud(6, 26, "START TRIES THE SCORE");
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.f / 60.f);
    audio();
    if (mode_ == Mode::Ring || mode_ == Mode::Leave) sys.setLight(220, 170, 40);
    else if (mode_ == Mode::Lose) sys.setLight(160, 30, 24);
    else sys.setLight(40, 50, 80);
    draw();
}

}  // namespace scorebell
