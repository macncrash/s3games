#include "game/mark.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string>

#include "console/gfx.h"
#include "version.h"

namespace shufflemark {
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
constexpr float MARK_L = 128.f;
constexpr float MARK_R = 192.f;
constexpr float MARK_T = 40.f;
constexpr float MARK_B = 72.f;
constexpr float MARK_CX = 160.f;
constexpr float MARK_CY = 56.f;

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_DISK = 2,
    PAL_WAX = 3,
    PAL_MARK = 4,
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

bool fullyInMark(float x, float y) {
    return (x - R) >= MARK_L && (x + R) <= MARK_R && (y - R) >= MARK_T && (y + R) <= MARK_B;
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

gs::Bitmap markArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(1, 1, 6, 6, 2);
    b.set(3, 3, 3);
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

}  // namespace

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_DISK, {0, gs::rgb4(15, 15, 13), gs::rgb4(14, 12, 6), gs::rgb4(8, 6, 2), gs::rgb4(4, 8, 12)});
    setPal(vdp, PAL_WAX, {0, gs::rgb4(13, 11, 7), gs::rgb4(11, 9, 6), gs::rgb4(8, 6, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(12, 8, 3), gs::rgb4(15, 12, 4), gs::rgb4(8, 4, 1)});
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
    mark_ = uploadTile(vdp, tiles, markArt());
    railH_ = uploadTile(vdp, tiles, hRailArt());
    railV_ = uploadTile(vdp, tiles, vRailArt());
    wood_ = uploadTile(vdp, tiles, woodArt());
    gutter_ = uploadTile(vdp, tiles, gutterArt());
    disk_ = gs::uploadMipped(vdp, diskArt());
    shadow_ = gs::uploadMipped(vdp, shadowArt());
    chev_ = gs::uploadMipped(vdp, chevArt());
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
    // Painted mark: cells 16..23 (x 128..192) and rows 5..8 (y 40..72).
    fill(16, 23, 5, 8, mark_, PAL_MARK);
    fill(16, 23, 5, 5, line_, PAL_LINE);
    fill(16, 23, 8, 8, line_, PAL_LINE);
    fill(16, 16, 5, 8, line_, PAL_LINE);
    fill(23, 23, 5, 8, line_, PAL_LINE);
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
    over_ = won_ = finished_ = onMark_ = false;
    throws_ = 0;
    frames_ = 0;
    hasDisk_ = false;
    aimX_ = MARK_CX;
    targetY_ = MARK_CY;
    say_ = "SLIDE ONTO THE MARK";
    sayPal_ = PAL_INK;
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::begin() {
    hasDisk_ = false;
    charging_ = false;
    meter_ = 0.35f;
    meterDir_ = 1.f;
    aimX_ = MARK_CX;
    targetY_ = MARK_CY;
    mode_ = Mode::Aim;
    say_ = "AIM AND SLIDE";
    sayPal_ = PAL_INK;
}

void Game::launch(float x, float targetY) {
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
    throws_++;
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

void Game::onRest() {
    if (live_ && fullyInMark(x_, y_)) {
        onMark_ = true;
        finished_ = true;
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        say_ = "FINISHED MARK";
        sayPal_ = PAL_GOLD;
        if (sys_) {
            sys_->apu.tone(0, 523.f, 0.22f);
            sys_->apu.tone(1, 659.f, 0.16f);
            sys_->rumble(0.2f, 0.45f, 90);
            sys_->setLight(210, 160, 40);
        }
        return;
    }
    onMark_ = false;
    if (!live_) {
        say_ = "OFF THE TABLE";
        sayPal_ = PAL_BAD;
    } else if (y_ > MARK_B) {
        say_ = "SHORT OF THE MARK";
        sayPal_ = PAL_BAD;
    } else if (y_ < MARK_T) {
        say_ = "PAST THE MARK";
        sayPal_ = PAL_BAD;
    } else {
        say_ = "ON THE LINE";
        sayPal_ = PAL_BAD;
    }
    if (bot_) {
        float next = MARK_CY;
        if (live_) next = MARK_CY - (y_ - MARK_CY);
        if (next < TABLE_FAR + R + 2.f) next = MARK_CY - 4.f;
        if (next > LAUNCH_Y - 20.f) next = MARK_CY + 4.f;
        targetY_ = next;
        aimX_ = MARK_CX;
        launch(aimX_, targetY_);
        return;
    }
    mode_ = Mode::Miss;
    hold_ = 0;
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
        hudC(1, "S3 SHUFFLEMARK", PAL_LOGO);
        hudC(24, "FINISH THE MARK", PAL_GOLD);
        hudC(25, "A START", PAL_INK);
        hudC(26, S3_VERSION_STRING, PAL_INK);
    } else {
        hud(1, 0, "SHUFFLE", PAL_LOGO);
        hud(28, 0, "MARK", PAL_GOLD);
        hudC(25, say_, sayPal_);
        if (mode_ == Mode::Win) {
            hudC(26, "LEAVE THE TABLE", PAL_GOLD);
        } else if (mode_ == Mode::Aim) {
            hudC(26, charging_ ? "RELEASE TO SLIDE" : "HOLD A  LEFT RIGHT", PAL_INK);
            int cells = 1 + int(std::lround(meter_ * 10.f));
            if (cells > 11) cells = 11;
            std::string bar(size_t(cells), '#');
            hud(14, 27, bar, charging_ ? PAL_GOLD : PAL_AIM);
        } else if (mode_ == Mode::Miss) {
            hudC(26, "A TO SLIDE AGAIN", PAL_INK);
        } else if (mode_ == Mode::Roll) {
            hudC(26, "LET IT STOP", PAL_INK);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    frames_++;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            launch(aimX_, targetY_);
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
                    launch(aimX_, LAUNCH_Y - dist);
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
    } else if (mode_ == Mode::Miss) {
        hold_++;
        if (bot_ || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_START) || hold_ > 40) {
            hasDisk_ = false;
            mode_ = Mode::Aim;
            say_ = "AIM AND SLIDE";
            sayPal_ = PAL_INK;
        }
    } else if (mode_ == Mode::Win) {
        if ((frames_ % 40) == 0) sys.apu.tone(0, 523.f, 0.06f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) sys.quit();
    }

    draw();
    if (mode_ == Mode::Win) sys.setLight(210, 160, 40);
    else if (mode_ == Mode::Aim) sys.setLight(180, 140, 40);
    else if (mode_ == Mode::Title) sys.setLight(40, 30, 24);
    else sys.setLight(20, 40, 30);
}

}  // namespace shufflemark
