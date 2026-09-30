#include "game/quilltape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>

#include "console/gfx.h"

namespace quilltape {

namespace {

constexpr float kWellX[kKinds] = {36.f, 88.f, 140.f, 192.f, 244.f, 292.f};
constexpr float kWellY = 168.f;
constexpr float kSlotX[kTapeN] = {78.f, 160.f, 242.f};
constexpr int kBotWell[kTapeN] = {0, 2, 4};
constexpr float kBend[kKinds] = {-6.f, 0.f, -22.f, 14.f, -30.f, 24.f};

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(p * 16 + i++, c);
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

gs::Bitmap quillBitmap() {
    gs::Bitmap b(18, 44);
    b.ellipse(7, 12, 6, 11, 3);
    b.ellipse(6, 11, 3, 7, 2);
    for (int i = 0; i < 5; i++) {
        float y = 6.f + float(i) * 2.8f;
        b.line(7, y, 2, y + 2.f, 3, 1.f);
    }
    b.line(8, 6, 11, 38, 1, 2.f);
    b.line(11, 32, 14, 42, 4, 1.4f);
    b.line(10, 36, 13, 42, 5, 1.f);
    b.set(13, 41, 5);
    return b;
}

gs::Bitmap wellBitmap() {
    gs::Bitmap b(22, 14);
    b.ellipse(11, 8, 10, 5, 1);
    b.ellipse(11, 7, 6, 3, 2);
    b.ellipse(11, 7, 2, 1, 3);
    return b;
}

gs::Bitmap blotBitmap() {
    gs::Bitmap b(4, 4);
    b.ellipse(1.6f, 1.6f, 1.6f, 1.5f, 1);
    return b;
}

gs::Bitmap slipBitmap() {
    gs::Bitmap b(52, 14);
    b.rect(0, 0, 52, 14, 1);
    b.rect(1, 1, 50, 12, 2);
    b.rect(0, 0, 52, 1, 3);
    return b;
}

gs::Image words(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += tapePay(i);
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return tapeName(i);
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return tapePay(i);
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Write) return 1;
    if (mode_ == Mode::Pick) return 2;
    return 3;
}

bool Game::audit() {
    auto bad = [&](const char* msg) {
        std::snprintf(reason_, sizeof(reason_), "%s", msg);
        rules_ = false;
        return false;
    };
    if (kTapeN != 3 || kKinds != 6) return bad("tape width");
    int seen[kTapeN];
    for (int i = 0; i < kTapeN; i++) seen[i] = -1;
    for (int i = 0; i < kKinds; i++) {
        int line = kStroke[i].line;
        if (line >= kTapeN) return bad("tape line out of range");
        if (line >= 0) {
            if (seen[line] >= 0) return bad("tape line repeated");
            seen[line] = i;
        }
    }
    for (int t = 0; t < kTapeN; t++)
        if (seen[t] < 0) return bad("tape line missing");
    if (std::strcmp(tapeName(0), "HAIR") != 0 || tapePay(0) != 1) return bad("hair drifted");
    if (std::strcmp(tapeName(1), "GOTH") != 0 || tapePay(1) != 2) return bad("goth drifted");
    if (std::strcmp(tapeName(2), "SWASH") != 0 || tapePay(2) != 3) return bad("swash drifted");
    const char* twinName[kTapeN] = {"RULE", "BOLD", "CURL"};
    for (int t = 0; t < kTapeN; t++) {
        int real = seen[t];
        int twin = -1;
        for (int i = 0; i < kKinds; i++) {
            if (i == real) continue;
            if (kStroke[i].pay == kStroke[real].pay && std::strcmp(kStroke[i].name, twinName[t]) == 0) twin = i;
        }
        if (twin < 0) return bad("twin missing");
        if (kStroke[twin].line >= 0) return bad("twin counted on the tape");
        if (kStroke[twin].pay != tapePay(t)) return bad("twin pay drifted");
        if (kStroke[real].line != t) return bad("firm stroke missed the tape");
    }
    if (tapeSum() != 6) return bad("tape does not sum to 6");
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) sum += tapeScore(i);
    if (sum != drawerScore() + tapeSum()) return bad("empty drawer");
    rules_ = true;
    reason_[0] = 0;
    return true;
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    pal(vdp, 0, {0, gs::rgb4(2, 1, 1)});
    pal(vdp, 1, {0, gs::rgb4(1, 1, 2), gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 8), gs::rgb4(8, 2, 1), gs::rgb4(15, 13, 6)});
    pal(vdp, 2, {0, gs::rgb4(3, 2, 1), gs::rgb4(10, 8, 5), gs::rgb4(14, 12, 9), gs::rgb4(6, 4, 2)});
    pal(vdp, 3, {0, gs::rgb4(9, 8, 6)});
    pal(vdp, 4, {0, gs::rgb4(4, 2, 1), gs::rgb4(12, 9, 5), gs::rgb4(15, 14, 11), gs::rgb4(7, 4, 2), gs::rgb4(11, 3, 2)});
    pal(vdp, 5, {0, gs::rgb4(5, 5, 5)});
    pal(vdp, 6, {0, gs::rgb4(1, 1, 1)});
    pal(vdp, 7, {0, gs::rgb4(2, 3, 8)});
    pal(vdp, 8, {0, gs::rgb4(10, 2, 2)});
    pal(vdp, 9, {0, gs::rgb4(2, 7, 3)});
    pal(vdp, 10, {0, gs::rgb4(8, 5, 2), gs::rgb4(14, 12, 8), gs::rgb4(6, 3, 1)});
    pal(vdp, 11, {0, gs::rgb4(12, 9, 3)});

    quill_ = gs::uploadImage(vdp, quillBitmap());
    wellImg_ = gs::uploadImage(vdp, wellBitmap());
    blot_ = gs::uploadImage(vdp, blotBitmap());
    slip_ = gs::uploadImage(vdp, slipBitmap());
    title_ = words(vdp, "S3 QUILL TAPE", 2);
    hint_ = words(vdp, "THE DRAWER HAS TO MATCH THE TAPE", 1);
    start_ = words(vdp, "PRESS START", 1);
    tapeWord_ = words(vdp, "TAPE", 1);
    drawerWord_ = words(vdp, "DRAWER", 1);
    const char* labels[kKinds] = {"HAIR", "RULE", "GOTH", "BOLD", "SWASH", "CURL"};
    for (int i = 0; i < kKinds; i++) name_[i] = words(vdp, labels[i], 1);
    done_ = words(vdp, "DRAWER MATCHES", 2);
    missed_ = words(vdp, "STILL OPEN", 2);
    ends_ = words(vdp, "THE DRAWER HAS TO MATCH THE TAPE", 1);

    gs::Bitmap desk(320, 224);
    desk.rect(0, 0, 320, 224, 1);
    desk.rect(12, 28, 296, 118, 2);
    desk.rect(18, 34, 284, 106, 3);
    uint32_t rng = 0x71A9E2u;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (rng >> 16) & 255u;
    };
    for (int i = 0; i < 90; i++) desk.set(22 + int(rnd() % 272), 38 + int(rnd() % 96), 2);
    desk.rect(16, 188, 288, 28, 2);
    for (int i = 0; i < kTapeN; i++) {
        int x = int(kSlotX[i]) - 28;
        desk.rect(x, 196, 56, 16, 3);
    }
    desk.rect(8, 8, 70, 16, 2);

    gs::TileAlloc tiles(vdp, 1);
    vdp.A.clear();
    vdp.B.clear();
    vdp.B.enabled = false;
    vdp.HUD.clear();
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, desk, 4);
    vdp.A.scroll(0, 0);
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.lineBackdrop[y] = gs::rgb4(3, 2, 1);
    vdp.setFogColor(gs::rgb4(3, 2, 1));
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    audit();
    x_ = kWellX[0];
    y_ = kWellY - 18.f;
    sys.apu.setMaster(0.8f);
}

void Game::spr(const gs::Image& im, float x, float y, int pal, bool nib) {
    if (!im.w) return;
    gs::Sprite s;
    s.img = im;
    s.w = int16_t(im.w);
    s.h = int16_t(im.h);
    s.x = int16_t(std::lround(x - im.w * 0.5f));
    s.y = int16_t(nib ? std::lround(y - im.h) : std::lround(y - im.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::drawScene() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();

    spr(tapeWord_, 36, 16, 11);
    for (int i = 0; i < kTapeN; i++) {
        int which = 0;
        for (int k = 0; k < kKinds; k++)
            if (kStroke[k].line == i) which = k;
        spr(name_[which], 78.f + float(i) * 70.f, 16, held_[i] ? 11 : 0);
    }

    for (const Dot& d : laid_) spr(blot_, d.x, d.y, d.pal);

    if (mode_ == Mode::Pick || mode_ == Mode::Title) {
        int row = std::min(strokes_, 2);
        float y = 52.f + float(row) * 26.f;
        for (int i = 0; i < 8; i++) spr(blot_, 48.f + float(i) * 22.f, y, 3);
    }

    for (int i = 0; i < kKinds; i++) {
        int pal = (kStroke[i].line >= 0) ? 2 : 5;
        if ((mode_ == Mode::Pick || mode_ == Mode::Title) && i == well_) pal = 11;
        spr(wellImg_, kWellX[i], kWellY, pal);
        spr(name_[i], kWellX[i], kWellY + 16.f, 0);
    }

    spr(drawerWord_, 28, 204, 11);
    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) continue;
        spr(slip_, kSlotX[i], 204, 10);
        int which = 0;
        for (int k = 0; k < kKinds; k++)
            if (kStroke[k].line == i) which = k;
        spr(name_[which], kSlotX[i], 204, 0);
    }

    float lift = (mode_ == Mode::Write) ? 0.f : 8.f;
    spr(quill_, x_, y_ - lift, 1, true);

    if (mode_ == Mode::Title) {
        spr(title_, 160, 48, 11);
        spr(hint_, 160, 70, 0);
        spr(start_, 160, 96, 0);
    } else if (mode_ == Mode::Leave) {
        spr(done_, 160, 64, 11);
        spr(ends_, 160, 86, 0);
    } else if (mode_ == Mode::Lose) {
        spr(missed_, 160, 64, 8);
        spr(ends_, 160, 86, 0);
    }
}

void Game::finish(bool win) {
    won_ = win && rules_ && matched() && traps_ == 0 && strokes_ == kTapeN && drawerScore() == tapeSum() &&
           board_ == tapeSum() && held_[0] && held_[1] && held_[2];
    say_ = won_ ? "DRAWER MATCHES THE TAPE" : "THE DRAWER MISSED THE TAPE";
    mode_ = won_ ? Mode::Leave : Mode::Lose;
    left_ = false;
    hold_ = 70;
    if (won_) {
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->apu.tone(1, 659.f, 0.1f);
        sys_->apu.tone(2, 784.f, 0.08f);
    } else {
        sys_->apu.tone(0, 120.f, 0.12f);
    }
}

void Game::tally() {
    const Stroke& s = kStroke[ink_];
    strokes_++;
    board_ += s.pay;
    if (s.line >= 0 && !held_[s.line]) {
        held_[s.line] = true;
        say_ = s.name;
        sys_->apu.tone(1, 440.f + float(s.line) * 80.f, 0.08f);
    } else {
        traps_++;
        say_ = "STAYS OUT";
        sys_->apu.tone(1, 140.f, 0.08f);
    }
    if (matched() && traps_ == 0) finish(true);
    else if (strokes_ >= kMaxStrokes || (matched() && traps_ > 0)) finish(false);
    else mode_ = Mode::Pick;
    wait_ = 0;
}

void Game::commit() {
    if (strokes_ >= kMaxStrokes) return;
    ink_ = well_;
    mode_ = Mode::Write;
    cursor_ = 0;
    path_.clear();
    const int n = 14;
    const int row = std::min(strokes_, 2);
    const float y0 = 52.f + float(row) * 26.f;
    const float bend = kBend[ink_];
    for (int i = 0; i <= n; i++) {
        float t = float(i) / float(n);
        float x = 40.f + (250.f - 40.f) * t;
        float y = y0 + std::sin(t * 3.14159f) * bend;
        if (ink_ == 4) y += std::sin(t * 6.28318f) * 8.f;
        path_.push_back({x, y});
    }
    x_ = kWellX[well_];
    y_ = kWellY;
    sys_->apu.noiseBurst(0.08f, 1200.f, 0.05f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (bot_ && sys.frame > 24) start = true;
        if (start) {
            mode_ = Mode::Pick;
            well_ = 0;
            wait_ = 0;
            strokes_ = traps_ = board_ = 0;
            held_[0] = held_[1] = held_[2] = false;
            won_ = left_ = over_ = false;
            laid_.clear();
            say_ = "SHORT QUILL";
            sys.apu.tone(0, 330.f, 0.08f);
        }
        drawScene();
        return;
    }

    if (mode_ == Mode::Leave || mode_ == Mode::Lose) {
        if (hold_ > 0) {
            hold_--;
            if (hold_ == 36) {
                if (mode_ == Mode::Leave) left_ = true;
                sys.apu.tone(0, 0, 0);
                sys.apu.tone(1, 0, 0);
                sys.apu.tone(2, 0, 0);
            }
            if (hold_ == 0) over_ = true;
        }
        drawScene();
        return;
    }

    if (mode_ == Mode::Pick) {
        wait_++;
        if (bot_) {
            if (strokes_ < kTapeN && wait_ == 6) well_ = kBotWell[strokes_];
            if (wait_ > 14) commit();
        } else {
            if (pad.pressed(gs::BTN_LEFT)) well_ = (well_ + kKinds - 1) % kKinds;
            if (pad.pressed(gs::BTN_RIGHT)) well_ = (well_ + 1) % kKinds;
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) commit();
        }
        float tx = kWellX[well_];
        float ty = kWellY - 6.f;
        x_ += (tx - x_) * 0.4f;
        y_ += (ty - y_) * 0.4f;
        drawScene();
        return;
    }

    if (path_.empty() || cursor_ >= int(path_.size())) {
        tally();
        drawScene();
        return;
    }
    float tx = path_[size_t(cursor_)].x;
    float ty = path_[size_t(cursor_)].y;
    float dx = tx - x_;
    float dy = ty - y_;
    float dist = std::sqrt(dx * dx + dy * dy);
    const float step = 9.f;
    if (dist <= step) {
        x_ = tx;
        y_ = ty;
        static const int kPal[kKinds] = {6, 5, 6, 7, 8, 9};
        laid_.push_back({x_, y_, kPal[ink_]});
        if (ink_ == 2 || ink_ == 3) laid_.push_back({x_ + 2.f, y_ + 1.f, kPal[ink_]});
        cursor_++;
        sys.apu.tone(0, 180.f + float(ink_) * 30.f, 0.03f);
    } else {
        x_ += dx / dist * step;
        y_ += dy / dist * step;
    }
    drawScene();
}

}  // namespace quilltape
