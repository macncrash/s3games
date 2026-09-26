#include "funicular.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace funicular {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kGrade = 6.4f;
constexpr float kMotor = 4.0f;
constexpr float kEase = 3.4f;
constexpr float kBrake = 5.8f;
constexpr float kDrag = 0.48f;
constexpr float kPast = 2.6f;
constexpr float kHold = 0.42f;
constexpr float kZoom = 30.f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.vol = 0.2f;
    p.echo = 0.42f;
    p.op[0].mul = 2.f;
    p.op[0].level = 0.7f;
    p.op[0].ar = 0.001f;
    p.op[0].dr = 0.18f;
    p.op[0].sl = 0.f;
    p.op[0].rr = 0.25f;
    p.op[1].mul = 1.f;
    p.op[1].level = 0.9f;
    p.op[1].ar = 0.001f;
    p.op[1].dr = 1.05f;
    p.op[1].sl = 0.f;
    p.op[1].rr = 0.7f;
    p.op[2].mul = 3.6f;
    p.op[2].level = 0.32f;
    p.op[2].ar = 0.001f;
    p.op[2].dr = 0.42f;
    p.op[2].sl = 0.f;
    p.op[2].rr = 0.35f;
    p.op[3].mul = 5.1f;
    p.op[3].level = 0.16f;
    p.op[3].ar = 0.001f;
    p.op[3].dr = 0.28f;
    p.op[3].sl = 0.f;
    p.op[3].rr = 0.3f;
    return p;
}

}  // namespace

const Game::Stop& Game::stop() const { return kStops[std::clamp(cleared_, 0, kStopsN - 1)]; }

float Game::coastA() const { return kGrade * (massB_ - massA_) / (massA_ + massB_); }

int Game::marker() const {
    if (over_ || mode_ == Mode::Result) return 4;
    if (mode_ == Mode::Title) return 0;
    if (dwell_ > 0.f) return 3;
    if (cleared_ < kStopsN && std::fabs(s_ - stop().at) < 3.2f) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    gs::FMPatch bell = bellPatch();
    sys.apu.setPatch(0, bell);
    sys.apu.setPatch(1, bell);
    sys.apu.setPatch(2, bell);
    sys.apu.setEcho(0.2f, 0.32f, 0.18f);
    sys.apu.setMaster(0.85f);
    report_[0] = 0;
    banner_[0] = 0;
    mode_ = Mode::Title;
}

void Game::applyLoad() {
    int i = std::clamp(cleared_, 0, kStopsN - 1);
    massA_ = kLoad[i].red;
    massB_ = kLoad[i].green;
}

void Game::begin() {
    s_ = 8.f;
    v_ = 0.f;
    cleared_ = 0;
    held_ = 0.f;
    dwell_ = 0.f;
    time_ = 0.f;
    cable_ = 0.f;
    over_ = false;
    won_ = false;
    pendingWin_ = false;
    bell_ = 0;
    post_ = int(std::floor(s_ / 3.2f));
    made_[0] = 0;
    banner_[0] = 0;
    report_[0] = 0;
    applyLoad();
    mode_ = Mode::Run;
}

void Game::bot(bool& wind, bool& brake, bool& ease) {
    const float mark = stop().at;
    const float tol = stop().tol;
    const float dist = mark - s_;
    const float coast = coastA();

    if (v_ < -1.15f) {
        brake = true;
        return;
    }
    if (dist < -tol) {
        if (v_ > 0.05f || v_ < -0.14f) brake = true;
        else ease = true;
        return;
    }
    if (std::fabs(dist) <= tol && std::fabs(v_) <= 0.25f) {
        brake = true;
        return;
    }
    if (std::fabs(dist) <= tol) {
        brake = true;
        return;
    }
    if (v_ < -0.05f) {
        wind = true;
        return;
    }

    const bool near = dist < 1.7f;
    if (near) {
        if (v_ > 0.40f) brake = true;
        else if (v_ < 0.16f && dist > tol * 0.15f) {
            if (coast > 0.8f) {
                if (v_ < 0.02f) wind = false;
            } else wind = true;
        } else if (coast > 0.8f) brake = true;
        return;
    }

    float vCap = coast > 0.8f ? 2.35f : 3.35f;
    if (dist < 8.f) vCap = std::min(vCap, 0.42f + dist * 0.28f);
    float decel = std::max(0.6f, kBrake - coast);
    float stopAt = s_ + (v_ > 0.f ? (v_ * v_) / (2.f * decel) : 0.f);
    if (v_ > 0.08f && stopAt >= mark - tol * 0.15f) {
        brake = true;
        return;
    }
    if (v_ > vCap) brake = true;
    else if (v_ < vCap - 0.14f && (coast < 0.45f || v_ < 0.2f)) wind = true;
}

void Game::arrive() {
    if (over_ || dwell_ > 0.f) return;
    std::snprintf(made_, sizeof made_, "%s", kStops[cleared_].name);
    std::snprintf(banner_, sizeof banner_, "LEVEL  %s", made_);
    cleared_++;
    held_ = 0.f;
    bell_ = 22;
    v_ = 0.f;
    sys_->rumble(0.2f, 0.06f, 80);
    sys_->setLight(50, 170, 80);
    if (cleared_ >= kStopsN) {
        pendingWin_ = true;
        dwell_ = 0.8f;
    } else dwell_ = 1.15f;
}

void Game::win() {
    won_ = true;
    over_ = true;
    pendingWin_ = false;
    mode_ = Mode::Result;
    std::snprintf(banner_, sizeof banner_, "LEVEL WITH THE PLATFORM");
    std::snprintf(report_, sizeof report_,
                  "S3 FUNICULAR  LEVEL  stopped level with the platform at ORCHARD, PASS, and CREST  (%.1f s)", time_);
    sys_->apu.keyOn(0, 392.f, 0.18f);
    sys_->apu.keyOn(1, 494.f, 0.15f);
    sys_->apu.keyOn(2, 587.f, 0.12f);
    sys_->setLight(70, 210, 110);
    sys_->rumble(0.3f, 0.18f, 200);
}

void Game::fail(const char* why) {
    if (over_) return;
    won_ = false;
    over_ = true;
    pendingWin_ = false;
    mode_ = Mode::Result;
    std::snprintf(banner_, sizeof banner_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 FUNICULAR  FAIL  %s", why);
    sys_->apu.noiseBurst(0.4f, 140.f, 0.2f);
    sys_->apu.keyOn(0, 110.f, 0.16f);
    sys_->setLight(180, 30, 24);
    sys_->rumble(0.45f, 0.12f, 160);
}

void Game::physics(bool wind, bool brake, bool ease) {
    if (over_) return;
    const float mark = stop().at;
    const float tol = stop().tol;
    const float coast = coastA();
    float a = coast - v_ * kDrag;
    bool locked = false;
    if (brake) {
        if (std::fabs(v_) < 0.11f) {
            v_ = 0.f;
            locked = true;
        } else if (v_ > 0.f) a -= kBrake;
        else a += kBrake;
    } else if (wind) a += kMotor;
    else if (ease) a -= kEase;

    if (!locked) {
        v_ += a * DT;
        v_ = std::clamp(v_, -3.6f, 4.4f);
        s_ += v_ * DT;
    }
    cable_ += v_ * DT;

    if (s_ < 0.6f) {
        s_ = 0.6f;
        if (v_ < -1.45f) {
            fail("hit the valley buffer");
            return;
        }
        if (v_ < 0.f) v_ = 0.f;
    }
    if (s_ > kLen - 0.6f) {
        s_ = kLen - 0.6f;
        if (v_ > 1.15f) {
            fail("hit the bullwheel");
            return;
        }
        if (v_ > 0.f) v_ = 0.f;
    }
    if (s_ > mark + tol + kPast) {
        char why[64];
        std::snprintf(why, sizeof why, "past the platform at %s", stop().name);
        fail(why);
        return;
    }
    if (brake && std::fabs(s_ - mark) <= tol && std::fabs(v_) <= 0.08f) {
        held_ += DT;
        if (held_ >= kHold) arrive();
    } else held_ = 0.f;
}

void Game::audio(bool wind, bool brake) {
    if (bell_ > 0) {
        if (bell_ == 20 || bell_ == 10) sys_->apu.keyOn(0, bell_ > 12 ? 659.f : 880.f, 0.16f);
        bell_--;
    }
    bool live = mode_ == Mode::Run && dwell_ <= 0.f && !over_;
    float hum = (live && wind) ? 0.045f : (live && std::fabs(v_) > 0.4f ? 0.02f : 0.f);
    sys_->apu.tone(0, 68.f + std::fabs(v_) * 18.f, hum);
    sys_->apu.tone(1, 136.f + std::fabs(v_) * 9.f, hum * 0.28f);
    float squeal = (live && brake && std::fabs(v_) > 0.35f) ? std::min(0.045f, std::fabs(v_) * 0.012f) : 0.f;
    sys_->apu.noise(squeal, 2100.f, false);
    if (live && std::fabs(v_) > 0.45f) {
        int step = int(std::floor(s_ / 3.2f));
        if (step != post_) {
            sys_->apu.noiseBurst(0.05f, 1600.f, 0.025f);
            post_ = step;
        }
    }
}

void Game::stamp(const gs::Image& img, float ax, float ay, float x, float y, float scale, int pal, int fog, bool shadow) {
    if (img.w < 1 || scale < 0.04f) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, int(std::lround(img.w * scale))));
    s.h = int16_t(std::max(1, int(std::lround(img.h * scale))));
    s.x = int16_t(std::lround(x - ax * scale));
    s.y = int16_t(std::lround(y - ay * scale));
    s.img = img;
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& img, float ax, float ay, float x, float y, float scale, int pal, int fog) {
    if (img.h < 1 || scale < 0.04f) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, int(std::lround(img.w * scale))));
    s.h = int16_t(std::max(1, int(std::lround(img.h * scale))));
    s.x = int16_t(std::lround(x - ax * scale));
    s.y = int16_t(std::lround(y - ay * scale));
    s.img = img.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27 || pal < 1 || pal > 5) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[pal][c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, PAL_HUD));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hudText(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::draw(bool wind, bool brake, bool ease) {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    const uint16_t skyTop = gs::rgb4(3, 5, 9);
    const uint16_t skyMid = gs::rgb4(6, 9, 13);
    const uint16_t skyHor = gs::rgb4(12, 11, 9);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / 223.f;
        v.lineBackdrop[y] = t < 0.55f ? lerpC(skyTop, skyMid, t / 0.55f) : lerpC(skyMid, skyHor, (t - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
    }

    const bool title = mode_ == Mode::Title;
    float show = title ? 14.f : s_;
    float showG = kLen - show;
    int gaugeStop = title ? 0 : std::clamp(cleared_, 0, kStopsN - 1);
    if (!title && (dwell_ > 0.f || mode_ == Mode::Result) && cleared_ > 0) gaugeStop = cleared_ - 1;
    const Stop& st = kStops[gaugeStop];
    float mark = st.at;
    float tol = st.tol;

    auto putCar = [&](float s, int side, int pal, float scale, int fog) {
        float x, y;
        railPoint(s, side, x, y);
        stamp(art_.shadow, 14.f, 3.f, x + 4.f, y + 8.f, scale, pal, fog, true);
        stamp(art_.car, art_.sillX, art_.sillY, x, y, scale, pal, fog);
    };

    if (title) stamp(art_.title, 0, 0, 8, 8, 1, PAL_HUD);
    else if (mode_ == Mode::Result) {
        const gs::Image& word = won_ ? art_.levelWord : art_.missWord;
        stamp(word, 0, 0, 8, 8, 1, PAL_HUD);
    } else {
        stamp(art_.sill, 11.f, 3.f, 78.f, 6.f + art_.lipY + std::clamp(mark - show, -0.55f, 0.55f) * kZoom, 1, PAL_RED);
        stamp(art_.gauge, 0, 0, 6, 6, 1, PAL_GAUGE);
    }

    for (int i = 0; i < 3; i++) {
        float bx = 168.f + i * 36.f;
        float by = 14.f + std::sin(t_ * 1.3f + i) * 3.f;
        stamp(art_.bird, 6, 3, bx, by, 1, PAL_HUD);
    }

    putCar(show, +1, PAL_RED, 1.f, 0);
    putCar(showG, -1, PAL_GREEN, 0.86f, 3);

    auto folkAt = [&](int stopIndex, float walk) {
        float x, y;
        railPoint(kStops[stopIndex].at, +1, x, y);
        for (int n = 0; n < 2; n++) {
            float px = x + 16.f + n * 12.f - walk;
            if (walk > 18.f) continue;
            stamp(art_.folk[n], 6.f, 17.f, px, y, 1.f, PAL_FOLK);
        }
    };
    if (!title) {
        if (dwell_ > 0.f && cleared_ > 0) {
            float walk = (1.15f - std::min(dwell_, 1.15f)) * 20.f;
            folkAt(cleared_ - 1, walk);
        }
        for (int i = cleared_; i < kStopsN; i++) folkAt(i, 0.f);
    } else {
        folkAt(0, 0.f);
        folkAt(1, 0.f);
    }

    for (int i = 0; i < kStopsN; i++) {
        float x, y;
        railPoint(kStops[i].at, +1, x, y);
        bool hot = !title && (i == std::min(cleared_, kStopsN - 1) || (dwell_ > 0.f && i == cleared_ - 1));
        if (hot) stamp(art_.tick, 3.f, 8.f, x - 2.f, y - 2.f, 1.f, PAL_DECK);
        stamp(art_.deck, art_.deckX, art_.deckY, x, y, 1.f, PAL_DECK);
    }
    {
        float x, y;
        railPoint(10.f, -1, x, y);
        stamp(art_.deck, art_.deckX, art_.deckY, x, y, 0.82f, PAL_DECK);
    }
    {
        float x, y;
        railPoint(1.4f, +1, x, y, 4.f);
        stamp(art_.buffer, 6.f, 14.f, x, y, 1.f, PAL_STEEL);
        railPoint(kLen - 1.2f, 0, x, y, 2.f);
        stamp(art_.buffer, 6.f, 14.f, x, y, 1.f, PAL_STEEL);
    }
    {
        float x, y;
        railPoint(kLen + 0.6f, 0, x, y, -16.f);
        stamp(art_.house, art_.houseX, art_.houseY, x, y, 1.f, PAL_HOUSE);
    }

    float phase = std::fmod(cable_, 3.4f);
    if (phase < 0.f) phase += 3.4f;
    for (int side : {1, -1}) {
        float start = side > 0 ? phase : 3.4f - phase;
        for (float s = start; s < kLen - 2.f; s += 3.4f) {
            float x, y;
            railPoint(s, side, x, y, -18.f);
            stamp(art_.bead, 2.5f, 2.f, x, y, 1.f, PAL_STEEL, side < 0 ? 4 : 0);
        }
    }

    stamp(art_.hill, 0, 0, 0, 0, 1, PAL_HILL);
    stamp(art_.sun, 8, 8, 196, 18, 1, PAL_HUD);
    for (int i = 0; i < 3; i++) stamp(art_.cloud, 20, 8, 24.f + i * 78.f, 28.f + (i == 1 ? 8.f : 0.f), 1, PAL_HUD);

    if (title) {
        hudC(25, "STOP LEVEL WITH THE PLATFORM", 1);
        hudC(26, "C WIND    X BRAKE    E EASE", 3);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", 5);
        else hudC(27, S3_VERSION_STRING, 2);
        return;
    }

    if (mode_ == Mode::Result) {
        hudC(25, banner_, won_ ? 5 : 4);
        if (!bot_) hudC(26, "ENTER", 3);
        return;
    }

    int redN = std::max(0, int(std::lround(massA_ - 10.f)));
    int greenN = std::max(0, int(std::lround(massB_ - 10.f)));
    char left[32], load[20], right[16];
    if (dwell_ > 0.f) std::snprintf(left, sizeof left, "LEVEL %s", made_);
    else std::snprintf(left, sizeof left, "NEXT %s", st.name);
    float coast = coastA();
    const char* pull = coast > 0.35f ? "PULLS UP" : coast < -0.35f ? "PULLS DOWN" : "EVEN";
    std::snprintf(load, sizeof load, "R%d G%d", redN, greenN);
    std::snprintf(right, sizeof right, "%.1f", std::fabs(v_));
    hudText(1, 25, left, dwell_ > 0.f ? 5 : 3);
    hudText(15, 25, load, 1);
    hudText(22, 25, pull, coast > 0.35f ? 4 : coast < -0.35f ? 3 : 2);
    hudText(34, 25, right, 1);
    hudText(1, 26, "C WIND", wind ? 5 : 2);
    hudText(10, 26, "X BRAKE", brake ? 4 : 2);
    hudText(19, 26, "E EASE", ease ? 3 : 2);
    hudText(34, 26, right, 1);

    float err = show - mark;
    const int N = 15;
    const int center = 7;
    float span = 1.1f;
    int pos = center + int(std::lround(std::clamp(err, -span, span) / span * center));
    pos = std::clamp(pos, 0, N - 1);
    char meter[16];
    for (int i = 0; i < N; i++) meter[i] = '-';
    meter[center] = '+';
    int half = std::max(1, int(std::lround(tol / span * center)));
    if (center - half >= 0) meter[center - half] = ':';
    if (center + half < N) meter[center + half] = ':';
    meter[pos] = 'O';
    meter[N] = 0;
    char row[40];
    int cm = int(std::lround(err * 100.f));
    const char* word = "LEVEL";
    if (dwell_ > 0.f || (std::fabs(err) <= tol && std::fabs(v_) < 0.2f)) word = "LEVEL";
    else if (cm > 0) word = "HIGH";
    else word = "LOW";
    std::snprintf(row, sizeof row, "%s %s %+d CM", word, meter, cm);
    int barPal = 1;
    if (std::fabs(err) <= tol && std::fabs(v_) < 0.2f) barPal = 5;
    else if (err > 0.f) barPal = 4;
    else barPal = 3;
    hudC(27, row, barPal);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    auto go = [&]() { return sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C) || sys.pad.pressed(gs::BTN_A); };
    if (sys.pad.pressed(gs::BTN_MODE) && !bot_) {
        if (mode_ == Mode::Title) sys.quit();
        else {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            pendingWin_ = false;
            dwell_ = 0.f;
        }
    }
    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.4f) begin();
        else if (!bot_ && go()) begin();
        cable_ += 0.35f * DT;
    } else if (mode_ == Mode::Result) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
    }

    bool wind = false, brake = false, ease = false;
    if (mode_ == Mode::Run && !over_) {
        time_ += DT;
        if (dwell_ > 0.f) {
            dwell_ -= DT;
            brake = true;
            v_ = 0.f;
            if (dwell_ <= 0.f) {
                dwell_ = 0.f;
                if (pendingWin_) win();
                else applyLoad();
            }
        } else {
            if (bot_) bot(wind, brake, ease);
            else {
                const gs::Pad& p = sys.pad;
                wind = p.down(gs::BTN_C) || p.down(gs::BTN_UP) || p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.accel > 0.22f;
                brake = p.down(gs::BTN_B) || p.down(gs::BTN_DOWN) || p.down(gs::BTN_X) || p.down(gs::BTN_TURBO) || p.brake > 0.22f;
                ease = p.down(gs::BTN_Z) || p.down(gs::BTN_LEFT) || p.down(gs::BTN_Y);
            }
            if (brake) {
                wind = false;
                ease = false;
            } else if (wind) ease = false;
            physics(wind, brake, ease);
        }
    }
    audio(wind, brake);
    draw(wind, brake, ease);
}

}  // namespace funicular
