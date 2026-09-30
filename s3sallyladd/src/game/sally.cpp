#include "game/sally.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sallyladd {
namespace {

constexpr float kWatch = 22.f;
constexpr float kWorld = 1280.f;
constexpr float kWalk = 2.55f;
constexpr float kJump = -6.6f;
constexpr float kGrav = 0.28f;
constexpr float kBody = 8.f;
constexpr float kClimb = 1.45f;
constexpr float kLadX = 1088.f;
constexpr float kLadTop = 72.f;
constexpr float kLossY = 236.f;

struct Deck {
    float a, b, y;
};

// Gaps are about 62px. A lip jump clears them and still lands on the next deck.
constexpr Deck kDecks[] = {
    {0.f, 230.f, 188.f},
    {292.f, 510.f, 170.f},
    {574.f, 790.f, 182.f},
    {852.f, 1240.f, 158.f},
};
constexpr int kDeckN = 4;
constexpr int kFar = 3;

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap heroArt() {
    gs::Bitmap b(22, 40);
    b.rect(7, 1, 8, 7, 3);
    b.rect(9, 3, 2, 2, 4);
    b.rect(13, 3, 2, 2, 4);
    b.rect(6, 8, 11, 5, 5);
    b.rect(5, 13, 13, 12, 1);
    b.rect(3, 14, 3, 8, 2);
    b.rect(16, 14, 4, 6, 6);
    b.rect(8, 25, 3, 11, 2);
    b.rect(13, 25, 3, 11, 2);
    b.rect(7, 35, 5, 3, 7);
    b.rect(13, 35, 5, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(28, 16);
    b.rect(0, 0, 28, 16, 1);
    b.rect(0, 0, 28, 3, 2);
    b.rect(0, 13, 28, 3, 3);
    b.line(9, 0, 9, 16, 3, 1);
    b.line(18, 0, 18, 16, 3, 1);
    return b;
}

gs::Bitmap rungArt() {
    gs::Bitmap b(18, 28);
    b.rect(1, 0, 3, 28, 1);
    b.rect(14, 0, 3, 28, 1);
    b.rect(1, 6, 16, 3, 2);
    b.rect(1, 16, 16, 3, 2);
    b.rect(1, 25, 16, 2, 2);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(40, 86);
    b.rect(0, 10, 8, 76, 1);
    b.rect(32, 10, 8, 76, 1);
    b.rect(0, 0, 40, 14, 2);
    b.rect(6, 3, 28, 7, 3);
    for (int y = 18; y < 82; y += 8) b.rect(12, y, 16, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ditchArt() {
    gs::Bitmap b(32, 12);
    b.rect(0, 2, 32, 8, 1);
    b.rect(4, 4, 10, 3, 2);
    b.rect(18, 5, 8, 2, 2);
    return b;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ && won_) return 3;
    if (over_) return 4;
    if (px_ >= kDecks[kFar].a) return 2;
    return 1;
}

int Game::deckAt(float x, float feet) const {
    for (int i = 0; i < kDeckN; i++) {
        const Deck& d = kDecks[i];
        if (x < d.a - kBody || x > d.b + kBody) continue;
        if (std::fabs(feet - d.y) <= 2.5f) return i;
    }
    return -1;
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 13), gs::rgb4(14, 9, 4), gs::rgb4(2, 1, 1), gs::rgb4(15, 5, 2)};
    const uint16_t stone[] = {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 11, 9), gs::rgb4(4, 4, 4), gs::rgb4(3, 3, 4),
                              gs::rgb4(1, 1, 2)};
    const uint16_t wood[] = {0, gs::rgb4(10, 6, 3), gs::rgb4(14, 10, 4), gs::rgb4(5, 3, 1)};
    const uint16_t hero[] = {0, gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 2),
                             gs::rgb4(7, 2, 2), gs::rgb4(12, 8, 3), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 2)};
    const uint16_t iron[] = {0, gs::rgb4(8, 9, 10), gs::rgb4(13, 13, 12), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3)};
    const uint16_t dawn[] = {0, gs::rgb4(15, 11, 6), gs::rgb4(12, 8, 8), gs::rgb4(6, 5, 8)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), gs::rgb4(15, 12, 8)};
    const uint16_t go[] = {0, gs::rgb4(8, 15, 6), gs::rgb4(3, 8, 3), gs::rgb4(14, 15, 10)};
    setPal(vdp, PAL_HUD, hud, 5);
    setPal(vdp, PAL_STONE, stone, 6);
    setPal(vdp, PAL_WOOD, wood, 4);
    setPal(vdp, PAL_HERO, hero, 9);
    setPal(vdp, PAL_IRON, iron, 5);
    setPal(vdp, PAL_DAWN, dawn, 4);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_GO, go, 4);
    vdp.setFogColor(gs::rgb4(6, 4, 5));
    for (int p = 0; p < gs::NUM_PALETTES; p++) {
        if (vdp.color(p * 16 + 1) == 0) vdp.setColor(p * 16 + 1, gs::rgb4(15, 15, 14));
        vdp.setColor(p * 16 + 15, gs::rgb4(1, 1, 2));
    }

    art_.hero = gs::uploadMipped(vdp, heroArt());
    art_.block = gs::uploadMipped(vdp, blockArt());
    art_.rung = gs::uploadMipped(vdp, rungArt());
    art_.gate = gs::uploadMipped(vdp, gateArt());
    art_.ditch = gs::uploadMipped(vdp, ditchArt());

    gs::TileAlloc tiles(vdp, 1);
    gs::TextStyle big{2, 1, 0, 15, 1};
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
        art_.font[c - 32] = t;
        art_.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.resize(64, 32);
    vdp.HUD.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    mode_ = Mode::Title;
    t_ = 0;
    cam_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    climbing_ = false;
    onGround_ = true;
    reason_ = "";
    px_ = 72.f;
    py_ = kDecks[0].y;
    vy_ = 0;
    face_ = 1;
    watch_ = 0;
    cam_ = 0;
    sys_->apu.tone(0, 0, 0);
}

void Game::finish(bool climbed, const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = climbed ? Mode::Won : Mode::Lost;
    over_ = true;
    won_ = climbed;
    reason_ = why;
    vy_ = 0;
    if (climbed) {
        climbing_ = true;
        py_ = kLadTop;
        px_ = kLadX;
        sys_->apu.tone(0, 520.f, 0.08f);
        sys_->rumble(0.25f, 0.7f, 180);
        sys_->setLight(40, 170, 70);
    } else {
        climbing_ = false;
        sys_->apu.noiseBurst(0.4f, 380.f, 0.22f);
        sys_->rumble(0.7f, 0.2f, 160);
        sys_->setLight(170, 30, 20);
    }
}

void Game::bot(bool& left, bool& right, bool& jump, bool& up) {
    left = false;
    right = false;
    jump = false;
    up = false;
    if (climbing_) {
        up = true;
        return;
    }
    int d = deckAt(px_, py_);
    if (d == kFar && px_ > kLadX - 10.f) {
        if (px_ < kLadX - 3.f) right = true;
        else if (px_ > kLadX + 3.f) left = true;
        else up = true;
        return;
    }
    right = true;
    if (d >= 0 && d < kFar) {
        const Deck& deck = kDecks[d];
        if (px_ > deck.b - 26.f) jump = true;
    }
}

void Game::stepPlay(bool left, bool right, bool jump, bool up) {
    watch_ += 1.f / 60.f;
    if (watch_ > kWatch) {
        finish(false, "THE WATCH IS OVER");
        return;
    }

    if (climbing_) {
        face_ = 1;
        px_ = kLadX;
        vy_ = 0;
        onGround_ = false;
        if (!up && (left || right)) {
            climbing_ = false;
            px_ = kLadX + (left ? -12.f : 12.f);
            py_ = kDecks[kFar].y;
            onGround_ = true;
            return;
        }
        if (up) py_ -= kClimb;
        if (py_ <= kLadTop) finish(true, "THE FAR LADDER");
        return;
    }

    float vx = 0;
    if (left) {
        vx -= kWalk;
        face_ = -1;
    }
    if (right) {
        vx += kWalk;
        face_ = 1;
    }
    if (jump && onGround_) {
        vy_ = kJump;
        onGround_ = false;
        sys_->apu.tone(0, 280.f, 0.05f);
    }
    vy_ = std::min(vy_ + kGrav, 8.f);
    float prev = py_;
    px_ = std::clamp(px_ + vx, 16.f, kWorld - 16.f);
    py_ += vy_;

    onGround_ = false;
    if (vy_ >= 0) {
        for (int i = 0; i < kDeckN; i++) {
            const Deck& d = kDecks[i];
            if (px_ < d.a - 4.f || px_ > d.b + 4.f) continue;
            if (prev <= d.y + 2.f && py_ >= d.y) {
                py_ = d.y;
                vy_ = 0;
                onGround_ = true;
                break;
            }
        }
    }

    if (onGround_ && std::fabs(px_ - kLadX) <= 12.f && py_ >= kDecks[kFar].y - 2.f && up) {
        climbing_ = true;
        px_ = kLadX;
        vy_ = 0;
        sys_->apu.tone(0, 360.f, 0.04f);
        return;
    }

    if (py_ > kLossY) finish(false, "THE DITCH");
}

void Game::spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool feet, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? foot - s.h : foot - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 64 || s.x + s.w < -64 || s.y > gs::SCREEN_H + 64 || s.y + s.h < -64) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        int x = col + i;
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float gh = g.h * scale;
        spr(g, x + g.w * scale * 0.5f, y + gh * 0.5f, gh, pal, false);
        x += g.w * scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int warm = y < 90 ? (90 - y) / 16 : 0;
        int low = y > 150 ? (y - 150) / 14 : 0;
        vdp.lineBackdrop[y] = gs::rgb4(5 + warm - low / 2, 4 + warm / 3, 7 - low / 2);
        vdp.lineFog[y] = uint8_t(y < 20 ? 1 : 0);
        vdp.road[y].on = false;
    }

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool feet = true, bool flip = false) {
        spr(m, wx - cam_, foot, h, pal, feet, flip);
    };

    for (float x = 40.f; x < kWorld; x += 36.f) world(art_.ditch, x, 214.f, 14.f, PAL_IRON, false);
    for (const Deck& d : kDecks) {
        for (float x = d.a + 14.f; x < d.b; x += 28.f) world(art_.block, x, d.y + 16.f, 18.f, PAL_STONE);
    }
    world(art_.gate, 36.f, kDecks[0].y, 120.f, PAL_STONE);
    world(art_.gate, 1188.f, kDecks[kFar].y, 128.f, PAL_STONE);
    for (float y = kLadTop + 8.f; y <= kDecks[kFar].y + 6.f; y += 22.f)
        world(art_.rung, kLadX, y, 26.f, PAL_WOOD, false);

    if (mode_ != Mode::Title) world(art_.hero, px_, py_, 48.f, PAL_HERO, true, face_ < 0);

    if (mode_ == Mode::Title) {
        text("S3 SALLY LADD", 160, 28, 1.0f, PAL_HUD);
        text("AT THE SALLY", 160, 54, 0.8f, PAL_DAWN);
        text("REACH THE FAR LADDER", 160, 76, 0.7f, PAL_GO);
        if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 108, 1.f, PAL_HUD);
        world(art_.hero, 120.f, kDecks[0].y, 48.f, PAL_HERO);
        hudC(25, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        return;
    }

    int left = std::max(0, int(std::ceil(kWatch - watch_)));
    char line[40];
    std::snprintf(line, sizeof line, "WATCH %d", left);
    hud(1, 1, line, left <= 4 ? PAL_ALERT : PAL_HUD);
    hud(28, 1, "LADDER", PAL_WOOD);
    if (mode_ == Mode::Play) hudC(25, "SALLY  THE FAR LADDER", PAL_HUD);
    if (mode_ == Mode::Pause) hudC(12, "PAUSED", PAL_HUD);
    if (mode_ == Mode::Won) {
        hudC(11, "THE FAR LADDER", PAL_GO);
        hudC(13, "THE WATCH HOLDS", PAL_DAWN);
    }
    if (mode_ == Mode::Lost) {
        hudC(11, "THE WATCH IS OVER", PAL_ALERT);
        hudC(13, reason_, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        cam_ = 20.f;
        if (bot_ && t_ > 0.45f) begin();
        else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        draw();
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        sys.apu.tone(0, 0, 0);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            t_ = 0;
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        draw();
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        return;
    }

    bool left = false, right = false, jump = false, up = false;
    if (bot_) bot(left, right, jump, up);
    else {
        left = pad.down(gs::BTN_LEFT);
        right = pad.down(gs::BTN_RIGHT);
        up = pad.down(gs::BTN_UP);
        if (pad.axisX < -0.4f) left = true;
        if (pad.axisX > 0.4f) right = true;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    }
    stepPlay(left, right, jump, up);
    sys.apu.tone(0, 0, 0);

    float want = px_ - 110.f;
    cam_ = std::clamp(want, 0.f, kWorld - float(gs::SCREEN_W));
    draw();
}

}  // namespace sallyladd
