#include "game/bell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string>

#include "console/gfx.h"
#include "version.h"

namespace shufflebell {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int SUB = 4;
constexpr float MU = 130.f;
constexpr float STOP = 3.f;
constexpr float R = 6.f;
constexpr float TABLE_L = 48.f;
constexpr float TABLE_R = 272.f;
constexpr float TABLE_FAR = 32.f;
constexpr float TABLE_NEAR = 184.f;
constexpr float LAUNCH_Y = 168.f;
constexpr float BELL_CX = 160.f;
constexpr float BELL_CY = 52.f;
constexpr float BELL_L = 144.f;
constexpr float BELL_R = 176.f;
constexpr float BELL_T = 40.f;
constexpr float BELL_B = 68.f;
constexpr float SWEET = 0.08f;

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_DISK = 2,
    PAL_WAX = 3,
    PAL_BELL = 4,
    PAL_LINE = 5,
    PAL_RAIL = 6,
    PAL_GUTTER = 7,
    PAL_AIM = 8,
    PAL_BAD = 9,
    PAL_LOGO = 10
};

void freeStep(float& x, float& y, float& vx, float& vy, float h) {
    float sp = std::hypot(vx, vy);
    if (sp < STOP) {
        vx = vy = 0;
        return;
    }
    x += vx * h;
    y += vy * h;
    float drop = MU * h;
    if (sp <= drop) {
        vx = vy = 0;
        return;
    }
    float k = (sp - drop) / sp;
    vx *= k;
    vy *= k;
    if (std::hypot(vx, vy) < STOP) vx = vy = 0;
}

float rangeOf(float speed) {
    float x = 0, y = 0, vx = 0, vy = -speed;
    const float h = DT / float(SUB);
    const int n = 60 * 8 * SUB;
    for (int i = 0; i < n; i++) {
        if (std::hypot(vx, vy) < STOP) break;
        freeStep(x, y, vx, vy, h);
    }
    return -y;
}

float speedForRange(float dist) {
    if (dist <= 1.f) return 0.f;
    float lo = 0.f, hi = 30.f;
    while (rangeOf(hi) < dist && hi < 5000.f) hi *= 2.f;
    for (int i = 0; i < 28; i++) {
        float mid = 0.5f * (lo + hi);
        if (rangeOf(mid) < dist) lo = mid;
        else hi = mid;
    }
    return hi;
}

float distForMeter(float m) {
    float minD = 18.f;
    float maxD = LAUNCH_Y - (TABLE_FAR - 10.f);
    return minD + std::clamp(m, 0.f, 1.f) * (maxD - minD);
}

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

int uploadTile(gs::VDP& vdp, gs::TileAlloc& tiles, const gs::Bitmap& b) {
    uint8_t px[64] = {};
    for (int y = 0; y < 8 && y < b.h; y++)
        for (int x = 0; x < 8 && x < b.w; x++) px[y * 8 + x] = uint8_t(b.get(x, y) & 15);
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

gs::Bitmap waxArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 3, 8, 1, 2);
    b.set(2, 6, 3);
    b.set(6, 1, 3);
    return b;
}

gs::Bitmap lineArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 2, 8, 4, 1);
    return b;
}

gs::Bitmap bellTileArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.ellipse(4, 4, 2.4f, 2.4f, 2);
    b.set(4, 4, 3);
    return b;
}

gs::Bitmap hRailArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 8, 1, 2);
    b.rect(0, 5, 8, 2, 3);
    return b;
}

gs::Bitmap vRailArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 1, 8, 2);
    b.rect(6, 0, 2, 8, 3);
    return b;
}

gs::Bitmap woodArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(2, 2, 2);
    b.set(5, 5, 2);
    b.set(6, 1, 2);
    return b;
}

gs::Bitmap gutterArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(1, 3, 2);
    b.set(4, 6, 2);
    b.set(6, 1, 2);
    return b;
}

gs::Bitmap diskArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7.2f, 7.2f, 3);
    b.ellipse(8, 8, 5.4f, 5.4f, 2);
    b.ellipse(8, 8, 2.1f, 2.1f, 4);
    b.ellipse(6.2f, 6.0f, 1.6f, 1.1f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 5.5f, 2.2f, 1);
    return b;
}

gs::Bitmap chevArt() {
    gs::Bitmap b(11, 8);
    b.line(1, 7, 5, 1, 1, 1);
    b.line(5, 1, 9, 7, 1, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(20, 18);
    b.rect(9, 0, 2, 3, 2);
    b.ellipse(10, 10, 8.2f, 6.4f, 3);
    b.ellipse(10, 11, 5.2f, 3.6f, 4);
    b.ellipse(10, 12, 2.2f, 1.4f, 1);
    b.rect(4, 15, 12, 2, 2);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(6, 8);
    b.rect(2, 0, 2, 4, 1);
    b.ellipse(3, 6, 2.1f, 1.8f, 2);
    return b;
}

bool fullyInBell(float x, float y) {
    return (x - R) >= BELL_L && (x + R) <= BELL_R && (y - R) >= BELL_T && (y + R) <= BELL_B;
}

}  // namespace

bool Game::inBell(float x, float y) const { return fullyInBell(x, y); }

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_DISK, {0, gs::rgb4(15, 15, 13), gs::rgb4(14, 12, 6), gs::rgb4(8, 6, 2), gs::rgb4(4, 8, 12)});
    setPal(vdp, PAL_WAX, {0, gs::rgb4(13, 11, 7), gs::rgb4(11, 9, 6), gs::rgb4(8, 6, 4)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 7, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_LINE, {0, gs::rgb4(15, 15, 13), gs::rgb4(7, 6, 4)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(6, 3, 1), gs::rgb4(12, 8, 3), gs::rgb4(14, 11, 5)});
    setPal(vdp, PAL_GUTTER, {0, gs::rgb4(1, 4, 3), gs::rgb4(3, 8, 5)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 5), gs::rgb4(4, 2, 1)});

    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        font_[c - 32] = t;
    }
    wax_ = uploadTile(vdp, tiles, waxArt());
    line_ = uploadTile(vdp, tiles, lineArt());
    bellTile_ = uploadTile(vdp, tiles, bellTileArt());
    railH_ = uploadTile(vdp, tiles, hRailArt());
    railV_ = uploadTile(vdp, tiles, vRailArt());
    wood_ = uploadTile(vdp, tiles, woodArt());
    gutter_ = uploadTile(vdp, tiles, gutterArt());
    disk_ = gs::uploadMipped(vdp, diskArt());
    shadow_ = gs::uploadMipped(vdp, shadowArt());
    chev_ = gs::uploadMipped(vdp, chevArt());
    bell_ = gs::uploadMipped(vdp, bellArt());
    clapper_ = gs::uploadMipped(vdp, clapperArt());
}

void Game::layTable() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.A.enabled = true;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = true;
    v.A.scroll(0, 0);
    auto fill = [&](int c0, int c1, int r0, int r1, int tile, int pal, int hf = 0, int vf = 0) {
        for (int cy = r0; cy <= r1; cy++)
            for (int cx = c0; cx <= c1; cx++) v.A.set(cx, cy, gs::entry(tile, pal, hf, vf));
    };
    fill(2, 3, 2, 23, wood_, PAL_RAIL);
    fill(36, 37, 2, 23, wood_, PAL_RAIL);
    fill(4, 4, 3, 22, railV_, PAL_RAIL, 0, 0);
    fill(35, 35, 3, 22, railV_, PAL_RAIL, 1, 0);
    fill(4, 35, 2, 2, railH_, PAL_RAIL, 0, 0);
    fill(4, 35, 23, 23, railH_, PAL_RAIL, 0, 1);
    fill(5, 34, 3, 3, gutter_, PAL_GUTTER);
    fill(5, 34, 22, 22, gutter_, PAL_GUTTER);
    fill(5, 5, 4, 21, gutter_, PAL_GUTTER);
    fill(34, 34, 4, 21, gutter_, PAL_GUTTER);
    fill(6, 33, 4, 21, wax_, PAL_WAX);
    fill(6, 33, 4, 4, line_, PAL_LINE);
    fill(6, 33, 9, 9, line_, PAL_LINE);
    fill(18, 21, 5, 8, bellTile_, PAL_BELL);
    fill(18, 21, 5, 5, line_, PAL_LINE);
    fill(18, 21, 8, 8, line_, PAL_LINE);
    fill(18, 18, 5, 8, line_, PAL_LINE);
    fill(21, 21, 5, 8, line_, PAL_LINE);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    layTable();
    sys.vdp.setFogColor(gs::rgb4(2, 2, 2));
    sys.apu.setMaster(0.65f);
    sys.apu.setEcho(0.12f, 0.22f, 0.12f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    rules_ = fullyInBell(BELL_CX, BELL_CY) && !fullyInBell(BELL_CX, LAUNCH_Y) && !fullyInBell(TABLE_L, BELL_CY);
    over_ = won_ = rung_ = false;
    dead_ = tryNo_ = 0;
    frames_ = 0;
    hasDisk_ = false;
    aimX_ = BELL_CX;
    targetY_ = BELL_CY;
    say_ = "RING THE BELL";
    sayPal_ = PAL_INK;
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::begin() {
    hasDisk_ = false;
    charging_ = false;
    meter_ = 0.f;
    meterDir_ = 1.f;
    aimX_ = BELL_CX;
    targetY_ = BELL_CY;
    dead_ = 0;
    tryNo_ = 0;
    won_ = over_ = rung_ = false;
    bellAmp_ = 0.2f;
    mode_ = Mode::Aim;
    say_ = "AIM AND SLIDE";
    sayPal_ = PAL_INK;
}

void Game::launch(float x, float targetY, bool trueShot) {
    float x0 = std::clamp(x, TABLE_L + R + 1.f, TABLE_R - R - 1.f);
    float dist = LAUNCH_Y - targetY;
    if (dist < 8.f) dist = 8.f;
    if (dist > 200.f) dist = 200.f;
    x_ = x0;
    y_ = LAUNCH_Y;
    vx_ = 0.f;
    vy_ = -speedForRange(dist);
    live_ = true;
    rest_ = false;
    hasDisk_ = true;
    trueShot_ = trueShot;
    mode_ = Mode::Roll;
    charging_ = false;
    say_ = "SLIDING";
    sayPal_ = PAL_GOLD;
    if (!sys_) return;
    sys_->apu.noiseBurst(0.22f, 1600.f, 0.05f);
    sys_->rumble(0.08f, 0.28f, 30);
}

void Game::stepDisk() {
    if (!hasDisk_ || !live_ || rest_) return;
    const float h = DT / float(SUB);
    for (int s = 0; s < SUB; s++) {
        if (vx_ == 0.f && vy_ == 0.f) {
            rest_ = true;
            break;
        }
        freeStep(x_, y_, vx_, vy_, h);
        bool off = x_ < TABLE_L || x_ > TABLE_R || y_ < TABLE_FAR || y_ > TABLE_NEAR;
        if (off) {
            if (x_ < TABLE_L) x_ = TABLE_L - R * 0.4f;
            if (x_ > TABLE_R) x_ = TABLE_R + R * 0.4f;
            if (y_ < TABLE_FAR) y_ = TABLE_FAR - R * 0.4f;
            if (y_ > TABLE_NEAR) y_ = TABLE_NEAR + R * 0.4f;
            vx_ = vy_ = 0;
            live_ = false;
            rest_ = true;
            break;
        }
        if (std::hypot(vx_, vy_) < STOP) {
            vx_ = vy_ = 0;
            rest_ = true;
        }
    }
}

void Game::ring() {
    if (rung_) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    bellAmp_ = 1.f;
    bellPh_ = 0.f;
    hold_ = 0;
    mode_ = Mode::Ring;
    say_ = "BELL";
    sayPal_ = PAL_GOLD;
    if (!sys_) return;
    sys_->apu.tone(0, 784.f, 0.16f);
    sys_->apu.tone(1, 1175.f, 0.1f);
    sys_->rumble(0.35f, 0.8f, 160);
    sys_->setLight(255, 196, 48);
}

void Game::dieTry() {
    if (rung_) return;
    dead_++;
    if (sys_) sys_->apu.tone(0, 140.f, 0.08f);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        say_ = "THIRD TRY DIED";
        sayPal_ = PAL_BAD;
        if (sys_) sys_->setLight(150, 28, 28);
        return;
    }
    mode_ = Mode::Dead;
    hold_ = 0;
    say_ = live_ ? "MISSED THE BELL" : "OFF THE TABLE";
    sayPal_ = PAL_BAD;
    if (sys_) sys_->setLight(120, 48, 28);
}

void Game::onRest() {
    if (trueShot_ && live_ && inBell(x_, y_)) {
        ring();
        return;
    }
    if (!live_) say_ = "OFF THE TABLE";
    else if (y_ > BELL_B) say_ = "SHORT OF THE BELL";
    else if (y_ < BELL_T) say_ = "PAST THE BELL";
    else say_ = "MISSED THE BELL";
    sayPal_ = PAL_BAD;
    dieTry();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (!sys_ || h < 1.f || m.h <= 0) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = font_[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 16) v.lineBackdrop[y] = gs::rgb4(1, 2, 3);
        else if (y < 192) v.lineBackdrop[y] = gs::rgb4(3, 2, 1);
        else v.lineBackdrop[y] = gs::rgb4(1, 1, 2);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float swing = std::sin(bellPh_) * 6.f * bellAmp_;
    float bx = BELL_CX + swing;
    float by = 28.f;
    spr(bell_, bx, by, 16.f, PAL_BELL);
    spr(clapper_, bx + swing * 0.4f, by + 6.f, 6.f, PAL_GOLD);

    for (int i = 0; i < 3; i++) {
        bool gone = i < dead_;
        spr(disk_, 28.f, 70.f + float(i) * 28.f, gone ? 8.f : 12.f, gone ? PAL_BAD : PAL_DISK);
    }

    if (mode_ == Mode::Title) {
        spr(disk_, 132.f, 148.f, 14.f, PAL_DISK);
        spr(shadow_, 136.f, 154.f, 6.f, PAL_DISK, true);
        spr(disk_, 188.f, 132.f, 14.f, PAL_DISK);
        spr(shadow_, 192.f, 138.f, 6.f, PAL_DISK, true);
    } else if (hasDisk_) {
        spr(shadow_, x_ + 2.f, y_ + 3.f, 6.f, PAL_DISK, true);
        spr(disk_, x_, y_, live_ ? 14.f : 11.f, PAL_DISK);
    }

    if (mode_ == Mode::Aim) {
        spr(chev_, aimX_, LAUNCH_Y + 10.f, 8.f, PAL_AIM);
        spr(disk_, aimX_, LAUNCH_Y, 14.f, PAL_DISK);
        spr(shadow_, aimX_ + 2.f, LAUNCH_Y + 4.f, 6.f, PAL_DISK, true);
    }

    if (mode_ == Mode::Title) {
        hudC(1, "S3 SHUFFLEBELL", PAL_LOGO);
        hudC(24, "RING BEFORE THE THIRD", PAL_GOLD);
        hudC(25, "A START", PAL_INK);
        hudC(26, S3_VERSION_STRING, PAL_INK);
    } else {
        hud(1, 0, "SHUFFLE", PAL_LOGO);
        hud(30, 0, "BELL", PAL_GOLD);
        char lives[16];
        std::snprintf(lives, sizeof lives, "TRY %d", std::min(3, dead_ + 1));
        hud(1, 1, lives, dead_ >= 2 ? PAL_BAD : PAL_INK);
        hudC(25, say_, sayPal_);
        if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
            hudC(26, "LEAVE THE TABLE", PAL_GOLD);
        } else if (mode_ == Mode::Aim) {
            hudC(26, charging_ ? "RELEASE TO SLIDE" : "HOLD A  LEFT RIGHT", PAL_INK);
            int cells = 1 + int(std::lround(meter_ * 10.f));
            if (cells > 11) cells = 11;
            std::string bar(size_t(cells), '#');
            bool sweet = std::fabs(meter_ - 0.5f) <= SWEET;
            hud(14, 27, bar, charging_ && sweet ? PAL_GOLD : PAL_AIM);
        } else if (mode_ == Mode::Dead) {
            hudC(26, "NEXT TRY", PAL_INK);
        } else if (mode_ == Mode::Roll) {
            hudC(26, "LET IT STOP", PAL_INK);
        } else if (mode_ == Mode::Over && !won_) {
            hudC(26, "BELL SILENT", PAL_BAD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    frames_++;
    bellPh_ += DT * (rung_ ? 14.f : 2.2f);
    if (rung_) bellAmp_ = std::max(0.22f, bellAmp_ - DT * 0.35f);
    if (rung_ && (frames_ % 16) == 0 && bellAmp_ > 0.4f) {
        sys.apu.tone(0, 784.f, 0.05f + 0.08f * bellAmp_);
        sys.apu.tone(1, 1175.f, 0.04f * bellAmp_);
    }

    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Aim) {
        bool sweet = std::fabs(meter_ - 0.5f) <= SWEET;
        if (bot_) {
            aimX_ = BELL_CX;
            if (sweet) launch(aimX_, BELL_CY, true);
            else {
                meter_ += meterDir_ * 0.02f;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                } else if (meter_ <= 0.f) {
                    meter_ = 0.f;
                    meterDir_ = 1.f;
                }
            }
        } else {
            if (pad.down(gs::BTN_LEFT)) aimX_ -= 1.4f;
            if (pad.down(gs::BTN_RIGHT)) aimX_ += 1.4f;
            aimX_ = std::clamp(aimX_, TABLE_L + R + 2.f, TABLE_R - R - 2.f);
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
                charging_ = true;
                meterDir_ = 1.f;
            }
            if (charging_) {
                meter_ += meterDir_ * 0.018f;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                } else if (meter_ <= 0.f) {
                    meter_ = 0.f;
                    meterDir_ = 1.f;
                }
                if (!pad.down(gs::BTN_A) && !pad.down(gs::BTN_C)) {
                    float dist = distForMeter(meter_);
                    float ty = LAUNCH_Y - dist;
                    bool ok = sweet && std::fabs(aimX_ - BELL_CX) < 8.f;
                    if (ok) ty = BELL_CY;
                    launch(aimX_, ty, ok);
                }
            }
            if (pad.pressed(gs::BTN_START)) {
                mode_ = Mode::Title;
                hasDisk_ = false;
            }
        }
    } else if (mode_ == Mode::Roll) {
        stepDisk();
        if (rest_) onRest();
    } else if (mode_ == Mode::Dead) {
        hold_++;
        if (hold_ > 36) {
            hasDisk_ = false;
            charging_ = false;
            meter_ = 0.f;
            meterDir_ = 1.f;
            mode_ = Mode::Aim;
            say_ = "AIM AND SLIDE";
            sayPal_ = PAL_INK;
        }
    } else if (mode_ == Mode::Ring) {
        hold_++;
        if (hold_ > 40) {
            mode_ = Mode::Leave;
            hold_ = 0;
            say_ = "LEAVE";
            sayPal_ = PAL_GOLD;
        }
    } else if (mode_ == Mode::Leave) {
        hold_++;
        if (hold_ > 50) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
            say_ = "LEFT THE TABLE";
            sayPal_ = PAL_GOLD;
        }
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) sys.quit();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) sys.quit();
        if (!bot_ && !won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }

    draw();
    if (rung_) sys.setLight(210, 160, 40);
    else if (mode_ == Mode::Aim) sys.setLight(180, 140, 40);
    else if (mode_ == Mode::Title) sys.setLight(40, 30, 24);
    else if (mode_ == Mode::Over && !won_) sys.setLight(80, 20, 20);
    else sys.setLight(20, 40, 30);
}

}  // namespace shufflebell
