#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace pouc {
namespace {

constexpr float kFloor = 168.f;
constexpr float kSpawn = 72.f;
constexpr float kPouch0 = 156.f;
constexpr float kWatch = 58.f;
constexpr float kGoal = 1408.f;
constexpr float kWalk = 1.72f;
constexpr float kShield = 1.02f;
constexpr float kJump = -5.15f;
constexpr float kGrav = 0.34f;

struct Span {
    float a, b;
};
constexpr Span kPier[] = {{24.f, 280.f}, {548.f, 860.f}, {1120.f, 1680.f}};
constexpr float kWindA = 600.f, kWindB = 800.f;

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap heroArt() {
    gs::Bitmap b(28, 44);
    b.rect(10, 2, 10, 8, 3);
    b.rect(11, 4, 3, 2, 4);
    b.rect(16, 4, 3, 2, 4);
    b.rect(8, 10, 14, 16, 1);
    b.rect(6, 12, 4, 10, 2);
    b.rect(20, 14, 6, 4, 5);
    b.rect(22, 10, 5, 5, 6);
    b.rect(11, 26, 4, 14, 2);
    b.rect(16, 26, 4, 14, 2);
    b.rect(10, 38, 6, 3, 7);
    b.rect(16, 38, 6, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap pouchArt() {
    gs::Bitmap b(18, 16);
    b.rect(3, 4, 12, 10, 1);
    b.rect(5, 6, 8, 4, 2);
    b.rect(7, 1, 4, 4, 3);
    b.rect(4, 12, 3, 3, 4);
    b.rect(11, 12, 3, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 12);
    b.rect(4, 4, 2, 7, 1);
    b.rect(2, 1, 6, 5, 2);
    b.rect(3, 2, 4, 3, 3);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 6, 3, 4, 1);
    b.ellipse(4, 5, 1, 2, 2);
    b.set(4, 2, 3);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(32, 12);
    b.rect(0, 1, 32, 8, 1);
    for (int x = 0; x < 32; x += 8) b.rect(x, 1, 1, 8, 2);
    b.rect(0, 9, 32, 2, 3);
    return b;
}

gs::Bitmap bargeArt() {
    gs::Bitmap b(56, 18);
    b.rect(2, 2, 52, 10, 1);
    b.rect(0, 6, 56, 4, 2);
    b.rect(8, 12, 6, 4, 3);
    b.rect(42, 12, 6, 4, 3);
    b.rect(24, 4, 8, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(22, 70);
    b.rect(8, 16, 6, 52, 1);
    b.rect(6, 64, 10, 5, 2);
    b.rect(4, 6, 14, 12, 3);
    b.rect(7, 8, 8, 7, 4);
    b.rect(10, 2, 2, 5, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 2, 2, 24, 1);
    b.rect(1, 24, 6, 3, 2);
    b.rect(2, 0, 4, 4, 3);
    return b;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ && won_) return 3;
    if (over_) return 4;
    if (held_) return 2;
    return 1;
}

void Game::dump(const char* where) const {
    std::fprintf(stderr,
                 "beaconpouc %s px %.1f py %.1f vy %.1f held %d ground %d flame %.2f watch %.2f phase %d %s\n",
                 where, px_, py_, vy_, held_ ? 1 : 0, onGround_ ? 1 : 0, flame_, watch_, phase_, reason_);
}

bool Game::pierAt(float x) const {
    for (const Span& s : kPier)
        if (x >= s.a && x <= s.b) return true;
    return false;
}

int Game::bargeAt(float x) const {
    for (int i = 0; i < 2; i++)
        if (std::fabs(x - barge_[i].cx) <= 44.f) return i;
    return -1;
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 10, 3), gs::rgb4(2, 2, 3), gs::rgb4(15, 4, 3)};
    const uint16_t night[] = {0, gs::rgb4(14, 14, 15), gs::rgb4(6, 8, 12)};
    const uint16_t wood[] = {0, gs::rgb4(10, 6, 3), gs::rgb4(6, 3, 2), gs::rgb4(4, 2, 1), gs::rgb4(13, 9, 4),
                             gs::rgb4(3, 2, 1)};
    const uint16_t pouch[] = {0, gs::rgb4(9, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(14, 12, 6), gs::rgb4(5, 3, 2),
                              gs::rgb4(2, 1, 1)};
    const uint16_t hero[] = {0, gs::rgb4(2, 3, 8), gs::rgb4(3, 4, 10), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 2),
                             gs::rgb4(12, 10, 4), gs::rgb4(15, 13, 4), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 2)};
    const uint16_t lamp[] = {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 8), gs::rgb4(15, 8, 2)};
    const uint16_t sea[] = {0, gs::rgb4(1, 3, 8), gs::rgb4(2, 6, 11), gs::rgb4(4, 8, 12), gs::rgb4(8, 12, 14)};
    const uint16_t iron[] = {0, gs::rgb4(6, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 5)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 1, 1), gs::rgb4(15, 12, 8)};
    const uint16_t go[] = {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 3)};
    setPal(vdp, PAL_HUD, hud, 5);
    setPal(vdp, PAL_NIGHT, night, 3);
    setPal(vdp, PAL_WOOD, wood, 6);
    setPal(vdp, PAL_POUCH, pouch, 6);
    setPal(vdp, PAL_HERO, hero, 9);
    setPal(vdp, PAL_LAMP, lamp, 5);
    setPal(vdp, PAL_SEA, sea, 5);
    setPal(vdp, PAL_IRON, iron, 4);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_GO, go, 3);
    vdp.setFogColor(gs::rgb4(1, 2, 5));
    for (int p = 0; p < gs::NUM_PALETTES; p++) {
        if (vdp.color(p * 16 + 1) == 0) vdp.setColor(p * 16 + 1, gs::rgb4(15, 15, 14));
        vdp.setColor(p * 16 + 15, gs::rgb4(1, 1, 2));
    }

    art_.hero = gs::uploadMipped(vdp, heroArt());
    art_.pouch = gs::uploadMipped(vdp, pouchArt());
    art_.lamp = gs::uploadMipped(vdp, lampArt());
    art_.flame = gs::uploadMipped(vdp, flameArt());
    art_.plank = gs::uploadMipped(vdp, plankArt());
    art_.barge = gs::uploadMipped(vdp, bargeArt());
    art_.tower = gs::uploadMipped(vdp, towerArt());
    art_.post = gs::uploadMipped(vdp, postArt());

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
    barge_[0] = {420.f, 128.f, 0.82f, 420.f, 420.f};
    barge_[1] = {990.f, 150.f, 0.74f, 990.f, 990.f};
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    sys.setLight(40, 28, 8);
}

void Game::begin() {
    px_ = kSpawn;
    py_ = kFloor;
    vy_ = 0;
    pouchX_ = kPouch0;
    held_ = false;
    onGround_ = true;
    riding_ = false;
    rideId_ = -1;
    rideDx_ = 0;
    face_ = 1;
    phase_ = 0;
    plant_ = 0;
    flame_ = 1.f;
    watch_ = 0;
    reason_ = "";
    over_ = false;
    won_ = false;
    mode_ = Mode::Play;
    sys_->apu.tone(0, 520.f, 0.05f);
    sys_->setLight(80, 60, 16);
}

void Game::finish(bool crossed, const char* why) {
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return;
    won_ = crossed;
    over_ = true;
    reason_ = why;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    vy_ = 0;
    if (crossed) {
        sys_->rumble(0.2f, 0.55f, 180);
        sys_->setLight(40, 140, 60);
        sys_->apu.tone(0, 660.f, 0.08f);
    } else {
        sys_->rumble(0.7f, 0.2f, 160);
        sys_->setLight(140, 24, 16);
        sys_->apu.tone(0, 90.f, 0.1f);
    }
}

const char* Game::hint() const {
    if (!held_) return "TAKE THE POUCH";
    if (px_ > kWindA - 40.f && px_ < kWindB && flame_ < 0.85f) return "SHIELD THE BEACON";
    if (px_ < 520.f) return "BOARD THE BARGE";
    if (px_ < 1120.f) return "THE FAR BARGE";
    return "TO THE BEACON";
}

void Game::bot(bool& left, bool& right, bool& jump, bool& shield) {
    left = right = jump = shield = false;
    const float c0 = barge_[0].cx;
    const float c1 = barge_[1].cx;
    const float d0 = barge_[0].cx - barge_[0].prev;
    const float d1 = barge_[1].cx - barge_[1].prev;
    // Deck overlaps the pier lip: centre within half-width of the edge.
    const bool ready0 = c0 < 318.f;
    const bool ready1 = c1 < 900.f;
    const bool leave0 = c0 > 516.f && d0 >= -0.2f;
    const bool leave1 = c1 > 1092.f && d1 >= -0.2f;
    const bool on0 = rideId_ == 0;
    const bool on1 = rideId_ == 1;

    if (!held_) {
        if (px_ < pouchX_ - 6.f) right = true;
        else if (px_ > pouchX_ + 6.f) left = true;
        return;
    }
    shield = px_ > kWindA - 8.f && px_ < kWindB + 8.f;

    if (phase_ < 2) {
        if (on0) phase_ = 2;
        else if (ready0 && px_ > 230.f) phase_ = 2;
        else if (px_ < 250.f) right = true;
        return;
    }
    if (phase_ == 2) {
        if (px_ >= 590.f && pierAt(px_)) phase_ = 3;
        else if (on0 && !leave0) {
            if (px_ < c0 - 4.f) right = true;
            else if (px_ > c0 + 6.f) left = true;
        } else right = true;
        return;
    }
    if (phase_ == 3) {
        if (on1) phase_ = 4;
        else if (ready1 && px_ > 800.f) phase_ = 4;
        else if (px_ < 820.f) right = true;
        return;
    }
    if (phase_ == 4) {
        if (px_ >= 1168.f && pierAt(px_)) phase_ = 5;
        else if (on1 && !leave1) {
            if (px_ < c1 - 4.f) right = true;
            else if (px_ > c1 + 6.f) left = true;
        } else right = true;
        return;
    }
    if (px_ < kGoal + 16.f) right = true;
}

void Game::stepPlay(bool left, bool right, bool jump, bool shield) {
    for (Barge& b : barge_) {
        b.prev = b.cx;
        b.cx = b.mid + std::sin(t_ * b.w) * b.amp;
    }
    float ride = 0.f;
    if (riding_ && rideId_ >= 0) ride = barge_[rideId_].cx - barge_[rideId_].prev;

    if (left) face_ = -1;
    if (right) face_ = 1;
    float spd = (shield && onGround_) ? kShield : kWalk;
    float dir = (right ? 1.f : 0.f) - (left ? 1.f : 0.f);
    px_ += dir * spd + ride;

    bool ground = false;
    int bid = -1;
    if (vy_ >= 0.f && py_ >= kFloor - 8.f && py_ <= kFloor + 10.f) {
        if (pierAt(px_)) ground = true;
        else {
            bid = bargeAt(px_);
            if (bid >= 0) ground = true;
        }
    }
    if (ground) {
        py_ = kFloor;
        vy_ = 0;
        onGround_ = true;
        if (jump) {
            vy_ = kJump;
            onGround_ = false;
            ground = false;
            bid = -1;
            py_ -= 1.f;
        }
    } else {
        vy_ = std::min(6.2f, vy_ + kGrav);
        py_ += vy_;
        onGround_ = false;
    }
    riding_ = ground && bid >= 0;
    rideId_ = riding_ ? bid : -1;

    if (!held_ && std::fabs(px_ - pouchX_) < 16.f && onGround_ && py_ <= kFloor + 2.f) {
        held_ = true;
        sys_->apu.tone(1, 440.f, 0.06f);
    }

    const bool wind = px_ > kWindA && px_ < kWindB;
    if (wind && !shield) flame_ -= 0.013f;
    else if (!wind) flame_ = std::min(1.f, flame_ + 0.004f);
    flame_ = std::max(0.f, flame_);

    if (py_ > 214.f) {
        finish(false, held_ ? "THE POUCH WENT INTO THE WATER" : "YOU LEFT THE BEACON");
        return;
    }
    if (flame_ <= 0.f) {
        finish(false, "THE BEACON WENT OUT");
        return;
    }
    if (!held_ && px_ > 1280.f && pierAt(px_)) {
        finish(false, "THE POUCH DID NOT CROSS");
        return;
    }
    watch_ += 1.f / 60.f;
    if (watch_ > kWatch) {
        finish(false, "THE WATCH IS OVER");
        return;
    }
    if (held_ && px_ >= kGoal && pierAt(px_) && onGround_) {
        if (++plant_ > 36) finish(true, "THE POUCH CROSSED");
    } else plant_ = 0;
}

void Game::spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? foot - s.h : foot - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
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
        int band = y < 90 ? (90 - y) / 18 : 0;
        int deep = y > 150 ? (y - 150) / 10 : 0;
        vdp.lineBackdrop[y] = gs::rgb4(1, 1 + band, 3 + band + deep);
        vdp.lineFog[y] = uint8_t(y < 36 ? (36 - y) / 6 : 0);
        vdp.road[y].on = false;
    }

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool feet = true) {
        spr(m, wx - cam_, foot, h, pal, feet);
    };

    for (const Span& s : kPier) {
        for (float x = s.a + 16.f; x < s.b; x += 32.f) world(art_.plank, x, kFloor + 14.f, 16, PAL_WOOD);
    }
    const float posts[] = {48.f, 250.f, 560.f, 820.f, 1160.f};
    for (float x : posts) world(art_.post, x, kFloor, 40, PAL_IRON);
    for (const Barge& b : barge_) world(art_.barge, b.cx, kFloor + 10.f, 30, PAL_WOOD);
    for (float x : {360.f, 460.f, 940.f, 1060.f}) {
        float bob = 4.f * std::sin(t_ * 2.f + x * 0.01f);
        world(art_.plank, x, 206.f + bob, 8, PAL_SEA);
    }

    world(art_.tower, 1540.f, kFloor, 108, PAL_IRON);
    float glow = 14.f + 3.f * std::sin(t_ * 6.f);
    world(art_.flame, 1540.f, kFloor - 112.f, glow, PAL_LAMP, false);

    if (mode_ != Mode::Title) {
        float lampX = px_ + face_ * 14.f;
        world(art_.lamp, lampX, py_ - 22.f, 14, PAL_LAMP, false);
        float fh = 7.f + flame_ * 6.f;
        if (flame_ > 0.05f) world(art_.flame, lampX, py_ - 32.f, fh, PAL_LAMP, false);
        world(art_.hero, px_, py_, 52, PAL_HERO);
        if (held_) world(art_.pouch, px_ - face_ * 10.f, py_ - 18.f, 16, PAL_POUCH, false);
        else world(art_.pouch, pouchX_, kFloor - 2.f, 18, PAL_POUCH);
    }

    if (mode_ == Mode::Title) {
        text("S3 BEACON POUC", 160, 28, 1.05f, PAL_HUD);
        text("YOU HAVE THE BEACON", 160, 54, 0.8f, PAL_LAMP);
        text("CARRY THE POUCH ACROSS", 160, 76, 0.75f, PAL_GO);
        if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 108, 1.f, PAL_HUD);
        world(art_.tower, 250.f, kFloor, 90, PAL_IRON);
        world(art_.flame, 250.f, kFloor - 96.f, 12, PAL_LAMP, false);
        world(art_.hero, 92.f, kFloor, 52, PAL_HERO);
        world(art_.lamp, 106.f, kFloor - 22.f, 14, PAL_LAMP, false);
        world(art_.flame, 106.f, kFloor - 32.f, 10, PAL_LAMP, false);
        world(art_.pouch, 150.f, kFloor - 2.f, 18, PAL_POUCH);
        hudC(25, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        return;
    }

    char line[48];
    int bars = std::clamp(int(std::lround(flame_ * 8.f)), 0, 8);
    std::snprintf(line, sizeof line, "BEACON %.*s%.*s", bars, "||||||||", 8 - bars, "--------");
    hud(1, 1, line, flame_ < 0.35f ? PAL_ALERT : PAL_LAMP);
    hud(28, 1, held_ ? "POUCH" : "EMPTY", held_ ? PAL_GO : PAL_ALERT);
    if (mode_ == Mode::Play) hudC(25, hint(), PAL_HUD);
    if (mode_ == Mode::Pause) hudC(12, "PAUSED", PAL_HUD);
    if (mode_ == Mode::Won) {
        hudC(11, "THE POUCH CROSSED", PAL_GO);
        hudC(13, "UNDER THE BEACON", PAL_LAMP);
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
        cam_ = 0;
        for (Barge& b : barge_) {
            b.prev = b.cx;
            b.cx = b.mid + std::sin(t_ * b.w) * b.amp;
        }
        if (bot_ && t_ > 0.35f) begin();
        else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        draw();
        sys.apu.tone(2, 0, 0);
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            t_ = 0;
            mode_ = Mode::Title;
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

    bool left = false, right = false, jump = false, shield = false;
    if (bot_) bot(left, right, jump, shield);
    else {
        left = pad.down(gs::BTN_LEFT);
        right = pad.down(gs::BTN_RIGHT);
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
        shield = pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_X);
    }
    stepPlay(left, right, jump, shield);

    float look = std::clamp(px_ - 130.f, 0.f, 1680.f - gs::SCREEN_W);
    cam_ += (look - cam_) * 0.18f;

    const bool wind = mode_ == Mode::Play && px_ > kWindA && px_ < kWindB;
    sys.apu.tone(2, wind ? 70.f : 0.f, wind ? 0.04f : 0.f);
    if (flame_ < 0.35f && mode_ == Mode::Play) sys.apu.tone(0, 180.f + flame_ * 200.f, 0.03f);
    else if (mode_ == Mode::Play) sys.apu.tone(0, 0, 0);

    draw();
}

}  // namespace pouc
