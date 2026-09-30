#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace keelpass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLen = 8.2f;
constexpr float kBeam = 2.05f;
constexpr float kMouth = 268.f;
constexpr float kClock = 40.f;
constexpr float kStart = 22.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::centerAt(float z) const {
    return 2.15f * std::sin(z * 0.026f) + 0.9f * std::sin(z * 0.051f + 0.7f);
}

float Game::halfAt(float z) const {
    float n = (z - 150.f) / 42.f;
    float pinch = std::exp(-n * n);
    float mouth = clampf((z - (kMouth - 30.f)) / 30.f, 0.f, 1.f);
    return 9.2f - pinch * 2.35f + mouth * 1.2f;
}

float Game::gustAt() const {
    return 1.15f * std::sin(z_ * 0.05f + time_ * 0.9f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.apu.setMaster(0.44f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    z_ = kStart;
    x_ = centerAt(z_);
    camZ_ = z_ - 18.f;
    storm_ = kClock;
}

void Game::begin() {
    z_ = kStart;
    x_ = centerAt(z_) + 0.35f;
    heading_ = 0.f;
    speed_ = 0.f;
    time_ = 0.f;
    storm_ = kClock;
    camZ_ = z_ - 18.f;
    shake_ = 0.f;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(230.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.12f);
    beep_ = 0.08f;
}

void Game::strike(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    speed_ *= 0.12f;
    sys_->apu.noiseBurst(0.42f, 760.f, 0.28f);
    sys_->apu.tone(1, 74.f, 0.22f);
    beep_ = 0.32f;
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    chime_ = 0;
    chimeT_ = 0;
    why_ = "clear";
}

void Game::pilot(float& sheet, float& helm) {
    const gs::Pad& pad = sys_->pad;
    sheet = 0.f;
    helm = 0.f;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.accel > 0.2f) sheet += 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) sheet -= 0.65f;
    if (pad.down(gs::BTN_RIGHT)) helm += 1.f;
    if (pad.down(gs::BTN_LEFT)) helm -= 1.f;
    if (std::fabs(pad.axisX) > 0.18f) helm = pad.axisX;
    if (!bot_) return;

    float look = 10.f + speed_ * 0.45f;
    float aim = centerAt(z_ + look);
    float err = aim - x_;
    helm = clampf(err * 0.85f - heading_ * 2.4f, -1.f, 1.f);
    float room = halfAt(z_ + 8.f) - kBeam * 0.5f - std::fabs(x_ - centerAt(z_));
    sheet = room < 2.2f ? 0.55f : 1.f;
}

void Game::stepRun(float sheet, float helm) {
    time_ += kDt;
    storm_ -= kDt;
    heading_ += helm * 1.35f * kDt;
    heading_ = clampf(heading_, -0.62f, 0.62f);
    heading_ *= std::exp(-kDt * 0.45f);

    float drive = std::max(0.f, std::cos(heading_ * 0.85f));
    float target = sheet > 0.f ? sheet * (7.2f + 5.4f * drive) : 0.f;
    if (sheet < 0.f) target = sheet * 3.f;
    speed_ += (target - speed_) * (sheet < 0.f ? 2.4f : 1.15f) * kDt;
    speed_ = clampf(speed_, -2.2f, 13.5f);

    float gust = gustAt();
    x_ += (std::sin(heading_) * speed_ + gust * 0.22f) * kDt;
    z_ += std::cos(heading_) * speed_ * kDt;

    auto cornerHits = [&](auto&& fn) {
        const float c = std::cos(heading_), s = std::sin(heading_);
        const float hx = kBeam * 0.5f, hz = kLen * 0.5f;
        const float ox[4] = {-hx, hx, hx, -hx};
        const float oz[4] = {-hz, -hz, hz, hz};
        for (int i = 0; i < 4; i++) {
            float wx = x_ + ox[i] * c - oz[i] * s;
            float wz = z_ + ox[i] * s + oz[i] * c;
            if (fn(wx, wz)) return true;
        }
        return false;
    };
    bool rock = cornerHits([&](float wx, float wz) {
        if (wz < 8.f || wz > kMouth + 6.f) return false;
        return std::fabs(wx - centerAt(wz)) > halfAt(wz) - 0.15f;
    });
    if (rock) {
        strike("hull on the rock");
        return;
    }
    if (storm_ <= 0.f) {
        strike("the other crew closed the pass");
        return;
    }
    if (z_ - kLen * 0.5f >= kMouth) finish();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        time_ += kDt;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || (bot_ && time_ > 0.25f)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float sheet = 0, helm = 0;
            pilot(sheet, helm);
            stepRun(sheet, helm);
        }
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        mode_ = Mode::Title;
        over_ = false;
        time_ = 0;
    }

    float want = z_ - 18.f;
    camZ_ += (want - camZ_) * (1.f - std::exp(-kDt * 3.6f));
    if (mode_ == Mode::Title) camZ_ = z_ - 18.f;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        int stepN = int(chimeT_ / 0.15f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(2, notes[stepN], 0.15f);
        }
        if (stepN >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    if (mode_ == Mode::Run && storm_ < 9.f) sys.apu.noise(0.05f, 280.f + (9.f - storm_) * 50.f);
    else if (mode_ != Mode::Fail) sys.apu.noise(0, 0);
    draw();
}

bool Game::project(float wx, float wz, float& sx, float& sy, float& sc) const {
    float dz = wz - camZ_;
    if (dz < 2.8f) return false;
    sc = 360.f / dz;
    float jx = (shake_ > 0.f) ? std::sin(time_ * 88.f) * shake_ * 3.f : 0.f;
    sx = 160.f + wx * sc + jx;
    sy = 62.f + 980.f / dz;
    return true;
}

void Game::stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip) {
    if (m.h < 1 || w < 1.f || h < 1.f) return;
    if (x + w < -24.f || x > gs::SCREEN_W + 24.f || y + h < -24.f || y > gs::SCREEN_H + 36.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 420L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 320L));
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::worldQuad(const gs::Mipped& m, float x0, float z0, float x1, float z1, float lift, int pal) {
    float sx0, sy0, sc0, sx1, sy1, sc1;
    if (!project(x0, z0, sx0, sy0, sc0)) return;
    if (!project(x1, z1, sx1, sy1, sc1)) return;
    float left = std::min(sx0, sx1);
    float right = std::max(sx0, sx1);
    float top = std::min(sy0, sy1) - lift * 0.5f * (sc0 + sc1);
    float bot = std::max(sy0, sy1);
    stamp(m, left, top, std::max(2.f, right - left), std::max(2.f, bot - top), pal);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float gloom = clampf(1.f - storm_ / kClock, 0.f, 1.f);
    if (mode_ == Mode::Title) gloom = 0.12f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H);
        int sky = y < 78;
        int r = sky ? int(2 + (1.f - gloom) * 2.f) : int(1 + t * 2.f);
        int g = sky ? int(4 + (1.f - gloom) * 4.f) : int(5 + t * 4.f);
        int b = sky ? int(7 + (1.f - gloom) * 5.f) : int(8 + t * 5.f);
        vdp.lineBackdrop[y] = gs::rgb4(std::max(1, r), std::max(2, g), std::max(3, b));
        int fog = 0;
        if (y < 86) fog = (86 - y) / 14;
        fog += int(gloom * (y < 90 ? 8.f : 3.f));
        vdp.lineFog[y] = uint8_t(std::min(16, fog));
        vdp.road[y].on = false;
    }

    for (int i = 0; i < 5; i++) {
        float cz = camZ_ + 30.f + i * 14.f + std::fmod(time_ * 2.f, 14.f);
        float cx = centerAt(cz) + ((i & 1) ? -6.f : 7.f);
        worldQuad(art_.cloud, cx - 3.f, cz, cx + 3.f, cz + 2.f, 26.f, PAL_CLOUD);
    }

    for (float z = camZ_ + 86.f; z > camZ_ + 3.5f; z -= 7.f) {
        float c = centerAt(z);
        float h = halfAt(z);
        worldQuad(art_.cliff, c - h - 4.6f, z, c - h, z + 7.f, 22.f, PAL_CLIFF);
        worldQuad(art_.cliff, c + h, z, c + h + 4.6f, z + 7.f, 22.f, PAL_CLIFF);
        if (int(z) % 21 < 7) {
            worldQuad(art_.pine, c - h - 1.2f, z, c - h + 0.2f, z + 3.f, 16.f, PAL_PINE);
            worldQuad(art_.pine, c + h - 0.2f, z, c + h + 1.2f, z + 3.f, 16.f, PAL_PINE);
        }
    }

    float bsx, bsy, bsc;
    if (project(x_, z_, bsx, bsy, bsc)) {
        float bh = std::max(16.f, bsc * 6.6f);
        float bw = bh * 0.46f;
        float lean = heading_ * 16.f + gustAt() * 1.4f;
        bool flip = lean < 0.f;
        stamp(art_.foam, bsx - bw * 0.75f, bsy + bh * 0.12f, bw * 1.4f, bh * 0.2f, PAL_FOAM);
        stamp(art_.sail, bsx - bw * 0.9f + lean, bsy - bh * 0.95f, bw * 1.75f, bh * 0.82f, PAL_SAIL, flip);
        stamp(art_.hull, bsx - bw * 0.5f + lean * 0.25f, bsy - bh * 0.42f, bw, bh, PAL_HULL);
    }

    if (mode_ == Mode::Win) {
        float tsx, tsy, tsc;
        if (project(x_, z_ + 8.f, tsx, tsy, tsc)) stamp(art_.clear, tsx - 48.f, tsy - 36.f, 96.f, 22.f, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        float tsx, tsy, tsc;
        if (project(x_, z_ + 6.f, tsx, tsy, tsc)) stamp(art_.rock, tsx - 40.f, tsy - 30.f, 80.f, 20.f, PAL_ALERT);
    }

    if (mode_ == Mode::Title) {
        float tsx, tsy, tsc;
        if (project(centerAt(z_ + 24.f), z_ + 24.f, tsx, tsy, tsc))
            stamp(art_.title, tsx - 78.f, tsy - 22.f, 156.f, 30.f, PAL_GOLD);
        hudC(16, "TAKE THE KEEL", PAL_HUD);
        hudC(18, "CLEAR THE PASS", PAL_HUD);
        hudC(20, "BEFORE THE STORM CLOCK", PAL_ALERT);
        hudC(23, "START  SHEET AND HELM", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(16, "CLEAR OF THE PASS", PAL_WIN);
        hudC(18, "AHEAD OF THE OTHER CREW", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
    } else {
        float pinch = halfAt(z_);
        const char* hint = "SHEET UP  HOLD THE CENTRE";
        if (pinch < 5.4f) hint = "THE PASS NARROWS  EASE THE SHEET";
        else if (z_ > kMouth - 40.f) hint = "THE MOUTH IS OPEN  DRIVE";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS  SHEET AND HELM", PAL_HUD);
    }

    hud(1, 0, "S3 KEEL PASS", PAL_GOLD);
    char buf[40];
    float left = std::max(0.f, storm_);
    std::snprintf(buf, sizeof(buf), "CREW %d", int(std::ceil(left)));
    hud(30, 0, buf, left < 8.f ? PAL_ALERT : PAL_CREW);
    if (mode_ == Mode::Run) {
        std::snprintf(buf, sizeof(buf), "KEEL %d", int(std::max(0.f, speed_)));
        hud(16, 0, buf, PAL_HUD);
    }
}

}  // namespace keelpass
