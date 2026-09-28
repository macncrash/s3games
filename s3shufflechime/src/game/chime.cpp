#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "console/gfx.h"
#include "version.h"

namespace shufflechime {
namespace {

constexpr int kFpc = 6;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 96;
constexpr int kGraceFrames = kGraceSec * kFpc - 1;
constexpr int kDisks = 3;
constexpr float DT = 1.f / 60.f;
constexpr int SUB = 4;
constexpr float MU = 124.f;
constexpr float STOP = 2.5f;
constexpr float R = 6.f;
constexpr float TABLE_L = 64.f;
constexpr float TABLE_R = 248.f;
constexpr float FAR = 36.f;
constexpr float NEAR = 188.f;
constexpr float Z3T = 40.f, Z3B = 64.f;
constexpr float Z2T = 72.f, Z2B = 96.f;
constexpr float Z1T = 104.f, Z1B = 128.f;
constexpr float LAUNCH_Y = 168.f;
constexpr float MID_X = 156.f;
constexpr float SWEET_Y = (Z3T + Z3B) * 0.5f;

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_DISK = 2,
    PAL_WAX = 3,
    PAL_Z3 = 4,
    PAL_Z2 = 5,
    PAL_Z1 = 6,
    PAL_LINE = 7,
    PAL_RAIL = 8,
    PAL_GUTTER = 9,
    PAL_AIM = 10,
    PAL_BAD = 11,
    PAL_FACE = 12,
    PAL_BELL = 13,
    PAL_WIN = 14,
    PAL_DIM = 15
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool fullyInThree(float x, float y, bool dead) {
    if (dead) return false;
    float top = y - R, bot = y + R, left = x - R, right = x + R;
    return top >= Z3T && bot <= Z3B && left >= TABLE_L && right <= TABLE_R;
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

gs::Bitmap solid(int c, int d) {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, c);
    b.set(1, 2, d);
    b.set(5, 6, d);
    b.set(6, 1, d);
    return b;
}

gs::Bitmap lineArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 3, 8, 2, 1);
    return b;
}

gs::Bitmap hRail() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 8, 1, 2);
    b.rect(0, 6, 8, 2, 3);
    return b;
}

gs::Bitmap vRail() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 1, 8, 2);
    b.rect(6, 0, 2, 8, 3);
    return b;
}

gs::Bitmap diskArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7.1f, 7.1f, 3);
    b.ellipse(8, 8, 5.2f, 5.2f, 2);
    b.ellipse(8, 8, 2.0f, 2.0f, 4);
    b.ellipse(6.1f, 5.8f, 1.5f, 1.0f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 5.4f, 2.1f, 1);
    return b;
}

gs::Bitmap chevArt() {
    gs::Bitmap b(11, 8);
    b.line(1, 7, 5, 1, 1, 1);
    b.line(5, 1, 9, 7, 1, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 16);
    b.rect(8, 0, 2, 3, 2);
    b.ellipse(9, 9, 7.4f, 5.6f, 3);
    b.ellipse(9, 10, 4.4f, 3.0f, 4);
    b.rect(3, 13, 12, 2, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(1.5f, 1.5f, 1.4f, 1.4f, 1);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 18.5f, 18.5f, 2);
    b.ellipse(20, 20, 16.2f, 16.2f, 1);
    b.ellipse(20, 20, 14.6f, 14.6f, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 0.5235988f - 1.5707963f;
        float x0 = 20.f + std::cos(a) * 12.2f;
        float y0 = 20.f + std::sin(a) * 12.2f;
        float x1 = 20.f + std::cos(a) * 15.2f;
        float y1 = 20.f + std::sin(a) * 15.2f;
        b.line(x0, y0, x1, y1, i % 3 == 0 ? 4 : 2, 1.1f);
    }
    b.ellipse(20, 20, 1.3f, 1.3f, 4);
    return b;
}

}  // namespace

bool Game::inScore() const { return won_ && fullyInThree(disk_.x, disk_.y, disk_.dead); }

bool Game::sliding() const { return mode_ == Mode::Roll && disk_.live; }

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return secLeft * kFpc - sub;
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    h = t / 3600;
    m = (t / 60) % 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    split(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    split(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    split(h, m, s);
    return s;
}

void Game::faceTime(int& h, int& m, int& s) const {
    if (mode_ == Mode::Title) {
        h = 11;
        m = 58;
        s = (titleFrames_ / 4) % 60;
        return;
    }
    split(h, m, s);
}

void Game::shove(Disk& d) const {
    const float h = DT / float(SUB);
    for (int i = 0; i < SUB; i++) {
        float sp = std::hypot(d.vx, d.vy);
        if (sp < STOP) {
            d.vx = d.vy = 0;
            return;
        }
        d.x += d.vx * h;
        d.y += d.vy * h;
        if (d.x - R < TABLE_L || d.x + R > TABLE_R || d.y - R < FAR || d.y + R > NEAR) {
            d.dead = true;
            d.vx = d.vy = 0;
            return;
        }
        float drop = MU * h;
        if (sp <= drop) {
            d.vx = d.vy = 0;
            return;
        }
        float k = (sp - drop) / sp;
        d.vx *= k;
        d.vy *= k;
        if (std::hypot(d.vx, d.vy) < STOP) d.vx = d.vy = 0;
    }
}

Game::End Game::coast(float x, float speed) const {
    Disk d;
    d.x = x;
    d.y = LAUNCH_Y;
    d.vx = 0;
    d.vy = -speed;
    d.live = true;
    int frames = 0;
    while (frames < 480 && !d.dead && std::hypot(d.vx, d.vy) >= STOP) {
        frames++;
        shove(d);
    }
    End e;
    e.x = d.x;
    e.y = d.y;
    e.frames = frames;
    e.dead = d.dead || std::hypot(d.vx, d.vy) >= STOP;
    e.score = fullyInThree(d.x, d.y, e.dead);
    return e;
}

void Game::solve() {
    float lo = 20.f, hi = 400.f;
    for (int i = 0; i < 26; i++) {
        float mid = 0.5f * (lo + hi);
        End e = coast(MID_X, mid);
        if (e.dead || e.y < SWEET_Y) hi = mid;
        else lo = mid;
    }
    float best = hi;
    float bestD = 1e9f;
    for (float s = hi - 8.f; s <= hi + 8.f; s += 0.15f) {
        if (s < 1.f) continue;
        End e = coast(MID_X, s);
        if (!e.score || e.frames < 20) continue;
        float d = std::fabs(e.y - SWEET_Y);
        if (d < bestD) {
            bestD = d;
            best = s;
        }
    }
    End e = coast(MID_X, best);
    solSpeed_ = best;
    aimSpeed_ = best;
    aimX_ = MID_X;
    runFrames_ = e.frames;
    preview_ = e;
    solved_ = e.score && e.frames > 20 && e.frames < 400;
    if (!solved_) std::fprintf(stderr, "s3shufflechime no rest  y %.2f frames %d\n", e.y, e.frames);
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t ink = gs::rgb4(15, 15, 13);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_DISK, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 9, 4), gs::rgb4(6, 4, 2), gs::rgb4(3, 7, 11)});
    setPal(vdp, PAL_WAX, {0, gs::rgb4(12, 9, 5), gs::rgb4(9, 7, 4)});
    setPal(vdp, PAL_Z3, {0, gs::rgb4(14, 11, 4), gs::rgb4(10, 7, 2)});
    setPal(vdp, PAL_Z2, {0, gs::rgb4(11, 9, 6), gs::rgb4(8, 6, 4)});
    setPal(vdp, PAL_Z1, {0, gs::rgb4(9, 8, 6), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_LINE, {0, gs::rgb4(15, 15, 13), gs::rgb4(7, 5, 3)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(5, 3, 1), gs::rgb4(11, 7, 3), gs::rgb4(14, 10, 5)});
    setPal(vdp, PAL_GUTTER, {0, gs::rgb4(2, 5, 4), gs::rgb4(4, 8, 6)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), gs::rgb4(10, 8, 5), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 6, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(15, 15, 10), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(9, 8, 6), gs::rgb4(3, 2, 1)});

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
    wax_ = uploadTile(vdp, tiles, solid(1, 2));
    z3_ = uploadTile(vdp, tiles, solid(1, 2));
    z2_ = uploadTile(vdp, tiles, solid(1, 2));
    z1_ = uploadTile(vdp, tiles, solid(1, 2));
    line_ = uploadTile(vdp, tiles, lineArt());
    railH_ = uploadTile(vdp, tiles, hRail());
    railV_ = uploadTile(vdp, tiles, vRail());
    wood_ = uploadTile(vdp, tiles, solid(1, 2));
    gutter_ = uploadTile(vdp, tiles, solid(1, 2));
    diskImg_ = gs::uploadMipped(vdp, diskArt());
    shadow_ = gs::uploadMipped(vdp, shadowArt());
    chev_ = gs::uploadMipped(vdp, chevArt());
    bell_ = gs::uploadMipped(vdp, bellArt());
    dot_ = gs::uploadImage(vdp, dotArt());
    face_ = gs::uploadImage(vdp, faceArt());
}

void Game::layTable() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.A.enabled = true;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = true;
    v.A.scroll(0, 0);
    auto fill = [&](int c0, int c1, int r0, int r1, int tile, int pal) {
        for (int cy = r0; cy <= r1; cy++)
            for (int cx = c0; cx <= c1; cx++) v.A.set(cx, cy, gs::entry(tile, pal));
    };
    fill(0, 39, 0, 27, wood_, PAL_RAIL);
    fill(4, 4, 4, 23, railV_, PAL_RAIL);
    fill(31, 31, 4, 23, railV_, PAL_RAIL);
    fill(4, 31, 4, 4, railH_, PAL_RAIL);
    fill(4, 31, 23, 23, railH_, PAL_RAIL);
    fill(5, 30, 5, 5, gutter_, PAL_GUTTER);
    fill(5, 30, 22, 22, gutter_, PAL_GUTTER);
    fill(5, 5, 6, 21, gutter_, PAL_GUTTER);
    fill(30, 30, 6, 21, gutter_, PAL_GUTTER);
    fill(6, 29, 6, 21, wax_, PAL_WAX);
    fill(6, 29, 5, 7, z3_, PAL_Z3);
    fill(6, 29, 9, 11, z2_, PAL_Z2);
    fill(6, 29, 13, 15, z1_, PAL_Z1);
    fill(6, 29, 8, 8, line_, PAL_LINE);
    fill(6, 29, 12, 12, line_, PAL_LINE);
    fill(6, 29, 16, 16, line_, PAL_LINE);
    auto stamp = [&](int col, int row, char ch, int pal) {
        if (ch < 32 || ch > 127) return;
        v.A.set(col, row, gs::entry(font_[ch - 32], pal));
    };
    stamp(17, 6, '3', PAL_GOLD);
    stamp(17, 10, '2', PAL_INK);
    stamp(17, 14, '1', PAL_INK);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    layTable();
    sys.vdp.setFogColor(gs::rgb4(3, 2, 2));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.16f, 0.26f, 0.14f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    solve();
    showTitle();
    draw();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    titleFrames_ = 0;
    playFrames_ = 0;
    over_ = false;
    won_ = false;
    thrown_ = 0;
    reason_ = "";
    disk_ = {};
    wasWindow_ = false;
    strikes_ = 0;
    aimX_ = MID_X;
    aimSpeed_ = solSpeed_;
    sys_->setLight(90, 70, 30);
    sys_->apu.keyOff(0);
}

void Game::newGame() {
    over_ = false;
    won_ = false;
    thrown_ = 0;
    playFrames_ = 0;
    againFrames_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    strikes_ = 0;
    reason_ = "";
    wasWindow_ = false;
    disk_ = {};
    aimX_ = MID_X;
    aimSpeed_ = solSpeed_;
    mode_ = Mode::Aim;
    sys_->setLight(80, 90, 120);
    sys_->apu.keyOff(0);
}

void Game::launch() {
    disk_ = {};
    disk_.x = aimX_;
    disk_.y = LAUNCH_Y;
    disk_.vx = 0;
    disk_.vy = -aimSpeed_;
    disk_.live = true;
    thrown_++;
    mode_ = Mode::Roll;
    sys_->rumble(0.1f, 0.2f, 40);
    sys_->apu.noiseBurst(0.1f, 700.f, 0.04f);
    sys_->apu.tone(2, 180.f, 0.04f);
}

void Game::beginAgain(const char* why) {
    reason_ = why;
    disk_.live = false;
    disk_.vx = disk_.vy = 0;
    mode_ = Mode::Again;
    againFrames_ = 0;
    sys_->setLight(140, 50, 30);
    sys_->apu.tone(2, 120.f, 0.05f);
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    disk_.vx = disk_.vy = 0;
    disk_.live = true;
    sys_->setLight(255, 190, 60);
    sys_->rumble(0.35f, 0.7f, 160);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    disk_.live = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    sys_->setLight(130, 24, 24);
    sys_->apu.tone(2, 90.f, 0.06f);
}

const char* Game::lieName() const {
    if (disk_.dead) return disk_.y < FAR + R ? "LONG" : "WIDE";
    if (fullyInThree(disk_.x, disk_.y, false)) return "THREE";
    float top = disk_.y - R, bot = disk_.y + R;
    auto over = [&](float a, float b) { return bot > a && top < b; };
    if (over(Z3B, Z2T) || over(Z2B, Z1T) || over(Z1B, Z1B + 10.f)) return "LINE";
    if (top >= Z2T && bot <= Z2B) return "TWO";
    if (top >= Z1T && bot <= Z1B) return "ONE";
    if (bot > Z1B) return "SHORT";
    if (top < Z3T) return "LONG";
    return "MISS";
}

void Game::settle() {
    disk_.vx = disk_.vy = 0;
    bool score = fullyInThree(disk_.x, disk_.y, disk_.dead);
    if (score && onHour()) {
        beginChime();
        return;
    }
    const char* why = lieName();
    if (score && !pastHour()) why = "EARLY";
    if (pastHour()) beginFail(score ? "LATE" : why);
    else if (thrown_ >= kDisks) beginFail(score ? "EARLY" : why);
    else beginAgain(why);
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!img.w || !img.h) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hand(float ang, float len, int n, int pal) {
    const float cx = 292.f, cy = 52.f;
    for (int i = 1; i <= n; i++) {
        float t = len * float(i) / float(n);
        spr(dot_, cx + std::cos(ang) * t, cy + std::sin(ang) * t, 3.f, 3.f, pal);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    gs::Plane& h = sys_->vdp.HUD;
    for (int i = 0; s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c > 127) continue;
        h.set(col + i, row, gs::entry(font_[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = (int)std::strlen(s);
    hud(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    int hh, mm, ss;
    faceTime(hh, mm, ss);
    float secA = (float(ss) / 60.f) * 6.2831853f - 1.5707963f;
    float minA = ((float(mm) + float(ss) / 60.f) / 60.f) * 6.2831853f - 1.5707963f;
    float hrA = ((float(hh % 12) + float(mm) / 60.f) / 12.f) * 6.2831853f - 1.5707963f;

    spr(face_, 292.f, 52.f, 40.f, 40.f, PAL_FACE);
    hand(hrA, 8.f, 3, PAL_INK);
    hand(minA, 12.f, 4, PAL_GOLD);
    spr(dot_, 292.f + std::cos(secA) * 14.f, 52.f + std::sin(secA) * 14.f, 3.f, 3.f, PAL_BAD);

    float swing = std::sin(bellPh_) * (mode_ == Mode::Chime || (mode_ == Mode::Over && won_) ? 5.f : 0.6f);
    spr(bell_.pick(16.f), 292.f + swing, 92.f, 16.f, 14.f, PAL_BELL);

    for (int i = 0; i < kDisks; i++) {
        bool used = i < thrown_ && !(i == thrown_ - 1 && (mode_ == Mode::Roll || mode_ == Mode::Chime || (mode_ == Mode::Over && won_)));
        spr(diskImg_.pick(10.f), 28.f, 78.f + float(i) * 22.f, 10.f, 10.f, used ? PAL_DIM : PAL_DISK);
    }

    if (mode_ == Mode::Title) {
        spr(shadow_.pick(6.f), 120.f, 150.f, 12.f, 6.f, PAL_DISK, true);
        spr(diskImg_.pick(14.f), 116.f, 144.f, 14.f, 14.f, PAL_DISK);
        spr(shadow_.pick(6.f), 196.f, 132.f, 12.f, 6.f, PAL_DISK, true);
        spr(diskImg_.pick(14.f), 192.f, 126.f, 14.f, 14.f, PAL_DISK);
    } else if (mode_ == Mode::Aim) {
        spr(chev_.pick(8.f), aimX_, LAUNCH_Y + 12.f, 10.f, 8.f, PAL_AIM);
        spr(shadow_.pick(6.f), aimX_ + 2.f, LAUNCH_Y + 4.f, 12.f, 6.f, PAL_DISK, true);
        spr(diskImg_.pick(14.f), aimX_, LAUNCH_Y, 14.f, 14.f, PAL_DISK);
        if (preview_.frames > 0 && !preview_.dead) {
            spr(diskImg_.pick(10.f), preview_.x, preview_.y, 10.f, 10.f, PAL_AIM);
        }
    } else if (disk_.live || mode_ == Mode::Again || mode_ == Mode::Fail || mode_ == Mode::Chime || mode_ == Mode::Over) {
        if (disk_.y > 0.f) {
            spr(shadow_.pick(6.f), disk_.x + 2.f, disk_.y + 3.f, 12.f, 6.f, PAL_DISK, true);
            spr(diskImg_.pick(14.f), disk_.x, disk_.y, 14.f, 14.f, PAL_DISK);
        }
    }

    char buf[48];
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hh, mm, ss);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 SHUFFLECHIME", PAL_GOLD);
        hudC(3, "THE HOUR HAS TO CHIME", PAL_INK);
        hud(30, 1, buf, PAL_WIN);
        hudC(24, "STOP IN THE THREE AT TWELVE", PAL_INK);
        hudC(25, "A SLIDE   ARROWS AIM", PAL_DIM);
        hudC(26, S3_VERSION_STRING, PAL_DIM);
    } else {
        hud(1, 0, "SHUFFLE", PAL_GOLD);
        hud(28, 0, buf, onHour() ? PAL_WIN : PAL_INK);
        std::snprintf(buf, sizeof buf, "DISK %d", std::min(kDisks, thrown_ + (mode_ == Mode::Aim ? 1 : 0)));
        hud(1, 1, buf, PAL_DIM);
        if (mode_ == Mode::Aim) {
            int until = framesUntilHour();
            bool window = preview_.score && until <= preview_.frames && until >= preview_.frames - kGraceFrames;
            hudC(25, window ? "A NOW" : "WAIT FOR TWELVE", window ? PAL_WIN : PAL_DIM);
            hudC(26, "LEFT RIGHT   UP DOWN WEIGHT", PAL_DIM);
        } else if (mode_ == Mode::Roll) {
            hudC(25, "LET IT STOP", PAL_INK);
        } else if (mode_ == Mode::Again) {
            hudC(25, reason_, PAL_BAD);
            hudC(26, "LIFTED", PAL_DIM);
        } else if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
            hudC(25, "THE HOUR CHIMES", PAL_WIN);
            hudC(26, "LEAVE THE TABLE", PAL_GOLD);
        } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
            hudC(25, reason_[0] ? reason_ : "LATE", PAL_BAD);
            hudC(26, "THE HOUR IS GONE", PAL_BAD);
        } else if (mode_ == Mode::Pause) {
            hudC(25, "PAUSED", PAL_GOLD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += 1.f;
    bellPh_ += (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) ? 0.42f : 0.04f;

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
        else if (back) showTitle();
        draw();
        return;
    }

    if (mode_ == Mode::Title) titleFrames_++;
    if (mode_ == Mode::Aim || mode_ == Mode::Roll) playFrames_++;

    if (back && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        else if (!bot_) sys.quit();
        draw();
        return;
    }
    if (back && !bot_ && mode_ != Mode::Chime && mode_ != Mode::Over && mode_ != Mode::Fail) {
        showTitle();
        draw();
        return;
    }
    if (start && !bot_ && mode_ != Mode::Title && mode_ != Mode::Over && mode_ != Mode::Fail && mode_ != Mode::Chime &&
        mode_ != Mode::Again) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        if ((int(anim_) % 8) == 0 && strikes_ < 12 && mode_ == Mode::Chime) {
            strikes_++;
            float f = 520.f + float(strikes_) * 18.f;
            sys.apu.tone(0, f, 0.08f);
            sys.apu.tone(1, f * 1.5f, 0.04f);
        }
    }

    if (mode_ == Mode::Title) {
        if (bot_) {
            if (titleFrames_ >= 36) newGame();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Aim) {
        if (!bot_) {
            if (pad.down(gs::BTN_LEFT)) aimX_ -= 1.2f;
            if (pad.down(gs::BTN_RIGHT)) aimX_ += 1.2f;
            aimX_ = clampf(aimX_, TABLE_L + R + 2.f, TABLE_R - R - 2.f);
            if (pad.down(gs::BTN_UP)) aimSpeed_ += 0.55f;
            if (pad.down(gs::BTN_DOWN)) aimSpeed_ -= 0.55f;
            aimSpeed_ = clampf(aimSpeed_, solSpeed_ * 0.72f, solSpeed_ * 1.28f);
        } else {
            aimX_ = MID_X;
            aimSpeed_ = solSpeed_;
        }
        preview_ = coast(aimX_, aimSpeed_);
        int until = framesUntilHour();
        bool window = preview_.score && until <= preview_.frames && until >= preview_.frames - kGraceFrames;
        if (window && !wasWindow_) sys.apu.tone(2, 880.f, 0.04f);
        wasWindow_ = window;
        sys.setLight(window ? 220 : 70, window ? 170 : 90, window ? 50 : 110);
        if (pastHour()) beginFail("LATE");
        else if (bot_) {
            if (preview_.score && until == preview_.frames) launch();
        } else if (fire) launch();
    } else if (mode_ == Mode::Roll) {
        if (!disk_.dead && std::hypot(disk_.vx, disk_.vy) >= STOP) shove(disk_);
        if (disk_.dead || std::hypot(disk_.vx, disk_.vy) < STOP) settle();
    } else if (mode_ == Mode::Again) {
        if (++againFrames_ >= 40) {
            if (pastHour()) beginFail("LATE");
            else {
                mode_ = Mode::Aim;
                wasWindow_ = false;
                disk_ = {};
                aimSpeed_ = solSpeed_;
            }
        }
    } else if (mode_ == Mode::Chime) {
        if (++chimeFrames_ >= 96) {
            mode_ = Mode::Over;
            over_ = true;
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ >= 48) {
            mode_ = Mode::Over;
            over_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (start || fire)) {
            if (won_) sys.quit();
            else newGame();
        }
    }

    draw();
}

}  // namespace shufflechime
