#include "game/mower.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace mower {
namespace {

constexpr int kFx = 16;
constexpr int kFy = 32;
constexpr int kTx = 2;
constexpr int kTy = 4;
constexpr float kXLo = 20.f;
constexpr float kXHi = 300.f;
constexpr float kYLo = 36.f;
constexpr float kYHi = 204.f;
constexpr float kCut = 13.6f;
constexpr float kMax = 2.05f;
constexpr int kRain = 100 * 60;
constexpr float kTau = 6.2831853f;

float wrap(float a) {
    while (a > 3.14159265f) a -= kTau;
    while (a < -3.14159265f) a += kTau;
    return a;
}

}  // namespace

int Game::rainSeconds() const {
    int left = rainFrames_ < 0 ? 0 : rainFrames_;
    return (left + 59) / 60;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float(rng_ >> 8) / 16777216.f;
}

float Game::bandY(int i) const { return float(kFy + i * 16 + 8); }

void Game::tune() {
    gs::FMPatch p;
    p.alg = 7;
    p.vol = 0.2f;
    p.fb = 0.08f;
    for (int i = 0; i < 4; i++) {
        p.op[i].mul = i == 0 ? 1.f : 2.4f;
        p.op[i].level = i == 0 ? 1.f : (i == 1 ? 0.3f : 0.f);
        p.op[i].ar = 0.004f;
        p.op[i].dr = 0.18f;
        p.op[i].sl = 0.12f;
        p.op[i].rr = 0.16f;
    }
    sys_->apu.setPatch(0, p);
}

void Game::paintBorder() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.B.enabled = false;
    for (int ty = 0; ty < 28; ty++) {
        for (int tx = 0; tx < 40; tx++) {
            bool grass = tx >= kTx && tx < kTx + kW && ty >= kTy && ty < kTy + kH;
            if (grass) continue;
            bool fenceRow = ty == kTy - 1 || ty == kTy + kH;
            bool fenceCol = (tx == kTx - 1 || tx == kTx + kW) && ty >= kTy - 1 && ty <= kTy + kH;
            if (fenceRow || fenceCol) {
                bool post = ((tx + ty) % 5) == 0;
                v.A.set(tx, ty, gs::entry(post ? art_.post : art_.rail, PAL_YARD));
            } else if (ty >= kTy - 1) {
                v.A.set(tx, ty, gs::entry(art_.gravel, PAL_YARD));
            }
        }
    }
}

void Game::paintCell(int r, int c) {
    int style = cell_[r][c];
    int tile = art_.tall[(r + c * 3) % 3];
    int pal = PAL_GRASS;
    if (style == 1) {
        tile = art_.cutH;
        pal = PAL_CUT;
    } else if (style == 2) {
        tile = art_.cutD;
        pal = PAL_CUT;
    } else if (style == 3) {
        tile = art_.cutV;
        pal = PAL_CUT;
    } else if (style == 4) {
        tile = art_.cutVD;
        pal = PAL_CUT;
    }
    sys_->vdp.A.set(kTx + c, kTy + r, gs::entry(tile, pal));
}

void Game::freshField() {
    uncut_ = kW * kH;
    stripes_ = 0;
    std::memset(cell_, 0, sizeof cell_);
    std::memset(band_, 0, sizeof band_);
    for (int i = 0; i < kStripes; i++) bandLeft_[i] = kW * 2;
    for (int r = 0; r < kH; r++)
        for (int c = 0; c < kW; c++) paintCell(r, c);
}

void Game::park() {
    x_ = kXLo;
    y_ = bandY(0);
    heading_ = 0;
    speed_ = 0;
    anchorX_ = x_;
    anchorY_ = y_;
    stuck_ = 0;
}

void Game::buildPath() {
    path_.clear();
    path_.reserve(size_t(kStripes) * 2);
    for (int i = 0; i < kStripes; i++) {
        float y = bandY(i);
        if ((i & 1) == 0) {
            path_.push_back({kXLo, y});
            path_.push_back({kXHi, y});
        } else {
            path_.push_back({kXHi, y});
            path_.push_back({kXLo, y});
        }
    }
    wp_ = 0;
    sweeps_ = 0;
}

bool Game::sweep() {
    if (sweeps_ >= 4 || uncut_ <= 0) return false;
    sweeps_++;
    path_.clear();
    for (int r = 0; r < kH; r++) {
        for (int c = 0; c < kW; c++) {
            if (cell_[r][c]) continue;
            path_.push_back({float(kFx + c * 8 + 4), float(kFy + r * 8 + 4)});
        }
    }
    wp_ = 0;
    return !path_.empty();
}

void Game::begin() {
    freshField();
    park();
    buildPath();
    rainFrames_ = kRain;
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    song_ = -1;
    songKind_ = 0;
    pop_ = 0;
    flash_ = 0;
    fresh_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    tune();
    paintBorder();
    freshField();
    park();
    rainFrames_ = kRain;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    for (int i = 0; i < 40; i++) {
        drop_[i].x = rnd() * 320.f;
        drop_[i].y = rnd() * 224.f;
        drop_[i].v = 3.3f + rnd() * 2.2f;
    }
    if (bot_) begin();
}

void Game::song(int kind) {
    if (songKind_ == 2 && kind != 2) return;
    song_ = 0;
    songKind_ = kind;
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0;
    song(2);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    speed_ = 0;
    song(3);
}

void Game::mark(int r, int c, int style) {
    bool fresh = cell_[r][c] == 0;
    if (cell_[r][c] != uint8_t(style)) {
        cell_[r][c] = uint8_t(style);
        paintCell(r, c);
    }
    if (!fresh) return;
    fresh_++;
    uncut_--;
    int b = r / 2;
    if (b >= 0 && b < kStripes && !band_[b]) {
        bandLeft_[b]--;
        if (bandLeft_[b] <= 0) {
            band_[b] = true;
            stripes_++;
            pop_ = 46;
            popY_ = bandY(b);
            song(1);
        }
    }
    if (uncut_ <= 0) win();
}

void Game::cut() {
    if (std::fabs(speed_) < 0.05f) return;
    float co = std::cos(heading_);
    float si = std::sin(heading_);
    bool horiz = std::fabs(co) >= std::fabs(si);
    int style = horiz ? (co >= 0.f ? 1 : 2) : (si < 0.f ? 3 : 4);
    int x0 = int(std::floor((x_ - kCut - kFx) / 8.f));
    int x1 = int(std::floor((x_ + kCut - kFx) / 8.f));
    int y0 = int(std::floor((y_ - kCut - kFy) / 8.f));
    int y1 = int(std::floor((y_ + kCut - kFy) / 8.f));
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= kW) x1 = kW - 1;
    if (y1 >= kH) y1 = kH - 1;
    float r2 = kCut * kCut;
    for (int r = y0; r <= y1; r++) {
        for (int col = x0; col <= x1; col++) {
            float cx = float(kFx + col * 8 + 4);
            float cy = float(kFy + r * 8 + 4);
            float dx = cx - x_;
            float dy = cy - y_;
            if (dx * dx + dy * dy <= r2) mark(r, col, style);
        }
    }
}

void Game::botDrive(float& steer, float& gas) {
    if (wp_ >= int(path_.size())) {
        if (!sweep()) {
            steer = 0;
            gas = 0;
            return;
        }
    }
    float tx = path_[size_t(wp_)].x;
    float ty = path_[size_t(wp_)].y;
    float dx = tx - x_;
    float dy = ty - y_;
    float dist = std::hypot(dx, dy);
    if (dist < 4.2f) {
        wp_++;
        if (wp_ >= int(path_.size())) {
            if (!sweep()) {
                steer = 0;
                gas = 0;
                return;
            }
        }
        tx = path_[size_t(wp_)].x;
        ty = path_[size_t(wp_)].y;
        dx = tx - x_;
        dy = ty - y_;
        dist = std::hypot(dx, dy);
    }
    if (dist < 8.f && wp_ + 1 < int(path_.size())) {
        float u = dist / 8.f;
        float nx = path_[size_t(wp_ + 1)].x;
        float ny = path_[size_t(wp_ + 1)].y;
        dx = (tx * u + nx * (1.f - u)) - x_;
        dy = (ty * u + ny * (1.f - u)) - y_;
    }
    float desired = std::atan2(dy, dx);
    float err = wrap(desired - heading_);
    steer = std::fabs(err) < 0.02f ? 0.f : std::clamp(err * 2.5f, -1.f, 1.f);
    if (std::fabs(err) > 0.85f && std::fabs(speed_) > 0.7f) gas = -0.55f;
    else if (std::fabs(err) > 0.45f) gas = 0.28f;
    else if (dist < 20.f) gas = 0.62f;
    else gas = 1.f;

    float moved = std::hypot(x_ - anchorX_, y_ - anchorY_);
    if (moved > 2.4f) {
        anchorX_ = x_;
        anchorY_ = y_;
        stuck_ = 0;
    } else if (++stuck_ > 70) {
        heading_ = desired;
        speed_ = std::max(speed_, 1.15f);
        stuck_ = 0;
        if (dist < 12.f) wp_++;
    }
}

void Game::drive(float& steer, float& gas) {
    steer = 0;
    gas = 0;
    if (bot_) {
        botDrive(steer, gas);
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.15f) steer = p.axisX;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO)) gas = 1.f;
    if (p.accel > 0.12f) gas = std::max(gas, p.accel);
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) gas = -0.7f;
    if (p.brake > 0.12f) gas = -p.brake;
    steer = std::clamp(steer, -1.f, 1.f);
}

void Game::move(float steer, float gas) {
    float rate = 0.052f + 0.11f * (1.f - std::min(1.f, std::fabs(speed_) / kMax));
    float steerSign = speed_ < -0.1f ? -1.f : 1.f;
    heading_ = wrap(heading_ + steer * steerSign * rate);
    if (gas > 0.05f) speed_ += 0.12f * gas;
    else if (gas < -0.05f) speed_ += 0.2f * gas;
    else speed_ *= 0.88f;
    speed_ *= 0.994f;
    speed_ = std::clamp(speed_, -1.2f, kMax);
    float co = std::cos(heading_);
    float si = std::sin(heading_);
    x_ += co * speed_;
    y_ += si * speed_;
    if (x_ < kXLo) x_ = kXLo;
    if (x_ > kXHi) x_ = kXHi;
    if (y_ < kYLo) y_ = kYLo;
    if (y_ > kYHi) y_ = kYHi;
}

void Game::sky() {
    float storm = 0;
    if (mode_ == Mode::Lose) storm = 1.f;
    else if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Win) {
        float t = 1.f - float(std::max(rainFrames_, 0)) / float(kRain);
        if (mode_ == Mode::Win) t *= 0.22f;
        storm = std::clamp(t, 0.f, 1.f);
    }
    storm_ = storm;
    int flash = flash_ > 0 ? 4 : 0;
    if (flash_ > 0) flash_--;
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / 223.f;
        int r = int((1.f - storm) * (6.f + u * 4.f) + storm * (4.f + u * 2.f)) + flash;
        int g = int((1.f - storm) * (10.f + u * 3.f) + storm * (5.f + u * 2.f)) + flash;
        int b = int((1.f - storm) * (15.f - u * 3.f) + storm * (8.f - u)) + flash;
        v.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
        int fog = 0;
        if (y > 24 && storm > 0.05f) fog = int(storm * 7.f * float(y - 24) / 200.f);
        v.lineFog[y] = uint8_t(std::min(10, fog));
        v.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool shadow) {
    if (!sys_ || h < 1.f || m.h <= 0) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
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
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    int fog = int(storm_ * 6.f);

    bool banner = mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose;
    if (banner) {
        if (mode_ == Mode::Title) {
            spr(art_.logo, 160, 58, float(art_.logo.h), PAL_TITLE);
            spr(art_.tag, 160, 86, float(art_.tag.h), PAL_TITLE);
            spr(art_.hint, 160, 112, float(art_.hint.h), PAL_TITLE);
            spr(art_.turn, 160, 126, float(art_.turn.h), PAL_TITLE);
        } else {
            spr(mode_ == Mode::Win ? art_.win : art_.lose, 160, 78, float(mode_ == Mode::Win ? art_.win.h : art_.lose.h),
                PAL_TITLE);
            spr(art_.plaque, 160, 82, 32, PAL_HUD);
        }
    }
    if (pop_ > 0 && mode_ == Mode::Play) {
        spr(art_.stripe, 160, std::clamp(popY_, 48.f, 176.f), float(art_.stripe.h), PAL_TITLE);
        pop_--;
    }

    if (storm_ > 0.58f && (mode_ == Mode::Play || mode_ == Mode::Lose)) {
        for (auto& d : drop_) {
            d.y += d.v;
            d.x += 0.45f;
            if (d.y > 228.f) {
                d.y = -rnd() * 30.f;
                d.x = rnd() * 320.f;
                d.v = 3.3f + rnd() * 2.2f;
            }
            if (d.x > 320.f) d.x -= 320.f;
            spr(art_.rain, d.x, d.y, 8, PAL_SKY, 0);
        }
    }

    for (auto& b : bit_) {
        if (b.t <= 0) continue;
        b.x += b.vx;
        b.y += b.vy;
        b.t -= 1.f;
        spr(art_.flower[1], b.x, b.y, 4, PAL_GRASS, fog);
    }

    float bob = std::sin(float(sys_->frame) * 0.45f) * std::min(1.f, std::fabs(speed_)) * 0.6f;
    float ang = heading_;
    float turns = ang / kTau;
    turns -= std::floor(turns);
    int fr = int(turns * 16.f + 0.5f) & 15;
    spr(art_.shadow, x_ + 3.f, y_ + 5.f, 12, PAL_MOWER, 0, true);
    spr(art_.mower[fr], x_, y_ + bob, 36, PAL_MOWER, fog / 2);

    spr(art_.flower[0], 36, 214, 14, PAL_FLOWER, fog);
    spr(art_.flower[1], 78, 216, 13, PAL_FLOWER, fog);
    spr(art_.flower[2], 124, 214, 14, PAL_FLOWER, fog);
    spr(art_.flower[0], 168, 216, 12, PAL_FLOWER, fog);
    spr(art_.flower[1], 214, 214, 14, PAL_FLOWER, fog);
    spr(art_.gnome, 258, 210, 18, PAL_GNOME, fog);
    spr(art_.mail, 300, 206, 16, PAL_YARD, fog);
    spr(art_.shed, 96, 20, 28, PAL_SHED, 0);
    spr(art_.tree, 36, 18, 32, PAL_TREE, 0);
    if (storm_ < 0.5f) spr(art_.sun, 286, 18, 16, PAL_SUN, 0);

    float cx[3] = {40.f, 150.f, 250.f};
    float cs[3] = {0.18f, 0.28f, 0.12f};
    for (int i = 0; i < 3; i++) {
        float x = std::fmod(cx[i] + float(sys_->frame) * cs[i], 400.f) - 40.f;
        spr(art_.cloud, x, 12.f + float(i) * 3.f, 14, PAL_SKY, 0);
    }

    for (int x = 0; x < 40; x++) v.HUD.set(x, 0, gs::entry(art_.bar, PAL_HUD));
    if (mode_ == Mode::Title) {
        bool blink = ((sys_->frame / 30) & 1) == 0;
        hud(1, 0, "S3 MOWER", PAL_HUD);
        if (blink) hud(28, 0, "START", PAL_HUD);
        else hud(26, 0, "Z MOWS", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_HUD);
    } else {
        char bar[16];
        for (int i = 0; i < kStripes; i++) bar[i] = band_[i] ? '#' : '-';
        bar[kStripes] = 0;
        char line[24];
        std::snprintf(line, sizeof line, "%s %02d/%02d", bar, stripes_, kStripes);
        hud(0, 0, line, PAL_HUD);
        int sec = rainSeconds();
        char clock[16];
        std::snprintf(clock, sizeof clock, "RAIN %d:%02d", sec / 60, sec % 60);
        int pal = (mode_ == Mode::Play && sec <= 10 && ((sys_->frame / 8) & 1)) ? PAL_ALERT : PAL_HUD;
        hud(40 - int(std::strlen(clock)), 0, clock, pal);
        if (mode_ == Mode::Win) hudC(12, "BEFORE THE RAIN", PAL_HUD);
        if (mode_ == Mode::Win) hudC(14, "START TO MOW AGAIN", PAL_HUD);
        if (mode_ == Mode::Lose) hudC(12, "THE GRASS WINS", PAL_HUD);
        if (mode_ == Mode::Lose) hudC(14, "START TO TRY AGAIN", PAL_HUD);
    }
}

void Game::audio() {
    float rpm = std::min(1.f, std::fabs(speed_) / kMax);
    bool engine = mode_ == Mode::Play && rpm > 0.08f;
    sys_->apu.tone(0, 46.f + rpm * 34.f, engine ? 0.03f + rpm * 0.02f : 0.f);
    sys_->apu.tone(1, 92.f + rpm * 60.f, engine ? 0.018f : 0.f);
    float nvol = engine ? 0.022f + rpm * 0.028f + (fresh_ > 0 ? 0.01f : 0.f) : 0.f;
    sys_->apu.noise(nvol, 480.f + rpm * 860.f, false);

    if (song_ >= 0) {
        struct N {
            int t;
            float f;
        };
        const N* ns = nullptr;
        int count = 0;
        int end = 0;
        static const N blip[] = {{0, 784.f}, {7, 1046.f}};
        static const N winN[] = {{0, 523.f}, {8, 659.f}, {16, 784.f}, {24, 1046.f}, {36, 784.f}, {44, 1175.f}};
        static const N loseN[] = {{0, 349.f}, {10, 311.f}, {20, 262.f}, {32, 196.f}};
        if (songKind_ == 2) {
            ns = winN;
            count = 6;
            end = 72;
        } else if (songKind_ == 3) {
            ns = loseN;
            count = 4;
            end = 58;
        } else {
            ns = blip;
            count = 2;
            end = 18;
        }
        float f = 0;
        for (int i = 0; i < count; i++)
            if (song_ >= ns[i].t) f = ns[i].f;
        sys_->apu.tone(2, f, f > 0 ? 0.07f : 0.f);
        for (int i = 0; i < count; i++)
            if (song_ == ns[i].t) sys_->apu.keyOn(0, ns[i].f, 0.16f);
        if (++song_ > end) {
            song_ = -1;
            if (songKind_ != 2) songKind_ = 0;
        }
    } else {
        sys_->apu.tone(2, 0, 0);
    }

    if (mode_ == Mode::Play && storm_ > 0.72f && (sys_->frame % 170) == 0) {
        flash_ = 4;
        sys_->apu.noiseBurst(0.11f, 2400.f, 0.16f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    fresh_ = 0;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    bool began = false;
    if (mode_ == Mode::Title) {
        if (start || bot_) {
            begin();
            began = true;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && start) {
            begin();
            began = true;
        }
    }

    if (mode_ == Mode::Play) {
        if (!bot_ && !began && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else {
            float steer = 0, gas = 0;
            drive(steer, gas);
            move(steer, gas);
            cut();
            if (fresh_ > 0 && (sys.frame & 1) == 0) {
                for (auto& b : bit_) {
                    if (b.t > 0) continue;
                    b.x = x_ - std::cos(heading_) * 8.f;
                    b.y = y_ - std::sin(heading_) * 8.f;
                    b.vx = -std::cos(heading_) * 0.6f + (rnd() - 0.5f);
                    b.vy = -std::sin(heading_) * 0.4f + (rnd() - 0.5f);
                    b.t = 12.f + rnd() * 8.f;
                    break;
                }
            }
            if (mode_ == Mode::Play) {
                if (uncut_ <= 0) win();
                else {
                    rainFrames_--;
                    if (rainFrames_ <= 0) lose();
                }
            }
        }
    }
    draw();
    audio();
}

}  // namespace mower
