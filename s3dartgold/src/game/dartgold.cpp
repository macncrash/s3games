#include "game/dartgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace dartgold {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 3.2f;
constexpr float kSweet = 0.2f;
constexpr int kFlight = 12;
constexpr int kStart = 501;
constexpr int kLeave = 4;  // 1, 2 and 3 have no gold double
constexpr float kTau = 6.2831853f;

void cluster(int slot, float& ox, float& oy) {
    static const float oxs[3] = {-2.4f, 2.4f, 0.f};
    static const float oys[3] = {1.7f, 1.7f, -2.2f};
    int i = slot < 0 ? 0 : (slot > 2 ? 2 : slot);
    ox = oxs[i];
    oy = oys[i];
}

}  // namespace

void Game::label(const Zone& z, char* out) const {
    if (z.kind == 'B') std::snprintf(out, 8, "BULL");
    else if (z.kind == 'O') std::snprintf(out, 8, "25");
    else if (z.kind == 'M' || z.score <= 0) std::snprintf(out, 8, "MISS");
    else std::snprintf(out, 8, "%c%d", z.kind, z.face);
}

void Game::buildBeds() {
    beds_.clear();
    auto add = [&](int sector, float rad) {
        float x, y;
        place(sector, rad, x, y);
        Zone z = zoneAt(x - kCx, y - kCy);
        Bed b;
        b.score = z.score;
        b.face = z.face;
        b.dbl = z.dbl;
        b.gold = z.gold;
        b.sector = sector;
        b.rad = rad;
        b.paint = z.paint;
        label(z, b.name);
        beds_.push_back(b);
    };
    for (int s = 0; s < 20; s++) {
        add(s, kRSing);
        add(s, kRTrip);
        add(s, kRDoub);
    }
    add(-1, 0.f);
    add(-2, kR25);
    std::sort(beds_.begin(), beds_.end(), [](const Bed& a, const Bed& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.dbl != b.dbl) return a.dbl && !b.dbl;
        return a.sector < b.sector;
    });
}

void Game::place(int sector, float rad, float& x, float& y) const {
    if (rad <= 0.f) {
        x = kCx;
        y = kCy;
        return;
    }
    float s = sector < 0 ? 0.f : float(sector);
    float ang = s * (3.14159265f / 10.f);
    x = kCx + std::sin(ang) * rad;
    y = kCy - std::cos(ang) * rad;
}

void Game::place(const Bed& b, float& x, float& y) const { place(b.sector, b.rad, x, y); }

bool Game::audit() {
    auto expect = [&](int sector, float rad, int score, bool dbl, int paint, char kind) {
        float x, y;
        place(sector, rad, x, y);
        Zone z = zoneAt(x - kCx, y - kCy);
        if (z.score != score || z.dbl != dbl || z.paint != paint || z.kind != kind) {
            std::fprintf(stderr, "s3dartgold zone s=%d r=%.1f got %c%d dbl %d paint %d\n", sector, rad, z.kind, z.score,
                         z.dbl ? 1 : 0, z.paint);
            return false;
        }
        return true;
    };
    int doubles = 0;
    bool saw4 = false, saw40 = false, saw2 = false;
    for (int s = 0; s < 20; s++) {
        int face = kSeg[s];
        bool gold = goldSector(s);
        if (!expect(s, kRSing, face, false, gold ? kPaintBlack : kPaintCream, 'S')) return false;
        if (!expect(s, kRTrip, face * 3, false, gold ? kPaintRed : kPaintGreen, 'T')) return false;
        if (gold) {
            if (!expect(s, kRDoub, face * 2, true, kPaintGold, 'D')) return false;
            doubles++;
        } else if (!expect(s, kRDoub, face, false, kPaintPale, 'C')) {
            return false;
        }
    }
    if (!expect(-1, 0.f, 50, true, kPaintGold, 'B')) return false;
    if (!expect(-2, kR25, 25, false, kPaintPale, 'O')) return false;
    doubles++;
    if (doubles != 11 || beds_.size() != 62) {
        std::fprintf(stderr, "s3dartgold beds %zu doubles %d\n", beds_.size(), doubles);
        return false;
    }
    for (const Bed& b : beds_) {
        float x, y;
        place(b, x, y);
        Zone z = zoneAt(x - kCx, y - kCy);
        char got[8];
        label(z, got);
        if (z.score != b.score || z.dbl != b.dbl || std::strcmp(got, b.name) != 0) {
            std::fprintf(stderr, "s3dartgold bed %s scored %s\n", b.name, got);
            return false;
        }
        if (b.dbl && b.score == 2) saw2 = true;
        if (b.dbl && b.score == 4) saw4 = true;
        if (b.dbl && b.score == 40) saw40 = true;
    }
    if (saw2 || !saw4 || !saw40) {
        std::fprintf(stderr, "s3dartgold double set D1 %d D2 %d D20 %d\n", saw2 ? 1 : 0, saw4 ? 1 : 0, saw40 ? 1 : 0);
        return false;
    }
    if (!proof()) return false;
    return true;
}

bool Game::proof() const {
    int remain = kStart;
    int thrown = 0;
    for (int guard = 0; guard < 40; guard++) {
        int darts = 3 - (thrown % 3);
        Bed plan[3];
        int n = 0;
        bool have = finish(remain, darts, plan, n);
        Bed shot = have ? plan[0] : bestSetup(remain);
        int next = remain - shot.score;
        if (shot.dbl && next == 0) return true;
        if (shot.score <= 0 || next < kLeave) {
            std::fprintf(stderr, "s3dartgold proof stuck %d on %s\n", remain, shot.name);
            return false;
        }
        remain = next;
        thrown++;
    }
    std::fprintf(stderr, "s3dartgold proof long %d\n", remain);
    return false;
}

bool Game::finish(int remain, int darts, Bed* plan, int& n) const {
    n = 0;
    if (darts <= 0 || remain < kLeave) return false;
    if (remain > 60 * (darts - 1) + 50) return false;
    for (const Bed& b : beds_) {
        int next = remain - b.score;
        if (next == 0) {
            if (!b.dbl) continue;
            plan[0] = b;
            n = 1;
            return true;
        }
        if (next < kLeave || darts == 1) continue;
        int sub = 0;
        if (finish(next, darts - 1, plan + 1, sub)) {
            plan[0] = b;
            n = sub + 1;
            return true;
        }
    }
    return false;
}

Game::Bed Game::bestSetup(int remain) const {
    Bed fallback = beds_.empty() ? Bed{} : beds_.back();
    for (const Bed& b : beds_) {
        int next = remain - b.score;
        if (next == 0 && b.dbl) return b;
        if (next >= kLeave) return b;
    }
    return fallback;
}

const Game::Bed& Game::target() const { return have_ && planN_ > 0 ? plan_[0] : setup_; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildBeds();
    rules_ = audit();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 1));
    sys.apu.setMaster(0.7f);
    sys.setLight(180, 140, 40);
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::clearPins() {
    visitN_ = 0;
    for (int i = 0; i < 3; i++) pin_[i].on = false;
}

void Game::begin() {
    remain_ = kStart;
    visitStart_ = kStart;
    thrown_ = 0;
    visitDart_ = 0;
    cream_ = 0;
    won_ = false;
    over_ = false;
    goldOut_ = false;
    botMiss_ = false;
    busting_ = false;
    closeVisit_ = false;
    last_[0] = 0;
    want_[0] = 0;
    out_[0] = 0;
    aimX_ = kCx;
    aimY_ = kCy;
    fanStep_ = -1;
    clearPins();
    enterAim();
}

void Game::enterAim() {
    int left = 3 - visitDart_;
    if (left < 1) left = 1;
    have_ = finish(remain_, left, plan_, planN_);
    if (!have_) {
        setup_ = bestSetup(remain_);
        planN_ = 0;
    }
    std::snprintf(want_, sizeof want_, "%s", target().name);
    mode_ = Mode::Aim;
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

float Game::pulse() const { return 0.5f * (1.f + std::sin(t_ * 4.f)); }

bool Game::sweet() const { return pulse() <= kSweet; }

bool Game::wantsThrow() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

void Game::moveAim() {
    const gs::Pad& p = sys_->pad;
    float mx = p.axisX;
    float my = -p.axisY;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    float m = std::hypot(mx, my);
    if (m > 1.f) {
        mx /= m;
        my /= m;
    }
    aimX_ += mx * kSpeed;
    aimY_ += my * kSpeed;
    float lim = kDoubOut + 18.f;
    aimX_ = std::clamp(aimX_, kCx - lim, kCx + lim);
    aimY_ = std::clamp(aimY_, kCy - lim, kCy + lim);
}

void Game::launch() {
    if (mode_ != Mode::Aim) return;
    landX_ = aimX_;
    landY_ = aimY_;
    if (!bot_) {
        float slip = 0.f;
        float p = pulse();
        if (p > kSweet) {
            float u = (p - kSweet) / (1.f - kSweet);
            slip = u * u * 30.f;
        }
        float dir = float(rnd() & 1023) * (kTau / 1024.f);
        landX_ += std::cos(dir) * slip;
        landY_ += std::sin(dir) * slip;
    }
    float ox, oy;
    cluster(visitN_, ox, oy);
    fromX_ = 160.f;
    fromY_ = float(gs::SCREEN_H) + 20.f;
    destX_ = landX_ + ox;
    destY_ = landY_ + oy;
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.14f, 2100.f, 0.04f);
}

void Game::stick() {
    Zone z = zoneAt(landX_ - kCx, landY_ - kCy);
    char name[8];
    label(z, name);
    if (bot_ && std::strcmp(name, target().name) != 0) {
        botMiss_ = true;
        won_ = false;
        over_ = true;
        std::snprintf(last_, sizeof last_, "%s", name);
        mode_ = Mode::Bust;
        return;
    }
    int slot = visitN_;
    if (slot < 3) {
        pin_[slot].x = landX_;
        pin_[slot].y = landY_;
        pin_[slot].on = true;
        std::snprintf(visit_[slot], sizeof visit_[slot], "%s", name);
        visitN_++;
    }
    thrown_++;
    visitDart_++;
    std::snprintf(last_, sizeof last_, "%s", name);
    if (z.score > 0 && !z.gold) cream_++;

    int next = remain_ - z.score;
    bool game = z.score > 0 && next == 0 && z.dbl && z.gold;
    bool bust = z.score > 0 && !game && next < kLeave;

    if (z.score == 0) {
        if (sys_) sys_->apu.noiseBurst(0.1f, 140.f, 0.07f);
        showT_ = 0.28f;
        mode_ = Mode::Show;
        if (visitDart_ >= 3) closeVisit_ = true;
        return;
    }
    if (game) {
        remain_ = 0;
        won_ = true;
        over_ = true;
        goldOut_ = true;
        std::snprintf(out_, sizeof out_, "%s", name);
        mode_ = Mode::Win;
        fanStep_ = 0;
        fanT_ = 0;
        if (sys_) {
            sys_->setLight(255, 200, 40);
            if (!sys_->headless) sys_->rumble(0.35f, 0.75f, 180);
        }
        return;
    }
    if (bust) {
        remain_ = visitStart_;
        busting_ = true;
        showT_ = 0.7f;
        mode_ = Mode::Bust;
        if (sys_) {
            sys_->apu.tone(0, 90.f, 0.1f);
            toneT_ = 0.28f;
            sys_->apu.noiseBurst(0.18f, 70.f, 0.12f);
            sys_->setLight(160, 30, 24);
        }
        return;
    }
    remain_ -= z.score;
    float freq = 392.f;
    if (z.kind == 'T') freq = 698.f;
    else if (z.dbl) freq = 880.f;
    else if (z.kind == 'C') freq = 294.f;
    else if (z.score >= 25) freq = 523.f;
    if (sys_) {
        sys_->apu.tone(0, freq, 0.07f);
        toneT_ = 0.08f;
    }
    if (visitDart_ >= 3) closeVisit_ = true;
    showT_ = 0.22f;
    mode_ = Mode::Show;
}

void Game::afterMark() {
    if (busting_ || closeVisit_ || visitDart_ >= 3) {
        busting_ = false;
        closeVisit_ = false;
        clearPins();
        visitDart_ = 0;
        visitStart_ = remain_;
    }
    enterAim();
}

void Game::botAim() {
    float x, y;
    place(target(), x, y);
    float dx = x - aimX_;
    float dy = y - aimY_;
    float d = std::hypot(dx, dy);
    if (d <= kSpeed) {
        aimX_ = x;
        aimY_ = y;
        launch();
    } else {
        aimX_ += dx / d * kSpeed;
        aimY_ += dy / d * kSpeed;
    }
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (fanStep_ < 0) return;
    fanT_ -= dt;
    if (fanT_ > 0.f) return;
    const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
    if (fanStep_ < 4) {
        sys_->apu.tone(1, notes[fanStep_], 0.1f);
        fanStep_++;
        fanT_ = 0.11f;
    } else {
        sys_->apu.tone(1, 0, 0);
        fanStep_ = -1;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) begin();
    } else if (mode_ == Mode::Aim) {
        if (!bot_) {
            if (p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
            else {
                moveAim();
                if (wantsThrow()) launch();
            }
        } else {
            botAim();
        }
    } else if (mode_ == Mode::Flight) {
        flightT_++;
        if (flightT_ >= kFlight) stick();
    } else if (mode_ == Mode::Show || mode_ == Mode::Bust) {
        showT_ -= kDt;
        if (showT_ <= 0.f || (!bot_ && (wantsThrow() || p.pressed(gs::BTN_START)))) afterMark();
    } else if (mode_ == Mode::Win) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) mode_ = Mode::Aim;
    }
    tickAudio(kDt);
    draw();
}

void Game::blit(const gs::Image& img, float cx, float cy, int w, int h, int pal) {
    if (!sys_ || w < 1 || h < 1 || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::dartAt(float x, float y, float h, int slot) {
    if (!sys_ || h < 1.f || art_.dart.h < 1) return;
    gs::Sprite s;
    int ih = std::max(1, int(std::lround(h)));
    int iw = std::max(1, int(std::lround(h * float(art_.dart.w) / float(art_.dart.h))));
    s.w = int16_t(iw);
    s.h = int16_t(ih);
    s.x = int16_t(std::lround(x - s.w * 0.5f));
    s.y = int16_t(std::lround(y - s.h * 0.5f));
    s.img = art_.dart.pick(float(ih));
    s.pal = uint8_t(PAL_DART0 + (slot % 3));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        float u = y / float(gs::SCREEN_H - 1);
        v.lineBackdrop[y] = gs::rgb4(1 + int(u * 2.f), 1, 1 + int((1.f - u) * 2.f));
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    if (mode_ == Mode::Title) blit(art_.title, 160.f, 12.f, art_.title.w, art_.title.h, PAL_TITLE);
    if (mode_ == Mode::Bust) blit(art_.bust, kCx, kCy - 8.f, art_.bust.w, art_.bust.h, PAL_BUST);
    if (mode_ == Mode::Win) blit(art_.win, kCx, kCy - 6.f, art_.win.w, art_.win.h, PAL_WIN);

    if (mode_ != Mode::Title) {
        char num[8];
        std::snprintf(num, sizeof num, "%d", remain_);
        int len = int(std::strlen(num));
        int gap = 1;
        float span = float(len * art_.digitW + (len - 1) * gap);
        float left = 160.f - span * 0.5f;
        for (int i = 0; i < len; i++) {
            int d = num[i] - '0';
            if (d < 0 || d > 9) continue;
            blit(art_.digit[d], left + art_.digitW * 0.5f, 12.f, art_.digit[d].w, art_.digit[d].h, PAL_SCORE);
            left += float(art_.digitW + gap);
        }
    }

    const bool aiming = mode_ == Mode::Aim || mode_ == Mode::Pause;
    if (aiming) {
        float rad = 4.f + pulse() * 22.f;
        int pal = sweet() ? PAL_SWEET : PAL_AIM;
        for (int i = 0; i < 10; i++) {
            float a = t_ * 1.4f + float(i) * (kTau / 10.f);
            blit(art_.dot, aimX_ + std::cos(a) * rad, aimY_ + std::sin(a) * rad, art_.dot.w, art_.dot.h, pal);
        }
        blit(art_.cross, aimX_, aimY_, art_.cross.w, art_.cross.h, pal);
    }

    if (mode_ == Mode::Flight) {
        float u = std::clamp(float(flightT_) / float(kFlight), 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = fromX_ + (destX_ - fromX_) * e;
        float y = fromY_ + (destY_ - fromY_) * e;
        dartAt(x, y, 24.f + (12.f - 24.f) * e, visitN_);
    }
    for (int i = 0; i < 3; i++) {
        if (!pin_[i].on) continue;
        float ox, oy;
        cluster(i, ox, oy);
        dartAt(pin_[i].x + ox, pin_[i].y + oy, 13.f, i);
    }
    if (mode_ == Mode::Title) {
        float x, y;
        place(0, kRDoub, x, y);
        dartAt(x, y, 14.f, 1);
    }

    gs::Sprite board;
    board.img = art_.board;
    board.x = int16_t(kBoardX);
    board.y = int16_t(kBoardY);
    board.w = int16_t(art_.board.w);
    board.h = int16_t(art_.board.h);
    board.pal = PAL_BOARD;
    v.sprite(board);

    hud(0, 0, "DART GOLD", PAL_GOLD);
    hud(33, 0, "501", PAL_INK);
    if (mode_ == Mode::Title) {
        hud(0, 4, "ARROWS", PAL_INK);
        hud(0, 5, "AIM", PAL_GOLD);
        hud(0, 8, "Z THROW", PAL_INK);
        hud(0, 9, "ON GREEN", PAL_GREEN);
        hud(33, 4, "CREAM", PAL_CREAM);
        hud(33, 5, "IS x1", PAL_INK);
        hud(33, 8, "GOLD", PAL_GOLD);
        hud(33, 9, "IS x2", PAL_GOLD);
        hudC(26, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(27, "ENTER START", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Pause) hud(0, 2, "PAUSED", PAL_GOLD);

    if (have_) {
        hud(0, 3, "OUT", PAL_GREEN);
        for (int i = 0; i < planN_ && i < 3; i++) {
            int pal = plan_[i].dbl ? PAL_GOLD : (plan_[i].name[0] == 'C' ? PAL_CREAM : PAL_INK);
            hud(0, 4 + i, plan_[i].name, pal);
        }
    } else if (!beds_.empty()) {
        hud(0, 3, "SET", PAL_GOLD);
        hud(0, 4, setup_.name, PAL_INK);
    }
    hud(0, 8, "DARTS", PAL_DIM);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d", thrown_);
    hud(0, 9, buf, PAL_INK);

    hud(33, 3, "VISIT", PAL_DIM);
    for (int i = 0; i < 3; i++) {
        const char* s = i < visitN_ ? visit_[i] : "---";
        int pal = PAL_DIM;
        if (i < visitN_) {
            if (s[0] == 'D' || std::strcmp(s, "BULL") == 0) pal = PAL_GOLD;
            else if (s[0] == 'C' || std::strcmp(s, "25") == 0) pal = PAL_CREAM;
            else if (s[0] == 'T') pal = PAL_RED;
            else pal = PAL_INK;
        }
        hud(33, 4 + i, s, pal);
    }

    if (aiming) {
        Zone here = zoneAt(aimX_ - kCx, aimY_ - kCy);
        char spot[8];
        label(here, spot);
        int pal = PAL_INK;
        if (here.dbl) pal = PAL_GOLD;
        else if (here.kind == 'C' || here.kind == 'O') pal = PAL_CREAM;
        else if (here.kind == 'T') pal = PAL_RED;
        else if (here.kind == 'M') pal = PAL_DIM;
        std::snprintf(buf, sizeof buf, "%s", spot);
        if (here.score > 0) std::snprintf(buf, sizeof buf, "%s=%d", spot, here.score);
        hud(33, 8, "SPOT", PAL_DIM);
        hud(32, 9, buf, pal);
        hudC(27, sweet() ? "THROW" : "WAIT FOR GREEN", sweet() ? PAL_GREEN : PAL_DIM);
    } else if (mode_ == Mode::Win) {
        hudC(27, "GOLD DOUBLE", PAL_GOLD);
    } else if (mode_ == Mode::Bust) {
        hudC(27, "NOT A GOLD DOUBLE", PAL_RED);
    }

    hudC(26, "GOLD x2   CREAM x1", PAL_GOLD);
}

}  // namespace dartgold
