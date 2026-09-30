#include "game/sally.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sally {
namespace {

constexpr float kFloor = 184.f;
constexpr float kSpawn = 64.f;
constexpr float kGoal = 1004.f;
constexpr float kWorld = 1120.f;
constexpr float kWatch = 16.f;
constexpr float kWalk = 2.6f;
constexpr float kJump = -6.4f;
constexpr float kGrav = 0.30f;
constexpr float kBody = 8.f;

struct Span {
    float a, b;
};
constexpr Span kGround[] = {{0.f, 236.f}, {300.f, 628.f}, {700.f, 1120.f}};

struct Box {
    float a, b, top;
};
constexpr Box kCrate = {452.f, 490.f, 184.f - 44.f};

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap heroArt() {
    gs::Bitmap b(26, 42);
    b.rect(9, 1, 9, 8, 3);
    b.rect(11, 3, 2, 2, 4);
    b.rect(16, 3, 2, 2, 4);
    b.rect(7, 9, 13, 6, 5);
    b.rect(6, 14, 15, 12, 1);
    b.rect(4, 16, 4, 8, 2);
    b.rect(18, 15, 6, 5, 6);
    b.rect(10, 26, 4, 12, 2);
    b.rect(15, 26, 4, 12, 2);
    b.rect(9, 37, 6, 3, 7);
    b.rect(15, 37, 6, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap pouchArt() {
    gs::Bitmap b(16, 14);
    b.ellipse(8, 8, 6, 5, 1);
    b.rect(6, 2, 4, 4, 2);
    b.rect(5, 1, 6, 2, 3);
    b.rect(7, 6, 2, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(24, 14);
    b.rect(0, 0, 24, 14, 1);
    b.rect(0, 0, 24, 2, 2);
    b.rect(0, 12, 24, 2, 3);
    b.line(8, 0, 8, 14, 3, 1);
    b.line(16, 0, 16, 14, 3, 1);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(28, 32);
    b.rect(1, 2, 26, 28, 1);
    b.line(1, 2, 27, 30, 2, 1);
    b.line(27, 2, 1, 30, 2, 1);
    b.rect(1, 2, 26, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(36, 78);
    b.rect(0, 8, 6, 70, 1);
    b.rect(30, 8, 6, 70, 1);
    b.rect(0, 0, 36, 12, 2);
    b.rect(4, 2, 28, 6, 3);
    b.rect(14, 14, 8, 56, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap barsArt() {
    gs::Bitmap b(28, 40);
    for (int x = 2; x < 28; x += 6) b.rect(x, 0, 2, 40, 1);
    b.rect(0, 6, 28, 3, 2);
    b.rect(0, 22, 28, 3, 2);
    return b;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ && won_) return 3;
    if (over_) return 4;
    if (px_ > 640.f) return 2;
    return 1;
}

bool Game::groundAt(float x) const {
    for (const Span& s : kGround)
        if (x >= s.a && x <= s.b) return true;
    return false;
}

bool Game::onCrate(float x, float feet) const {
    return x >= kCrate.a - kBody && x <= kCrate.b + kBody && std::fabs(feet - kCrate.top) < 1.5f;
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 13), gs::rgb4(14, 8, 3), gs::rgb4(2, 1, 1), gs::rgb4(15, 4, 2)};
    const uint16_t stone[] = {0, gs::rgb4(9, 8, 7), gs::rgb4(12, 11, 9), gs::rgb4(5, 4, 4), gs::rgb4(3, 3, 3)};
    const uint16_t wood[] = {0, gs::rgb4(11, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(14, 10, 4), gs::rgb4(3, 2, 1)};
    const uint16_t pouch[] = {0, gs::rgb4(10, 4, 2), gs::rgb4(14, 11, 5), gs::rgb4(6, 3, 1), gs::rgb4(15, 13, 6),
                              gs::rgb4(2, 1, 1)};
    const uint16_t hero[] = {0, gs::rgb4(4, 5, 9), gs::rgb4(3, 3, 6), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 2),
                             gs::rgb4(8, 2, 2), gs::rgb4(12, 9, 3), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 2)};
    const uint16_t iron[] = {0, gs::rgb4(7, 8, 9), gs::rgb4(11, 12, 12), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3),
                             gs::rgb4(1, 1, 2)};
    const uint16_t dawn[] = {0, gs::rgb4(15, 10, 5), gs::rgb4(15, 13, 8), gs::rgb4(8, 6, 8)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), gs::rgb4(15, 12, 8)};
    const uint16_t go[] = {0, gs::rgb4(8, 15, 6), gs::rgb4(3, 8, 3), gs::rgb4(14, 15, 10)};
    setPal(vdp, PAL_HUD, hud, 5);
    setPal(vdp, PAL_STONE, stone, 5);
    setPal(vdp, PAL_WOOD, wood, 5);
    setPal(vdp, PAL_POUCH, pouch, 6);
    setPal(vdp, PAL_HERO, hero, 9);
    setPal(vdp, PAL_IRON, iron, 6);
    setPal(vdp, PAL_DAWN, dawn, 4);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_GO, go, 4);
    vdp.setFogColor(gs::rgb4(8, 4, 3));
    for (int p = 0; p < gs::NUM_PALETTES; p++) {
        if (vdp.color(p * 16 + 1) == 0) vdp.setColor(p * 16 + 1, gs::rgb4(15, 15, 14));
        vdp.setColor(p * 16 + 15, gs::rgb4(1, 1, 2));
    }

    art_.hero = gs::uploadMipped(vdp, heroArt());
    art_.pouch = gs::uploadMipped(vdp, pouchArt());
    art_.block = gs::uploadMipped(vdp, blockArt());
    art_.crate = gs::uploadMipped(vdp, crateArt());
    art_.gate = gs::uploadMipped(vdp, gateArt());
    art_.bars = gs::uploadMipped(vdp, barsArt());

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
    over_ = false;
    won_ = false;
    t_ = 0;
    sys.setLight(90, 40, 16);
}

void Game::begin() {
    px_ = kSpawn;
    py_ = kFloor;
    vy_ = 0;
    face_ = 1;
    onGround_ = true;
    plant_ = 0;
    watch_ = 0;
    cam_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    sys_->apu.tone(0, 220.f, 0.08f);
}

void Game::finish(bool crossed, const char* why) {
    won_ = crossed;
    over_ = true;
    reason_ = why;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    plant_ = 0;
    if (crossed) sys_->apu.tone(0, 523.f, 0.12f);
    else sys_->apu.tone(0, 110.f, 0.1f);
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = false;
    right = true;
    jump = false;
    if (!onGround_) return;
    const float edge[] = {236.f, 628.f};
    for (float e : edge) {
        if (px_ > e - 22.f && px_ < e - 2.f && groundAt(px_)) jump = true;
    }
    if (py_ > kCrate.top + 8.f && px_ > kCrate.a - 28.f && px_ < kCrate.a - 2.f) jump = true;
    if (px_ >= kGoal) right = false;
}

void Game::stepPlay(bool left, bool right, bool jump) {
    if (left && !right) {
        face_ = -1;
        px_ -= kWalk;
    } else if (right && !left) {
        face_ = 1;
        px_ += kWalk;
    }
    px_ = std::clamp(px_, 16.f, kWorld - 16.f);

    const bool bodyIn = onGround_ && py_ > kCrate.top + 8.f && py_ < kFloor + 2.f;
    if (bodyIn && px_ + kBody > kCrate.a && px_ - kBody < kCrate.b) {
        if (face_ > 0) px_ = kCrate.a - kBody;
        else px_ = kCrate.b + kBody;
    }

    if (jump && onGround_) {
        vy_ = kJump;
        onGround_ = false;
        sys_->apu.tone(1, 330.f, 0.05f);
    }
    vy_ += kGrav;
    if (vy_ > 7.f) vy_ = 7.f;
    float ny = py_ + vy_;
    onGround_ = false;
    if (vy_ >= 0.f) {
        if (px_ + kBody > kCrate.a && px_ - kBody < kCrate.b && py_ <= kCrate.top + 1.f && ny >= kCrate.top) {
            ny = kCrate.top;
            vy_ = 0;
            onGround_ = true;
        } else if (groundAt(px_) && py_ <= kFloor + 1.f && ny >= kFloor) {
            ny = kFloor;
            vy_ = 0;
            onGround_ = true;
        }
    }
    py_ = ny;

    if (py_ > 230.f) {
        finish(false, "THE POUCH FELL IN THE DITCH");
        return;
    }

    watch_ += 1.f / 60.f;
    if (watch_ > kWatch) {
        finish(false, "THE PORTCULLIS CLOSED");
        return;
    }

    if (onGround_ && groundAt(px_) && px_ >= kGoal && !onCrate(px_, py_)) {
        if (++plant_ > 28) finish(true, "THE POUCH CROSSED");
    } else {
        plant_ = 0;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool feet, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? foot - s.h : foot - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
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
        int warm = y < 120 ? (120 - y) / 18 : 0;
        int dusk = y > 160 ? (y - 160) / 16 : 0;
        vdp.lineBackdrop[y] = gs::rgb4(6 + warm - dusk, 4 + warm / 2, 8 - warm / 3);
        vdp.lineFog[y] = uint8_t(y < 28 ? (28 - y) / 8 : 0);
        vdp.road[y].on = false;
    }

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool feet = true, bool flip = false) {
        spr(m, wx - cam_, foot, h, pal, feet, flip);
    };

    for (const Span& s : kGround) {
        for (float x = s.a + 12.f; x < s.b; x += 24.f) world(art_.block, x, kFloor + 16.f, 18, PAL_STONE);
    }
    world(art_.crate, (kCrate.a + kCrate.b) * 0.5f, kFloor, 48, PAL_WOOD);
    world(art_.gate, 28.f, kFloor, 110, PAL_STONE);
    world(art_.gate, 1068.f, kFloor, 110, PAL_STONE);

    float drop = std::clamp(watch_ / kWatch, 0.f, 1.f) * 62.f;
    if (mode_ == Mode::Title) drop = 8.f;
    world(art_.bars, 1048.f, kFloor - 78.f + drop, 52, PAL_IRON);

    if (mode_ != Mode::Title) {
        world(art_.hero, px_, py_, 50, PAL_HERO, true, face_ < 0);
        world(art_.pouch, px_ + face_ * 12.f, py_ - 20.f, 16, PAL_POUCH, false);
    }

    if (mode_ == Mode::Title) {
        text("S3 SALLY POUC", 160, 26, 1.0f, PAL_HUD);
        text("ONE SALLY", 160, 52, 0.85f, PAL_DAWN);
        text("CARRY THE POUCH ACROSS", 160, 74, 0.7f, PAL_GO);
        if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 104, 1.f, PAL_HUD);
        world(art_.hero, 108.f, kFloor, 50, PAL_HERO);
        world(art_.pouch, 122.f, kFloor - 20.f, 16, PAL_POUCH, false);
        hudC(25, "THEN IT IS DONE", PAL_ALERT);
        return;
    }

    int left = std::max(0, int(std::ceil(kWatch - watch_)));
    char line[40];
    std::snprintf(line, sizeof line, "SALLY %d", left);
    hud(1, 1, line, left <= 4 ? PAL_ALERT : PAL_HUD);
    hud(30, 1, "POUCH", PAL_POUCH);
    if (mode_ == Mode::Play) hudC(25, "ONE SALLY  CROSS THE YARD", PAL_HUD);
    if (mode_ == Mode::Pause) hudC(12, "PAUSED", PAL_HUD);
    if (mode_ == Mode::Won) {
        hudC(11, "THE POUCH CROSSED", PAL_GO);
        hudC(13, "THE SALLY IS DONE", PAL_DAWN);
    }
    if (mode_ == Mode::Lost) {
        hudC(11, "THE SALLY FAILED", PAL_ALERT);
        hudC(13, reason_, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        cam_ = 40.f;
        if (bot_ && t_ > 0.4f) begin();
        else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        draw();
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
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

    bool left = false, right = false, jump = false;
    if (bot_) bot(left, right, jump);
    else {
        left = pad.down(gs::BTN_LEFT);
        right = pad.down(gs::BTN_RIGHT);
        if (pad.axisX < -0.4f) left = true;
        if (pad.axisX > 0.4f) right = true;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_UP);
    }
    stepPlay(left, right, jump);

    float want = px_ - 120.f;
    cam_ = std::clamp(want, 0.f, kWorld - float(gs::SCREEN_W));
    draw();
}

}  // namespace sally
