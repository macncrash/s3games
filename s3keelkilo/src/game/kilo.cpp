#include "kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace keelkilo {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kFinish = 1000.f;
constexpr float kRival = 92.f;
constexpr float kHub = 5.55f;
constexpr float kBank = 4.15f;
constexpr float kGate = 1.65f;
constexpr float kBeam = 0.95f;
constexpr float kLen = 3.3f;

struct Wheel {
    float z, side, r;
};

constexpr Wheel kWheels[] = {
    {160.f, -1.f, 3.15f}, {310.f, 1.f, 3.35f}, {470.f, -1.f, 3.45f},
    {620.f, 1.f, 3.05f},  {760.f, -1.f, 3.40f}, {900.f, 1.f, 2.95f},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.apu.setMaster(0.45f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    z_ = 18.f;
    x_ = 0.2f;
    camZ_ = z_ - 18.f;
    time_ = 0;
}

void Game::begin() {
    z_ = 8.f;
    x_ = 0.f;
    heading_ = 0.f;
    speed_ = 0.f;
    time_ = 0.f;
    camZ_ = z_ - 18.f;
    shake_ = 0.f;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(220.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.12f);
    beep_ = 0.08f;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    speed_ *= 0.12f;
    sys_->apu.noiseBurst(0.42f, 700.f, 0.28f);
    sys_->apu.tone(1, 70.f, 0.24f);
    beep_ = 0.32f;
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    why_ = "the kilometer";
    chime_ = 0;
    chimeT_ = 0;
}

float Game::aimX() const {
    if (z_ > 940.f) return 0.f;
    float best = 1e9f;
    float side = 0.f;
    for (const Wheel& w : kWheels) {
        float dz = w.z - z_;
        if (dz < -4.f || dz > 78.f) continue;
        if (dz < best) {
            best = dz;
            side = w.side;
        }
    }
    if (side == 0.f) return 0.f;
    return -side * 1.85f;
}

bool Game::wheelHit(float cx, float wz, float rad) const {
    float c = std::cos(heading_);
    float s = std::sin(heading_);
    float lim = rad + kBeam;
    for (float a : {-kLen, 0.f, kLen}) {
        float px = x_ + s * a;
        float pz = z_ + c * a;
        float dx = px - cx;
        float dz = pz - wz;
        if (dx * dx + dz * dz < lim * lim) return true;
    }
    return false;
}

void Game::pilot(float& thrust, float& steer) {
    thrust = 0.f;
    steer = 0.f;
    if (bot_) {
        float want = aimX();
        float err = want - x_;
        steer = clampf(err * 0.85f, -1.f, 1.f);
        thrust = 1.f;
        float near = 1e9f;
        for (const Wheel& w : kWheels) near = std::min(near, w.z - z_);
        if (near > 0.f && near < 22.f && std::fabs(err) > 0.65f) thrust = 0.35f;
        return;
    }
    const gs::Pad& pad = sys_->pad;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.accel > 0.2f) thrust += 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.2f) thrust -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
}

void Game::stepBoat(float thrust, float steer) {
    float drive = thrust > 0.f ? thrust : 0.f;
    float brake = thrust < 0.f ? -thrust : 0.f;
    speed_ += (drive * 5.6f - brake * 9.f - speed_ * 0.08f) * kDt;
    speed_ = clampf(speed_, 0.f, 13.5f);
    float wantH = steer * 0.42f;
    heading_ += (wantH - heading_) * std::min(1.f, kDt * 5.f);
    float c = std::cos(heading_);
    float s = std::sin(heading_);
    x_ += s * speed_ * kDt;
    z_ += c * speed_ * kDt;
    time_ += kDt;

    if (z_ + kLen >= kFinish) {
        if (std::fabs(x_) <= kGate) finish();
        else fail("MISSED THE END");
        return;
    }
    if (std::fabs(x_) > kBank - 0.15f) {
        fail("MISSED THE END");
        return;
    }
    for (const Wheel& w : kWheels) {
        if (std::fabs(w.z - z_) > 12.f) continue;
        if (wheelHit(w.side * kHub, w.z, w.r)) {
            fail("TOUCHED A WHEEL");
            return;
        }
    }
    if (time_ > kRival) fail("THE OTHER CREW");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float thrust = 0, steer = 0;
            pilot(thrust, steer);
            stepBoat(thrust, steer);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }

    float want = z_ - 18.f;
    camZ_ += (want - camZ_) * (1.f - std::exp(-kDt * 4.f));
    if (mode_ == Mode::Title) camZ_ = z_ - 18.f;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    } else if (mode_ == Mode::Run && speed_ > 1.f) {
        sys.apu.tone(0, 62.f + speed_ * 3.f, 0.035f);
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        int stepN = int(chimeT_ / 0.16f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(2, notes[stepN], 0.16f);
        }
        if (stepN >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    draw();
}

bool Game::project(float wx, float wz, float& sx, float& sy, float& sc) const {
    float dz = wz - camZ_;
    if (dz < 3.4f) return false;
    sc = 380.f / dz;
    float jx = (shake_ > 0.f) ? std::sin(time_ * 90.f) * shake_ * 3.f : 0.f;
    sx = 160.f + wx * sc + jx;
    sy = 62.f + 1040.f / dz;
    return true;
}

void Game::stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (m.h < 1 || w < 1.f || h < 1.f) return;
    if (x + w < -30.f || x > gs::SCREEN_W + 30.f || y + h < -30.f || y > gs::SCREEN_H + 40.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 420L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 300L));
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H);
        int sky = y < 70;
        int r = sky ? 3 : 1;
        int g = sky ? 7 + int((70 - y) / 18.f) : 5 + int(t * 4.f);
        int b = sky ? 11 : 8 + int(t * 4.f);
        vdp.lineBackdrop[y] = gs::rgb4(r, std::min(14, g), std::min(15, b));
        vdp.lineFog[y] = uint8_t(y < 78 ? (78 - y) / 7 : 0);
        vdp.road[y].on = false;
    }

    float bsx = 0, bsy = 0, bsc = 0;
    bool boat = project(x_, z_, bsx, bsy, bsc);
    if (boat) {
        float bh = std::max(16.f, bsc * 6.4f);
        float bw = bh * 0.42f;
        float lean = heading_ * 16.f;
        stamp(art_.sail, bsx - bw * 0.85f + lean, bsy - bh * 0.95f, bw * 1.7f, bh * 0.85f, PAL_SAIL);
        stamp(art_.hull, bsx - bw * 0.5f + lean * 0.3f, bsy - bh * 0.42f, bw, bh, PAL_HULL);
        stamp(art_.foam, bsx - bw * 0.7f, bsy + bh * 0.12f, bw * 1.3f, bh * 0.2f, PAL_FOAM);
    }

    auto mark = [&](float mz) {
        worldQuad(art_.post, -kGate - 0.35f, mz, -kGate + 0.15f, mz + 1.2f, 26.f, PAL_POST);
        worldQuad(art_.post, kGate - 0.15f, mz, kGate + 0.35f, mz + 1.2f, 26.f, PAL_POST);
        worldQuad(art_.tape, -kGate, mz, kGate, mz + 0.4f, 22.f, PAL_GOLD);
    };
    if (kFinish > camZ_ && kFinish < camZ_ + 110.f) mark(kFinish);

    for (const Wheel& w : kWheels) {
        if (w.z < camZ_ + 4.f || w.z > camZ_ + 100.f) continue;
        float cx = w.side * kHub;
        float sx, sy, sc;
        if (!project(cx, w.z, sx, sy, sc)) continue;
        float d = std::max(8.f, w.r * 2.f * sc);
        stamp(art_.wheel, sx - d * 0.5f, sy - d * 0.55f, d, d, PAL_WHEEL);
        float mx0 = cx + w.side * (w.r * 0.2f);
        float mx1 = mx0 + w.side * 2.4f;
        if (mx0 > mx1) std::swap(mx0, mx1);
        worldQuad(art_.mill, mx0, w.z - 1.5f, mx1, w.z + 2.f, 20.f, PAL_MILL);
    }

    for (float z = camZ_ + 96.f; z > camZ_ + 4.f; z -= 8.f) {
        worldQuad(art_.reed, -kBank - 2.4f, z, -kBank, z + 7.f, 14.f, PAL_BANK);
        worldQuad(art_.reed, kBank, z, kBank + 2.4f, z + 7.f, 14.f, PAL_BANK);
    }

    if (mode_ == Mode::Title) {
        float tsx, tsy, tsc;
        if (project(0.f, z_ + 28.f, tsx, tsy, tsc)) stamp(art_.title, tsx - 78.f, tsy - 20.f, 156.f, 30.f, PAL_GOLD);
        hudC(16, "FINISH THE KILOMETER", PAL_HUD);
        hudC(18, "DO NOT TOUCH A WHEEL", PAL_ALERT);
        hudC(20, "MISSING THE END FAILS", PAL_HUD);
        hudC(23, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(16, "KILOMETER FINISHED", PAL_WIN);
        hudC(18, "NO WHEEL WAS TOUCHED", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
    } else {
        hudC(25, "SHEET IN AND MISS THE WHEELS", PAL_HUD);
        hud(1, 26, "ARROWS  DRIVE AND STEER", PAL_HUD);
    }

    hud(1, 0, "S3 KEEL KILO", PAL_GOLD);
    char buf[40];
    int shown = int(clampf(z_, 0.f, kFinish));
    std::snprintf(buf, sizeof(buf), "%d M", shown);
    hud(16, 0, buf, PAL_HUD);
    int left = int(std::ceil(std::max(0.f, kRival - time_)));
    std::snprintf(buf, sizeof(buf), "CREW %d", left);
    hud(30, 0, buf, left < 10 ? PAL_ALERT : PAL_POST);
}

}  // namespace keelkilo
