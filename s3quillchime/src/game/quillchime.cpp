#include "game/quillchime.h"

#include <cmath>
#include <initializer_list>

#include "console/gfx.h"

namespace quillchime {

namespace {

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(p * 16 + i++, c);
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

gs::Image bake(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

gs::Bitmap quillBitmap() {
    gs::Bitmap b(18, 44);
    b.ellipse(7, 12, 6, 11, 3);
    b.ellipse(6, 11, 3, 7, 2);
    for (int i = 0; i < 5; i++) b.line(7, 6.f + i * 3.2f, 2, 8.f + i * 3.2f, 3, 1.f);
    b.line(8, 6, 11, 40, 1, 2.f);
    b.line(11, 32, 14, 42, 4, 1.4f);
    b.set(13, 41, 5);
    return b;
}

gs::Bitmap blotBitmap() {
    gs::Bitmap b(4, 4);
    b.ellipse(1.6f, 1.6f, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap bellBitmap() {
    gs::Bitmap b(34, 28);
    b.ellipse(17, 16, 14, 10, 1);
    b.ellipse(17, 15, 8, 5, 2);
    b.rect(4, 22, 26, 3, 3);
    b.rect(15, 2, 4, 6, 3);
    return b;
}

gs::Bitmap clapperBitmap() {
    gs::Bitmap b(5, 8);
    b.line(2, 0, 2, 4, 1, 1.f);
    b.ellipse(2, 6, 2, 2, 2);
    return b;
}

gs::Bitmap wellBitmap() {
    gs::Bitmap b(22, 16);
    b.ellipse(11, 9, 10, 6, 1);
    b.ellipse(11, 8, 6, 3, 2);
    return b;
}

}  // namespace

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

bool Game::inRing() const {
    float dx = x_ - kRingX;
    float dy = y_ - kRingY;
    return dx * dx + dy * dy <= kRingR * kRingR;
}

void Game::faceTime(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::hour() const {
    int h, m, s;
    faceTime(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    faceTime(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    faceTime(h, m, s);
    return s;
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    pal(vdp, 0, {0, gs::rgb4(14, 12, 8)});
    pal(vdp, 1, {0, gs::rgb4(2, 2, 5), gs::rgb4(6, 5, 10)});
    pal(vdp, 2, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 9, 4), gs::rgb4(15, 14, 10), gs::rgb4(13, 11, 4), gs::rgb4(2, 1, 1)});
    pal(vdp, 3, {0, gs::rgb4(9, 8, 7)});
    pal(vdp, 4, {0, gs::rgb4(1, 1, 4), gs::rgb4(3, 3, 8), gs::rgb4(8, 8, 12), gs::rgb4(14, 12, 4), gs::rgb4(12, 3, 3)});
    pal(vdp, 5, {0, gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 2), gs::rgb4(10, 7, 3)});
    pal(vdp, 6, {0, gs::rgb4(15, 13, 6), gs::rgb4(12, 9, 2), gs::rgb4(7, 5, 1)});
    pal(vdp, 7, {0, gs::rgb4(4, 6, 8)});

    quill_ = gs::uploadImage(vdp, quillBitmap());
    blot_ = gs::uploadImage(vdp, blotBitmap());
    bell_ = gs::uploadImage(vdp, bellBitmap());
    clapper_ = gs::uploadImage(vdp, clapperBitmap());
    well_ = gs::uploadImage(vdp, wellBitmap());
    {
        gs::Bitmap p(3, 3);
        p.ellipse(1, 1, 1.2f, 1.2f, 1);
        pip_ = gs::uploadImage(vdp, p);
    }
    title_ = bake(vdp, "S3 QUILLCHIME", 2);
    hint_ = bake(vdp, "A SHORT QUILL  THE HOUR MUST CHIME", 1);
    start_ = bake(vdp, "PRESS START", 1);
    chime_ = bake(vdp, "THE HOUR CHIMES", 2);
    lost_ = bake(vdp, "THE HOUR IS GONE", 2);
    shortW_ = bake(vdp, "SHORT", 1);
    colon_ = bake(vdp, ":", 1);
    for (int d = 0; d < 10; d++) {
        char ch[2] = {char('0' + d), 0};
        digit_[d] = bake(vdp, ch, 1);
    }

    gs::Bitmap desk(256, 168);
    desk.rect(0, 0, 256, 168, 3);
    desk.rect(5, 5, 246, 158, 1);
    uint32_t rng = 0xC411u;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (rng >> 16) & 255;
    };
    for (int i = 0; i < 80; i++) desk.set(8 + int(rnd() % 236), 10 + int(rnd() % 146), 2);
    for (int y = 28; y < 150; y += 16) desk.rect(16, float(y), 224, 1, 4);
    desk.rect(16, 18, 2, 128, 5);
    desk.ellipse(196, 36, 28, 22, 2);
    desk.ellipse(196, 36, 22, 16, 1);

    gs::TileAlloc tiles(vdp, 1);
    vdp.A.clear();
    vdp.B.clear();
    vdp.B.enabled = false;
    vdp.HUD.clear();
    gs::bitmapToPlane(tiles, vdp.A, 4, 3, desk, 5);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int lift = y < 28 ? 1 : 0;
        vdp.lineBackdrop[y] = gs::rgb4(1 + lift, 1, 3);
        vdp.road[y].on = false;
    }
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    reason_ = "";
    face_ = "";
    inking_ = false;
    ink_ = 0;
    stroke_ = 0;
    dips_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    titleWait_ = 0;
    x_ = 72.f;
    y_ = 168.f;
    prevX_ = x_;
    prevY_ = y_;
    dots_.clear();
}

void Game::begin() {
    toTitle();
    mode_ = Mode::Write;
    sys_->apu.silence();
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    face_ = "SHORT";
    ink_ = stroke_;
    inking_ = false;
    hold_ = 0;
    mode_ = Mode::Chime;
    sys_->apu.keyOn(0, 523.f, 0.35f);
    sys_->apu.keyOn(1, 784.f, 0.22f);
    sys_->apu.noiseBurst(0.1f, 180.f, 0.05f);
    sys_->rumble(0.2f, 0.4f, 80);
}

void Game::beginGap(const char* why) {
    reason_ = why;
    inking_ = false;
    hold_ = 0;
    mode_ = Mode::Gap;
    sys_->apu.noiseBurst(0.16f, 70.f, 0.1f);
}

void Game::beginFail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    inking_ = false;
    hold_ = 0;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 90.f, 0.06f);
}

void Game::snap() {
    face_ = "LONG";
    dips_++;
    stroke_ = 0;
    dots_.clear();
    if (pastHour() || dips_ >= kDips) beginFail(pastHour() ? "HOUR" : "LONG");
    else beginGap("LONG");
}

void Game::commit() {
    inking_ = false;
    const bool shortInk = stroke_ >= kInkLo && stroke_ <= kInkHi;
    const bool ring = inRing();
    const bool hour = onHour();
    if (shortInk) face_ = "SHORT";
    else if (stroke_ < kInkLo) face_ = "DRY";
    else face_ = "LONG";
    if (shortInk && ring && hour) {
        beginChime();
        return;
    }
    dips_++;
    if (pastHour() || dips_ >= kDips) {
        beginFail(pastHour() ? "HOUR" : (shortInk ? (ring ? "EARLY" : "MISS") : face_));
        return;
    }
    if (shortInk && ring) beginGap("EARLY");
    else if (!ring) beginGap("MISS");
    else beginGap(face_);
    stroke_ = 0;
    dots_.clear();
}

void Game::bot() {
    if (mode_ == Mode::Title) {
        if (titleWait_ > 24) begin();
        return;
    }
    if (mode_ == Mode::Gap) return;
    if (mode_ != Mode::Write) return;

    float tx = kRingX;
    float ty = kRingY;
    if (!inking_) {
        if (!inRing() || !onHour()) {
            float dx = tx - x_;
            float dy = ty - y_;
            float d = std::sqrt(dx * dx + dy * dy);
            float step = 2.4f;
            if (d > step) {
                x_ += dx / d * step;
                y_ += dy / d * step;
            } else {
                x_ = tx;
                y_ = ty;
            }
        }
        if (inRing() && onHour()) {
            inking_ = true;
            stroke_ = 0;
            dots_.clear();
            orbit_ = 0.f;
        }
        return;
    }
    orbit_ += 0.55f;
    x_ = kRingX + std::cos(orbit_) * 7.f;
    y_ = kRingY + std::sin(orbit_) * 7.f;
    if (stroke_ >= (kInkLo + kInkHi) / 2) commit();
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, bool nib) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(nib ? std::lround(cy - img.h) : std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::words(const gs::Image& img, float cx, float cy) { spr(img, cx, cy, 0); }

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    int h, m, s;
    faceTime(h, m, s);

    spr(well_, 56, 176, 2);
    spr(bell_, kRingX, kRingY - 36, mode_ == Mode::Chime ? 6 : 2);
    float swing = (mode_ == Mode::Chime) ? std::sin(hold_ * 0.45f) * 6.f : 0.f;
    spr(clapper_, kRingX + swing, kRingY - 28, 6);

    for (int i = 0; i < 12; i++) {
        float a = float(i) / 12.f * 6.28318f;
        spr(pip_, kRingX + std::sin(a) * 22.f, kRingY - 36.f - std::cos(a) * 16.f, 6);
    }
    float ha = (float(h % 12) + m / 60.f) / 12.f * 6.28318f;
    float ma = m / 60.f * 6.28318f;
    spr(pip_, kRingX + std::sin(ha) * 8.f, kRingY - 36.f - std::cos(ha) * 6.f, 4);
    spr(pip_, kRingX + std::sin(ma) * 12.f, kRingY - 36.f - std::cos(ma) * 9.f, 0);

    for (int i = 0; i < 16; i++) {
        float a = float(i) / 16.f * 6.28318f;
        spr(pip_, kRingX + std::cos(a) * kRingR, kRingY + std::sin(a) * kRingR, onHour() ? 6 : 4);
    }
    for (const Dot& d : dots_) spr(blot_, d.x, d.y, 4);

    int palQ = inking_ ? 4 : 2;
    spr(quill_, x_ + 6.f, y_, palQ, true);

    if (mode_ == Mode::Title) {
        words(title_, 160, 28);
        words(hint_, 160, 52);
        words(start_, 160, 200);
    } else if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        words(chime_, 150, 24);
        words(shortW_, 150, 48);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        words(lost_, 150, 24);
    } else {
        words(hint_, 150, 16);
    }

    auto glyph = [&](int n, float x, float y) {
        if (n >= 0 && n <= 9) words(digit_[n], x, y);
    };
    float cx = 28.f;
    if (h >= 10) glyph(1, cx, 28);
    cx += 8;
    glyph(h % 10, cx, 28);
    cx += 8;
    words(colon_, cx, 28);
    cx += 8;
    glyph(m / 10, cx, 28);
    cx += 8;
    glyph(m % 10, cx, 28);
    cx += 8;
    words(colon_, cx, 28);
    cx += 8;
    glyph(s / 10, cx, 28);
    cx += 8;
    glyph(s % 10, cx, 28);

    int dip = dips_ + (mode_ == Mode::Write ? 1 : 0);
    if (dip > kDips) dip = kDips;
    words(digit_[dip], 36, 46);
    words(colon_, 46, 46);
    words(digit_[kDips], 56, 46);

    int show = inking_ ? stroke_ : ink_;
    if (show > 99) show = 99;
    words(digit_[show / 10], 36, 62);
    words(digit_[show % 10], 46, 62);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    toTitle();
    sys.apu.setMaster(0.7f);
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        titleWait_++;
        if (bot_) bot();
        else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Write) {
        playFrames_++;
        prevX_ = x_;
        prevY_ = y_;
        if (bot_) {
            bot();
        } else {
            float sp = 2.1f;
            if (pad.down(gs::BTN_LEFT)) x_ -= sp;
            if (pad.down(gs::BTN_RIGHT)) x_ += sp;
            if (pad.down(gs::BTN_UP)) y_ -= sp;
            if (pad.down(gs::BTN_DOWN)) y_ += sp;
            if (x_ < 8) x_ = 8;
            if (y_ < 70) y_ = 70;
            if (x_ > 310) x_ = 310;
            if (y_ > 210) y_ = 210;
            bool held = pad.down(gs::BTN_A);
            if (held && !inking_) {
                inking_ = true;
                stroke_ = 0;
                dots_.clear();
            }
            if (!held && inking_) commit();
        }
        if (mode_ == Mode::Write && inking_) {
            float dx = x_ - prevX_;
            float dy = y_ - prevY_;
            int step = int(std::sqrt(dx * dx + dy * dy) + 0.5f);
            if (step > 0) {
                stroke_ += step;
                if (dots_.size() < 48) dots_.push_back({x_, y_});
            }
            if (stroke_ > kInkHi) snap();
        }
        if (mode_ == Mode::Write && pastHour()) beginFail("HOUR");
    } else if (mode_ == Mode::Gap) {
        playFrames_++;
        hold_++;
        if (pastHour()) beginFail("HOUR");
        else if (hold_ > 28) {
            mode_ = Mode::Write;
            inking_ = false;
            stroke_ = 0;
        }
    } else if (mode_ == Mode::Chime || mode_ == Mode::Fail) {
        hold_++;
        if (mode_ == Mode::Chime && (hold_ % 18) == 0) sys.apu.keyOn(0, 523.f, 0.28f);
        if (hold_ > 70) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) toTitle();
    }

    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(6, 5, 1);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(4, 1, 2);
    }

    draw();
}

}  // namespace quillchime
