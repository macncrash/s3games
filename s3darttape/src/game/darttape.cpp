#include "game/darttape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace darttape {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSweet = 0.09f;
constexpr float kMeterRate = 0.90f;
constexpr float kAim = 1.7f;
constexpr float kBot = 6.5f;
constexpr int kFlight = 18;
constexpr int kMaxDarts = 8;
constexpr float kPi = 3.14159265f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < 3; i++)
        if (held_[i]) s += rule::score(i);
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i > 2) return "";
    return label_[i];
}

int Game::tapeScore(int i) const {
    if (i < 0 || i > 2) return 0;
    return rule::score(i);
}

const char* Game::modeName() const {
    switch (mode_) {
    case Mode::Title: return "TITLE";
    case Mode::Aim: return "AIM";
    case Mode::Flight: return "FLIGHT";
    case Mode::Pocket: return "POCKET";
    case Mode::Judge: return "JUDGE";
    case Mode::Leave: return "LEAVE";
    case Mode::Lose: return "LOSE";
    case Mode::Pause: return "PAUSE";
    case Mode::Over: return "OVER";
    }
    return "?";
}

int Game::phase() const {
    if (!rules_ || mode_ == Mode::Lose) return 4;
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Flight) return 2;
    if (mode_ == Mode::Leave || mode_ == Mode::Over || matched()) return 3;
    return 1;
}

bool Game::flying() const { return mode_ == Mode::Flight && flightT_ >= 4 && flightT_ <= 12; }

bool Game::sweet() const { return std::fabs(meter_ - 0.5f) <= kSweet; }

bool Game::audit() {
    bool ok = true;
    auto bad = [&](const char* what, const Hit& h, float x, float y) {
        char got[16];
        hitName(h, got, int(sizeof got));
        std::fprintf(stderr, "s3darttape %s got %s kind %d n %d score %d at %.1f %.1f\n", what, got, int(h.kind),
                     h.number, h.score, x, y);
        ok = false;
    };
    if (rule::score(0) == rule::score(1) || rule::score(1) == rule::score(2) || rule::score(0) == rule::score(2)) {
        std::fprintf(stderr, "s3darttape tape scores are not distinct\n");
        ok = false;
    }
    int producers[3] = {};
    const float rads[4] = {kRadInner, kRadTriple, kRadSingle, kRadDouble};
    const int muls[4] = {1, 3, 1, 2};
    const Kind kinds[4] = {Kind::Single, Kind::Triple, Kind::Single, Kind::Double};
    for (int seg = 0; seg < 20; seg++) {
        int n = kSeg[seg];
        for (int m = 1; m <= 3; m++) {
            int sc = n * m;
            for (int i = 0; i < 3; i++)
                if (sc == rule::score(i)) producers[i]++;
        }
        for (int r = 0; r < 4; r++) {
            float x, y;
            bedPoint(n, rads[r], x, y);
            Hit h = classify(x, y);
            if (h.kind != kinds[r] || h.number != n || h.mul != muls[r] || h.score != n * muls[r]) {
                char what[24];
                std::snprintf(what, sizeof what, "bed %d x%d", n, muls[r]);
                bad(what, h, x, y);
            }
        }
    }
    for (int i = 0; i < 3; i++) {
        if (producers[i] != 2 || rule::score(i) != rule::twinScore(i)) {
            std::fprintf(stderr, "s3darttape line %d producers %d score %d twin %d\n", i, producers[i], rule::score(i),
                         rule::twinScore(i));
            ok = false;
        }
        float x, y;
        bedPoint(rule::twin(i), kRadTriple, x, y);
        Hit t = classify(x, y);
        if (t.kind != Kind::Triple || t.number != rule::twin(i) || t.score != rule::score(i)) bad("twin", t, x, y);
        if (tapeIndex(t) >= 0) {
            std::fprintf(stderr, "s3darttape twin %d counted as the tape\n", rule::twin(i));
            ok = false;
        }
    }
    Hit bull = classify(kCx, kCy);
    if (bull.kind != Kind::Bull || bull.score == 30 || bull.score == 24 || bull.score == 36) bad("bull", bull, kCx, kCy);
    float ox = kCx;
    float oy = kCy - (kBullIn + kBullOut) * 0.5f;
    Hit outer = classify(ox, oy);
    if (outer.kind != Kind::Outer || outer.score == rule::score(0)) bad("outer", outer, ox, oy);
    Hit miss = classify(kCx, kCy - (kDoubOut + 5.f));
    if (miss.kind != Kind::Miss) bad("wire", miss, kCx, kCy - (kDoubOut + 5.f));
    Hit wood = classify(kCx, kCy - (kRim + 8.f));
    if (wood.kind != Kind::Miss) bad("wood", wood, kCx, kCy - (kRim + 8.f));
    return ok;
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) toneT_ = hold;
    else tickT_ = hold;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    left_ = false;
    darts_ = 0;
    traps_ = 0;
    pinN_ = 0;
    shake_ = 0;
    flySlip_ = -1;
    wasSweet_ = false;
    clock_ = 0;
    meter_ = 0;
    meterDir_ = 1.f;
    pocketT_ = judgeT_ = leaveT_ = 0;
    reason_[0] = 0;
    last_[0] = 0;
    aimX_ = kCx;
    aimY_ = kCy;
    for (int i = 0; i < 3; i++) held_[i] = false;
    for (Pin& p : pin_) p.on = false;
    if (sys_) sys_->apu.silence();
}

void Game::newGame() {
    won_ = false;
    over_ = false;
    left_ = false;
    darts_ = 0;
    traps_ = 0;
    pinN_ = 0;
    shake_ = 0;
    flySlip_ = -1;
    reason_[0] = 0;
    last_[0] = 0;
    aimX_ = kCx;
    aimY_ = kCy;
    for (int i = 0; i < 3; i++) held_[i] = false;
    for (Pin& p : pin_) p.on = false;
    beginAim();
}

void Game::beginAim() {
    int i = nextOpen();
    bedPoint(rule::number(i), kRadDouble, targetX_, targetY_);
    meter_ = 0;
    meterDir_ = 1.f;
    wasSweet_ = false;
    pocketT_ = 0;
    judgeT_ = 0;
    flySlip_ = -1;
    mode_ = Mode::Aim;
}

int Game::nextOpen() const {
    for (int i = 0; i < 3; i++)
        if (!held_[i]) return i;
    return 0;
}

void Game::remember(float x, float y, bool board) {
    if (pinN_ >= 8) {
        for (int i = 1; i < 8; i++) pin_[i - 1] = pin_[i];
        pinN_ = 7;
    }
    pin_[pinN_].x = x;
    pin_[pinN_].y = y;
    pin_[pinN_].on = true;
    pin_[pinN_].board = board;
    pinN_++;
}

void Game::moveAim(float mx, float my) {
    float m = std::hypot(mx, my);
    if (m > 0.f) {
        float scale = kAim * (m > 1.f ? 1.f / m : 1.f);
        aimX_ += mx * scale;
        aimY_ += my * scale;
    }
    float lim = kRim + 18.f;
    aimX_ = clampf(aimX_, kCx - lim, kCx + lim);
    aimY_ = clampf(aimY_, kCy - lim, kCy + lim);
}

void Game::launch() {
    if (mode_ != Mode::Aim || !rules_ || darts_ >= kMaxDarts) return;
    float off = std::fabs(meter_ - 0.5f);
    fromX_ = kHandX;
    fromY_ = kHandY;
    if (off <= kSweet) {
        fate_ = Fate::True;
        landX_ = aimX_;
        landY_ = aimY_;
    } else if (meter_ < 0.5f) {
        fate_ = Fate::Short;
        landX_ = 262.f + float(pinN_) * 4.f;
        landY_ = 158.f + float(pinN_ % 3) * 3.f;
    } else {
        fate_ = Fate::Hot;
        float u = clampf((meter_ - 0.5f - kSweet) / (0.5f - kSweet), 0.f, 1.f);
        float dx = aimX_ - kCx;
        float dy = aimY_ - kCy;
        float d = std::hypot(dx, dy);
        if (d < 1.f) {
            dx = 0.f;
            dy = -1.f;
            d = 1.f;
        }
        float rad = kRim + 14.f + u * 10.f;
        landX_ = kCx + dx / d * rad;
        landY_ = kCy + dy / d * rad;
    }
    flightT_ = 0;
    darts_++;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.16f, 2000.f, 0.045f);
}

void Game::stick() {
    if (mode_ != Mode::Flight) return;
    Hit h = classify(landX_, landY_);
    bool onBoard = fate_ == Fate::True && h.kind != Kind::Miss;
    remember(landX_, landY_, onBoard);
    if (fate_ == Fate::Short) std::snprintf(last_, sizeof last_, "SHORT");
    else if (fate_ == Fate::Hot) std::snprintf(last_, sizeof last_, "HOT");
    else hitName(h, last_, int(sizeof last_));

    if (fate_ != Fate::True) {
        std::snprintf(reason_, sizeof reason_, "%s", fate_ == Fate::Short ? "SHORT" : "HOT");
        mode_ = Mode::Judge;
        judgeT_ = 0;
        shake_ = 6;
        if (sys_) {
            blip(1, 90.f, 0.07f, 0.2f);
            sys_->apu.noiseBurst(0.22f, 280.f, 0.08f);
            sys_->rumble(0.35f, 0.08f, 70);
            sys_->setLight(160, 40, 28);
        }
        return;
    }

    int tape = tapeIndex(h);
    if (tape >= 0) {
        if (held_[tape]) {
            std::snprintf(reason_, sizeof reason_, "ALREADY IN");
            mode_ = Mode::Judge;
            judgeT_ = 0;
            shake_ = 5;
            blip(1, 140.f, 0.06f, 0.16f);
            return;
        }
        held_[tape] = true;
        flySlip_ = tape;
        slipFromX_ = landX_;
        slipFromY_ = landY_;
        pocketT_ = 0;
        mode_ = Mode::Pocket;
        std::snprintf(reason_, sizeof reason_, "IN THE DRAWER");
        if (sys_) {
            blip(1, 523.f + float(tape) * 70.f, 0.07f, 0.22f);
            sys_->rumble(0.2f, 0.45f, 80);
            sys_->setLight(80, 170, 70);
        }
        return;
    }

    int count = countIndex(h);
    if (count >= 0) {
        traps_++;
        char name[12];
        hitName(h, name, int(sizeof name));
        std::snprintf(reason_, sizeof reason_, "%s IS NOT D%d", name, rule::number(count));
        mode_ = Mode::Judge;
        judgeT_ = 0;
        shake_ = 8;
        if (sys_) {
            blip(1, 110.f, 0.08f, 0.24f);
            sys_->apu.noiseBurst(0.28f, 220.f, 0.1f);
            sys_->rumble(0.45f, 0.12f, 90);
            sys_->setLight(170, 36, 28);
        }
        return;
    }

    if (tapeNumberHit(h)) std::snprintf(reason_, sizeof reason_, "NOT THE DOUBLE");
    else if (h.kind == Kind::Miss) std::snprintf(reason_, sizeof reason_, "MISS");
    else std::snprintf(reason_, sizeof reason_, "NOT THE TAPE");
    mode_ = Mode::Judge;
    judgeT_ = 0;
    shake_ = 5;
    blip(1, h.kind == Kind::Miss ? 80.f : 180.f, 0.06f, 0.16f);
}

void Game::beginLeave() {
    if (!matched()) return;
    mode_ = Mode::Leave;
    leaveT_ = 0;
    left_ = true;
    flySlip_ = -1;
    std::snprintf(reason_, sizeof reason_, "THE DRAWER MATCHES THE TAPE");
    if (!sys_) return;
    sys_->apu.tone(0, 523.f, 0.06f);
    sys_->apu.tone(1, 659.f, 0.05f);
    toneT_ = 0.45f;
    sys_->rumble(0.3f, 0.7f, 160);
    sys_->setLight(255, 200, 80);
}

void Game::beginLose() {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    left_ = false;
    if (!reason_[0]) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
    if (sys_) {
        blip(0, 82.f, 0.08f, 0.35f);
        sys_->setLight(150, 28, 28);
    }
}

void Game::afterPocket() {
    if (matched()) beginLeave();
    else if (darts_ >= kMaxDarts) beginLose();
    else beginAim();
}

void Game::afterJudge() {
    if (matched()) beginLeave();
    else if (darts_ >= kMaxDarts) beginLose();
    else beginAim();
}

void Game::botAim() {
    float dx = targetX_ - aimX_;
    float dy = targetY_ - aimY_;
    float d = std::hypot(dx, dy);
    if (d > 0.35f) {
        float step = std::min(kBot, d);
        aimX_ += dx / d * step;
        aimY_ += dy / d * step;
        return;
    }
    aimX_ = targetX_;
    aimY_ = targetY_;
    if (sweet()) launch();
}

void Game::humanAim(const gs::Pad& p, bool fire) {
    float mx = 0, my = 0;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    if (std::fabs(p.axisX) > 0.18f) mx = p.axisX;
    if (std::fabs(p.axisY) > 0.18f) my = -p.axisY;
    moveAim(mx, my);
    bool sw = sweet();
    if (sw && !wasSweet_) {
        Hit live = classify(aimX_, aimY_);
        int tape = tapeIndex(live);
        bool open = tape >= 0 && !held_[tape];
        blip(2, open ? 988.f : (countIndex(live) >= 0 ? 196.f : 520.f), 0.035f, 0.04f);
    }
    wasSweet_ = sw;
    if (fire) launch();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    for (int i = 0; i < 3; i++) std::snprintf(label_[i], sizeof label_[i], "D%d", rule::number(i));
    rules_ = audit();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.11f, 0.18f, 0.08f);
    toTitle();
    if (!rules_) {
        over_ = true;
        won_ = false;
        mode_ = Mode::Lose;
        std::snprintf(reason_, sizeof reason_, "RULES");
        std::fprintf(stderr, "s3darttape rules failed\n");
    }
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Flight) sys_->apu.noise(0.028f, 1700.f, true);
    else sys_->apu.noise(0, 1000);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    if (shake_ > 0) shake_--;

    if (mode_ == Mode::Title || mode_ == Mode::Aim) {
        meter_ += meterDir_ * kMeterRate * kDt;
        if (meter_ >= 1.f) {
            meter_ = 1.f;
            meterDir_ = -1.f;
        } else if (meter_ <= 0.f) {
            meter_ = 0.f;
            meterDir_ = 1.f;
        }
    }

    Mode before = mode_;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool back = p.pressed(gs::BTN_MODE);
    bool fire = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = heldMode_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        if (bot_ && clock_ > 0.40f) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Over || mode_ == Mode::Lose) {
        if (!bot_ && (start || fire)) newGame();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && mode_ == Mode::Aim && (start || back)) {
        heldMode_ = Mode::Aim;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        if (bot_) botAim();
        else humanAim(p, fire);
    }

    if (mode_ == Mode::Flight && before == Mode::Flight) {
        flightT_++;
        if (flightT_ >= kFlight) stick();
    } else if (mode_ == Mode::Pocket && before == Mode::Pocket) {
        pocketT_ += kDt;
        if (pocketT_ > 0.46f) afterPocket();
    } else if (mode_ == Mode::Judge && before == Mode::Judge) {
        judgeT_ += kDt;
        if (judgeT_ > 0.72f) afterJudge();
    } else if (mode_ == Mode::Leave && before == Mode::Leave) {
        leaveT_ += kDt;
        if (leaveT_ > 1.05f) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    }

    tickAudio(kDt);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool hflip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (m.h <= 0) return;
    float ih = std::max(6.f, h);
    float iw = ih * float(m.w) / float(m.h);
    spr(m.pick(ih), cx, cy, iw, ih, pal);
}

void Game::stamp(const gs::Image& img, float x, float y, int pal, float jx) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(x + jx));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::dartAt(float x, float y, float h) {
    sprM(art_.dart, x, y - h * 0.35f, h, PAL_DART);
    spr(art_.shadow, x + 2.f, y + 3.f, h * 0.55f, 4.f, PAL_BOARD, false, true);
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
        uint16_t c;
        if (y < 32) c = gs::rgb4(3, 1, 1);
        else if (y < 182) {
            int panel = ((y / 8) & 1);
            c = gs::rgb4(5 + panel, 2, 1);
        } else {
            c = gs::rgb4(3, 2, 1);
        }
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float jx = 0.f;
    if (shake_ > 0) jx = std::sin(float(shake_) * 1.7f) * 2.2f;

    bool leaving = mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    float px = kPlayerX + (mode_ == Mode::Leave ? leaveT_ * 46.f : 0.f);
    float py = kPlayerY + std::sin(clock_ * 2.2f) * ((mode_ == Mode::Aim || mode_ == Mode::Title) ? 0.8f : 0.f);
    float hx = kHandX + (px - kPlayerX);
    float hy = kHandY + (py - kPlayerY);

    bool showCross = mode_ == Mode::Aim || mode_ == Mode::Pause || mode_ == Mode::Title;
    float crossX = aimX_;
    float crossY = aimY_;
    if (mode_ == Mode::Title) {
        bedPoint(rule::number(0), kRadDouble, crossX, crossY);
        crossY += std::sin(clock_ * 3.f);
    }
    Hit live = classify(crossX, crossY);
    int liveTape = tapeIndex(live);
    int liveCount = countIndex(live);
    bool sweetNow = mode_ == Mode::Aim && sweet();

    if (mode_ == Mode::Title) spr(art_.title, kCx, 17.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_))
        spr(art_.paid, kCx, 17.f, float(art_.paid.w), float(art_.paid.h), PAL_TITLE);
    if (mode_ == Mode::Lose || (mode_ == Mode::Over && !won_))
        spr(art_.open, kCx, 17.f, float(art_.open.w), float(art_.open.h), PAL_ALERT);

    if (mode_ == Mode::Aim || mode_ == Mode::Pause) {
        for (int i = 0; i < 9; i++) {
            float x = kCx + float(i - 4) * 11.f;
            float pos = float(i) / 8.f;
            bool on = std::fabs(meter_ - pos) <= 0.07f;
            bool mid = i == 4;
            int pal = mid ? PAL_GOLD : PAL_INK;
            if (on && sweet()) pal = PAL_GREEN;
            else if (on) pal = PAL_ALERT;
            float s = mid ? 8.f : 6.f;
            spr(art_.pip, x, 28.f, s, s, pal);
        }
    }

    if (showCross) {
        int pal = PAL_AIM;
        if (mode_ == Mode::Title) pal = PAL_GOLD;
        else if (liveTape >= 0 && !held_[liveTape] && sweetNow) pal = PAL_GREEN;
        else if (liveTape >= 0 && !held_[liveTape]) pal = PAL_GOLD;
        else if (liveTape >= 0) pal = PAL_INK;
        else if (liveCount >= 0) pal = PAL_ALERT;
        spr(art_.cross, crossX, crossY, float(art_.cross.w), float(art_.cross.h), pal);
    }

    if (mode_ == Mode::Flight) {
        float u = clampf(float(flightT_) / float(kFlight), 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = fromX_ + (landX_ - fromX_) * e;
        float y = fromY_ + (landY_ - fromY_) * e;
        float lift = std::sin(u * kPi) * 18.f;
        float h = 26.f + (12.f - 26.f) * e;
        dartAt(x, y - lift, h);
    } else if (mode_ == Mode::Title || mode_ == Mode::Aim || mode_ == Mode::Pause) {
        dartAt(hx, hy, 16.f);
    }

    for (int i = pinN_ - 1; i >= 0; --i) {
        if (!pin_[i].on) continue;
        dartAt(pin_[i].x, pin_[i].y, pin_[i].board ? 12.f : 16.f);
    }

    if (flySlip_ >= 0 && mode_ == Mode::Pocket) {
        float u = clampf(pocketT_ / 0.46f, 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = slipFromX_ + (slotX(flySlip_) - slipFromX_) * e;
        float y = slipFromY_ + (kSlotY - slipFromY_) * e;
        spr(art_.slip[flySlip_], x, y, float(art_.slip[flySlip_].w), float(art_.slip[flySlip_].h), PAL_PAPER);
    }
    for (int i = 0; i < 3; i++) {
        if (!held_[i]) continue;
        if (mode_ == Mode::Pocket && i == flySlip_) continue;
        spr(art_.slip[i], slotX(i) + jx, kSlotY, float(art_.slip[i].w), float(art_.slip[i].h), PAL_PAPER);
    }

    for (int i = 0; i < 3; i++) {
        if (held_[i]) continue;
        float x, y;
        bedPoint(rule::number(i), kRadDouble, x, y);
        float s = 3.4f + std::sin(clock_ * 5.f + float(i)) * 0.5f;
        spr(art_.dot, x, y, s, s, PAL_GOLD);
    }

    spr(art_.dot, kDrawerL + float(kDrawerW) - 12.f + jx, kSlotY, matched() ? 7.f : 5.f, matched() ? 7.f : 5.f,
        matched() ? PAL_GREEN : PAL_ALERT);

    if (!(won_ && mode_ == Mode::Over)) {
        int step = leaving ? (int(leaveT_ * 8.f) & 1) : (int(clock_ * 2.f) & 1);
        spr(art_.shadow, px, py + 18.f, 16.f, 4.f, PAL_BOARD, false, true);
        spr(art_.player[step], px, py, float(art_.player[step].w), float(art_.player[step].h), PAL_PLAYER, leaving, false);
    }

    stamp(art_.drawer, kDrawerL, kDrawerT, PAL_WOOD, jx);
    stamp(art_.paper, kPaperX, kPaperY, PAL_PAPER);

    gs::Sprite board;
    board.img = art_.board;
    board.x = int16_t(kBoardX);
    board.y = int16_t(kBoardY);
    board.w = int16_t(art_.board.w);
    board.h = int16_t(art_.board.h);
    board.pal = PAL_BOARD;
    v.sprite(board);

    char buf[48];
    hud(1, 0, "S3 DARTTAPE", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
    hud(16, 0, buf, matched() ? PAL_GREEN : PAL_INK);
    std::snprintf(buf, sizeof buf, "LEFT %d", std::max(0, kMaxDarts - darts_));
    hud(32, 0, buf, darts_ >= kMaxDarts - 1 ? PAL_ALERT : PAL_INK);

    hud(3, 6, "TAPE", PAL_GOLD);
    for (int i = 0; i < 3; i++) {
        std::snprintf(buf, sizeof buf, "%c%-3d%2d", 'D', rule::number(i), rule::score(i));
        int pal = PAL_INK;
        if (held_[i]) pal = PAL_GREEN;
        else if (showCross && liveTape == i) pal = PAL_GOLD;
        hud(2, 8 + i * 2, buf, pal);
    }
    hud(3, 15, "OUT", PAL_ALERT);
    for (int i = 0; i < 3; i++) {
        std::snprintf(buf, sizeof buf, "%c%-3d%2d", 'T', rule::twin(i), rule::twinScore(i));
        int pal = PAL_ALERT;
        if (showCross && live.kind == Kind::Triple && live.number == rule::twin(i)) pal = PAL_GOLD;
        hud(2, 16 + i, buf, pal);
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Pause) hudC(2, "HIT THE GOLD", PAL_GOLD);

    if (mode_ == Mode::Title) {
        hudC(26, "THE COUNT IS NOT THE BED", PAL_INK);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS AIM   Z THROWS", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(26, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME   ESC TITLE", PAL_INK);
        return;
    }
    if (mode_ == Mode::Lose || (mode_ == Mode::Over && !won_)) {
        hudC(26, "DOES NOT MATCH", PAL_ALERT);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(26, "THE DRAWER MATCHES THE TAPE", PAL_GREEN);
        hudC(27, "YOU LEAVE", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Pocket) {
        hudC(26, "IN THE DRAWER", PAL_GREEN);
        if (last_[0]) hudC(27, last_, PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Judge) {
        hudC(26, reason_, PAL_ALERT);
        if (last_[0]) hudC(27, last_, PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Flight) {
        hudC(26, "IN THE AIR", PAL_GOLD);
        return;
    }

    char name[12];
    hitName(live, name, int(sizeof name));
    if (live.kind == Kind::Miss) std::snprintf(buf, sizeof buf, "MISS");
    else std::snprintf(buf, sizeof buf, "%s %d", name, live.score);
    int pal = PAL_INK;
    const char* hint = "ON THE BOARD";
    if (liveTape >= 0 && !held_[liveTape]) {
        pal = sweetNow ? PAL_GREEN : PAL_GOLD;
        hint = sweetNow ? "THROW" : "WAIT FOR GREEN";
    } else if (liveTape >= 0) {
        pal = PAL_INK;
        hint = "ALREADY IN";
    } else if (liveCount >= 0) {
        pal = PAL_ALERT;
        std::snprintf(buf, sizeof buf, "%s %d", name, live.score);
        hint = "SAME COUNT STAYS OUT";
    } else if (tapeNumberHit(live)) {
        pal = PAL_ALERT;
        hint = "NOT THE DOUBLE";
    } else if (live.kind == Kind::Miss) {
        pal = PAL_ALERT;
        hint = "ON THE BOARD";
    } else {
        pal = PAL_INK;
        hint = "NOT THE TAPE";
    }
    hudC(26, buf, pal);
    hudC(27, hint, (sweetNow && liveTape >= 0 && !held_[liveTape]) ? PAL_GREEN : PAL_AIM);
}

}  // namespace darttape
