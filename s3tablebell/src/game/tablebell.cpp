#include "game/tablebell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tablebell {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr int kFlight = 32;
}  // namespace

bool Game::audit() const {
    if (spotAt(kBellX, kBellY) != Spot::Bell) return false;
    if (spotAt(kBellX + kBellR - 0.4f, kBellY) != Spot::Bell) return false;
    if (spotAt(kBellX, kBellY + kBellR - 0.4f) != Spot::Bell) return false;
    if (spotAt(kServeX, kNetY) != Spot::Net) return false;
    if (spotAt(kServeX, kServeY) != Spot::Short) return false;
    if (spotAt(kBellX, (kT + kNetY) * 0.5f) != Spot::Wide && spotAt(kBellX + 40.f, kBellY) != Spot::Wide)
        return false;
    if (spotAt(8.f, 8.f) != Spot::Hot) return false;
    if (kBellY >= kNetY - kNetH) return false;
    return true;
}

bool Game::sweet() const { return meter_ >= 0.46f && meter_ <= 0.54f; }

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    why_ = Death::None;
    aimX_ = kServeX;
    aimY_ = 150.f;
    ballX_ = kServeX;
    ballY_ = kServeY;
    hop_ = 0;
    meter_ = 0;
    meterDir_ = 1;
    bellAmp_ = 0.2f;
    firm_ = false;
}

void Game::newGame() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    why_ = Death::None;
    bellAmp_ = 0.2f;
    beginAim();
}

void Game::beginAim() {
    mode_ = Mode::Aim;
    firm_ = false;
    flightT_ = 0;
    hop_ = 0;
    ballX_ = kServeX;
    ballY_ = kServeY;
    aimX_ = kServeX;
    aimY_ = 150.f;
    meter_ = bot_ ? 0.1f : meter_;
    meterDir_ = 1;
}

void Game::launch(bool firm) {
    firm_ = firm;
    tryNo_++;
    flightT_ = 0;
    mode_ = Mode::Flight;
    float x = aimX_;
    float y = aimY_;
    if (!firm) {
        float dx = x - kBellX;
        float dy = y - kBellY;
        float d = std::hypot(dx, dy);
        if (d < 1.f) {
            dx = 1.f;
            dy = 0.2f;
            d = 1.f;
        }
        x += dx / d * 36.f;
        y += dy / d * 22.f;
    } else if (spotAt(x, y) == Spot::Bell) {
        x = kBellX;
        y = kBellY;
    }
    landX_ = x;
    landY_ = y;
    blip(2, 180.f, 0.08f, 0.05f);
}

void Game::ring() {
    rung_ = true;
    won_ = true;
    mode_ = Mode::Ring;
    holdT_ = 1.3f;
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ballX_ = kBellX;
    ballY_ = kBellY;
    hop_ = 0;
}

void Game::dieTry(Death why) {
    why_ = why;
    dead_++;
    mode_ = Mode::Dead;
    holdT_ = 1.05f;
    hop_ = 0;
    ballX_ = landX_;
    ballY_ = landY_;
    blip(2, 90.f, 0.07f, 0.12f);
}

void Game::resolve() {
    Spot s = spotAt(landX_, landY_);
    if (s == Spot::Bell && firm_) {
        ring();
        return;
    }
    if (s == Spot::Bell) s = Spot::Lip;
    Death why = Death::Hot;
    if (s == Spot::Short) why = Death::Short;
    else if (s == Spot::Net) why = Death::Net;
    else if (s == Spot::Wide || s == Spot::Lip) why = Death::Lip;
    dieTry(why);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        holdT_ = 0;
    }
}

void Game::botAim() {
    float dx = kBellX - aimX_;
    float dy = kBellY - aimY_;
    float d = std::hypot(dx, dy);
    if (d > 1.1f) {
        float step = std::min(4.f, d);
        aimX_ += dx / d * step;
        aimY_ += dy / d * step;
    }
    if (d <= 1.6f && sweet()) launch(true);
}

void Game::humanAim(const gs::Pad& p, bool fire) {
    float mx = 0, my = 0;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    mx += p.axisX;
    my -= p.axisY;
    aimX_ = std::clamp(aimX_ + mx * 2.1f, kL + 4.f, kR - 4.f);
    aimY_ = std::clamp(aimY_ + my * 2.1f, kT + 4.f, kB - 4.f);
    if (fire) launch(sweet());
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 2) tickT_ = hold;
    else toneT_ = hold;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (chiming) {
        bellTick_ -= dt;
        if (bellTick_ <= 0.f && bellAmp_ > 0.35f) {
            float vol = 0.05f + 0.09f * bellAmp_;
            sys_->apu.tone(0, 698.f, vol);
            sys_->apu.tone(1, 1046.f, vol * 0.65f);
            toneT_ = 0.16f;
            bellTick_ = 0.28f;
        }
    }
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            if (!chiming) sys_->apu.tone(1, 0, 0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    art_.load(sys.vdp);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.scroll(0, 0);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3tablebell rules failed\n");
    if (bot_) newGame();
    else toTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 13.f : 2.1f);
    if (rung_) bellAmp_ = std::max(0.16f, bellAmp_ - kDt * 0.4f);
    tickAudio(kDt);

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool fire = p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B);

    if (mode_ == Mode::Title) {
        if (start || p.anyPressed()) newGame();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
    } else if (mode_ == Mode::Aim) {
        if (!bot_ && start) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else {
            meter_ += meterDir_ * 0.021f;
            if (meter_ >= 1.f) {
                meter_ = 1.f;
                meterDir_ = -1.f;
            } else if (meter_ <= 0.f) {
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            if (bot_) botAim();
            else humanAim(p, fire);
        }
    } else if (mode_ == Mode::Flight) {
        flightT_++;
        float u = std::clamp(float(flightT_) / float(kFlight), 0.f, 1.f);
        ballX_ = kServeX + (landX_ - kServeX) * u;
        ballY_ = kServeY + (landY_ - kServeY) * u;
        hop_ = std::sin(u * 3.14159265f) * 46.f;
        if (flightT_ >= kFlight) resolve();
    } else if (mode_ == Mode::Dead) {
        holdT_ -= kDt;
        if (holdT_ <= 0.f) beginAim();
    } else if (mode_ == Mode::Ring) {
        holdT_ -= kDt;
        if (holdT_ <= 0.f) {
            mode_ = Mode::Leave;
            holdT_ = 0.8f;
        }
    } else if (mode_ == Mode::Leave) {
        holdT_ -= kDt;
        if (holdT_ <= 0.f) {
            mode_ = Mode::Over;
            over_ = true;
            won_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) newGame();
    }

    draw();
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
        if (y < 28) c = gs::rgb4(2, 2, 3);
        else if (y < 200) c = gs::rgb4(3, 3, 4);
        else c = gs::rgb4(4, 3, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::max(1.f, w));
    s.h = int16_t(std::max(1.f, h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float cx = (kL + kR) * 0.5f;
    float cy = (kT + kB) * 0.5f;
    spr(art_.cloth, cx, cy, float(art_.cloth.w), float(art_.cloth.h), PAL_CLOTH);
    spr(art_.net, cx, kNetY, float(art_.net.w), float(art_.net.h), PAL_NET);

    float swing = std::sin(bellPh_) * 3.2f * bellAmp_;
    spr(art_.bell, kBellX + swing, kBellY, float(art_.bell.w), float(art_.bell.h), PAL_BELL);

    if (mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Pause) {
        spr(art_.cross, aimX_, aimY_, float(art_.cross.w), float(art_.cross.h), PAL_AIM);
        spr(art_.bat, kServeX - 18.f, kServeY + 4.f, float(art_.bat.w), float(art_.bat.h), PAL_BAT);
        spr(art_.ball, kServeX, kServeY, float(art_.ball.w), float(art_.ball.h), PAL_BALL);
    } else {
        spr(art_.bat, kServeX - 18.f, kServeY + 4.f, float(art_.bat.w), float(art_.bat.h), PAL_BAT);
        float by = ballY_ - hop_;
        if (hop_ > 6.f) {
            gs::Sprite sh;
            sh.img = art_.ball;
            sh.w = art_.ball.w;
            sh.h = 4;
            sh.x = int16_t(std::lround(ballX_ - art_.ball.w * 0.5f));
            sh.y = int16_t(std::lround(ballY_ + 2.f));
            sh.pal = PAL_BALL;
            sh.shadow = true;
            v.sprite(sh);
        }
        spr(art_.ball, ballX_, by, float(art_.ball.w), float(art_.ball.h), PAL_BALL);
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Flight) {
        int x0 = 12;
        int filled = int(meter_ * 16.f);
        for (int i = 0; i < 16; i++) {
            int pal = (i >= 7 && i <= 8) ? PAL_GREEN : PAL_INK;
            if (i < filled) hud(x0 + i, 26, "I", sweet() && i >= 7 && i <= 8 ? PAL_GREEN : pal);
            else hud(x0 + i, 26, "-", PAL_INK);
        }
    }

    if (mode_ == Mode::Title) {
        hudC(2, "S3 TABLEBELL", PAL_GOLD);
        hudC(22, "RING THE BELL", PAL_GOLD);
        hudC(23, "BEFORE THE THIRD TRY DIES", PAL_INK);
        hudC(24, "FIRM IN THE CUP", PAL_GREEN);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(25, "PRESS START", PAL_GOLD);
        else hudC(25, "ARROWS AIM   Z IN GREEN", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(2, "PAUSED", PAL_GOLD);
        hudC(24, "START RESUME", PAL_INK);
        return;
    }
    if (mode_ == Mode::Over && won_) {
        hudC(2, "BELL", PAL_GOLD);
        hudC(23, "LEFT BEFORE THE THIRD TRY", PAL_INK);
        if (!bot_) hudC(25, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Over) {
        hudC(2, "THIRD TRY DEAD", PAL_ALERT);
        hudC(23, "BELL SILENT", PAL_INK);
        if (!bot_) hudC(25, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Ring) {
        hudC(2, "BELL", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Leave) {
        hudC(2, "LEAVE", PAL_GREEN);
        hudC(23, "THE BELL RANG", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Dead) {
        hudC(2, "TRY DIED", PAL_ALERT);
        hudC(3, dead_ == 1 ? "TWO LEFT" : "ONE LEFT", PAL_GOLD);
        if (why_ == Death::Short) hudC(23, "DIED SHORT", PAL_ALERT);
        else if (why_ == Death::Net) hudC(23, "DIED IN THE NET", PAL_ALERT);
        else if (why_ == Death::Hot) hudC(23, "DIED HOT", PAL_ALERT);
        else hudC(23, "DIED ON THE LIP", PAL_INK);
        return;
    }

    char buf[40];
    std::snprintf(buf, sizeof buf, "TRY %d OF 3", dead_ + 1);
    hud(1, 0, buf, dead_ == 2 ? PAL_ALERT : PAL_GOLD);
    std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
    hud(32, 0, buf, PAL_INK);
    hud(1, 1, "S3 TABLEBELL", PAL_GOLD);
    if (mode_ == Mode::Flight) {
        hudC(24, "IN THE AIR", PAL_GOLD);
        return;
    }
    Spot live = spotAt(aimX_, aimY_);
    const char* name = "OFF THE WOOD";
    int pal = PAL_ALERT;
    if (live == Spot::Bell) {
        name = sweet() ? "CUP  THROW" : "CUP  WAIT";
        pal = sweet() ? PAL_GREEN : PAL_GOLD;
    } else if (live == Spot::Short) {
        name = "YOUR HALF";
    } else if (live == Spot::Net) {
        name = "ON THE NET";
    } else if (live == Spot::Wide) {
        name = "FAR CLOTH";
        pal = PAL_INK;
    }
    hudC(24, name, pal);
    hudC(25, sweet() ? "GREEN  Z" : "WAIT FOR GREEN", sweet() ? PAL_GREEN : PAL_AIM);
}

}  // namespace tablebell
