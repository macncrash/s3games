#include "game/score.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace score {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap deskArt() {
    Bitmap b(300, 70);
    b.rect(0, 18, 300, 36, 2);
    b.rect(0, 18, 300, 8, 1);
    b.rect(0, 46, 300, 8, 3);
    b.rect(24, 52, 10, 16, 3);
    b.rect(266, 52, 10, 16, 3);
    b.rect(8, 8, 16, 14, 4);
    b.rect(276, 8, 16, 14, 4);
    b.outline(3, false);
    return b;
}

Bitmap sheetArt() {
    Bitmap b(250, 96);
    b.rect(0, 0, 250, 96, 1);
    b.rect(6, 6, 238, 84, 2);
    for (int i = 0; i < 5; i++) b.rect(28, 28 + i * 10, 200, 2, 3);
    b.rect(214, 24, 3, 46, 3);
    b.rect(222, 24, 2, 46, 3);
    b.outline(4, false);
    return b;
}

Bitmap clefArt() {
    Bitmap b(28, 64);
    b.ellipse(14, 40, 8, 10, 1);
    b.rect(16, 10, 4, 42, 1);
    b.ellipse(18, 16, 6, 6, 1);
    b.ellipse(14, 40, 3, 4, 2);
    b.rect(10, 48, 8, 3, 1);
    return b;
}

Bitmap noteArt() {
    Bitmap b(22, 40);
    b.ellipse(8, 30, 7, 5, 1);
    b.rect(14, 4, 3, 28, 1);
    b.rect(14, 4, 7, 3, 1);
    return b;
}

Bitmap restArt() {
    Bitmap b(16, 28);
    b.rect(3, 6, 10, 3, 1);
    b.rect(3, 12, 10, 3, 2);
    b.rect(3, 18, 10, 3, 1);
    return b;
}

Bitmap barArt() {
    Bitmap b(8, 56);
    b.rect(2, 0, 4, 56, 1);
    return b;
}

Bitmap markArt() {
    Bitmap b(36, 36);
    b.rect(1, 1, 34, 34, 2);
    b.rect(4, 4, 28, 28, 1);
    b.rect(10, 8, 4, 20, 3);
    b.rect(10, 8, 14, 4, 3);
    b.rect(10, 16, 10, 3, 3);
    b.rect(22, 10, 3, 8, 3);
    return b;
}

Bitmap lampArt() {
    Bitmap b(30, 70);
    b.rect(13, 22, 4, 44, 2);
    b.poly({{4, 24}, {15, 4}, {26, 24}}, 1);
    b.rect(6, 22, 18, 6, 3);
    b.ellipse(15, 16, 4, 3, 4);
    return b;
}

Bitmap handArt() {
    Bitmap b(40, 28);
    b.ellipse(16, 16, 12, 8, 1);
    b.rect(22, 12, 16, 4, 2);
    b.rect(34, 8, 3, 14, 3);
    b.rect(8, 20, 6, 4, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

const float kFreq[4] = {262.f, 330.f, 392.f, 494.f};

}  // namespace

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 11), gs::rgb4(8, 7, 6), gs::rgb4(15, 11, 4), gs::rgb4(12, 4, 3),
                          gs::rgb4(6, 12, 8), gs::rgb4(8, 11, 14), shadow, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(6, 4, 3),
                           gs::rgb4(14, 12, 8), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 11, 8), gs::rgb4(3, 2, 2), gs::rgb4(7, 6, 5),
                            shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 2, 4), gs::rgb4(5, 5, 7), gs::rgb4(1, 1, 2), shadow, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(10, 7, 2), gs::rgb4(6, 3, 1), gs::rgb4(15, 15, 10),
                           shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROOM, {0, gs::rgb4(14, 12, 7), gs::rgb4(6, 4, 3), gs::rgb4(12, 9, 4), gs::rgb4(15, 14, 8),
                           shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    loadFont(vdp, art_);
    art_.desk = gs::uploadMipped(vdp, deskArt());
    art_.sheet = gs::uploadMipped(vdp, sheetArt());
    art_.clef = gs::uploadMipped(vdp, clefArt());
    art_.note = gs::uploadMipped(vdp, noteArt());
    art_.rest = gs::uploadMipped(vdp, restArt());
    art_.bar = gs::uploadMipped(vdp, barArt());
    art_.mark = gs::uploadMipped(vdp, markArt());
    art_.lamp = gs::uploadMipped(vdp, lampArt());
    art_.hand = gs::uploadMipped(vdp, handArt());
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    const float beats[kNotes] = {0.85f, 1.45f, 2.05f, 2.65f, 3.40f, 4.00f, 4.60f, 5.25f};
    const int lanes[kNotes] = {0, 1, 2, 1, 3, 2, 0, 3};
    for (int i = 0; i < kNotes; i++) {
        notes_[i].lane = lanes[i];
        notes_[i].beat = beats[i];
        notes_[i].state = 0;
    }
    sys.apu.setMaster(0.42f);
    gs::FMPatch tone;
    tone.alg = 0;
    tone.vol = 0.22f;
    tone.op[0] = {1, 1, 0.01f, 0.12f, 0.55f, 0.18f, 0};
    tone.op[1] = {2, 0.28f, 0.01f, 0.16f, 0.3f, 0.22f, 0};
    tone.op[2] = {1, 0.12f, 0.02f, 0.2f, 0.2f, 0.3f, 0};
    tone.op[3] = {1, 0.35f, 0.01f, 0.18f, 0.4f, 0.2f, 0};
    sys.apu.setPatch(0, tone);
    sys.apu.setPatch(1, tone);
    msg_ = "FINISH THE MARK";
    msgT_ = 4;
    draw();
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::fanfare() {
    sys_->apu.keyOn(0, 523, 0.24f);
    sys_->apu.keyOn(1, 659, 0.18f);
}

void Game::hud(int col, int row, const std::string& s) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
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

bool Game::press(gs::Button b) const { return sys_->pad.pressed(b); }

float Game::noteX(const Note& n) const { return kLine + (n.beat - song_) * kSpeed; }

void Game::strike(Note& n) {
    n.state = 1;
    struck_++;
    blip(kFreq[n.lane]);
    if (struck_ >= kNotes) {
        mode_ = Mode::Seal;
        t_ = 0;
        seal_ = 0;
        msg_ = "FINISHED MARK";
        msgT_ = 6;
        fanfare();
    }
}

void Game::miss(Note& n) {
    if (n.state != 0) return;
    n.state = 2;
    tries_--;
    msg_ = tries_ > 0 ? "OPEN MARK" : "MARK OPEN";
    msgT_ = 1.2f;
    blip(98);
    if (tries_ <= 0) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
    }
}

int Game::lanePressed() const {
    if (press(gs::BTN_LEFT)) return 0;
    if (press(gs::BTN_DOWN)) return 1;
    if (press(gs::BTN_UP)) return 2;
    if (press(gs::BTN_RIGHT)) return 3;
    if (press(gs::BTN_A)) return 0;
    if (press(gs::BTN_B)) return 1;
    if (press(gs::BTN_C)) return 2;
    if (press(gs::BTN_X)) return 3;
    return -1;
}

void Game::backdrop() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = 3 + int((1.0f - u) * 2);
        int g = 2 + int((1.0f - u) * 2);
        int b = 4 + int(u * 2);
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::update(float dt) {
    t_ += dt;
    if (msgT_ > 0) msgT_ -= dt;

    if (mode_ == Mode::Title) {
        if (bot_ ? t_ > 0.4f : (press(gs::BTN_START) || press(gs::BTN_A))) {
            mode_ = Mode::Play;
            t_ = 0;
            song_ = 0;
            msg_ = "HIT THE NOTE ON THE BAR";
            msgT_ = 2.2f;
            blip(392);
        }
        return;
    }

    if (mode_ == Mode::Play) {
        song_ += dt;
        for (int i = 0; i < kNotes; i++) {
            Note& n = notes_[i];
            if (n.state != 0) continue;
            if (noteX(n) < kLine - 18.f) miss(n);
        }
        if (over_) return;

        if (bot_) {
            for (int i = 0; i < kNotes; i++) {
                Note& n = notes_[i];
                if (n.state != 0) continue;
                float x = noteX(n);
                if (x <= kLine + 8.f && x >= kLine - 8.f) {
                    strike(n);
                    break;
                }
            }
        } else {
            int lane = lanePressed();
            if (lane >= 0) {
                int best = -1;
                float bestD = 1e9f;
                for (int i = 0; i < kNotes; i++) {
                    if (notes_[i].state != 0) continue;
                    float d = std::fabs(noteX(notes_[i]) - kLine);
                    if (d < 20.f && d < bestD) {
                        bestD = d;
                        best = i;
                    }
                }
                if (best >= 0) {
                    if (notes_[best].lane == lane) strike(notes_[best]);
                    else miss(notes_[best]);
                }
            }
        }
        return;
    }

    if (mode_ == Mode::Seal) {
        seal_ = std::min(1.f, seal_ + dt * 0.85f);
        if (t_ > 1.6f) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();

    spr(art_.lamp, 28, 78, 64, PAL_ROOM);
    spr(art_.desk, 160, 176, 62, PAL_WOOD);
    spr(art_.sheet, 168, 108, 92, PAL_PAPER);
    spr(art_.clef, 62, 108, 48, PAL_INK);
    spr(art_.bar, kLine, 108, 46, PAL_INK);

    const float laneY[4] = {128.f, 116.f, 104.f, 92.f};
    for (int i = 0; i < kNotes; i++) {
        const Note& n = notes_[i];
        float x = (mode_ == Mode::Title) ? (118.f + i * 18.f) : noteX(n);
        if (x < 40 || x > 310) continue;
        int pal = n.state == 1 ? PAL_GOLD : PAL_INK;
        if (n.state == 2) pal = PAL_WOOD;
        spr(art_.note, x, laneY[n.lane], n.state == 1 ? 30.f : 26.f, pal);
    }

    float markX = 248.f;
    float markY = mode_ == Mode::Seal || (mode_ == Mode::Over && won_) ? 108.f - (1.f - seal_) * 36.f : 62.f;
    if (mode_ == Mode::Title) markY = 62.f;
    if (mode_ == Mode::Over && won_) markY = 108.f;
    spr(art_.mark, markX, markY, won_ ? 34.f : 28.f, won_ ? PAL_GOLD : PAL_INK);

    float handX = kLine - 8.f;
    if (mode_ == Mode::Play) {
        for (int i = 0; i < kNotes; i++) {
            if (notes_[i].state == 0) {
                handX = std::clamp(noteX(notes_[i]), 70.f, 200.f);
                break;
            }
        }
    }
    spr(art_.hand, handX, 154, 22, PAL_ROOM);

    hud(11, 1, "S3 SCOREMARK");
    if (mode_ == Mode::Title) {
        hud(10, 3, "PLAY THE SCORE");
        hud(4, 24, "ARROWS ON THE BAR  START");
    } else {
        char line[24];
        std::snprintf(line, sizeof(line), "TRIES %d", tries_);
        hud(1, 1, line);
        std::snprintf(line, sizeof(line), "NOTES %d", struck_);
        hud(30, 1, line);
        if (msgT_ > 0) {
            int col = std::max(0, 20 - int(msg_.size()) / 2);
            hud(col, 25, msg_);
        }
        if (won_) hud(13, 23, "MARK CLOSED");
        if (over_ && !won_) hud(14, 23, "MARK OPEN");
        if (mode_ == Mode::Play) hud(1, 26, "L D U R");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.0f / 60.0f);
    draw();
}

}  // namespace score
