#include "game/tape.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "console/gfx.h"

namespace shuffletape {

namespace {

constexpr int kTape[3] = {3, 2, 1};
constexpr float kBoardX = 118.f;
constexpr float kBoardY = 22.f;
constexpr float kBoardW = 84.f;
constexpr float kBoardH = 188.f;
constexpr float kRail = 6.f;
constexpr float kR = 6.f;
constexpr float kDrag = 0.986f;

float playL() { return kBoardX + kRail + kR; }
float playR() { return kBoardX + kBoardW - kRail - kR; }
float foulY() { return kBoardY + kRail + kR; }
float releaseY() { return kBoardY + kBoardH - 14.f; }

const char* zoneName(int z) {
    if (z == 3) return "3";
    if (z == 2) return "2";
    if (z == 1) return "1";
    return "-";
}

}  // namespace

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i > 2) return "-";
    return zoneName(kTape[i]);
}

const char* Game::drawerLabel(int i) const {
    if (i < 0 || i >= filled_) return "-";
    return zoneName(drawer_[i]);
}

void Game::tone(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 7;
}

int Game::zoneAt(float y) const {
    float top = kBoardY + 14.f;
    if (y < top) return 0;
    if (y < top + 36.f) return 3;
    if (y < top + 72.f) return 2;
    if (y < top + 108.f) return 1;
    return 0;
}

int Game::settle(Disk d) const {
    for (int i = 0; i < 900; i++) {
        if (!d.live) return 0;
        d.x += d.vx;
        d.y += d.vy;
        d.vx *= kDrag;
        d.vy *= kDrag;
        if (d.x < playL()) {
            d.x = playL();
            d.vx = std::fabs(d.vx) * 0.3f;
        } else if (d.x > playR()) {
            d.x = playR();
            d.vx = -std::fabs(d.vx) * 0.3f;
        }
        if (d.y > releaseY()) {
            d.y = releaseY();
            d.vy = -std::fabs(d.vy) * 0.2f;
        }
        if (d.y < foulY()) return -1;
        if (std::fabs(d.vx) + std::fabs(d.vy) < 0.035f) return zoneAt(d.y);
    }
    return d.live ? zoneAt(d.y) : -1;
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 2 + (y * 3) / gs::SCREEN_H;
        vdp.lineBackdrop[y] = gs::rgb4(1, g, 3);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 2, 2));

    const uint16_t ink[] = {0, gs::rgb4(15, 15, 13), gs::rgb4(14, 10, 3), gs::rgb4(3, 2, 2),
                            gs::rgb4(12, 3, 3), gs::rgb4(8, 14, 9)};
    const uint16_t wood[] = {0,
                             gs::rgb4(13, 9, 4),
                             gs::rgb4(10, 6, 3),
                             gs::rgb4(6, 4, 2),
                             gs::rgb4(3, 2, 1),
                             gs::rgb4(15, 14, 9),
                             gs::rgb4(8, 5, 2)};
    const uint16_t brass[] = {0, gs::rgb4(15, 13, 6), gs::rgb4(12, 8, 2), gs::rgb4(6, 4, 1), gs::rgb4(15, 15, 12)};
    const uint16_t paper[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 3), gs::rgb4(15, 12, 4)};
    auto loadPal = [&](int pal, const uint16_t* c, int n) {
        for (int i = 0; i < n; i++) vdp.setColor(pal * 16 + i, c[i]);
    };
    loadPal(0, ink, 6);
    loadPal(1, wood, 7);
    loadPal(2, brass, 5);
    loadPal(3, paper, 4);

    gs::Bitmap board{int(kBoardW), int(kBoardH)};
    uint32_t n = 0x51u;
    for (int y = 0; y < board.h; y++) {
        for (int x = 0; x < board.w; x++) {
            n = n * 1664525u + 1013904223u;
            bool edge = x < 5 || y < 5 || x >= board.w - 5 || y >= board.h - 5;
            int grain = int((n >> 28) & 3);
            int c = edge ? 4 : (grain == 0 ? 3 : (grain == 1 ? 2 : 1));
            board.set(x, y, c);
        }
    }
    for (int y = 0; y < 12; y++)
        for (int x = 6; x < board.w - 6; x++) board.set(x, y, 6);
    auto rule = [&](int y) {
        for (int t = 0; t < 2; t++)
            for (int x = 6; x < board.w - 6; x++) board.set(x, y + t, 5);
    };
    rule(12);
    rule(48);
    rule(84);
    rule(120);
    gs::TextStyle num;
    num.scale = 2;
    num.color = 4;
    num.shadow = 5;
    const char* labels = "321";
    const int rows[] = {22, 58, 94};
    for (int i = 0; i < 3; i++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, labels[i]), num);
        board.blit(g, 10, rows[i]);
    }
    board_ = gs::uploadImage(vdp, board);

    gs::Bitmap puck(16, 16);
    puck.ellipse(8, 8, 7.2f, 7.2f, 3);
    puck.ellipse(8, 8, 6.0f, 6.0f, 1);
    puck.ellipse(7.2f, 6.6f, 3.1f, 2.4f, 2);
    puck.ellipse(6.2f, 5.6f, 1.3f, 1.0f, 4);
    diskImg_ = gs::uploadImage(vdp, puck);

    gs::Bitmap slip(28, 16);
    slip.rect(0, 0, 28, 16, 1);
    slip.rect(1, 1, 26, 14, 2);
    slip_ = gs::uploadImage(vdp, slip);

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    for (int ch = 32; ch < 127; ch++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(ch)), st);
        glyphs_[ch - 32] = gs::uploadImage(vdp, g);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    over_ = won_ = false;
    filled_ = 0;
    drawer_[0] = drawer_[1] = drawer_[2] = 0;
    aim_ = (playL() + playR()) * 0.5f;
    power_ = 0.55f;
    disk_ = {};
    mode_ = Mode::Title;
}

void Game::blitText(const std::string& s, float x, float y, int pal) {
    float cx = x;
    for (char ch : s) {
        int i = int((unsigned char)ch) - 32;
        if (i < 0 || i > 95) {
            cx += 4;
            continue;
        }
        const gs::Image& g = glyphs_[i];
        if (ch != ' ' && g.w) {
            gs::Sprite sp;
            sp.img = g;
            sp.x = int16_t(cx);
            sp.y = int16_t(y);
            sp.w = g.w;
            sp.h = g.h;
            sp.pal = uint8_t(pal);
            sys_->vdp.sprite(sp);
        }
        cx += (g.w ? g.w : 4) + 1;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    auto spr = [&](gs::Image img, float x, float y, float w, float h, int pal) {
        if (!img.w || w < 1 || h < 1) return;
        gs::Sprite s;
        s.img = img;
        s.x = int16_t(std::lround(x));
        s.y = int16_t(std::lround(y));
        s.w = int16_t(std::lround(w));
        s.h = int16_t(std::lround(h));
        s.pal = uint8_t(pal);
        vdp.sprite(s);
    };

    blitText("SHUFFLE TAPE", 96, 4, 0);
    if (mode_ == Mode::Title) {
        blitText("MATCH THE TAPE", 88, 78, 0);
        blitText("3  2  1", 124, 96, 2);
        blitText("A SLIDES THE DISK", 76, 120, 0);
        blitText("PRESS START", 104, 148, 2);
    } else {
        blitText("TAPE", 16, 28, 2);
        blitText("DRAWER", 230, 28, 0);
        for (int i = 0; i < 3; i++) {
            float y = 48.f + i * 28.f;
            spr(slip_, 12, y, 28, 16, 3);
            blitText(tapeLabel(i), 22, y + 4, 0);
            spr(slip_, 236, y, 28, 16, 3);
            blitText(i < filled_ ? drawerLabel(i) : ".", 246, y + 4, i < filled_ ? 2 : 0);
        }
        if (mode_ == Mode::Aim) {
            int bars = std::max(1, int(power_ * 8.f));
            for (int i = 0; i < bars; i++) spr(slip_, 8, 168.f - i * 6.f, 10, 4, 3);
            blitText("AIM", 18, 180, 0);
            float ax = std::max(playL(), std::min(playR(), aim_));
            spr(diskImg_, ax - 8, releaseY() - 8, 16, 16, 2);
        }
        if (mode_ == Mode::Win) {
            blitText("DRAWER MATCHES", 80, 168, 2);
            blitText("YOU LEAVE", 108, 184, 0);
        } else if (mode_ == Mode::Slide || mode_ == Mode::Pause) {
            blitText(filled_ < 3 ? "OPEN" : "HELD", 20, 180, 0);
        }
        if (disk_.live) spr(diskImg_, disk_.x - 8, disk_.y - 8, 16, 16, 2);
    }
    spr(board_, kBoardX, kBoardY, kBoardW, kBoardH, 1);
}

void Game::stepDisk() {
    for (int s = 0; s < 4; s++) {
        if (!disk_.live) return;
        disk_.x += disk_.vx;
        disk_.y += disk_.vy;
        disk_.vx *= kDrag;
        disk_.vy *= kDrag;
        if (disk_.x < playL()) {
            disk_.x = playL();
            disk_.vx = std::fabs(disk_.vx) * 0.3f;
        } else if (disk_.x > playR()) {
            disk_.x = playR();
            disk_.vx = -std::fabs(disk_.vx) * 0.3f;
        }
        if (disk_.y > releaseY()) {
            disk_.y = releaseY();
            disk_.vy = -std::fabs(disk_.vy) * 0.2f;
        }
        if (disk_.y < foulY()) {
            disk_.live = false;
            commit(0);
            return;
        }
        if (std::fabs(disk_.vx) + std::fabs(disk_.vy) < 0.035f) {
            disk_.live = false;
            commit(zoneAt(disk_.y));
            return;
        }
    }
}

void Game::commit(int zone) {
    if (filled_ < 3 && zone == kTape[filled_] && zone > 0) {
        drawer_[filled_] = zone;
        filled_++;
        tone(280.f + filled_ * 70.f, 0.1f);
        if (filled_ == 3) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            tone(523.f, 0.14f);
            return;
        }
    } else {
        tone(90.f, 0.08f);
    }
    mode_ = Mode::Pause;
    pause_ = 18;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ <= 0) sys.apu.tone(0, 0, 0);

    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) mode_ = Mode::Aim;
        draw();
        return;
    }
    if (mode_ == Mode::Win) {
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (--pause_ <= 0) mode_ = Mode::Aim;
        draw();
        return;
    }
    if (mode_ == Mode::Slide) {
        stepDisk();
        draw();
        return;
    }

    if (bot_) {
        float lo = 0.22f, hi = 1.0f, pick = 0.5f;
        int want = kTape[filled_];
        for (int it = 0; it < 14; it++) {
            float mid = (lo + hi) * 0.5f;
            Disk d;
            d.live = true;
            d.x = aim_;
            d.y = releaseY();
            d.vy = -mid * 3.6f;
            int z = settle(d);
            pick = mid;
            if (z == want) break;
            if (z < 0 || z > want) hi = mid;
            else lo = mid;
        }
        power_ = pick;
        disk_.live = true;
        disk_.x = aim_;
        disk_.y = releaseY();
        disk_.vx = 0;
        disk_.vy = -power_ * 3.6f;
        mode_ = Mode::Slide;
        tone(180.f, 0.06f);
        draw();
        return;
    }

    if (sys.pad.down(gs::BTN_LEFT)) aim_ -= 1.4f;
    if (sys.pad.down(gs::BTN_RIGHT)) aim_ += 1.4f;
    aim_ = std::max(playL(), std::min(playR(), aim_));
    if (sys.pad.down(gs::BTN_UP)) power_ = std::min(1.f, power_ + 0.012f);
    if (sys.pad.down(gs::BTN_DOWN)) power_ = std::max(0.2f, power_ - 0.012f);
    if (sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_B)) {
        disk_.live = true;
        disk_.x = aim_;
        disk_.y = releaseY();
        disk_.vx = 0;
        disk_.vy = -power_ * 3.6f;
        mode_ = Mode::Slide;
        tone(180.f, 0.06f);
    }
    draw();
}

}  // namespace shuffletape
